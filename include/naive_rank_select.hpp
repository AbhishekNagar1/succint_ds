#ifndef NAIVE_RANK_SELECT_HPP
#define NAIVE_RANK_SELECT_HPP

#include "bit_vector.hpp"
#include <limits>
#include <cstdint>

namespace succinct {

constexpr uint64_t NOT_FOUND = std::numeric_limits<uint64_t>::max();

class naive_rank_select {
public:
    explicit naive_rank_select(const bit_vector* bv = nullptr) : m_bv(bv) {}

    void set_bit_vector(const bit_vector* bv) {
        m_bv = bv;
    }

    // Naive Rank1(i): Number of 1s in prefix B[0 ... i-1]
    // Time Complexity: O(i), worst case O(n)
    // Extra Index Space: O(1)
    uint64_t rank1(size_t i) const {
        if (!m_bv || i == 0) return 0;
        size_t limit = (i > m_bv->size()) ? m_bv->size() : i;
        uint64_t count = 0;
        for (size_t j = 0; j < limit; ++j) {
            if (m_bv->get(j)) {
                count++;
            }
        }
        return count;
    }

    // Rank0(i): Number of 0s in prefix B[0 ... i-1]
    uint64_t rank0(size_t i) const {
        if (!m_bv || i == 0) return 0;
        size_t limit = (i > m_bv->size()) ? m_bv->size() : i;
        return limit - rank1(limit);
    }

    // Naive Select1(k): 0-based bit position of the k-th 1 (k is 1-based)
    // Time Complexity: O(n)
    // Extra Space: O(1)
    uint64_t select1(uint64_t k) const {
        if (!m_bv || k == 0) return NOT_FOUND;
        uint64_t seen = 0;
        size_t n = m_bv->size();
        for (size_t j = 0; j < n; ++j) {
            if (m_bv->get(j)) {
                seen++;
                if (seen == k) {
                    return j;
                }
            }
        }
        return NOT_FOUND;
    }

    // Naive Select0(k): 0-based bit position of the k-th 0 (k is 1-based)
    uint64_t select0(uint64_t k) const {
        if (!m_bv || k == 0) return NOT_FOUND;
        uint64_t seen = 0;
        size_t n = m_bv->size();
        for (size_t j = 0; j < n; ++j) {
            if (!m_bv->get(j)) {
                seen++;
                if (seen == k) {
                    return j;
                }
            }
        }
        return NOT_FOUND;
    }

    size_t index_bytes() const {
        return sizeof(*this);
    }

private:
    const bit_vector* m_bv;
};

} // namespace succinct

#endif // NAIVE_RANK_SELECT_HPP
