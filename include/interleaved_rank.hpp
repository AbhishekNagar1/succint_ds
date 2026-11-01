#ifndef INTERLEAVED_RANK_HPP
#define INTERLEAVED_RANK_HPP

#include "bit_vector.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace succinct {

// Gog & Petri (2014) RANK-1L: Cache-friendly interleaved bitvector representation
// Stores cumulative rank and data words contiguously in memory to achieve single cache-line hits.
template <size_t BlockBits = 256>
class interleaved_rank {
public:
    static_assert(BlockBits % 64 == 0, "BlockBits must be a multiple of 64");
    static constexpr size_t WORDS_PER_BLOCK = BlockBits / 64;

    struct alignas(64) InterleavedBlock {
        uint64_t cumulative_rank;
        uint64_t words[WORDS_PER_BLOCK];
    };

    interleaved_rank() : m_size(0) {}

    explicit interleaved_rank(const bit_vector* bv) : m_size(0) {
        if (bv) {
            build(*bv);
        }
    }

    void build(const bit_vector& bv) {
        m_size = bv.size();
        size_t total_blocks = (m_size + BlockBits - 1) / BlockBits + 1; // +1 sentinel
        m_blocks.resize(total_blocks);

        uint64_t running_rank = 0;
        size_t total_words = bv.word_count();

        for (size_t blk = 0; blk < total_blocks; ++blk) {
            m_blocks[blk].cumulative_rank = running_rank;
            for (size_t w = 0; w < WORDS_PER_BLOCK; ++w) {
                size_t global_word_idx = blk * WORDS_PER_BLOCK + w;
                if (global_word_idx < total_words) {
                    uint64_t word_val = bv.get_word(global_word_idx);
                    m_blocks[blk].words[w] = word_val;
                    running_rank += popcount64(word_val);
                } else {
                    m_blocks[blk].words[w] = 0ULL;
                }
            }
        }
    }

    // Rank1(i): Number of 1s in prefix B[0 ... i-1]
    // Single memory access locality!
    inline uint64_t rank1(size_t i) const {
        if (i == 0) return 0;
        if (i > m_size) i = m_size;

        size_t blk_idx = i / BlockBits;
        size_t offset_bits = i % BlockBits;

        const InterleavedBlock& blk = m_blocks[blk_idx];
        uint64_t result = blk.cumulative_rank;

        size_t full_words = offset_bits >> 6;
        size_t rem_bits = offset_bits & 63;

        for (size_t w = 0; w < full_words; ++w) {
            result += popcount64(blk.words[w]);
        }

        if (rem_bits != 0) {
            uint64_t mask = (1ULL << rem_bits) - 1ULL;
            result += popcount64(blk.words[full_words] & mask);
        }

        return result;
    }

    inline uint64_t rank0(size_t i) const {
        if (i == 0) return 0;
        if (i > m_size) i = m_size;
        return i - rank1(i);
    }

    // Direct access to original bit
    inline bool get(size_t i) const {
        size_t blk_idx = i / BlockBits;
        size_t offset_bits = i % BlockBits;
        size_t w = offset_bits >> 6;
        size_t b = offset_bits & 63;
        return (m_blocks[blk_idx].words[w] >> b) & 1ULL;
    }

    size_t size() const { return m_size; }
    size_t block_count() const { return m_blocks.size(); }

    size_t index_bytes() const {
        // Overhead is the extra cumulative_rank per block
        return m_blocks.size() * sizeof(uint64_t);
    }

    size_t total_allocated_bytes() const {
        return sizeof(*this) + m_blocks.capacity() * sizeof(InterleavedBlock);
    }

    double overhead_percent() const {
        // Ratio of cumulative_rank metadata to bitvector data
        return (1.0 / static_cast<double>(WORDS_PER_BLOCK)) * 100.0;
    }

private:
    size_t m_size;
    std::vector<InterleavedBlock> m_blocks;
};

// Common configurations from Gog & Petri (2014)
using interleaved_rank_256 = interleaved_rank<256>;   // 25% overhead (RANK-1L)
using interleaved_rank_512 = interleaved_rank<512>;   // 12.5% overhead
using interleaved_rank_1024 = interleaved_rank<1024>; // 6.25% overhead

} // namespace succinct

#endif // INTERLEAVED_RANK_HPP
