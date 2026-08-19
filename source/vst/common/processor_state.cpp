#include "processor_state.h"

#include <cstring>

namespace netsdr {

namespace {

void appendU32(std::string& out, std::uint32_t v) {
    // little-endian
    out.push_back(static_cast<char>(v & 0xFFu));
    out.push_back(static_cast<char>((v >> 8) & 0xFFu));
    out.push_back(static_cast<char>((v >> 16) & 0xFFu));
    out.push_back(static_cast<char>((v >> 24) & 0xFFu));
}

std::uint32_t readU32(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

void appendDouble(std::string& out, double v) {
    // All target platforms (x86/x64/arm64) are little-endian for doubles.
    const char* p = reinterpret_cast<const char*>(&v);
    out.append(p, sizeof(double));
}

} // namespace

std::string ProcessorState::serialize() const {
    std::string out;
    out.reserve(kSerializedSize);
    appendU32(out, kVersion);
    appendDouble(out, freqHz);
    appendDouble(out, volume);
    appendU32(out, mute ? 1u : 0u);
    return out;
}

bool ProcessorState::deserialize(const std::string& bytes) {
    if (bytes.size() < kSerializedSize) {
        return false;
    }
    const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
    if (readU32(p) != kVersion) {
        return false;
    }
    double freq = 0.0;
    double vol = 0.0;
    std::memcpy(&freq, p + 4, sizeof(double));
    std::memcpy(&vol, p + 12, sizeof(double));
    const std::uint32_t mute = readU32(p + 20);

    freqHz = freq;
    volume = vol;
    this->mute = mute != 0;
    return true;
}

} // namespace netsdr
