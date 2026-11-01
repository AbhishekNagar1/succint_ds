#include "primitives.hpp"
#include <cstring>
#include <cassert>

namespace succinct {

uint8_t g_popcount_table[256];
uint8_t g_select_table[256 * 8];
uint64_t g_psoverflow_table[64];

static bool g_primitives_initialized = false;

void init_primitives() {
    if (g_primitives_initialized) return;

    // 1. Initialize 8-bit Popcount table
    for (int i = 0; i < 256; ++i) {
        uint8_t cnt = 0;
        for (int b = 0; b < 8; ++b) {
            if ((i >> b) & 1) cnt++;
        }
        g_popcount_table[i] = cnt;
    }

    // 2. Initialize 8-bit Select table
    // g_select_table[(byte << 3) | j] = position of the (j+1)-th set bit (0-based)
    std::memset(g_select_table, 0xFF, sizeof(g_select_table));
    for (int byte_val = 0; byte_val < 256; ++byte_val) {
        int seen = 0;
        for (int bit = 0; bit < 8; ++bit) {
            if ((byte_val >> bit) & 1) {
                g_select_table[(byte_val << 3) | seen] = static_cast<uint8_t>(bit);
                seen++;
            }
        }
    }

    // 3. Initialize PsOverflow table for Gog & Petri (2014) select64_blt
    // When cumulative byte sum + (127 - i) >= 128, the MSB (bit 7) of that byte is set.
    for (uint32_t i = 0; i < 64; ++i) {
        uint64_t val = (uint64_t)(127 - i);
        g_psoverflow_table[i] = val * 0x0101010101010101ULL;
    }

    g_primitives_initialized = true;
}

// Ensure auto-initialization before main runs
struct PrimitivesInitializer {
    PrimitivesInitializer() {
        init_primitives();
    }
};
static PrimitivesInitializer s_init;

// 1. TBL: Byte-wise popcount via table + bitwise scan in target byte
uint32_t select64_tbl(uint64_t x, uint32_t i) {
    uint32_t cur_rank = 0;
    for (uint32_t byte_idx = 0; byte_idx < 8; ++byte_idx) {
        uint8_t byte_val = (x >> (byte_idx << 3)) & 0xFF;
        uint32_t byte_pop = g_popcount_table[byte_val];
        if (cur_rank + byte_pop > i) {
            // Target 1 is in this byte
            uint32_t bit_in_byte = g_select_table[(byte_val << 3) | (i - cur_rank)];
            return (byte_idx << 3) + bit_in_byte;
        }
        cur_rank += byte_pop;
    }
    return 64;
}

// 2. BW: Broadword select64 (Vigna 2008 / Knuth)
uint32_t select64_bw(uint64_t x, uint32_t i) {
    // Broadword byte sums
    uint64_t s = x;
    s = s - ((s >> 1) & 0x5555555555555555ULL);
    s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
    s = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    s = s * 0x0101010101010101ULL;

    // Determine target byte by testing cumulative sums
    uint64_t b = (s + g_psoverflow_table[i]) & 0x8080808080808080ULL;
    if (b == 0) return 64;

    int bytenr = __builtin_ctzll(b) >> 3;
    uint32_t rank_before = 0;
    if (bytenr > 0) {
        rank_before = (s >> ((bytenr - 1) << 3)) & 0xFF;
    }
    uint32_t rem = i - rank_before;
    uint8_t target_byte = (x >> (bytenr << 3)) & 0xFF;
    return (bytenr << 3) + g_select_table[(target_byte << 3) | rem];
}

// 3. BLT: Gog & Petri (2014) Listing 1
uint32_t select64_blt(uint64_t x, uint32_t i) {
    uint64_t s = x;
    s = s - ((s >> 1) & 0x5555555555555555ULL);
    s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
    s = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    s = s * 0x0101010101010101ULL;

    uint64_t b = (s + g_psoverflow_table[i]) & 0x8080808080808080ULL;
    if (b == 0) return 64;

    int bytenr = __builtin_ctzll(b) >> 3;
    s <<= 8;
    uint32_t j = i - (uint32_t)(((const uint8_t*)&s)[bytenr]);
    uint8_t byte_val = (x >> (bytenr << 3)) & 0xFF;
    return (bytenr << 3) + g_select_table[(byte_val << 3) | j];
}

} // namespace succinct
