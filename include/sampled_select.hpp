#ifndef SAMPLED_SELECT_HPP
#define SAMPLED_SELECT_HPP

#include "bit_vector.hpp"
#include "naive_rank_select.hpp"
#include "primitives.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>

namespace succinct {

// Sampled Select: Stores position of every s-th 1 (Slide 8)
// Key trade-off: Smaller s -> more memory, less local scanning
template <size_t SampleInterval = 64>
class sampled_select {
public:
    explicit sampled_select(const bit_vector* bv = nullptr) : m_bv(bv), m_total_ones(0) {
        if (bv) {
            build();
        }
    }

    void build() {
        if (!m_bv) return;
        m_samples.clear();
        m_total_ones = 0;

        size_t n = m_bv->size();
        size_t total_words = m_bv->word_count();

        for (size_t w = 0; w < total_words; ++w) {
            uint64_t word_val = m_bv->get_word(w);
            uint64_t pop = popcount64(word_val);

            if (pop == 0) continue;

            // Check if any sample falls in this word
            while (true) {
                // Next target 1 count to sample: (m_samples.size() * SampleInterval) + 1
                uint64_t next_sample_k = m_samples.size() * SampleInterval + 1;
                if (next_sample_k > m_total_ones + pop) {
                    break;
                }
                // The sample falls within this word!
                uint32_t rank_in_word = static_cast<uint32_t>(next_sample_k - m_total_ones - 1);
                uint32_t bit_pos = select64(word_val, rank_in_word);
                uint64_t global_bit = (w << 6) + bit_pos;
                if (global_bit < n) {
                    m_samples.push_back(global_bit);
                } else {
                    break;
                }
            }
            m_total_ones += pop;
        }
    }

    // Select1(k): 0-based bit position of the k-th 1 (1-based k)
    // Precondition: 1 <= k <= total_ones
    inline uint64_t select1(uint64_t k) const {
        if (!m_bv || k == 0 || k > m_total_ones) {
            return NOT_FOUND;
        }

        size_t sample_idx = (k - 1) / SampleInterval;
        if (sample_idx >= m_samples.size()) {
            return NOT_FOUND;
        }

        uint64_t pos = m_samples[sample_idx];
        uint64_t base_k = sample_idx * SampleInterval + 1;
        uint64_t rem = k - base_k;

        if (rem == 0) {
            return pos;
        }

        // Scan forward from bit pos + 1
        uint64_t curr_bit = pos + 1;
        size_t w = curr_bit >> 6;
        size_t bit_offset = curr_bit & 63;

        // Process partial starting word
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

        // Process full words with popcount
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
    size_t sample_count() const { return m_samples.size(); }
    const std::vector<uint64_t>& get_samples() const { return m_samples; }

    size_t index_bytes() const {
        return sizeof(*this) + m_samples.capacity() * sizeof(uint64_t);
    }

    double overhead_percent() const {
        if (!m_bv || m_bv->allocated_bytes() == 0) return 0.0;
        return (static_cast<double>(index_bytes()) / m_bv->allocated_bytes()) * 100.0;
    }

private:
    const bit_vector* m_bv;
    uint64_t m_total_ones;
    std::vector<uint64_t> m_samples;
};

// Aliases for varying sample intervals
using sampled_select_32 = sampled_select<32>;
using sampled_select_64 = sampled_select<64>;
using sampled_select_128 = sampled_select<128>;
using sampled_select_256 = sampled_select<256>;
using sampled_select_512 = sampled_select<512>;

} // namespace succinct

#endif // SAMPLED_SELECT_HPP
