#ifndef CLARK_SELECT_HPP
#define CLARK_SELECT_HPP

#include "bit_vector.hpp"
#include "naive_rank_select.hpp"
#include "primitives.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace succinct {

// Two-Level Practical Clark Select (SEL-C from Gog & Petri 2014, Section 4)
// Superblocks sample every SuperSample 1s (e.g. 512 or 4096)
// Short superblocks store relative positions of every SubSample 1s (e.g. 64)
template <size_t SuperSample = 512, size_t SubSample = 64>
class clark_select {
public:
    static_assert(SuperSample % SubSample == 0, "SuperSample must be a multiple of SubSample");

    explicit clark_select(const bit_vector* bv = nullptr) : m_bv(bv), m_total_ones(0) {
        if (bv) {
            build();
        }
    }

    void build() {
        if (!m_bv) return;
        m_superblocks.clear();
        m_subblocks.clear();
        m_subblock_offsets.clear();
        m_total_ones = 0;

        // Collect all 1 positions
        std::vector<uint64_t> one_positions;
        size_t total_words = m_bv->word_count();
        size_t n = m_bv->size();

        for (size_t w = 0; w < total_words; ++w) {
            uint64_t word_val = m_bv->get_word(w);
            while (word_val != 0) {
                uint32_t bit_pos = __builtin_ctzll(word_val);
                uint64_t global_bit = (w << 6) + bit_pos;
                if (global_bit < n) {
                    one_positions.push_back(global_bit);
                }
                word_val &= (word_val - 1ULL); // Clear lowest bit
            }
        }

        m_total_ones = one_positions.size();
        if (m_total_ones == 0) return;

        size_t num_super = (m_total_ones + SuperSample - 1) / SuperSample;
        m_superblocks.resize(num_super);
        m_subblock_offsets.resize(num_super);

        for (size_t s = 0; s < num_super; ++s) {
            size_t one_idx = s * SuperSample;
            uint64_t base_pos = one_positions[one_idx];
            m_superblocks[s] = base_pos;
            m_subblock_offsets[s] = m_subblocks.size();

            size_t end_one_idx = std::min(one_idx + SuperSample, static_cast<size_t>(m_total_ones));
            for (size_t sub = one_idx; sub < end_one_idx; sub += SubSample) {
                uint64_t rel_offset = one_positions[sub] - base_pos;
                // Store relative position in 32-bit int
                m_subblocks.push_back(static_cast<uint32_t>(rel_offset));
            }
        }
    }

    inline uint64_t select1(uint64_t k) const {
        if (!m_bv || k == 0 || k > m_total_ones) {
            return NOT_FOUND;
        }

        size_t one_idx = k - 1;
        size_t s_idx = one_idx / SuperSample;
        size_t sub_in_s = (one_idx % SuperSample) / SubSample;

        uint64_t base_pos = m_superblocks[s_idx];
        uint32_t rel_pos = m_subblocks[m_subblock_offsets[s_idx] + sub_in_s];
        uint64_t start_bit = base_pos + rel_pos;

        size_t sampled_one_rank = s_idx * SuperSample + sub_in_s * SubSample + 1;
        uint64_t rem = k - sampled_one_rank;

        if (rem == 0) {
            return start_bit;
        }

        // Local word scan for remainder
        uint64_t curr_bit = start_bit + 1;
        size_t w = curr_bit >> 6;
        size_t bit_offset = curr_bit & 63;

        if (bit_offset != 0) {
            uint64_t word_val = m_bv->get_word(w) >> bit_offset;
            uint64_t pop = popcount64(word_val);
            if (rem <= pop) {
                uint32_t bit_in_partial = select64(word_val, static_cast<uint32_t>(rem - 1));
                return curr_bit + bit_in_partial;
            }
            rem -= pop;
            w++;
        }

        size_t total_words = m_bv->word_count();
        while (w < total_words) {
            uint64_t word_val = m_bv->get_word(w);
            uint64_t pop = popcount64(word_val);
            if (rem <= pop) {
                uint32_t bit_in_word = select64(word_val, static_cast<uint32_t>(rem - 1));
                return (w << 6) + bit_in_word;
            }
            rem -= pop;
            w++;
        }

        return NOT_FOUND;
    }

    uint64_t total_ones() const { return m_total_ones; }

    size_t index_bytes() const {
        return sizeof(*this) +
               m_superblocks.capacity() * sizeof(uint64_t) +
               m_subblocks.capacity() * sizeof(uint32_t) +
               m_subblock_offsets.capacity() * sizeof(size_t);
    }

    double overhead_percent() const {
        if (!m_bv || m_bv->allocated_bytes() == 0) return 0.0;
        return (static_cast<double>(index_bytes()) / m_bv->allocated_bytes()) * 100.0;
    }

private:
    const bit_vector* m_bv;
    uint64_t m_total_ones;
    std::vector<uint64_t> m_superblocks;
    std::vector<uint32_t> m_subblocks;
    std::vector<size_t> m_subblock_offsets;
};

using clark_select_512_64 = clark_select<512, 64>;

} // namespace succinct

#endif // CLARK_SELECT_HPP
