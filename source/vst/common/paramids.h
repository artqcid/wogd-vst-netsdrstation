#pragma once
// Parameter IDs for the NetSDRStation plugin (Milestone M1 sine synth).
//
// The IDs are used consistently by the processor, the controller and the UI
// bridge. They are part of the public plugin contract and must stay stable.
//
// Pure C++ (no VST3 SDK dependency) so the DSP/registry layer can be unit
// tested in isolation. Steinberg::Vst::ParamID is uint32, so these constants
// are binary-compatible with the VST3 parameter IDs.

#include <cstdint>

namespace netsdr {

using ParamID = std::uint32_t;

constexpr ParamID kParamFreq = 0;   // Oscillator frequency in Hz
constexpr ParamID kParamVolume = 1; // Output gain 0..1
constexpr ParamID kParamMute = 2;   // Mute on/off (0 or 1)
constexpr ParamID kNumParams = 3;

} // namespace netsdr
