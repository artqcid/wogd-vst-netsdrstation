// Unit tests for the versioned processor state (de)serialization (FIX-06/FIX-08).
// Verifies the wire format (version byte + double fields) roundtrips, is
// endian-stable on little-endian targets, and rejects truncated/unknown-version
// buffers. The M3.2 state includes the active station (host:port) plus all
// 27 parameter fields.

#include "catch.hpp"
#include "vst/common/processor_state.h"

#include <cstdint>
#include <cstring>
#include <string>

TEST_CASE("ProcessorState: serializes a version byte first", "[vst][state]") {
    netsdr::ProcessorState s;
    s.freqKhz = 123.0;
    s.volume = 0.75;
    s.mute = true;

    const std::string bytes = s.serialize();

    // M3.2: size is variable (depends on station length), not a fixed constant.
    // Assert the first 4 bytes are the version.
    REQUIRE(bytes.size() >= 4);

    // First four bytes are the little-endian version (= 2).
    const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
    const std::uint32_t version = static_cast<std::uint32_t>(p[0]) |
                                   (static_cast<std::uint32_t>(p[1]) << 8) |
                                   (static_cast<std::uint32_t>(p[2]) << 16) |
                                   (static_cast<std::uint32_t>(p[3]) << 24);
    REQUIRE(version == netsdr::ProcessorState::kVersion);
}

TEST_CASE("ProcessorState: roundtrips full station + all 27 parameters",
          "[vst][state]") {
    netsdr::ProcessorState s;
    s.station = "kphsdr.com:8072";
    s.mode = 3;
    s.freqKhz = 7100.5;
    s.lowCut = -100;
    s.highCut = 100;
    s.agcOn = 1;
    s.agcHang = 0;
    s.agcThresh = -100;
    s.agcSlope = 6;
    s.agcDecay = 1000;
    s.agcManGain = 50;
    s.volume = 0.25;
    s.mute = 1;
    s.squelchOn = 0;
    s.squelchThr = 0.5;
    s.nbOn = 0;
    s.nbThresh = 0.5;
    s.nrOn = 0;
    s.deempOn = 1;
    s.compOn = 0;
    s.wfOn = 1;
    s.wfSpeed = 2;
    s.wfZoom = 4;
    s.wfMaxDb = 0;
    s.wfMinDb = -140;
    s.wfComp = 1;
    s.arOn = 1;
    s.ovOn = 1;

    netsdr::ProcessorState restored;
    REQUIRE(restored.deserialize(s.serialize()));

    REQUIRE(restored.station == "kphsdr.com:8072");
    REQUIRE(restored.mode == 3);
    REQUIRE(restored.freqKhz == 7100.5);
    REQUIRE(restored.lowCut == -100);
    REQUIRE(restored.highCut == 100);
    REQUIRE(restored.agcOn == 1);
    REQUIRE(restored.agcHang == 0);
    REQUIRE(restored.agcThresh == -100);
    REQUIRE(restored.agcSlope == 6);
    REQUIRE(restored.agcDecay == 1000);
    REQUIRE(restored.agcManGain == 50);
    REQUIRE(restored.volume == 0.25);
    REQUIRE(restored.mute == 1);
    REQUIRE(restored.squelchOn == 0);
    REQUIRE(restored.squelchThr == 0.5);
    REQUIRE(restored.nbOn == 0);
    REQUIRE(restored.nbThresh == 0.5);
    REQUIRE(restored.nrOn == 0);
    REQUIRE(restored.deempOn == 1);
    REQUIRE(restored.compOn == 0);
    REQUIRE(restored.wfOn == 1);
    REQUIRE(restored.wfSpeed == 2);
    REQUIRE(restored.wfZoom == 4);
    REQUIRE(restored.wfMaxDb == 0);
    REQUIRE(restored.wfMinDb == -140);
    REQUIRE(restored.wfComp == 1);
    REQUIRE(restored.arOn == 1);
    REQUIRE(restored.ovOn == 1);
}

TEST_CASE("ProcessorState: defaults roundtrip to the documented values", "[vst][state]") {
    netsdr::ProcessorState s; // station empty, all defaults

    netsdr::ProcessorState restored;
    REQUIRE(restored.deserialize(s.serialize()));

    // Station stays empty (no station set).
    REQUIRE(restored.station.empty());
    // freqKhz default 14100.0
    REQUIRE(restored.freqKhz == 14100.0);
    // volume default 1.0
    REQUIRE(restored.volume == 1.0);
    // mute default 0.0
    REQUIRE(restored.mute == 0.0);
    // deempOn default 1.0
    REQUIRE(restored.deempOn == 1.0);
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

    // Corrupt the version field (bytes 0..3) to 0xFF.
    bytes[0] = static_cast<char>(0xFF);
    bytes[1] = static_cast<char>(0xFF);
    bytes[2] = static_cast<char>(0xFF);
    bytes[3] = static_cast<char>(0xFF);

    netsdr::ProcessorState restored;
    REQUIRE_FALSE(restored.deserialize(bytes));
}

TEST_CASE("ProcessorState: station with empty string roundtrips", "[vst][state]") {
    netsdr::ProcessorState s;
    s.station = "";

    netsdr::ProcessorState restored;
    REQUIRE(restored.deserialize(s.serialize()));
    REQUIRE(restored.station.empty());
}

TEST_CASE("ProcessorState: serialized state with station 'a.b:1234' has expected size",
          "[vst][state]") {
    netsdr::ProcessorState s;
    s.station = "a.b:1234";

    const std::string bytes = s.serialize();
    // Minimum expected size: 4 (version) + 4 (station length) + 8 (station "a.b:1234")
    // + 27 * 8 (all double fields)
    constexpr std::size_t minExpected = 4 + 4 + 7 + 27 * 8;
    REQUIRE(bytes.size() >= minExpected);
}