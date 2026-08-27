#include "dsp/resampler.h"

#include <cmath>
#include <cstring>

extern "C" {
#include <samplerate.h>
}

namespace netsdr {
// ---------------------------------------------------------------------------
// Resampler implementation
// ---------------------------------------------------------------------------

Resampler::Resampler(double inputRate, double outputRate, Quality quality) noexcept
    : input_rate_(inputRate),
      output_rate_(outputRate),
      current_ratio_(outputRate / inputRate),
      input_buffer_(kMaxStagingFrames),
      stagingHead_(0),
      stagingSize_(0) {
    // Select converter type based on quality parameter.
    const int converterType = (quality == Quality::Best)
        ? SRC_SINC_BEST_QUALITY
        : SRC_SINC_MEDIUM_QUALITY;
    int error = 0;
    state_ = src_new(converterType, 1, &error);
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
    // Reset staging indices; buffer capacity is preserved (no reallocation).
    stagingHead_ = 0;
    stagingSize_ = 0;
}

std::size_t Resampler::process(const float* in, std::size_t numInFrames,
                               float* out, std::size_t maxOutFrames) {
    std::size_t total_out = 0;

    // ---------- Phase 1: Feed new input into the bounded staging buffer ----------
    // Loop until all input is consumed or the output buffer fills.
    // The staging buffer is fixed-capacity (kMaxStagingFrames); we compact
    // (in-place memmove, NO allocation) whenever unconsumed frames sit behind a
    // non-zero head, so the write position is always exactly stagingSize_ and
    // the free space is exactly kMaxStagingFrames - stagingSize_.
    while (numInFrames > 0 && total_out < maxOutFrames) {
        // Compact first: move unconsumed frames to the front so head == 0.
        if (stagingHead_ > 0) {
            if (stagingSize_ > 0) {
                std::memmove(input_buffer_.data(), input_buffer_.data() + stagingHead_,
                             stagingSize_ * sizeof(float));
            }
            stagingHead_ = 0;
        }

        // Copy up to free_space frames from the caller's input into the staging
        // buffer (write position == stagingSize_ after the compaction above).
        std::size_t free_space = kMaxStagingFrames - stagingSize_;
        std::size_t frames_to_copy = std::min(numInFrames, free_space);
        if (frames_to_copy > 0) {
            std::memcpy(input_buffer_.data() + stagingSize_, in,
                        frames_to_copy * sizeof(float));
            stagingSize_ += frames_to_copy;
            in += frames_to_copy;
            numInFrames -= frames_to_copy;
        }

        // If after copying we still have no staged data (e.g. first call with
        // no prior input), trigger the drain path immediately.
        if (stagingSize_ == 0) {
            // No input to process; try one drain src_process call.
            SRC_DATA data{};
            data.src_ratio = current_ratio_;
            data.data_in = nullptr;
            data.input_frames = 0;
            data.data_out = out + static_cast<long>(total_out);
            data.output_frames = static_cast<long>(maxOutFrames - static_cast<long>(total_out));
            data.end_of_input = 0;
            src_process(static_cast<SRC_STATE*>(state_), &data);
            total_out += static_cast<std::size_t>(data.output_frames_gen);
            break;
        }

        // ---------- Phase 2: Process the staged window with src_process ----------
        SRC_DATA data{};
        data.src_ratio = current_ratio_;
        data.data_in = input_buffer_.data() + stagingHead_;
        data.input_frames = static_cast<long>(stagingSize_);
        data.data_out = out + static_cast<long>(total_out);
        data.output_frames = static_cast<long>(maxOutFrames - static_cast<long>(total_out));
        data.end_of_input = 0;   // streaming: more input may come later

        src_process(static_cast<SRC_STATE*>(state_), &data);

        std::size_t gen = static_cast<std::size_t>(data.output_frames_gen);
        std::size_t used = static_cast<std::size_t>(data.input_frames_used);

        if (gen > 0) {
            total_out += gen;
        }

        // Advance head and reduce staged count by the number of frames consumed.
        // src_process consumes 'used' frames STARTING at stagingHead_.
        // We must NOT double-decrement; the remaining staged frames shift left
        // automatically because the read pointer moves forward.
        stagingHead_ += used;
        stagingSize_ -= used;

        // If all staged data has been consumed, reset head to 0 so that the
        // next copy target (stagingHead_ + stagingSize_) stays within the
        // pre-allocated buffer.  This prevents stagingHead_ from growing
        // beyond kMaxStagingFrames and causing out-of-bounds writes.
        if (stagingSize_ == 0) {
            stagingHead_ = 0;
        }

        // Guard against infinite loop: if src_process consumed 0 input frames
        // while we still have staged data, we are in the initial filter transient.
        // Break out so the caller can call process() again later.
        if (used == 0 && stagingSize_ > 0) {
            break;
        }

        // ---------- Phase 3: Compact if the head has advanced far enough ----------
        // Move remaining unconsumed data to the front with memmove (NO allocation),
        // so that stagingHead_ resets to 0 and the buffer stays within capacity.
        if (stagingHead_ + stagingSize_ > kMaxStagingFrames / 2) {
            if (stagingSize_ > 0) {
                std::memmove(input_buffer_.data(), input_buffer_.data() + stagingHead_,
                             stagingSize_ * sizeof(float));
                stagingHead_ = 0;
            }
        }
    }  // end while (numInFrames > 0 && total_out < maxOutFrames)

    // ---------- Phase 4: Drain phase ----------
    // If no new input was supplied (numInFrames == 0 after the feeding loop)
    // and the output buffer is not yet full, try one more src_process call with
    // data_in=nullptr to emit any leftover filter state (the "drain" behaviour).
    // This mirrors the original code's behaviour when available == 0.
    if (numInFrames == 0 && total_out < maxOutFrames && stagingSize_ == 0 && stagingHead_ == 0) {
        SRC_DATA data{};
        data.src_ratio = current_ratio_;
        data.data_in = nullptr;
        data.input_frames = 0;
        data.data_out = out + static_cast<long>(total_out);
        data.output_frames = static_cast<long>(maxOutFrames - static_cast<long>(total_out));
        data.end_of_input = 0;
        src_process(static_cast<SRC_STATE*>(state_), &data);
        total_out += static_cast<std::size_t>(data.output_frames_gen);
    }

    return total_out;
}

double Resampler::inputRate() const noexcept {
    return input_rate_;
}

double Resampler::outputRate() const noexcept {
    return output_rate_;
}

void Resampler::setRatio(double ratio) noexcept {
    const double nominal = output_rate_ / input_rate_;
    const double minRatio = nominal * 0.5;
    const double maxRatio = nominal * 2.0;
    current_ratio_ = (ratio < minRatio) ? minRatio : (ratio > maxRatio) ? maxRatio : ratio;
}

double Resampler::currentRatio() const noexcept {
    return current_ratio_;
}

// ---------------------------------------------------------------------------
// Goertzel helper (inline, defined in the header for convenience)
// ---------------------------------------------------------------------------

} // namespace netsdr