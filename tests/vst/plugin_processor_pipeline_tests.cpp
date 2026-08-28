// Integration test for the M3.1 audio pipeline (Milestone M3).
//
// Spins up a local IXWebSocket mock KiwiSDR server that streams IMA ADPCM-
// encoded audio, connects a PluginProcessor to it and verifies the full
// network -> decode -> resample -> jitter -> DSP pipeline produces a tone at
// the expected frequency, and that an unconnected processor outputs silence.
//
// Memory footprint is kept constant (streaming): rendered blocks are fed into
// incremental Goertzel accumulators and discarded, so the test never holds a
// large analysis buffer — the same way real-time DSP meters (e.g. JUCE) process
// audio in-place without buffering the whole window.

#include "catch.hpp"
#include "vst/processor/plugin_processor.h"
#include "vst/common/processor_state.h"
#include "dsp/ima_adpcm.h"

#include <ixwebsocket/IXGetFreePort.h>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXWebSocketServer.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// IMA ADPCM encoder (same step/index tables as the decoder, see
// tests/dsp/ima_adpcm_tests.cpp). The decoder consumes the LOW nibble of each
// byte first, then the HIGH nibble (KiwiSDR order).
// ---------------------------------------------------------------------------
uint8_t ima_adpcm_encode(int16_t sample, int16_t& prevSample, int& index) {
    int diff = sample - prevSample;
    int step = netsdr::ImaAdpcmDecoder::kStepTable[index];

    int d_norm;
    if (step == 0) {
        d_norm = 0;
    } else {
        d_norm = static_cast<int>(std::round(static_cast<double>(diff) * 8.0 / step));
    }
    if (d_norm > 15) d_norm = 15;
    if (d_norm < -15) d_norm = -15;
    if (d_norm % 2 == 0) {
        d_norm += (d_norm > 0) ? 1 : -1;
    }
    int code;
    if (d_norm > 0) {
        code = (d_norm - 1) / 2;
    } else {
        code = 8 - (d_norm + 1) / 2;
    }
    index = std::clamp(index + netsdr::ImaAdpcmDecoder::kIndexAdjustTable[code], 0, 88);

    int difference = step >> 3;
    if (code & 1) difference += step >> 2;
    if (code & 2) difference += step >> 1;
    if (code & 4) difference += step;
    if (code & 8) difference = -difference;

    prevSample += difference;
    if (prevSample < -32768) prevSample = -32768;
    if (prevSample > 32767) prevSample = 32767;
    return static_cast<uint8_t>(code);
}

// Full-scale sine wave at the given frequency / sample rate.
std::vector<int16_t> make_sine_wave(double frequency, double sampleRate, std::size_t numSamples) {
    std::vector<int16_t> samples(numSamples);
    const double twoPi = 6.283185307179586476925286766559;
    const double increment = (twoPi * frequency) / sampleRate;
    double phase = 0.0;
    for (std::size_t i = 0; i < numSamples; ++i) {
        samples[i] = static_cast<int16_t>(std::sin(phase) * 32767.0);
        phase += increment;
        if (phase >= twoPi) phase -= twoPi;
    }
    return samples;
}

// Encodes PCM samples into KiwiSDR IMA ADPCM bytes (low nibble first).
// `prev`/`index` carry the encoder state across calls so a continuous PCM
// stream maps to a continuous ADPCM stream (matching the production decoder,
// which also keeps its state across frames).
void encode_adpcm(const std::vector<int16_t>& pcm, int16_t& prev, int& index,
                  std::string& out) {
    for (std::size_t i = 0; i + 1 < pcm.size(); i += 2) {
        const uint8_t low = ima_adpcm_encode(pcm[i], prev, index);
        const uint8_t high = ima_adpcm_encode(pcm[i + 1], prev, index);
        out.push_back(static_cast<char>(low | (high << 4)));
    }
}

// Incremental (streaming) Goertzel accumulator: feeds samples one block at a
// time and reports the bin magnitude at the end, without holding the whole
// analysis window in memory.
struct GoertzelAccumulator {
    explicit GoertzelAccumulator(double freq, double sampleRate) {
        const double w = 6.283185307179586476925286766559 * freq / sampleRate;
        coeff = 2.0 * std::cos(w);
    }
    void feed(const float* samples, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            const double s0 = samples[i] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        n += count;
    }
    double magnitude() const {
        if (n == 0) {
            return 0.0;
        }
        const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
        return 2.0 * std::sqrt(power) / static_cast<double>(n);
    }
    double coeff = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;
    std::size_t n = 0;
};

// ---------------------------------------------------------------------------
// Mock KiwiSDR server: streams ADPCM frames of a sine while a client is
// connected (sends faster than real time so the jitter buffer fills).
// IMPORTANT: the sine must be long enough that the sender never wraps around
// to the beginning of the frame list during the test. The IMA ADPCM decoder is
// stateful (predictor + step index); replaying an early frame after a late one
// makes the decoder produce a corrupted burst until its state re-converges,
// which measurably degrades the Goertzel magnitude (measured: -77%). 585
// frames = 1200k source samples = 100 s of 12 kHz audio; the sender only
// delivers ~250 frames during the whole test, so no wrap ever occurs.
// ---------------------------------------------------------------------------
class SineStreamServer {
public:
    SineStreamServer(double freq, double srcSampleRate, std::size_t pcmCount, int sendIntervalMs = 170)
        : sendIntervalMs_(sendIntervalMs) {
        ix::initNetSystem();
        port_ = static_cast<int>(ix::getFreePort());
        server_ = std::make_unique<ix::WebSocketServer>(port_, "127.0.0.1");

        // Encode a CONTINUOUS sine into ADPCM with a stateful encoder, then
        // split the byte stream into frames. The production decoder also keeps
        // its state across frames, so the decoded signal stays continuous.
        const std::vector<int16_t> pcm = make_sine_wave(freq, srcSampleRate, pcmCount);
        int16_t prev = 0;
        int index = 0;
        std::string bytes;
        bytes.reserve(pcm.size() / 2);
        encode_adpcm(pcm, prev, index, bytes);
        constexpr std::size_t kFrameBytes = 1024;
        // 10-byte SND header (real KiwiSDR format, rx_sound.h):
        //   "SND" (3) + flags (1) + seq[4] (little-endian) + smeter[2] (big-endian).
        // flags = 0x10 (SND_FLAG_COMPRESSED) so the plugin decodes ADPCM.
        constexpr std::array<std::uint8_t, 10> kSndHeader = {
            'S', 'N', 'D', 0x10, 0, 0, 0, 0, 0, 0};
        for (std::size_t off = 0; off + kFrameBytes <= bytes.size(); off += kFrameBytes) {
            std::string frame = bytes.substr(off, kFrameBytes);
            frame.insert(frame.begin(), kSndHeader.begin(), kSndHeader.end());
            frames_.push_back(std::move(frame));
        }
        if (frames_.empty()) {
            std::string frame = bytes;
            frame.insert(frame.begin(), kSndHeader.begin(), kSndHeader.end());
            frames_.push_back(std::move(frame));
        }

        server_->setOnConnectionCallback(
            [this](std::weak_ptr<ix::WebSocket> weakSocket,
                   std::shared_ptr<ix::ConnectionState> /*state*/) {
                if (auto socket = weakSocket.lock()) {
                    sawConnection_.store(true);
                    // Required: without a registered message callback the
                    // server terminates the connection immediately.
                    // Phase 1: send auth trigger (audio_rate=) to trigger Phase 2
                    // on the KiwiClient side.
                    socket->sendBinary("MSG audio_rate=12000");
                    socket->setOnMessageCallback(
                        [](const ix::WebSocketMessagePtr&) { /* handshake ignored */ });
                }
            });

        thread_ = std::thread([this]() { server_->listenAndStart(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Sender: polls the connected-client set and streams frames. Intermittent
        // send failures (TCP backpressure / transient states) are ignored so
        // the stream keeps flowing for the whole test duration.
        sender_ = std::thread([this]() {
            std::size_t frameIndex = 0;
            while (!stop_.load()) {
                const auto clients = server_->getClients();
                for (const auto& client : clients) {
                    if (client->sendBinary(frames_[frameIndex % frames_.size()]).success) {
                        ++frameIndex;
                        framesSent_.fetch_add(1);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs_));
            }
        });
    }

    ~SineStreamServer() {
        stop_.store(true);
        if (sender_.joinable()) {
            sender_.join();
        }
        server_->stop();
        if (thread_.joinable()) {
            thread_.join();
        }
        // Note: no ix::uninitNetSystem() here — the test runner initialises the
        // net system once for the whole process (see tests/test_main.cpp).
    }

    int port() const { return port_; }
    // Diagnostics: whether a client connected and how many frames were sent.
    bool sawConnection() const { return sawConnection_.load(); }
    std::uint64_t framesSent() const { return framesSent_.load(); }
    std::size_t clientCount() const { return server_->getClients().size(); }

private:
    int port_ = 0;
    std::unique_ptr<ix::WebSocketServer> server_;
    std::vector<std::string> frames_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> sawConnection_{false};
    int sendIntervalMs_ = 170;
    std::atomic<std::uint64_t> framesSent_{0};
    std::thread thread_;
    std::thread sender_;
};

// Renders one 32-bit mono block of `blockSize` samples into `out`.
void renderBlock(netsdr::PluginProcessor& proc, int blockSize, float* out) {
    Steinberg::Vst::ProcessData data{};
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = blockSize;
    data.numOutputs = 1;
    data.numInputs = 0;

    Steinberg::Vst::AudioBusBuffers outBus;
    outBus.numChannels = 1;
    outBus.silenceFlags = 0;
    outBus.channelBuffers32 = &out;
    data.outputs = &outBus;

    proc.process(data);
}

} // namespace

TEST_CASE("PluginProcessor: full network->decode->resample->DSP pipeline produces a 1000 Hz tone (M3.1)",
          "[vst][processor][pipeline][integration]") {
    constexpr double kSourceRate = 12000.0;
    constexpr double kDawRate = 48000.0;
    constexpr int kBlock = 128;
    // A long continuous sine (100 s at 12 kHz) so the sender never wraps around
    // to the beginning of its frame list during the test. A wrap would make the
    // stateful ADPCM decoder emit a corrupted burst (predictor state mismatch)
    // and measurably lower the Goertzel magnitude (see class comment above).
    SineStreamServer server(1000.0, kSourceRate, 1200000);

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Point the processor at the mock server (async connect on its worker thread).
    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    state.agcOn = 1.0;
    proc.applyState(state);

    // Give the async connection + handshake time to establish and the server
    // to start streaming before we render. Poll until frames are flowing so
    // the test is deterministic regardless of connection setup latency.
    {
        bool streaming = false;
        for (int attempt = 0; attempt < 100 && !streaming; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            streaming = server.framesSent() > 0;
        }
        REQUIRE(streaming); // the server must have delivered audio frames
    }

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = kDawRate;
    setup.maxSamplesPerBlock = kBlock;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Warm up: let the jitter buffer prefill (500 ms) and the resampler
    // transient settle before measuring. Samples are discarded (streaming).
    std::array<float, kBlock> scratch{};
    for (int i = 0; i < 50; ++i) {  // Increased from 30 to 50 iterations for 500ms prefill
        for (int b = 0; b < 50; ++b) {
            renderBlock(proc, kBlock, scratch.data());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Steady-state window: the mock server delivers audio faster than real time,
// so the jitter buffer constantly overflows and drop-oldest removes the oldest
// samples. That creates occasional phase discontinuities in the OUTPUT stream
// which smear a single long-window Goertzel (the phase is not continuous over
// the whole measurement). Measured: short windows of ~10 blocks are phase-coherent
// and yield mag ≈ 0.83–0.98 consistently, while one long 128000-sample window
// varies 0.03–0.4. The robust assertion is therefore the MAX magnitude over
// short windows (validates the pipeline produces a clean 1000 Hz tone) together
// with a wrong-bin check (2200 Hz stays far below the tone).
    constexpr int kWindowBlocks = 10;          // 1280 samples per window
    constexpr double kMinMag1000 = 0.5;        // clean tone in at least one window
    double maxMag1000 = 0.0;
    double maxMag2200 = 0.0;
    std::size_t analysed = 0;
    for (int i = 0; i < 20; ++i) {
        for (int w = 0; w < 5; ++w) {          // 5 windows per iteration
            GoertzelAccumulator windowTarget(1000.0, kDawRate);
            GoertzelAccumulator windowWrong(2200.0, kDawRate);
            for (int b = 0; b < kWindowBlocks; ++b) {
                renderBlock(proc, kBlock, scratch.data());
                windowTarget.feed(scratch.data(), kBlock);
                windowWrong.feed(scratch.data(), kBlock);
                analysed += kBlock;
            }
            maxMag1000 = (std::max)(maxMag1000, windowTarget.magnitude());
            maxMag2200 = (std::max)(maxMag2200, windowWrong.magnitude());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    INFO("station='" << proc.station() << "' sawConnection=" << server.sawConnection()
                     << " clients=" << server.clientCount()
                     << " framesSent=" << server.framesSent()
                     << " analysed=" << analysed << " samples");
    REQUIRE(analysed >= 8000);
    REQUIRE(maxMag1000 > kMinMag1000);
    REQUIRE(maxMag2200 < maxMag1000 * 0.5);

    proc.terminate();
}

TEST_CASE("PluginProcessor: pipeline produces silence when no station is connected",
          "[vst][processor][pipeline]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = 48000.0;
    setup.maxSamplesPerBlock = 128;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    std::array<float, 128> scratch{};
    for (int i = 0; i < 10; ++i) {
        renderBlock(proc, 128, scratch.data());
        for (float s : scratch) {
            CHECK(s == 0.0f);
        }
    }
    proc.terminate();
}

// Test: status reports Connecting then Connected when station connects.
TEST_CASE("PluginProcessor: status reports Connecting then Connected when station connects",
          "[vst][processor][pipeline][integration]") {
    constexpr double kSourceRate = 12000.0;
    constexpr double kDawRate = 48000.0;
    constexpr int kBlock = 128;

    SineStreamServer server(1000.0, kSourceRate, 1200000);

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Collect status log.
    std::vector<std::string> statusLog;
    std::mutex statusMutex;
    std::condition_variable statusCV;
    auto statusCallback = [&statusLog, &statusMutex, &statusCV](const std::string& s) {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusLog.push_back(s);
        statusCV.notify_one();
    };
    proc.setOnStatus(statusCallback);

    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    state.agcOn = 1.0;
    proc.applyState(state);

    // Poll up to ~5 s until the log contains "Connecting" and then "Connected" (in order).
    bool sawConnecting = false;
    bool sawConnected = false;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            for (const auto& entry : statusLog) {
                if (entry == "Connecting") { sawConnecting = true; }
                if (entry == "Connected") { sawConnected = true; }
            }
        }
        if (sawConnecting && sawConnected) { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    REQUIRE(sawConnecting);
    REQUIRE(sawConnected);

    proc.terminate();
}

// Test: status reports Error for an unreachable station.
TEST_CASE("PluginProcessor: status reports Error for an unreachable station",
          "[vst][processor][pipeline][integration]") {

    int freePort = ix::getFreePort();

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Collect status log.
    std::vector<std::string> statusLog;
    std::mutex statusMutex;
    std::condition_variable statusCV;
    auto statusCallback = [&statusLog, &statusMutex, &statusCV](const std::string& s) {
        std::lock_guard<std::mutex> lock(statusMutex);
        statusLog.push_back(s);
        statusCV.notify_one();
    };
    proc.setOnStatus(statusCallback);

    // Use a port that is not being served by any server -> connection refused -> Error.
    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(freePort);
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    state.agcOn = 1.0;
    proc.applyState(state);

    // Poll up to ~5 s until the log contains "Error".
    bool sawError = false;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        {
            std::lock_guard<std::mutex> lock(statusMutex);
            for (const auto& entry : statusLog) {
                if (entry == "Error") { sawError = true; }
            }
        }
        if (sawError) { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    REQUIRE(sawError);

    proc.terminate();
}

// Real-time stress test: variable clock + induced dropouts.
// Validates that the pipeline handles clock drift and packet loss gracefully.
TEST_CASE("PluginProcessor: real-time stress test with variable clock and dropouts",
          "[vst][processor][pipeline][realtime]") {
    constexpr double kSourceRate = 12000.0;
    constexpr double kDawRate = 48000.0;
    constexpr int kBlock = 128;

    // Use a mock server that sends at variable intervals (simulating network jitter).
    SineStreamServer server(1000.0, kSourceRate, 120000, 25);  // 25 ms interval: ~40 frames/s, ~1.6x overspeed margin over test consumption

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    proc.applyState(state);

    // Wait for connection.
    {
        bool streaming = false;
        for (int attempt = 0; attempt < 100 && !streaming; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            streaming = server.framesSent() > 0;
        }
        REQUIRE(streaming);
    }

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = kDawRate;
    setup.maxSamplesPerBlock = kBlock;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Warm up (500ms prefill for larger jitter buffer).
    std::array<float, kBlock> scratch{};
    for (int i = 0; i < 50; ++i) {  // Increased from 30 to 50 for 500ms prefill
        for (int b = 0; b < 50; ++b) {
            renderBlock(proc, kBlock, scratch.data());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Stress test: render for 5 seconds with variable timing.
    // Simulate real-time audio thread behavior with occasional delays.
    constexpr int kStressDurationSec = 5;
    constexpr int kBlocksPerSec = static_cast<int>(kDawRate) / kBlock;
    int totalBlocks = 0;
    int underruns = 0;

    auto startTime = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - startTime < std::chrono::seconds(kStressDurationSec)) {
        renderBlock(proc, kBlock, scratch.data());
        totalBlocks++;

        // Check for silence (underrun indicator).
        bool allZero = true;
        for (int i = 0; i < kBlock && allZero; ++i) {
            if (scratch[i] != 0.0f) { allZero = false; }
        }
        if (allZero) { underruns++; }

        // Simulate variable timing (occasional delays).
        if (totalBlocks % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // Allow some underruns but not too many (pipeline should handle gracefully).
    const int maxAllowedUnderruns = totalBlocks / 10; // 10% threshold
    INFO("totalBlocks=" << totalBlocks << " underruns=" << underruns);
    REQUIRE(underruns < maxAllowedUnderruns);

    proc.terminate();
}

// Test: clock-drift controller does NOT change smoothedRatio_ when disconnected.
TEST_CASE("PluginProcessor: clock-drift controller stays nominal when disconnected",
          "[vst][processor][pipeline][clock_drift]") {
    constexpr double kDawRate = 48000.0;
    constexpr int kBlock = 256;
    constexpr int kBlocksPerSec = static_cast<int>(kDawRate) / kBlock;  // 187.5
    constexpr int kBlocksTotal = kBlocksPerSec * 3;                    // ~563 blocks for 3 s
    constexpr double kNominalRatio = 4.0;  // 48000.0 / 12000.0
    constexpr double kRatioTolerance = 0.001;

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = kDawRate;
    setup.maxSamplesPerBlock = kBlock;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Render for 3 seconds without any KiwiSDR connection.
    // The clock-drift controller must NOT change smoothedRatio_ when disconnected.
    std::array<float, kBlock> scratch{};
    for (int i = 0; i < kBlocksTotal; ++i) {
        renderBlock(proc, kBlock, scratch.data());
    }

    // The smoothed ratio must still be nominal (4.0) — no drift should occur
    // when no server is connected.
    const double actualRatio = proc.smoothedRatioForTest();
    INFO("smoothedRatioForTest=" << actualRatio << " (expected=" << kNominalRatio << ")");
    REQUIRE(actualRatio >= (kNominalRatio - kRatioTolerance));
    REQUIRE(actualRatio <= (kNominalRatio + kRatioTolerance));

    proc.terminate();
}

// Test: clock-drift CRITICAL counter stays zero during normal jitter (BUG-3).
//
// The buffer fills to ~target+150ms during steady-state streaming, which is
// normal network jitter. CRITICAL must NOT fire at this level — only at
// |bufferedMs - 300ms| > 300ms (i.e. buffer > 600ms or buffer < 0ms).
//
// With the old threshold (>150ms) the counter would be > 0 after 2 seconds.
// With the fixed threshold (>300ms) the counter stays at 0.
TEST_CASE("PluginProcessor: clock-drift CRITICAL does not fire on normal jitter",
          "[vst][processor][pipeline][clock_drift]") {
    constexpr double kSourceRate = 12000.0;
    constexpr double kDawRate    = 48000.0;
    constexpr int    kBlock      = 256;

    // SineStreamServer sends faster than real-time so the jitter buffer fills
    // to ~450ms (within the normal jitter band of ±200ms around 300ms target).
    SineStreamServer server(1000.0, kSourceRate, 1200000, /*intervalMs=*/50);

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate          = kDawRate;
    setup.maxSamplesPerBlock  = kBlock;
    setup.processMode         = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize  = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Connect to mock server.
    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 1000.0;
    state.volume  = 1.0;
    state.mute    = 0.0;
    state.agcOn   = 1.0;
    proc.applyState(state);

    // Wait for connection and buffer prefill (~500ms target).
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    std::array<float, kBlock> scratch{};

    // Warmup: render 1 second to let the clock-drift controller stabilise.
    constexpr int kWarmupBlocks = static_cast<int>(kDawRate) / kBlock; // ~187
    for (int i = 0; i < kWarmupBlocks; ++i) {
        renderBlock(proc, kBlock, scratch.data());
    }

    // Reset the CRITICAL counter, then render 2 more seconds.
    proc.resetCriticalDriftCountForTest();

    constexpr int kMeasureBlocks = 2 * static_cast<int>(kDawRate) / kBlock; // ~375
    for (int i = 0; i < kMeasureBlocks; ++i) {
        renderBlock(proc, kBlock, scratch.data());
    }

    // During normal jitter the counter must stay at zero.
    // A non-zero count means the CRITICAL threshold is too low (BUG-3).
    const int critCount = proc.criticalDriftCountForTest();
    INFO("criticalDriftCount=" << critCount
         << " (must be 0; non-zero means threshold fired on normal jitter)");
    REQUIRE(critCount == 0);

    proc.terminate();
}

// ---------------------------------------------------------------------------
// BUG-06: parameter changes after connect must be flushed to the KiwiSDR
// server (SET mod=... freq=...). Regression test for the bug where
// sendPendingParams() was only triggered once on connect, so tuning changes
// were silently dropped.
// ---------------------------------------------------------------------------
namespace {

// Text-capturing mock server: records every text frame and, on the auth frame,
// replies with `audio_rate=` so the KiwiClient handshake completes (phase 2).
class HandshakeCaptureServer {
public:
    HandshakeCaptureServer() {
        ix::initNetSystem();
        port_ = static_cast<std::uint16_t>(ix::getFreePort());
        server_ = std::make_unique<ix::WebSocketServer>(port_, "127.0.0.1");
        server_->setOnClientMessageCallback(
            [this](std::shared_ptr<ix::ConnectionState> /*state*/,
                   ix::WebSocket& socket, const ix::WebSocketMessagePtr& msg) {
                if (msg->type != ix::WebSocketMessageType::Message || msg->binary) {
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    frames_.push_back(msg->str);
                }
                if (msg->str == "SET auth t=kiwi p=") {
                    socket.sendBinary("MSG audio_rate=12000");
                }
                cv_.notify_all();
            });
        thread_ = std::thread([this] { server_->listenAndStart(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~HandshakeCaptureServer() {
        server_->stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::uint16_t port() const { return port_; }

    bool waitForFrames(std::size_t n, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, n] { return frames_.size() >= n; });
    }

    // Returns true once a captured frame contains `needle`.
    bool waitForFrameContaining(const std::string& needle, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, &needle] {
            for (const auto& f : frames_) {
                if (f.find(needle) != std::string::npos) {
                    return true;
                }
            }
            return false;
        });
    }

    std::vector<std::string> frames() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return frames_;
    }

private:
    std::uint16_t port_ = 0;
    std::unique_ptr<ix::WebSocketServer> server_;
    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::string> frames_;
};

} // namespace

// BUG-06: after connect, changing the frequency flushes a new SET mod/freq
// frame to the server.
TEST_CASE("PluginProcessor: sends updated tuning to server after connect (BUG-06)",
          "[vst][processor][pipeline][integration]") {
    HandshakeCaptureServer server;

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    constexpr int kBlock = 128;
    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = 48000.0;
    setup.maxSamplesPerBlock = kBlock;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    state.agcOn = 1.0;
    proc.applyState(state);

    // Wait for the initial handshake (options/auth/AR OK/squelch/genattn/gen/
    // mod/freq/agc/keepalive => 9 frames).
    REQUIRE(server.waitForFrames(9, std::chrono::seconds(5)));

    // Change the frequency (plain kHz -> normalized) and render a block to
    // trigger the rate-limited flush in process().
    const double newFreqKhz = 14021.5;
    proc.applyParamValue(netsdr::kParamFreqKhz,
                         proc.registry().toNormalized(netsdr::kParamFreqKhz, newFreqKhz));
    std::array<float, kBlock> scratch{};
    renderBlock(proc, kBlock, scratch.data());

    // The server must receive a new SET mod frame carrying the updated freq.
    REQUIRE(server.waitForFrameContaining("freq=14021.500", std::chrono::seconds(5)));

    proc.terminate();
}

// BUG-07: the local volume gain must actually scale the audio output.
TEST_CASE("PluginProcessor: volume gain scales the output (BUG-07)",
          "[vst][processor][pipeline][integration]") {
    constexpr double kSourceRate = 12000.0;
    constexpr double kDawRate = 48000.0;
    constexpr int kBlock = 128;

    SineStreamServer server(1000.0, kSourceRate, 1200000);

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    state.agcOn = 1.0;
    proc.applyState(state);

    {
        bool streaming = false;
        for (int attempt = 0; attempt < 100 && !streaming; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            streaming = server.framesSent() > 0;
        }
        REQUIRE(streaming);
    }

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = kDawRate;
    setup.maxSamplesPerBlock = kBlock;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Warm up so the jitter buffer has audible samples.
    std::array<float, kBlock> scratch{};
    for (int i = 0; i < 50; ++i) {
        for (int b = 0; b < 50; ++b) {
            renderBlock(proc, kBlock, scratch.data());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Render ~0.4 s of audio at near-real-time pace and report the peak. The
    // mock server streams at ~real-time, so pacing the render lets the jitter
    // buffer stay non-empty (a single block could hit a momentary underrun).
    auto maxPeak = [&]() {
        float p = 0.0f;
        for (int i = 0; i < 200; ++i) {
            renderBlock(proc, kBlock, scratch.data());
            for (float s : scratch) {
                p = (std::max)(p, std::fabs(s));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return p;
    };

    // Baseline: audible output at volume = 1.
    const float peakAtFull = maxPeak();
    REQUIRE(peakAtFull > 0.0f);

    // Volume -> 0: the output must go silent immediately (local gain).
    proc.applyParamValue(netsdr::kParamVolume, 0.0);
    for (int i = 0; i < 5; ++i) {
        renderBlock(proc, kBlock, scratch.data());
        for (float s : scratch) {
            REQUIRE(s == 0.0f);
        }
    }

    // Volume back to 1: audio returns.
    proc.applyParamValue(netsdr::kParamVolume, 1.0);
    REQUIRE(maxPeak() > 0.0f);

    proc.terminate();
}

TEST_CASE("PluginProcessor: S-meter level (RMS->dBm) tracks a full-scale tone (M4.6)",
          "[vst][processor][pipeline][smetert]") {
    constexpr double kSourceRate = 12000.0;
    constexpr double kDawRate = 48000.0;
    constexpr int kBlock = 128;
    // Full-scale 1000 Hz sine (amplitude 32767 in the mock server).
    SineStreamServer server(1000.0, kSourceRate, 1200000);

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    proc.applyState(state);

    {
        bool streaming = false;
        for (int attempt = 0; attempt < 100 && !streaming; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            streaming = server.framesSent() > 0;
        }
        REQUIRE(streaming);
    }

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = kDawRate;
    setup.maxSamplesPerBlock = kBlock;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Warm up and render until the jitter buffer actually delivers audio
    // (deterministic regardless of connection latency; the full suite may be
    // slower than the isolated run). Poll the output peak.
    std::array<float, kBlock> scratch{};
    bool heardAudio = false;
    for (int attempt = 0; attempt < 40 && !heardAudio; ++attempt) {
        for (int b = 0; b < 50; ++b) {
            renderBlock(proc, kBlock, scratch.data());
            for (float s : scratch) {
                if (std::fabs(s) > 1e-3f) {
                    heardAudio = true;
                    break;
                }
            }
            if (heardAudio) break;
        }
        if (!heardAudio) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    REQUIRE(heardAudio); // the pipeline must deliver audible audio

    // Full-scale sine -> rms = 32767/sqrt(2)/32768 ~= 0.707 -> +7 dBm
    // (20*log10(0.707)+10). Tolerate resampler transients: assert well above
    // the -140 dBm floor and in a plausible band around +7 dBm.
    const float dbm = proc.signalLevelDbMForTest();
    REQUIRE(dbm > -100.0f);
    REQUIRE(dbm >= -10.0f);
    REQUIRE(dbm <= 20.0f);

    // Silence (mute) drops the level back toward the floor.
    proc.applyParamValue(netsdr::kParamVolume, 0.0);
    for (int i = 0; i < 20; ++i) {
        renderBlock(proc, kBlock, scratch.data());
    }
    REQUIRE(proc.signalLevelDbMForTest() < -100.0f);

    proc.terminate();
}

TEST_CASE("PluginProcessor: waterfall spectrum frames flow to the UI callback (M4.7)",
          "[vst][processor][pipeline][waterfall]") {
    constexpr double kSourceRate = 12000.0;
    constexpr double kDawRate = 48000.0;
    constexpr int kBlock = 128;
    // A strong 1000 Hz tone from the mock server.
    SineStreamServer server(1000.0, kSourceRate, 1200000);

    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    netsdr::ProcessorState state;
    state.station = "127.0.0.1:" + std::to_string(server.port());
    state.freqKhz = 14100.0;
    state.volume = 1.0;
    state.mute = 0.0;
    proc.applyState(state);

    // Collect waterfall frames via the UI callback (what the editor binds).
    std::vector<std::vector<float>> received;
    proc.setOnWaterfall([&received](const std::vector<float>& bins) {
        received.push_back(bins);
    });

    {
        bool streaming = false;
        for (int attempt = 0; attempt < 100 && !streaming; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            streaming = server.framesSent() > 0;
        }
        REQUIRE(streaming);
    }

    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = kDawRate;
    setup.maxSamplesPerBlock = kBlock;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Render for a while so the 10 Hz waterfall limiter fires frames.
    std::array<float, kBlock> scratch{};
    for (int i = 0; i < 60; ++i) {
        for (int b = 0; b < 30; ++b) {
            renderBlock(proc, kBlock, scratch.data());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    REQUIRE(!received.empty());
    const auto& first = received.front();
    REQUIRE(first.size() == 256); // binCount for a 512-sample window
    for (float b : first) {
        REQUIRE(b >= -160.0f);
        REQUIRE(b <= 0.0f);
    }
    // A 1000 Hz tone at 48 kHz is bin 1000*512/48000 ~= 10.7 -> peak near bin
    // 10-11. Find the peak and allow the first captured frame a tolerance
    // window (spectrum stabilises over frames).
    float maxPeak = -160.0f;
    std::size_t peakBin = 0;
    for (const auto& frame : received) {
        for (std::size_t i = 0; i < frame.size(); ++i) {
            if (frame[i] > maxPeak) {
                maxPeak = frame[i];
                peakBin = i;
            }
        }
    }
    REQUIRE(maxPeak > -40.0f); // a strong tone must be clearly visible
    REQUIRE(peakBin >= 5);
    REQUIRE(peakBin <= 20);

    proc.terminate();
}
