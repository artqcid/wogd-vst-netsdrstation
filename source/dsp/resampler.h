#pragma once
#include <cstddef>
#include <vector>
// Resampler: high-quality sample-rate conversion using libsamplerate (Secret Rabbit Code).
// Converts audio between sample rates (e.g. KiwiSDR 12 kHz / 24 kHz -> DAW rate).
// Uses SRC_SINC_MEDIUM_QUALITY for a good quality/perf trade-off in real-time.
// NOT real-time safe: allocations may occur in process(); runs on network/worker thread.
//
// Streaming pattern:
//   - Each process() call appends input to an internal staging buffer,
//     then loops src_process() until the output buffer is full or all input
//     is consumed and no more output can be generated.
//   - Unconsumed input frames are preserved across process() calls via
//     an internal std::vector<float> input buffer.
//
// Class is mono (1 channel); float sample type.
namespace netsdr {

class Resampler {
public:
    // Creates the SRC state with SRC_SINC_MEDIUM_QUALITY, 1 channel.
    // If src_new fails, stores an error flag; isValid() will return false.
    Resampler(double inputRate, double outputRate) noexcept;
    ~Resampler();

    // True if the SRC state was created successfully.
    bool isValid() const noexcept;

    // Clear the internal SRC state (useful for re-initialisation).
    void reset();

    // Feed numInFrames input samples, produce up to maxOutFrames output samples.
    // Returns the number of output frames actually produced.
    // Accumulates input into an internal staging buffer; preserves unconsumed
    // frames across calls. May call src_process multiple times per process().
    std::size_t process(const float* in, std::size_t numInFrames,
                        float* out, std::size_t maxOutFrames);

    // Sample-rate accessors.
    double inputRate() const noexcept;
    double outputRate() const noexcept;

private:
    // Opaque libsamplerate state handle (SRC_STATE*). Kept as void* so the
    // header does not need the C header; the .cpp casts to SRC_STATE*.
    double input_rate_;
    double output_rate_;
    void* state_ = nullptr;
    bool valid_ = false;

    // Internal staging buffer for streaming input across process() calls.
    std::vector<float> input_buffer_;
    std::size_t input_offset_ = 0;  // index into input_buffer_ of first unconsumed frame

    // Disable copying / assignment.
    Resampler(const Resampler&) = delete;
    Resampler& operator=(const Resampler&) = delete;
};

} // namespace netsdr