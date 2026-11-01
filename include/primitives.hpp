#ifndef PRIMITIVES_HPP
#define PRIMITIVES_HPP

#include <cstdint>
#include <cstddef>

namespace succinct {

// External lookup tables initialized via init_primitives()
extern uint8_t g_popcount_table[256];
extern uint8_t g_select_table[256 * 8]; // For byte-level select: Select256[(byte << 3) | j]
extern uint64_t g_psoverflow_table[64]; // PsOverflow[i] for broadword BLT select64

// Initialize lookup tables
void init_primitives();

// ==========================================
// 64-bit POPCOUNT Implementations
// ==========================================

// 1. TBL: 8-bit Lookup Table popcount (González et al. 2005)
inline uint64_t popcount_tbl(uint64_t x) {
    return g_popcount_table[x & 0xFF] +
           g_popcount_table[(x >> 8) & 0xFF] +
           g_popcount_table[(x >> 16) & 0xFF] +
           g_popcount_table[(x >> 24) & 0xFF] +
           g_popcount_table[(x >> 32) & 0xFF] +
           g_popcount_table[(x >> 40) & 0xFF] +
           g_popcount_table[(x >> 48) & 0xFF] +
           g_popcount_table[(x >> 56) & 0xFF];
}

// 2. BW: Broadword popcount (Knuth / Vigna 2008)
inline uint64_t popcount_bw(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (x * 0x0101010101010101ULL) >> 56;
}

// 3. BLT: CPU built-in popcount instruction (POPCNT / CNT)
inline uint64_t popcount_blt(uint64_t x) {
    return __builtin_popcountll(x);
}

// Default popcount wrapper (uses fastest BLT)
inline uint64_t popcount64(uint64_t x) {
    return popcount_blt(x);
}

// ==========================================
// 64-bit SELECT64 Implementations
// Returns 0-based bit position of the (i+1)-th set bit in x (i is 0-based).
// Precondition: 0 <= i < popcount(x).
// ==========================================

// Reference / Verification: Linear bit test
inline uint32_t select64_linear(uint64_t x, uint32_t i) {
    uint32_t seen = 0;
    for (uint32_t bit = 0; bit < 64; ++bit) {
        if ((x >> bit) & 1ULL) {
            if (seen == i) return bit;
            seen++;
        }
    }
    return 64; // Not found
}

// 1. TBL: Byte-wise popcount via table + bitwise scan in target byte
uint32_t select64_tbl(uint64_t x, uint32_t i);

// 2. BW: Broadword select64 (Vigna 2008)
uint32_t select64_bw(uint64_t x, uint32_t i);

// 3. BLT: Fast branchless broadword + __builtin_ctzll + Select256 table
// Exactly as proposed by Gog & Petri (2014) Listing 1, p. 1297
uint32_t select64_blt(uint64_t x, uint32_t i);

// Default select64 wrapper (uses fastest BLT)
inline uint32_t select64(uint64_t x, uint32_t i) {
    return select64_blt(x, i);
}

} // namespace succinct

#endif // PRIMITIVES_HPP
