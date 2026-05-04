#pragma once
#include <cstdint>
#include <cstring>

namespace utils {

    // Converts a float16 (stored as uint16_t) to float32
    inline float half_to_float(uint16_t h) {
        uint32_t sign = (h >> 15) & 0x00000001;
        uint32_t exp  = (h >> 10) & 0x0000001F;
        uint32_t frac =  h        & 0x000003FF;

        uint32_t f_exp;
        uint32_t f_frac;

        if (exp == 0) {
            if (frac == 0) {
                // Zero
                f_exp = 0;
                f_frac = 0;
            } else {
                // Denormalized
                while ((frac & 0x00000400) == 0) {
                    frac <<= 1;
                    exp--;
                }
                exp++;
                frac &= ~0x00000400;
                f_exp = exp + (127 - 15);
                f_frac = frac << 13;
            }
        } else if (exp == 0x1F) {
            // Inf or NaN
            f_exp = 0xFF;
            f_frac = frac << 13;
        } else {
            // Normalized
            f_exp = exp + (127 - 15);
            f_frac = frac << 13;
        }

        uint32_t f_bits = (sign << 31) | (f_exp << 23) | f_frac;
        float result;
        std::memcpy(&result, &f_bits, sizeof(result));
        return result;
    }

}
