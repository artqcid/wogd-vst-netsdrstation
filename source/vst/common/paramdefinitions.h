#pragma once
// Shared factory of the parameter definitions for the NetSDRStation plugin.
//
// Both the processor and the edit controller build their parameter registry
// from this single source of truth, so the parameter model (IDs, ranges,
// defaults) can never drift between the two sides (CCD Red: DRY).
//
// This is the complete KiwiSDR receiver/audio/display parameter set (M3.2).
// The active station is NOT a parameter (see paramids.h).

#include "vst/common/paramids.h"
#include "vst/common/parameter_registry.h"

#include <vector>

namespace netsdr {

// The 18 KiwiSDR modulation modes in server order (KiwiSDR `mod=` values).
inline constexpr const char* kKiwiModeNames[] = {
    "am",  "amn", "amw", "usb", "usn", "lsb", "lsn",
    "cw",  "cwn", "nbfm", "nnfm", "iq", "drm",
    "sam", "sau", "sal", "sas", "qam"};
inline constexpr int kNumKiwiModes = 18;

// Default tuning frequency in kHz (international shortwave broadcast band).
inline constexpr double kDefaultFreqKhz = 14100.0;
// Default KiwiSDR passband for AM broadcast.
inline constexpr int kDefaultLowCut = -4900;
inline constexpr int kDefaultHighCut = 4900;

inline std::vector<ParameterDefinition> createParameterDefinitions() {
    return {
        // Core
        ParameterDefinition{kParamMode,     "Mode",     "",    0.0,     17.0,   0.0,   false, 17},
        ParameterDefinition{kParamFreqKhz,  "Frequency", "kHz", 0.001,  30000.0, kDefaultFreqKhz, false, 0},
        ParameterDefinition{kParamLowCut,   "Low Cut",  "Hz",  -8000.0, 0.0,    static_cast<double>(kDefaultLowCut), false, 0},
        ParameterDefinition{kParamHighCut,  "High Cut", "Hz",  0.0,     8000.0, static_cast<double>(kDefaultHighCut), false, 0},

        // AGC
        ParameterDefinition{kParamAgcOn,     "AGC On",      "",  0.0, 1.0, 1.0, false, 1},
        ParameterDefinition{kParamAgcHang,   "AGC Hang",    "",  0.0, 1.0, 0.0, false, 1},
        ParameterDefinition{kParamAgcThresh, "AGC Threshold", "dB", -130.0, 0.0, -100.0, false, 0},
        ParameterDefinition{kParamAgcSlope,  "AGC Slope",   "dB", 0.0,  10.0, 6.0, false, 0},
        ParameterDefinition{kParamAgcDecay,  "AGC Decay",   "ms", 20.0, 5000.0, 1000.0, false, 0},
        ParameterDefinition{kParamAgcManGain, "AGC Manual Gain", "dB", 0.0, 120.0, 50.0, false, 0},

        // Audio
        ParameterDefinition{kParamVolume,    "Volume",  "", 0.0, 1.0, 1.0, false, 0},
        ParameterDefinition{kParamMute,      "Mute",    "", 0.0, 1.0, 0.0, false, 1},
        ParameterDefinition{kParamSquelchOn, "Squelch", "", 0.0, 1.0, 0.0, false, 1},
        ParameterDefinition{kParamSquelchThr, "Squelch Threshold", "", 0.0, 1.0, 0.5, false, 0},
        ParameterDefinition{kParamNbOn,      "Noise Blanker", "", 0.0, 1.0, 0.0, false, 1},
        ParameterDefinition{kParamNbThresh,  "Noise Blanker Threshold", "", 0.0, 1.0, 0.5, false, 0},
        ParameterDefinition{kParamNrOn,      "Noise Reduction", "", 0.0, 1.0, 0.0, false, 1},
        ParameterDefinition{kParamDeempOn,   "De-emphasis", "", 0.0, 1.0, 1.0, false, 1},
        ParameterDefinition{kParamCompOn,    "Compressor", "", 0.0, 1.0, 0.0, false, 1},

        // Display / Waterfall
        ParameterDefinition{kParamWfOn,    "Waterfall",      "", 0.0,  1.0,   1.0,   false, 1},
        ParameterDefinition{kParamWfSpeed, "Waterfall Speed", "", 0.0,  3.0,   2.0,   false, 3},
        ParameterDefinition{kParamWfZoom,  "Waterfall Zoom", "", 0.0,  14.0,  3.0,   false, 0},
        ParameterDefinition{kParamWfMaxDb, "Waterfall Max dB", "dBFS", -10.0, 0.0, 0.0, false, 0},
        ParameterDefinition{kParamWfMinDb, "Waterfall Min dB", "dBFS", -160.0, -60.0, -120.0, false, 0},
        ParameterDefinition{kParamWfComp,  "Waterfall CIC Comp", "", 0.0, 1.0, 1.0, false, 1},
        ParameterDefinition{kParamArOn,    "Aperture Auto-Range", "", 0.0, 1.0, 1.0, false, 1},
        ParameterDefinition{kParamOvOn,    "Spectrum Overlap", "", 0.0, 1.0, 0.0, false, 1},
    };
}

} // namespace netsdr