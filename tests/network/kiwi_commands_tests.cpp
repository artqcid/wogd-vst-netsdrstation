// Unit tests for the KiwiSDR SET-command serialization (Milestone M2.2).
// Verifies the exact text frames the server expects (reference: kiwiclient
// and KiwiSDR rx/rx_cmd.cpp).

#include "catch.hpp"
#include "network/kiwi_commands.h"

#include <string>

using namespace netsdr;

TEST_CASE("Kiwi commands: auth is anonymous and needs no user", "[network][kiwi]") {
    REQUIRE(kiwiAuthCommand() == "SET auth t=kiwi p=");
}

TEST_CASE("Kiwi commands: ident_user is omitted for an empty name", "[network][kiwi]") {
    REQUIRE(kiwiIdentUserCommand("").empty());
}

TEST_CASE("Kiwi commands: ident_user is sent for a configured name", "[network][kiwi]") {
    REQUIRE(kiwiIdentUserCommand("NetSDRStation-VST") ==
            "SET ident_user=NetSDRStation-VST");
}

TEST_CASE("Kiwi commands: agc command carries all parameters", "[network][kiwi]") {
    REQUIRE(kiwiAgcCommand(true, false, -100, 6, 1000, 50) ==
            "SET agc=1 hang=0 thresh=-100 slope=6 decay=1000 manGain=50");
}

TEST_CASE("Kiwi commands: agc off disables the AGC", "[network][kiwi]") {
    REQUIRE(kiwiAgcCommand(false, false, -100, 6, 1000, 50) ==
            "SET agc=0 hang=0 thresh=-100 slope=6 decay=1000 manGain=50");
}

TEST_CASE("Kiwi commands: mod/freq command formats frequency with 3 decimals",
          "[network][kiwi]") {
    REQUIRE(kiwiSetModFreqCommand("am", -4900, 4900, 14100.0) ==
            "SET mod=am low_cut=-4900 high_cut=4900 freq=14100.000");
}

TEST_CASE("Kiwi commands: keepalive is a fixed frame", "[network][kiwi]") {
    REQUIRE(kiwiKeepaliveCommand() == "SET keepalive");
}

TEST_CASE("Kiwi commands: options marks the connection as external", "[network][kiwi]") {
    REQUIRE(kiwiOptionsCommand() == "SET options=1");
}

TEST_CASE("Kiwi commands: AR OK acknowledges the server rate and requests the same output",
          "[network][kiwi]") {
    REQUIRE(kiwiArOkCommand(12000, 12000) == "SET AR OK in=12000 out=12000");
    REQUIRE(kiwiArOkCommand(11025, 11025) == "SET AR OK in=11025 out=11025");
}

TEST_CASE("Kiwi commands: squelch max gate uses the reference format", "[network][kiwi]") {
    REQUIRE(kiwiSquelchMaxCommand(false, 0) == "SET squelch=0 max=0");
    REQUIRE(kiwiSquelchMaxCommand(true, 1) == "SET squelch=1 max=1");
}

TEST_CASE("Kiwi commands: generator is disabled with freq=0 mix=-1", "[network][kiwi]") {
    REQUIRE(kiwiGenCommand(0, -1) == "SET gen=0 mix=-1");
}

TEST_CASE("Kiwi commands: generator attenuation", "[network][kiwi]") {
    REQUIRE(kiwiGenAttnCommand(0) == "SET genattn=0");
}
