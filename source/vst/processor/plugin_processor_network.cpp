// Network-side implementation of PluginProcessor (M3.7 refactoring):
//   - sendPendingParams()  (push dirty params to the KiwiSDR server)
//   - decodeAndQueue()     (SND frame -> IMA ADPCM decode -> audio queue)
//   - connectToStation()   (KiwiClient setup + handshake wiring)
//   - disconnectStation()  (clean disconnect without auto-reconnect)

#include "plugin_processor.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "util/file_logger.h"
#include "vst/common/paramdefinitions.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string>

namespace netsdr {

// ---------------------------------------------------------------------------
// sendPendingParams — worker thread
// ---------------------------------------------------------------------------

void PluginProcessor::sendPendingParams() {
    if (!kiwiClient_ || !kiwiClient_->isConnected()) {
        return;
    }
    if (!paramsDirty_.exchange(false)) {
        return;
    }

    const int modeIdx = mode_.load();
    const std::string modeStr =
        (modeIdx >= 0 && modeIdx < kNumKiwiModes) ? kKiwiModeNames[modeIdx] : kKiwiModeNames[0];

    kiwiClient_->setTuning(modeStr, lowCut_.load(), highCut_.load(), freqKhz_.load());
    kiwiClient_->setAgc(agcOn_.load(), agcHang_.load() > 0, agcThresh_.load(),
                        agcSlope_.load(), agcDecay_.load(), agcManGain_.load());
    kiwiClient_->setSquelch(squelchOn_.load(), squelchThr_.load());
    kiwiClient_->setNb(nbOn_.load(), nbThresh_.load());
    kiwiClient_->setNr(nrOn_.load());
    kiwiClient_->setDeemp(deempOn_.load());
    kiwiClient_->setComp(compOn_.load());
}

// ---------------------------------------------------------------------------
// decodeAndQueue — network thread (KiwiClient binary callback)
// ---------------------------------------------------------------------------

void PluginProcessor::decodeAndQueue(const std::string& data) {
    static std::atomic<int> frameCount{0};
    const int currentFrame = frameCount.fetch_add(1);
    const bool shouldLog   = (currentFrame % 100) == 0;

    if (data.empty()) {
        NETSDR_LOG_INFO("decodeAndQueue: empty data received (frame %d)", currentFrame);
        return;
    }
    if (!adpcmDecoder_) {
        NETSDR_LOG_INFO("decodeAndQueue: CRITICAL - no adpcmDecoder (frame %d)", currentFrame);
        return;
    }

    const auto* bytes  = reinterpret_cast<const std::uint8_t*>(data.data());
    const std::size_t numBytes = data.size();

    // KiwiSDR SND frame header: 10 bytes (id[3] + flags[1] + seq[4] + smeter[2]).
    // Audio payload starts at offset 10.
    static constexpr std::size_t kSndHeaderSize   = 10;
    static constexpr std::uint8_t kSndFlagCompressed = 0x10;
    static constexpr std::uint8_t kSndFlagIq         = 0x08;

    const std::uint8_t flags  = numBytes > 3 ? bytes[3] : 0;
    const std::size_t  offset = kSndHeaderSize;

    if (shouldLog) {
        NETSDR_LOG_DEBUG("decodeAndQueue: frame %d, numBytes=%zu, flags=0x%02X",
                         currentFrame, numBytes, flags);
    }

    // IQ mode carries raw I/Q samples, not ADPCM — skip.
    if ((flags & kSndFlagIq) != 0) {
        if (currentFrame == 0) {
            NETSDR_LOG_INFO("decodeAndQueue: IQ mode detected, audio pipeline disabled");
        }
        return;
    }
    // Uncompressed PCM is unexpected; the handshake always requests ADPCM.
    if ((flags & kSndFlagCompressed) == 0) {
        if (currentFrame == 0) {
            NETSDR_LOG_INFO("decodeAndQueue: uncompressed mode, expected ADPCM");
        }
        return;
    }

    // Decode the ADPCM payload into AudioSampleBlocks and push to the queue.
    AudioSampleBlock block;
    std::size_t pos = offset;
    int blocksDecoded = 0;
    int blocksPushed  = 0;

    while (pos < numBytes) {
        const std::size_t chunk = (std::min)(
            numBytes - pos,
            AudioSampleBlock::kMaxSamples / 2);  // 2 nibbles per byte → 2 samples

        adpcmDecoder_->decode(bytes + pos, chunk, block.samples.data());
        block.sampleCount = chunk * 2;
        block.sequence    = sequence_.fetch_add(1);
        blocksDecoded++;

        // Sequence gap detection.
        const std::uint32_t expectedSeq = lastSequence_ + 1;
        if (lastSequence_ != 0 && block.sequence != expectedSeq) {
            telemetry_.sequenceGaps.fetch_add(1);
        }
        lastSequence_ = block.sequence;

        // Bounded push: drop if queue is full (real-time safe, no blocking).
        const std::size_t queueSizeBefore = audioQueue_.sizeApprox();
        audioQueue_.tryPush(block);
        const std::size_t queueSizeAfter  = audioQueue_.sizeApprox();
        if (queueSizeAfter == queueSizeBefore) {
            const int overflow = static_cast<int>(telemetry_.overflows.fetch_add(1)) + 1;
            telemetry_.droppedBlocks.fetch_add(1);
            if (overflow == 1) {
                NETSDR_LOG_INFO("decodeAndQueue: queue FULL, dropping blocks (queueSize=%zu)",
                                queueSizeBefore);
            } else if (shouldLog) {
                NETSDR_LOG_DEBUG("decodeAndQueue: queue FULL, dropped block (overflow=%d)", overflow);
            }
        } else {
            blocksPushed++;
        }
        pos += chunk;
    }

    telemetry_.queueDepth.store(static_cast<std::int32_t>(audioQueue_.sizeApprox()));

    if (shouldLog) {
        NETSDR_LOG_DEBUG("decodeAndQueue: frame %d decoded %d blocks, pushed %d",
                         currentFrame, blocksDecoded, blocksPushed);
    }
    if (blocksDecoded > 0 && blocksPushed == 0 && currentFrame < 10) {
        NETSDR_LOG_INFO("decodeAndQueue: CRITICAL - decoded %d blocks but pushed 0 (frame %d)",
                        blocksDecoded, currentFrame);
    }
}

// ---------------------------------------------------------------------------
// connectToStation — worker thread
// ---------------------------------------------------------------------------

void PluginProcessor::connectToStation(const std::string& hostPort) {
    if (hostPort.empty()) {
        return;
    }

    // Parse "host:port".
    std::string host;
    std::uint16_t port = kiwiDefaultPort;
    const auto colon = hostPort.rfind(':');
    if (colon != std::string::npos) {
        host = hostPort.substr(0, colon);
        try {
            port = static_cast<std::uint16_t>(std::stoul(hostPort.substr(colon + 1)));
        } catch (...) {
            port = kiwiDefaultPort;
        }
    } else {
        host = hostPort;
    }

    {
        std::lock_guard<std::mutex> lock(stationMutex_);
        station_ = hostPort;
    }

    NETSDR_LOG_INFO("Connecting to station: %s", hostPort.c_str());
    emitStatus("Connecting");

    // Disconnect any existing client first.
    if (kiwiClient_) {
        kiwiClient_->disconnect();
        kiwiClient_.reset();
    }

    KiwiClientConfig cfg;
    cfg.host      = host;
    cfg.port      = port;
    cfg.freqKhz   = freqKhz_.load();
    const int modeIdx = mode_.load();
    cfg.mode = (modeIdx >= 0 && modeIdx < kNumKiwiModes)
               ? kKiwiModeNames[modeIdx]
               : kKiwiModeNames[0];
    cfg.lowCut    = lowCut_.load();
    cfg.highCut   = highCut_.load();
    cfg.agcOn     = agcOn_.load();
    cfg.sampleRate = serverSampleRate_.load();

    kiwiClient_ = std::make_unique<KiwiClient>();

    kiwiClient_->setOnBinaryMessage([this](const std::string& data) {
        decodeAndQueue(data);
    });

    kiwiClient_->setOnOpen([this]() {
        NETSDR_LOG_INFO("Station connected");
        emitStatus("Connected");
        resetPipelineFlag_.store(true);
        paramsDirty_.store(true);
        worker_.post([this]() { sendPendingParams(); });
    });

    kiwiClient_->setOnError([this]() {
        NETSDR_LOG_INFO("Station connection error");
        emitStatus("Error");
    });

    kiwiClient_->setOnClose([this]() {
        NETSDR_LOG_INFO("Station disconnected");
        emitStatus("Disconnected");
    });

    if (!kiwiClient_->connect(cfg)) {
        NETSDR_LOG_INFO("Failed to start connection to %s", hostPort.c_str());
        emitStatus("Error");
        kiwiClient_.reset();
    }
}

// ---------------------------------------------------------------------------
// disconnectStation — worker thread
// ---------------------------------------------------------------------------

void PluginProcessor::disconnectStation() {
    // Destroying the client disconnects cleanly: KiwiClient's destructor sets
    // `destroying_` before closing, so the auto-reconnect path does NOT fire.
    if (kiwiClient_) {
        kiwiClient_.reset();
    }
    {
        std::lock_guard<std::mutex> lock(stationMutex_);
        station_.clear();
    }
    // Flush buffered audio so the output goes silent immediately.
    resetPipelineFlag_.store(true);
    NETSDR_LOG_INFO("Station disconnected by user");
    emitStatus("Disconnected");
}

} // namespace netsdr
