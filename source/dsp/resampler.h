#pragma once
#include <cstddef>
#include <vector>
#include <cstring>
// Resampler: high-quality sample-rate conversion using libsamplerate (Secret Rabbit Code).
// Converts audio between sample rates (e.g. KiwiSDR 12 kHz / 24 kHz -> DAW rate).
// Quality is selectable: MEDIUM (default, good quality/perf) or BEST (highest quality).
// Real-time safe: all allocation happens in the constructor; process()/reset()
// never heap-allocate on the audio thread.
//
// kMaxStagingFrames limits the internal staging buffer; compaction moves
// unconsumed data to the front with std::memmove (no reallocation).
//
// Streaming pattern:
//   - Each process() call appends input to a pre-reserved staging buffer,
//     then loops src_process() until the output buffer is full or all input
//     is consumed and no more output can be generated.
//   - Unconsumed input frames are preserved across process() calls via
//     a bounded internal buffer indexed by stagingHead_/stagingSize_.
//
// Class is mono (1 channel); float sample type.
namespace netsdr {

class Resampler {
public:
    // Maximum number of frames the internal staging buffer can hold.
    // Also the output scratch size used in the production audio path (renderPipeline).
    static constexpr std::size_t kMaxStagingFrames = 4096;

    // Resampler quality levels.
    enum class Quality {
        Medium,  // SRC_SINC_MEDIUM_QUALITY (~-60dB, fast)
        Best     // SRC_SINC_BEST_QUALITY (~-120dB, slower)
    };

    // Creates the SRC state with the specified quality, 1 channel.
    // If src_new fails, stores an error flag; isValid() will return false.
    // Allocation of the staging buffer happens here; process()/reset() are allocation-free.
    explicit Resampler(double inputRate, double outputRate, Quality quality = Quality::Medium) noexcept;
    ~Resampler();

    // True if the SRC state was created successfully.
    bool isValid() const noexcept;

    // Clear the internal SRC state (useful for re-initialisation).
    // Does not reallocate; just resets head/count and preserves buffer capacity.
    void reset();

    // Feed numInFrames input samples, produce up to maxOutFrames output samples.
    // Returns the number of output frames actually produced.
    // Accumulates input into the bounded staging buffer; preserves unconsumed
    // frames across calls. May call src_process multiple times per process().
    // Allocation-free in steady state on the audio path.
    std::size_t process(const float* in, std::size_t numInFrames,
                        float* out, std::size_t maxOutFrames);

    // Sample-rate accessors.
    double inputRate() const noexcept;
    double outputRate() const noexcept;

    // Dynamic ratio adjustment for clock-drift compensation.
    // ratio: actual output/input ratio (e.g. 48000.0/11998.9 instead of 48000.0/12000.0).
    // Clamped to [0.5*nominal, 2.0*nominal] for safety.
    void setRatio(double ratio) noexcept;
    double currentRatio() const noexcept;

private:
    // Opaque libsamplerate state handle (SRC_STATE*). Kept as void* so the
    // header does not need the C header; the .cpp casts to SRC_STATE*.
    double input_rate_;
    double output_rate_;
    double current_ratio_;  // Dynamic ratio for clock-drift compensation
    void* state_ = nullptr;
    bool valid_ = false;

    // Bounded staging buffer for streaming input across process() calls.
    // Pre-allocated once in the constructor with kMaxStagingFrames elements;
    // process()/reset() never reallocate.
    std::vector<float> input_buffer_;
    // Index within input_buffer_ of the first unconsumed frame.
    std::size_t stagingHead_ = 0;
    // Number of valid frames staged, starting from stagingHead_.
    // Valid range is [stagingHead_, stagingHead_ + stagingSize_).
    std::size_t stagingSize_ = 0;

    // Disable copying / assignment.
    Resampler(const Resampler&) = delete;
    Resampler& operator=(const Resampler&) = delete;
};

} // namespace netsdr