// DFT-based spectrum analyzer (see spectrum_analyzer.h). Uses a per-bin
// Goertzel recurrence to compute the magnitude at each frequency: O(N*B)
// where N = window size and B = bin count. For N=512, B=256 that is ~131k
// multiply-adds per frame — fine on the worker thread at 10 Hz.

#include "spectrum_analyzer.h"

#include <algorithm>

namespace netsdr {

std::vector<float> SpectrumAnalyzer::computeDbF(std::size_t sampleRate,
                                                const float* samples) const {
    std::vector<float> bins(binCount_, -160.0f);
    if (samples == nullptr) {
        return bins;
    }

    // Hann window coefficients (precomputed once per call is fine at 10 Hz).
    std::vector<float> window(windowSize_);
    for (std::size_t n = 0; n < windowSize_; ++n) {
        const float t = static_cast<float>(n) / static_cast<float>(windowSize_ - 1);
        window[n] = 0.5f * (1.0f - std::cos(2.0f * 3.141592653589793f * t));
    }

    const float kTwoPi = 2.0f * 3.141592653589793f;

    for (std::size_t k = 0; k < binCount_; ++k) {
        // Bin frequency: k * sampleRate / windowSize (Hz).
        const float freq =
            static_cast<float>(k) * static_cast<float>(sampleRate) /
            static_cast<float>(windowSize_);
        // Normalized angular frequency for the DFT at this bin.
        const float omega = kTwoPi * freq / static_cast<float>(sampleRate);
        const float cosW = std::cos(omega);

        // Goertzel recurrence over the windowed samples.
        double s0 = 0.0, s1 = 0.0, s2 = 0.0;
        for (std::size_t n = 0; n < windowSize_; ++n) {
            const double x = static_cast<double>(samples[n]) * window[n];
            s0 = x + 2.0 * cosW * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        const double magSq = s1 * s1 + s2 * s2 - 2.0 * cosW * s1 * s2;
        const double mag = std::sqrt(std::max(0.0, magSq));

        // dBFS: a full-scale sine at bin centre produces X[k] = A*N/4 under the
        // Hann window, so 0 dBFS corresponds to mag = windowSize_*0.25.
        const double db = 20.0 * std::log10(mag / static_cast<double>(windowSize_ * 0.25) + 1e-12);
        bins[k] = static_cast<float>(std::max(-160.0, std::min(0.0, db)));
    }

    return bins;
}

} // namespace netsdr