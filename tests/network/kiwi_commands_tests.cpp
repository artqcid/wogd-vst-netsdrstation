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
