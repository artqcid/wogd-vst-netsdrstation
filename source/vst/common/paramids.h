#pragma once
// Parameter IDs for the NetSDRStation plugin (Milestone M3 KiwiSDR receiver).
//
// The IDs are used consistently by the processor, the controller and the UI
// bridge. They are part of the public plugin contract and must stay stable
// once released.
//
// Pure C++ (no VST3 SDK dependency) so the DSP/registry layer can be unit
// tested in isolation. Steinberg::Vst::ParamID is uint32, so these constants
// are binary-compatible with the VST3 parameter IDs.
//
// The active station (host:port) is intentionally NOT a VST3 parameter
// (VST3 parameters are double-typed). It is stored in the plugin state and
// exchanged via a dedicated `setStation` bridge message (M3.2 Option A,
// see doc/M3-implementation-plan.md).

#include <cstdint>

namespace netsdr {

using ParamID = std::uint32_t;

// --- Core receiver controls (GUI) -----------------------------------------
constexpr ParamID kParamMode = 0;    // modulation mode, enum 0..17 (see kKiwiModeNames)
constexpr ParamID kParamFreqKhz = 1; // tuning frequency in kHz, 0.001 .. 30000
constexpr ParamID kParamLowCut = 2;  // passband low cut in Hz, -8000 .. 0
constexpr ParamID kParamHighCut = 3; // passband high cut in Hz, 0 .. 8000

// --- AGC --------------------------------------------------------------------
constexpr ParamID kParamAgcOn = 4;      // bool toggle
constexpr ParamID kParamAgcHang = 5;     // bool toggle
constexpr ParamID kParamAgcThresh = 6;   // dB, -130 .. 0
constexpr ParamID kParamAgcSlope = 7;    // dB, 0 .. 10
constexpr ParamID kParamAgcDecay = 8;    // ms, 20 .. 5000
constexpr ParamID kParamAgcManGain = 9;  // dB, 0 .. 120

// --- Audio ------------------------------------------------------------------
constexpr ParamID kParamVolume = 10;     // output gain 0..1
constexpr ParamID kParamMute = 11;       // mute on/off (0 or 1)
constexpr ParamID kParamSquelchOn = 12;  // bool toggle
constexpr ParamID kParamSquelchThr = 13; // squelch threshold 0..1
constexpr ParamID kParamNbOn = 14;       // noise blanker, bool toggle
constexpr ParamID kParamNbThresh = 15;   // noise blanker threshold 0..1
constexpr ParamID kParamNrOn = 16;       // noise reduction, bool toggle
constexpr ParamID kParamDeempOn = 17;    // de-emphasis, bool toggle
constexpr ParamID kParamCompOn = 18;     // compressor, bool toggle

// --- Display / Waterfall ----------------------------------------------------
constexpr ParamID kParamWfOn = 19;    // waterfall on/off
constexpr ParamID kParamWfSpeed = 20; // waterfall speed, enum 0..3 (Pause/Slow/Med/Fast)
constexpr ParamID kParamWfZoom = 21;  // waterfall zoom, 0..14
constexpr ParamID kParamWfMaxDb = 22; // waterfall max dBFS, -10 .. 0
constexpr ParamID kParamWfMinDb = 23; // waterfall min dBFS, -160 .. -60
constexpr ParamID kParamWfComp = 24;  // waterfall CIC compensation, bool
constexpr ParamID kParamArOn = 25;    // aperture auto-range, bool
constexpr ParamID kParamOvOn = 26;    // spectrum overlap, bool

constexpr ParamID kNumParams = 27;

} // namespace netsdr