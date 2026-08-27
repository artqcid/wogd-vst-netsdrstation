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

bool readDouble(const unsigned char* p, double& out) {
    std::memcpy(&out, p, sizeof(double));
    return true;
}

} // namespace

std::string ProcessorState::serialize() const {
    std::string out;
    appendU32(out, kVersion);
    appendU32(out, static_cast<std::uint32_t>(station.size()));
    out.append(station);

    appendDouble(out, mode);
    appendDouble(out, freqKhz);
    appendDouble(out, lowCut);
    appendDouble(out, highCut);
    appendDouble(out, agcOn);
    appendDouble(out, agcHang);
    appendDouble(out, agcThresh);
    appendDouble(out, agcSlope);
    appendDouble(out, agcDecay);
    appendDouble(out, agcManGain);
    appendDouble(out, volume);
    appendDouble(out, mute);
    appendDouble(out, squelchOn);
    appendDouble(out, squelchThr);
    appendDouble(out, nbOn);
    appendDouble(out, nbThresh);
    appendDouble(out, nrOn);
    appendDouble(out, deempOn);
    appendDouble(out, compOn);
    appendDouble(out, wfOn);
    appendDouble(out, wfSpeed);
    appendDouble(out, wfZoom);
    appendDouble(out, wfMaxDb);
    appendDouble(out, wfMinDb);
    appendDouble(out, wfComp);
    appendDouble(out, arOn);
    appendDouble(out, ovOn);
    return out;
}

bool ProcessorState::deserialize(const std::string& bytes) {
    const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
    std::size_t offset = 0;

    // Fixed header: version + station length + station bytes.
    if (bytes.size() < 8) {
        return false;
    }
    if (readU32(p + offset) != kVersion) {
        return false;
    }
    offset += 4;
    const std::uint32_t stationLen = readU32(p + offset);
    offset += 4;
    if (stationLen > bytes.size() - offset) {
        return false;
    }
    station.assign(reinterpret_cast<const char*>(p + offset), stationLen);
    offset += stationLen;

    // Scalar fields (each 8 bytes). A truncated buffer is rejected before any
    // field is committed so a failed parse leaves the state unchanged.
    const std::size_t numScalars = 27;
    if (bytes.size() - offset < numScalars * sizeof(double)) {
        return false;
    }

    double mode_, freqKhz_, lowCut_, highCut_;
    double agcOn_, agcHang_, agcThresh_, agcSlope_, agcDecay_, agcManGain_;
    double volume_, mute_, squelchOn_, squelchThr_, nbOn_, nbThresh_, nrOn_, deempOn_, compOn_;
    double wfOn_, wfSpeed_, wfZoom_, wfMaxDb_, wfMinDb_, wfComp_, arOn_, ovOn_;

    readDouble(p + offset, mode_); offset += 8;
    readDouble(p + offset, freqKhz_); offset += 8;
    readDouble(p + offset, lowCut_); offset += 8;
    readDouble(p + offset, highCut_); offset += 8;
    readDouble(p + offset, agcOn_); offset += 8;
    readDouble(p + offset, agcHang_); offset += 8;
    readDouble(p + offset, agcThresh_); offset += 8;
    readDouble(p + offset, agcSlope_); offset += 8;
    readDouble(p + offset, agcDecay_); offset += 8;
    readDouble(p + offset, agcManGain_); offset += 8;
    readDouble(p + offset, volume_); offset += 8;
    readDouble(p + offset, mute_); offset += 8;
    readDouble(p + offset, squelchOn_); offset += 8;
    readDouble(p + offset, squelchThr_); offset += 8;
    readDouble(p + offset, nbOn_); offset += 8;
    readDouble(p + offset, nbThresh_); offset += 8;
    readDouble(p + offset, nrOn_); offset += 8;
    readDouble(p + offset, deempOn_); offset += 8;
    readDouble(p + offset, compOn_); offset += 8;
    readDouble(p + offset, wfOn_); offset += 8;
    readDouble(p + offset, wfSpeed_); offset += 8;
    readDouble(p + offset, wfZoom_); offset += 8;
    readDouble(p + offset, wfMaxDb_); offset += 8;
    readDouble(p + offset, wfMinDb_); offset += 8;
    readDouble(p + offset, wfComp_); offset += 8;
    readDouble(p + offset, arOn_); offset += 8;
    readDouble(p + offset, ovOn_); offset += 8;

    // Commit only after a fully valid parse.
    mode = mode_;
    freqKhz = freqKhz_;
    lowCut = lowCut_;
    highCut = highCut_;
    agcOn = agcOn_;
    agcHang = agcHang_;
    agcThresh = agcThresh_;
    agcSlope = agcSlope_;
    agcDecay = agcDecay_;
    agcManGain = agcManGain_;
    volume = volume_;
    mute = mute_;
    squelchOn = squelchOn_;
    squelchThr = squelchThr_;
    nbOn = nbOn_;
    nbThresh = nbThresh_;
    nrOn = nrOn_;
    deempOn = deempOn_;
    compOn = compOn_;
    wfOn = wfOn_;
    wfSpeed = wfSpeed_;
    wfZoom = wfZoom_;
    wfMaxDb = wfMaxDb_;
    wfMinDb = wfMinDb_;
    wfComp = wfComp_;
    arOn = arOn_;
    ovOn = ovOn_;
    return true;
}

} // namespace netsdr