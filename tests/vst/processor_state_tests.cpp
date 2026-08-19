// Unit tests for the versioned processor state (de)serialization (FIX-06/FIX-08).
// Verifies the wire format (version byte + double fields) roundtrips, is
// endian-stable on little-endian targets, and rejects truncated/unknown-version
// buffers.

#include "catch.hpp"
#include "vst/common/processor_state.h"

#include <cstdint>
#include <cstring>
#include <string>

TEST_CASE("ProcessorState: serializes a version byte first", "[vst][state]") {
    netsdr::ProcessorState s;
    s.freqHz = 123.0;
    s.volume = 0.75;
    s.mute = true;

    const std::string bytes = s.serialize();

    REQUIRE(bytes.size() == netsdr::ProcessorState::kSerializedSize);

    // First four bytes are the little-endian version (= 1).
    const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
    const std::uint32_t version = static_cast<std::uint32_t>(p[0]) |
                                   (static_cast<std::uint32_t>(p[1]) << 8) |
                                   (static_cast<std::uint32_t>(p[2]) << 16) |
                                   (static_cast<std::uint32_t>(p[3]) << 24);
    REQUIRE(version == netsdr::ProcessorState::kVersion);
}

TEST_CASE("ProcessorState: roundtrips freq/volume/mute exactly", "[vst][state]") {
    netsdr::ProcessorState s;
    s.freqHz = 12345.678;
    s.volume = 0.314159;
    s.mute = true;

    netsdr::ProcessorState restored;
    REQUIRE(restored.deserialize(s.serialize()));
    REQUIRE(restored.freqHz == 12345.678);
    REQUIRE(restored.volume == 0.314159);
    REQUIRE(restored.mute == true);
}

TEST_CASE("ProcessorState: defaults roundtrip to the documented values", "[vst][state]") {
    netsdr::ProcessorState s; // freqHz=440, volume=1, mute=false

    netsdr::ProcessorState restored;
    REQUIRE(restored.deserialize(s.serialize()));
    REQUIRE(restored.freqHz == 440.0);
    REQUIRE(restored.volume == 1.0);
    REQUIRE(restored.mute == false);
}

TEST_CASE("ProcessorState: rejects a truncated buffer", "[vst][state]") {
    netsdr::ProcessorState s;
    const std::string full = s.serialize();

    netsdr::ProcessorState restored;
    REQUIRE_FALSE(restored.deserialize(full.substr(0, full.size() - 1)));
    REQUIRE_FALSE(restored.deserialize(std::string{}));
}

TEST_CASE("ProcessorState: rejects an unknown version", "[vst][state]") {
    netsdr::ProcessorState s;
    std::string bytes = s.serialize();

    // Corrupt the version field (bytes 0..3) to a different value.
    bytes[0] = static_cast<char>(0xFF);
    bytes[1] = static_cast<char>(0xFF);
    bytes[2] = static_cast<char>(0xFF);
    bytes[3] = static_cast<char>(0xFF);

    netsdr::ProcessorState restored;
    REQUIRE_FALSE(restored.deserialize(bytes));
}

TEST_CASE("ProcessorState: mute=0/1 is preserved across the roundtrip", "[vst][state]") {
    netsdr::ProcessorState unmuted;
    unmuted.mute = false;
    netsdr::ProcessorState muted;
    muted.mute = true;

    netsdr::ProcessorState a;
    netsdr::ProcessorState b;
    REQUIRE(a.deserialize(unmuted.serialize()));
    REQUIRE(b.deserialize(muted.serialize()));
    REQUIRE(a.mute == false);
    REQUIRE(b.mute == true);
}
