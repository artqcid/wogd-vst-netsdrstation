#include "catch.hpp"
#include "vst/processor/plugin_processor.h"
#include "vst/common/paramdefinitions.h"
#include "vst/common/paramids.h"
#include "vst/common/parameter_registry.h"
#include "vst/common/processor_state.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/common/memorystream.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

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

TEST_CASE("PluginProcessor: applyParamValue stores normalized values (TEST-03)",
          "[vst][processor]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    netsdr::ParameterRegistry registry(netsdr::createParameterDefinitions());

    // Store frequency 7100.5 kHz via applyParamValue
    double normFreq = registry.toNormalized(netsdr::kParamFreqKhz, 7100.5);
    proc.applyParamValue(netsdr::kParamFreqKhz, normFreq);

    // Store volume, mute, mode
    proc.applyParamValue(netsdr::kParamVolume, 0.5);
    proc.applyParamValue(netsdr::kParamMute, 1.0);
    proc.applyParamValue(netsdr::kParamMode, registry.toNormalized(netsdr::kParamMode, 3));

    // Roundtrip via getState / deserialize to verify values are stored
    Steinberg::MemoryStream outStream;
    REQUIRE(proc.getState(&outStream) == Steinberg::kResultOk);
    REQUIRE(outStream.getSize() > 8); // verify state writes bytes

    // Deserialize and verify
    netsdr::ProcessorState state;
    REQUIRE(state.deserialize(std::string(
        static_cast<const char*>(outStream.getData()), outStream.getSize())));

    // Verify roundtripped values
    // Note: freqKhz is stored as plain kHz value
    CHECK(std::abs(state.freqKhz - 7100.5) < 1.0);
    CHECK(std::abs(state.volume - 0.5) < 0.01);
    CHECK(state.mute == 1.0);
    CHECK(std::abs(state.mode - 3.0) < 0.1);
    proc.terminate();
}

TEST_CASE("PluginProcessor: process sets silenceFlags when muted (TEST-04)",
          "[vst][processor]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Unmute: apply volume=1.0 to get audible output path
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
    proc.terminate();
}

TEST_CASE("PluginProcessor: process outputs silence without a connection (no crash)",
          "[vst][processor]") {
    netsdr::PluginProcessor proc;
    proc.initialize(nullptr);

    // Setup processing at 48kHz (no connection → audio is pure silence)
    Steinberg::Vst::ProcessSetup setup;
    setup.sampleRate = 48000.0;
    setup.maxSamplesPerBlock = 128;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    proc.setupProcessing(setup);

    // Run ~10 process() blocks; with no connection all samples must be 0.0f
    // and silenceFlags must be 0 (mute off, but no audio source)
    for (int i = 0; i < 10; ++i) {
        Steinberg::Vst::ProcessData data{};
        data.processMode = Steinberg::Vst::kRealtime;
        data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.numSamples = 128;
        data.numOutputs = 1;
        data.numInputs = 0;

        Steinberg::Vst::AudioBusBuffers outBus;
        outBus.numChannels = 1;
        outBus.silenceFlags = 0;
        float* outPtr = new float[128]();
        outBus.channelBuffers32 = &outPtr;
        data.outputs = &outBus;

        proc.process(data);

        // Verify all samples are 0.0f
        float* outChan = outPtr;
        for (Steinberg::int32 j = 0; j < 128; ++j) {
            CHECK(outChan[j] == 0.0f);
        }
        CHECK(data.outputs[0].silenceFlags == 0);
    }
    proc.terminate();
}

// Goertzel helper (kept for potential use by other tests)
namespace {
double goertzelMagnitude(const std::vector<float>& samples, double freq, double sampleRate) {
    const double w = 2.0 * 3.14159265358979323846 * freq / sampleRate;
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
} // namespace