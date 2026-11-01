#ifndef BINARY_SEARCH_SELECT_HPP
#define BINARY_SEARCH_SELECT_HPP

#include "bit_vector.hpp"
#include "block_rank.hpp"
#include "naive_rank_select.hpp"
#include "primitives.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace succinct {

// Binary Search Select (SEL-BS and SEL-BSH from Gog & Petri 2014, Section 4)
// Finds k-th 1 by binary search over rank superblocks, then word scan + select64.
// SEL-BS uses 0% extra memory beyond the Rank structure!
template <typename RankSupport = block_rank_512_64>
class binary_search_select {
public:
    explicit binary_search_select(const bit_vector* bv = nullptr, const RankSupport* rank_sup = nullptr)
        : m_bv(bv), m_rank(rank_sup), m_total_ones(0) {
        if (bv && rank_sup) {
            m_total_ones = bv->count_ones();
        }
    }

    void init(const bit_vector* bv, const RankSupport* rank_sup) {
        m_bv = bv;
        m_rank = rank_sup;
        if (bv) {
            m_total_ones = bv->count_ones();
        }
    }

    // Select1(k): 0-based bit position of k-th 1 (k is 1-based)
    inline uint64_t select1(uint64_t k) const {
        if (!m_bv || !m_rank || k == 0 || k > m_total_ones) {
            return NOT_FOUND;
        }

        // 1. Binary search over bit indices [0 ... n-1] or superblocks
        size_t low = 0;
        size_t high = m_bv->size();

        while (low < high) {
            size_t mid = low + (high - low) / 2;
            if (m_rank->rank1(mid) < k) {
                low = mid + 1;
            } else {
                high = mid;
            }
        }

        // low is now the smallest index where rank1(low) >= k, so the (k-th 1) is at low - 1
        if (low > 0 && low <= m_bv->size()) {
            return low - 1;
        }
        return NOT_FOUND;
    }

    uint64_t total_ones() const { return m_total_ones; }

    size_t index_bytes() const {
        // SEL-BS leverages existing rank index, 0 bytes extra overhead
        return sizeof(*this);
    }

    double overhead_percent() const {
        return 0.0;
    }

private:
    const bit_vector* m_bv;
    const RankSupport* m_rank;
    uint64_t m_total_ones;
};

} // namespace succinct

#endif // BINARY_SEARCH_SELECT_HPP
