#pragma once
// Versioned processor state (de)serialization (FIX-06/FIX-08).
//
// Pure C++ with no VST3 SDK dependency so the wire format can be unit-tested
// in isolation. The processor writes these bytes via IBStreamer in getState()
// and reads them back in setState(); the controller mirrors them into its
// parameter display in setComponentState().
//
// Little-endian layout (24 bytes):
//   [0..3]   uint32 version (= kVersion)
//   [4..11]  double freqHz
//   [12..19] double volume
//   [20..23] uint32 mute (0/1)

#include <cstddef>
#include <cstdint>
#include <string>

namespace netsdr {

struct ProcessorState {
    static constexpr std::uint32_t kVersion = 1;
    static constexpr std::size_t kSerializedSize = 24;

    double freqHz = 440.0;
    double volume = 1.0;
    bool mute = false;

    // Serializes to the little-endian byte layout above.
    std::string serialize() const;

    // Deserializes from the byte layout. Returns false on a truncated buffer
    // or an unknown version, leaving the state unchanged.
    bool deserialize(const std::string& bytes);
};

} // namespace netsdr
