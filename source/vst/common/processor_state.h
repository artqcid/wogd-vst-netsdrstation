#pragma once
// Versioned processor state (de)serialization (FIX-06/FIX-08, extended M3.2).
//
// Pure C++ with no VST3 SDK dependency so the wire format can be unit-tested
// in isolation. The processor writes these bytes via IBStreamer in getState()
// and reads them back in setState(); the controller mirrors them into its
// parameter display in setComponentState().
//
// The M3 state persists the complete KiwiSDR parameter set plus the active
// station (host:port), which is not a VST3 parameter (see paramids.h).
//
// Little-endian layout (v2):
//   [0..3]      uint32 version (= kVersion)
//   [4..7]      uint32 station length N
//   [8..8+N)    station bytes (UTF-8, "host:port")
//   then each scalar field, in declaration order:
//     double  mode, freqKhz, lowCut, highCut,
//     double  agcOn, agcHang, agcThresh, agcSlope, agcDecay, agcManGain,
//     double  volume, mute (0/1),
//     double  squelchOn, squelchThr, nbOn, nbThresh, nrOn, deempOn, compOn,
//     double  wfOn, wfSpeed, wfZoom, wfMaxDb, wfMinDb, wfComp, arOn, ovOn
//
// All parameter fields hold PLAIN values (Hz/kHz/dB/...), matching the
// registry <-> controller conversion contract.

#include <cstddef>
#include <cstdint>
#include <string>

namespace netsdr {

struct ProcessorState {
    static constexpr std::uint32_t kVersion = 2;

    // Active station "host:port". Empty means "use the built-in default".
    std::string station;

    // Plain parameter values (in the units of paramdefinitions.h).
    double mode = 0.0;          // enum index 0..17
    double freqKhz = 14100.0;
    double lowCut = -4900.0;
    double highCut = 4900.0;
    double agcOn = 1.0;
    double agcHang = 0.0;
    double agcThresh = -100.0;
    double agcSlope = 6.0;
    double agcDecay = 1000.0;
    double agcManGain = 50.0;
    double volume = 1.0;
    double mute = 0.0;          // 0/1
    double squelchOn = 0.0;
    double squelchThr = 0.5;
    double nbOn = 0.0;
    double nbThresh = 0.5;
    double nrOn = 0.0;
    double deempOn = 1.0;
    double compOn = 0.0;
    double wfOn = 1.0;
    double wfSpeed = 2.0;       // enum index 0..3
    double wfZoom = 3.0;
    double wfMaxDb = 0.0;
    double wfMinDb = -120.0;
    double wfComp = 1.0;
    double arOn = 1.0;
    double ovOn = 0.0;

    // Serializes to the little-endian byte layout above.
    std::string serialize() const;

    // Deserializes from the byte layout. Returns false on a truncated buffer
    // or an unknown version, leaving the state unchanged.
    bool deserialize(const std::string& bytes);
};

} // namespace netsdr