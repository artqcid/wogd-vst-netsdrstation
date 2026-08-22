#include "dsp/ima_adpcm.h"

#include <algorithm>

namespace netsdr {

int16_t ImaAdpcmDecoder::decodeSample(uint8_t code) {
    // Read step from current index, then update index.
    int step = kStepTable[index_];
    index_ = std::clamp(index_ + kIndexAdjustTable[code], 0, 88);

    // Compute the difference from the step size and the code.
    int difference = step >> 3;
    if (code & 1) difference += step >> 2;
    if (code & 2) difference += step >> 1;
    if (code & 4) difference += step;
    if (code & 8) difference = -difference;

    // Clamp the new sample to int16 range.
    int sample = prevSample_ + difference;
    if (sample < -32768) sample = -32768;
    if (sample > 32767) sample = 32767;

    prevSample_ = sample;
    return static_cast<int16_t>(sample);
}

void ImaAdpcmDecoder::decode(const uint8_t* in, std::size_t numBytes, int16_t* out) {
    for (std::size_t i = 0; i < numBytes; ++i) {
        // Decode LOW nibble first, then HIGH nibble.
        out[2 * i + 0] = decodeSample(in[i] & 0x0F);
        out[2 * i + 1] = decodeSample(in[i] >> 4);
    }
}

} // namespace netsdr