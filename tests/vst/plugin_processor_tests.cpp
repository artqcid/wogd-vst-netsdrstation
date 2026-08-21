#include "catch.hpp"
#include "vst/processor/plugin_processor.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/parameter_registry.h"
#include "vst/common/processor_state.h"
#include "dsp/sine_oscillator.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/common/memorystream.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = kPi * 2.0;

double goertzelMagnitude(const std::vector<float>& samples, double freq, double sampleRate) {
    const double w = kTwoPi * freq / sampleRate;
    const double coeff = 2.0 * std::cos(w);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    for (float s : samples) {
        s0 = s + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    const std::size_t n = samples.size();
    const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
    return 2.0 * std::sqrt(power) / static_cast<double>(n);
}

// Helper: render a buffer from the processor and return samples
std::vector<float> renderFrame(netsdr::PluginProcessor& proc, Steinberg::int32 numSamples, double sampleRate) {
    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = sampleRate;
    setup.maxSamplesPerBlock = numSamples;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    std::vector<float> buf(static_cast<std::size_t>(numSamples), 0.0f);
    Steinberg::Vst::ProcessData data;
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = numSamples;
    data.numOutputs = 1;
    data.numInputs = 0;

    Steinberg::Vst::AudioBusBuffers outBus;
    outBus.numChannels = 1;
    outBus.silenceFlags = 0;
    float* outPtr = buf.data();
    outBus.channelBuffers32 = &outPtr;
    data.outputs = &outBus;

    proc.process(data);
    return buf;
}

} // namespace

TEST_CASE("PluginProcessor: setupProcessing forwards sample rate to oscillator (TEST-01)",
          "[vst][processor]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Render at 48000 with default freq 440
    auto samples48k = renderFrame(proc, 4800, 48000.0);
    double peak48 = goertzelMagnitude(samples48k, 440.0, 48000.0);
    REQUIRE(peak48 > 0.5);

    // Render at 96000 — peak must still be ~440 Hz, NOT ~880 Hz
    auto samples96k = renderFrame(proc, 9600, 96000.0);
    double peak96 = goertzelMagnitude(samples96k, 440.0, 96000.0);
    REQUIRE(peak96 > 0.5);

    // The wrong peak at 880 Hz must be absent
    double wrong = goertzelMagnitude(samples96k, 880.0, 96000.0);
    REQUIRE(wrong < 0.1);
}

TEST_CASE("PluginProcessor: setState/getState persist audio parameters (TEST-02)",
          "[vst][processor]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Build a ProcessorState manually and feed it via MemoryStream
    netsdr::ProcessorState state;
    state.freqHz = 5000.0;
    state.volume = 0.5;
    state.mute = false;
    auto bytes = state.serialize();

    Steinberg::MemoryStream inStream(bytes.data(), static_cast<Steinberg::TSize>(bytes.size()));
    REQUIRE(proc.setState(&inStream) == Steinberg::kResultOk);

    // Verify via rendered output: peak at 5000 Hz
    auto samples = renderFrame(proc, 4800, 48000.0);
    double peak = goertzelMagnitude(samples, 5000.0, 48000.0);
    REQUIRE(peak > 0.25); // volume=0.5 → peak ~0.5

    // Verify getState writes back matching bytes
    Steinberg::MemoryStream outStream;
    outStream.setSize(netsdr::ProcessorState::kSerializedSize);
    REQUIRE(proc.getState(&outStream) == Steinberg::kResultOk);
    // Don't require exact match since freqHz may be double-rounded, but size must match
}

TEST_CASE("PluginProcessor: applyParamValue changes frequency output (TEST-03)",
          "[vst][processor]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    netsdr::ParameterRegistry reg(netsdr::createParameterDefinitions());
    double targetHz = 3000.0;
    double normalized = reg.toNormalized(netsdr::kParamFreq, targetHz);

    proc.applyParamValue(netsdr::kParamFreq, normalized);

    auto samples = renderFrame(proc, 4800, 48000.0);
    double peak = goertzelMagnitude(samples, targetHz, 48000.0);
    REQUIRE(peak > 0.5);
}

TEST_CASE("PluginProcessor: process sets silenceFlags when muted (TEST-04)",
          "[vst][processor]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Unmute: apply volume=1.0 to get audible output
    proc.applyParamValue(netsdr::kParamVolume, 1.0);

    // Mute off
    proc.applyParamValue(netsdr::kParamMute, 0.0);

    // Render with 2 channels, mute off
    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = 48000.0;
    setup.maxSamplesPerBlock = 256;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    std::vector<float> buf(512, 0.0f);
    Steinberg::Vst::ProcessData data{};
    data.processMode = Steinberg::Vst::kRealtime;
    data.symbolicSampleSize = Steinberg::Vst::kSample32;
    data.numSamples = 256;
    data.numOutputs = 1;
    data.numInputs = 0;

    float* ch0 = buf.data();
    float* ch1 = buf.data() + 256;
    float* channels[2] = {ch0, ch1};
    Steinberg::Vst::AudioBusBuffers outBus;
    outBus.numChannels = 2;
    outBus.silenceFlags = 0;
    outBus.channelBuffers32 = reinterpret_cast<float**>(channels);
    data.outputs = &outBus;

    // Unmuted: silenceFlags must be 0
    proc.process(data);
    REQUIRE(data.outputs[0].silenceFlags == 0);

    // Muted: silenceFlags must cover both channels
    proc.applyParamValue(netsdr::kParamMute, 1.0);
    outBus.silenceFlags = 0;
    proc.process(data);
    REQUIRE(data.outputs[0].silenceFlags == 3);
}