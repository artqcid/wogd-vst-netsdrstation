#include "dsp/resampler.h"

#include <cmath>
#include <vector>

extern "C" {
#include <samplerate.h>
}

namespace netsdr {

// ---------------------------------------------------------------------------
// Resampler implementation
// ---------------------------------------------------------------------------

Resampler::Resampler(double inputRate, double outputRate) noexcept
    : input_rate_(inputRate),
      output_rate_(outputRate) {
    // Include the C header with extern "C" already handled in the header via
    // the struct SRApi / SRC_STATE forward-declaration trick, but we need the
    // actual C function declarations. Since samplerate.h is a C header, we
    // include it here with extern "C" to avoid C++ name mangling.
    // The header resampler.h does NOT include samplerate.h directly (it forward-
    // declares the types) to keep it C++-clean; the .cpp includes it.

    int error = 0;
    // SRC_SINC_MEDIUM_QUALITY: good quality / perf trade-off for real-time.
    // 1 channel (mono).
    state_ = src_new(SRC_SINC_MEDIUM_QUALITY, 1, &error);
    valid_ = (error == 0);
    (void)inputRate;   // suppress unused-parameter warning if needed
    (void)outputRate;
}

Resampler::~Resampler() {
    if (state_ != nullptr) {
        src_delete(static_cast<SRC_STATE*>(state_));
    }
}

bool Resampler::isValid() const noexcept {
    return valid_;
}

void Resampler::reset() {
    if (state_ != nullptr) {
        src_reset(static_cast<SRC_STATE*>(state_));
    }
    // Also clear the internal staging buffer so no stale input persists.
    input_buffer_.clear();
    input_offset_ = 0;
}

std::size_t Resampler::process(const float* in, std::size_t numInFrames,
                               float* out, std::size_t maxOutFrames) {
    // Append new input samples to the internal staging buffer.
    // This allows unconsumed frames to persist across process() calls.
    input_buffer_.insert(input_buffer_.end(), in, in + numInFrames);

    std::size_t total_out = 0;

    // Loop calling src_process until the output buffer is full or no more
    // output can be generated.  The standard streaming pattern:
    //   - Set up SRC_DATA pointing at the current staging-buffer offset.
    //   - Call src_process().  It may consume some input and produce some output.
    //   - Advance the offset by input_frames_used, and the output pointer
    //     by output_frames_gen.
    //   - Repeat until output is full or all input is consumed and drained.
    while (total_out < maxOutFrames) {
        // Calculate how many unconsumed frames are available in the staging buffer.
        std::size_t available = input_buffer_.size() - input_offset_;
        if (available == 0) {
            // No more input to process.  Try one more src_process call with
            // input_frames = 0 to drain any remaining internal filter state
            // (this can produce a few final output samples).
            SRC_DATA data{};
            data.src_ratio = output_rate_ / input_rate_;
            data.data_in = nullptr;
            data.input_frames = 0;
            data.data_out = out + total_out;
            data.output_frames = static_cast<long>(maxOutFrames - total_out);
            data.end_of_input = 0;
src_process(static_cast<SRC_STATE*>(state_), &data);
            size_t gen = static_cast<std::size_t>(data.output_frames_gen);
            if (gen > 0) {
                total_out += gen;
            }
            break;
        }

        SRC_DATA data{};
        data.src_ratio = output_rate_ / input_rate_;
        data.data_in = input_buffer_.data() + static_cast<long>(input_offset_);
        data.input_frames = static_cast<long>(available);
        data.data_out = out + static_cast<long>(total_out);
        data.output_frames = static_cast<long>(maxOutFrames - static_cast<long>(total_out));
        data.end_of_input = 0;   // may give more input later

        src_process(static_cast<SRC_STATE*>(state_), &data);

        std::size_t gen = static_cast<std::size_t>(data.output_frames_gen);
        std::size_t used = static_cast<std::size_t>(data.input_frames_used);

        if (gen == 0) {
            // No output generated this iteration.
            // This can happen during the initial filter transient if we haven't
            // supplied enough input yet, or if the output buffer was already full.
            // If we have input remaining but no output, it may be the transient;
            // break to avoid an infinite loop – the caller may call process() again
            // (e.g. with end-of-input semantics) to drain the remaining output.
            if (input_offset_ >= input_buffer_.size()) {
                // All input consumed and no output – nothing more to do.
                break;
            }
            // Otherwise we have input but no output yet (initial transient).
            // Break here; further output may be obtained by calling process()
            // again or by setting end_of_input downstream.
            break;
        }

        total_out += gen;
        input_offset_ += used;

        // If we have consumed all input that was available, we keep the loop
        // going; src_process may still produce output from its internal delay
        // line even with no new input (the "drain" phase).  The loop condition
        // (total_out < maxOutFrames) will stop us when the output buffer fills.
    }

    // --- Compact the staging buffer ---
    // Remove consumed frames from the front so the buffer does not grow
    // unboundedly.  We move the remaining (unconsumed) input to the front.
    if (input_offset_ > 0) {
        // If there is remaining input, shift it to the beginning.
        if (input_offset_ < input_buffer_.size()) {
            std::move(input_buffer_.begin() + static_cast<long>(input_offset_),
                      input_buffer_.end(),
                      input_buffer_.begin());
        }
        input_buffer_.resize(static_cast<std::size_t>(input_buffer_.size() - input_offset_));
        input_offset_ = 0;
    }

    return total_out;
}

double Resampler::inputRate() const noexcept {
    return input_rate_;
}

double Resampler::outputRate() const noexcept {
    return output_rate_;
}

// ---------------------------------------------------------------------------
// Goertzel helper (inline, defined in the header for convenience)
// ---------------------------------------------------------------------------

} // namespace netsdr