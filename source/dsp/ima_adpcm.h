#pragma once
// IMA ADPCM decoder — KiwiSDR compatible.
// Decodes mono IMA ADPCM at 12 kHz (narrowband) or 20.25 kHz (wideband).
// Each input byte contains TWO 4-bit ADPCM nibbles; the LOW nibble is
// decoded first, then the HIGH nibble. Output is 16-bit signed PCM.
//
// Based on the KiwiSDR reference implementation.
//
// State: int index (0..88) and int prevSample (initial 0).
// Step size table: 89 entries, index 0..88.
// Index adjustment table: 16 entries, indexed by the 4-bit code 0..15.

#include <cstddef>
#include <cstdint>

namespace netsdr {

class ImaAdpcmDecoder {
public:
    // Step size table (89 entries, index 0..88).
    // Used by the encoder/decoder for ADPCM step-size adaptation.
    static constexpr int kStepTable[89] = {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
        19, 21, 23, 25, 28, 31, 34, 37, 41,
        45, 50, 55, 60, 66, 73, 80, 88, 97,
        107, 118, 130, 143, 157, 173, 190, 209,
        230, 253, 279, 307, 337, 371, 408, 449,
        494, 544, 598, 658, 724, 796, 876, 963,
        1060, 1166, 1282, 1411, 1552, 1707, 1878,
        2066, 2272, 2499, 2749, 3024, 3327, 3660,
        4026, 4428, 4871, 5358, 5894, 6484, 7132,
        7845, 8630, 9493, 10442, 11487, 12635,
        13899, 15289, 16818, 18500, 20350, 22385,
        24623, 27086, 29794, 32767
    };

    // Index adjustment table (16 entries, indexed by 4-bit code 0..15).
    // Used by the encoder/decoder to update the step-index.
    static constexpr int kIndexAdjustTable[16] = {
        -1, -1, -1, -1, 2, 4, 6, 8,
        -1, -1, -1, -1, 2, 4, 6, 8
    };

    ImaAdpcmDecoder() { reset(); }

    // Reset state to initial values (index=0, prevSample=0).
    void reset() {
        index_ = 0;
        prevSample_ = 0;
    }

    // Decode a single 4-bit nibble and advance state.
    // The nibble code is the low 4 bits of the argument;
    // only the nibble value matters, not the byte context.
    int16_t decodeSample(uint8_t code);

    // Decode `numBytes` bytes from `in` into `out`.
    // Exactly 2 * numBytes samples are produced (low nibble first,
    // then high nibble for each byte).
    void decode(const uint8_t* in, std::size_t numBytes, int16_t* out);

    // Accessors for tests.
    int index() const { return index_; }
    int prevSample() const { return prevSample_; }

private:
    int index_;
    int prevSample_;
};

} // namespace netsdr