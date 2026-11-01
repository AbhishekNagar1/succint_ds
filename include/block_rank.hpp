#ifndef BLOCK_RANK_HPP
#define BLOCK_RANK_HPP

#include "bit_vector.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace succinct {

// Two-level Jacobson / Vigna style Block Rank
// Rs: 64-bit Superblock counters every S bits
// Rb: 16-bit Block counters relative to Superblock every B bits
template <size_t SuperblockSize = 512, size_t BlockSize = 64>
class block_rank {
public:
    static_assert(SuperblockSize % BlockSize == 0, "SuperblockSize must be a multiple of BlockSize");
    static_assert(BlockSize % 64 == 0, "BlockSize must be a multiple of 64 bits");
    static_assert(SuperblockSize <= 65536, "SuperblockSize must fit within 16-bit relative counters");

    explicit block_rank(const bit_vector* bv = nullptr) : m_bv(bv) {
        if (bv) {
            build();
        }
    }

    void build() {
        if (!m_bv) return;
        size_t n = m_bv->size();
        size_t num_superblocks = n / SuperblockSize + 1;
        size_t num_blocks = n / BlockSize + 1;

        m_superblocks.assign(num_superblocks + 1, 0);
        m_blocks.assign(num_blocks + 1, 0);

        uint64_t cumulative = 0;
        size_t next_sb_bit = 0;
        size_t next_blk_bit = 0;
        size_t sb_idx = 0;
        size_t blk_idx = 0;

        size_t total_words = m_bv->word_count();
        for (size_t w = 0; w < total_words; ++w) {
            size_t bit_start = w << 6;
            if (bit_start == next_blk_bit) {
                if (bit_start == next_sb_bit) {
                    m_superblocks[sb_idx] = cumulative;
                    sb_idx++;
                    next_sb_bit += SuperblockSize;
                }
                size_t current_sb = (sb_idx > 0) ? sb_idx - 1 : 0;
                m_blocks[blk_idx] = static_cast<uint16_t>(cumulative - m_superblocks[current_sb]);
                blk_idx++;
                next_blk_bit += BlockSize;
            }
            cumulative += popcount64(m_bv->get_word(w));
        }

        // Sentinels at the end
        while (sb_idx <= num_superblocks) {
            m_superblocks[sb_idx] = cumulative;
            sb_idx++;
        }
        while (blk_idx <= num_blocks) {
            size_t current_sb = (blk_idx * BlockSize) / SuperblockSize;
            if (current_sb >= m_superblocks.size()) current_sb = m_superblocks.size() - 1;
            uint64_t rel = (cumulative >= m_superblocks[current_sb]) ? (cumulative - m_superblocks[current_sb]) : 0;
            m_blocks[blk_idx] = static_cast<uint16_t>(rel);
            blk_idx++;
        }
    }

    // Rank1(i): Number of 1s in B[0 ... i-1]
    inline uint64_t rank1(size_t i) const {
        if (!m_bv || i == 0) return 0;
        if (i > m_bv->size()) i = m_bv->size();

        size_t sb_idx = i / SuperblockSize;
        size_t blk_idx = i / BlockSize;

        if (sb_idx >= m_superblocks.size()) sb_idx = m_superblocks.size() - 1;
        if (blk_idx >= m_blocks.size()) blk_idx = m_blocks.size() - 1;

        uint64_t result = m_superblocks[sb_idx] + m_blocks[blk_idx];

        // Scan the words between the block start and i
        size_t block_start_bit = blk_idx * BlockSize;
        size_t word_start = block_start_bit >> 6;
        size_t target_word = i >> 6;
        size_t bit_offset = i & 63;

        for (size_t w = word_start; w < target_word; ++w) {
            result += popcount64(m_bv->get_word(w));
        }

        if (bit_offset != 0) {
            uint64_t mask = (1ULL << bit_offset) - 1ULL;
            result += popcount64(m_bv->get_word(target_word) & mask);
        }

        return result;
    }

    inline uint64_t rank0(size_t i) const {
        if (!m_bv || i == 0) return 0;
        if (i > m_bv->size()) i = m_bv->size();
        return i - rank1(i);
    }

    size_t index_bytes() const {
        return sizeof(*this) +
               m_superblocks.capacity() * sizeof(uint64_t) +
               m_blocks.capacity() * sizeof(uint16_t);
    }

    double overhead_percent() const {
        if (!m_bv || m_bv->allocated_bytes() == 0) return 0.0;
        return (static_cast<double>(index_bytes()) / m_bv->allocated_bytes()) * 100.0;
    }

    size_t superblock_count() const { return m_superblocks.size(); }
    size_t block_count() const { return m_blocks.size(); }

    const std::vector<uint64_t>& get_superblocks() const { return m_superblocks; }
    const std::vector<uint16_t>& get_blocks() const { return m_blocks; }

private:
    const bit_vector* m_bv;
    std::vector<uint64_t> m_superblocks;
    std::vector<uint16_t> m_blocks;
};

// Aliases for common configurations
using block_rank_512_64 = block_rank<512, 64>;
using block_rank_1024_128 = block_rank<1024, 128>;
using block_rank_2048_256 = block_rank<2048, 256>;

} // namespace succinct

#endif // BLOCK_RANK_HPP
