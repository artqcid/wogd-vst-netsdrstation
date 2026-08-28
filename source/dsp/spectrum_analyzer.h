#pragma once
// DFT-based spectrum analyzer for the M4.7 waterfall display (simulated
// spectrum per M4 plan §7a alternative): computes magnitude bins (dBFS) from
// a window of PCM samples. The window is a power-of-two sized DFT (Goertzel
// per bin) over the last N samples of the audio stream. Deliberately simple —
// it replaces the real KiwiSDR STREAM_WATERFALL (M5+) behind the same
// "one spectrum frame per tick" interface.
//
// Not used on the audio thread; the worker thread computes frames at ~10 Hz.

#include <cmath>
#include <cstddef>
#include <vector>

namespace netsdr {

class SpectrumAnalyzer {
public:
    // windowSize must be a power of two (e.g. 512); produces
    // binCount = windowSize/2 magnitude values (0 .. Nyquist).
    explicit SpectrumAnalyzer(std::size_t windowSize = 512)
        : windowSize_(windowSize), binCount_(windowSize / 2) {}

    // Applies a Hann window and computes the per-bin magnitude in dBFS for
    // the given audio window (must have exactly windowSize() samples).
    // Returns binCount() values in [-160, 0] dBFS (silence floors at -160).
    std::vector<float> computeDbF(std::size_t sampleRate,
                                  const float* samples) const;

    std::size_t windowSize() const { return windowSize_; }
    std::size_t binCount() const { return binCount_; }

    // Frequency of a bin in Hz for a given sample rate.
    static float binFrequency(std::size_t bin, std::size_t sampleRate,
                              std::size_t binCount) {
        return static_cast<float>(bin) * static_cast<float>(sampleRate) /
               (2.0f * static_cast<float>(binCount));
    }

private:
    std::size_t windowSize_;
    std::size_t binCount_;
};

} // namespace netsdr