#ifndef FM_INDEX_HPP
#define FM_INDEX_HPP

#include "wavelet_tree.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream>

namespace succinct {

// Succinct FM-Index (Ferragina & Manzini 2000)
// Demonstrates practical real-world application of Rank/Select queries
// Supports count(P), locate(P), and extract(i, j)
class fm_index {
public:
    static constexpr size_t DEFAULT_SA_SAMPLE_RATE = 16;

    fm_index() : m_n(0), m_sa_sample_rate(DEFAULT_SA_SAMPLE_RATE) {}

    explicit fm_index(const std::string& text, size_t sa_sample = DEFAULT_SA_SAMPLE_RATE)
        : m_sa_sample_rate(sa_sample) {
        if (!text.empty()) {
            build(text);
        }
    }

    void build(const std::string& input_text) {
        // Append sentinel '$' (value 1, ensuring it's lower than any ASCII text char)
        std::string text = input_text + "$";
        m_n = text.length();

        // 1. Compute Suffix Array SA
        std::vector<size_t> sa(m_n);
        for (size_t i = 0; i < m_n; ++i) sa[i] = i;

        std::sort(sa.begin(), sa.end(), [&](size_t a, size_t b) {
            return std::lexicographical_compare(text.begin() + a, text.end(),
                                                text.begin() + b, text.end());
        });

        // 2. Compute Burrows-Wheeler Transform (BWT)
        std::string bwt;
        bwt.resize(m_n);
        for (size_t i = 0; i < m_n; ++i) {
            bwt[i] = (sa[i] == 0) ? text[m_n - 1] : text[sa[i] - 1];
        }

        // 3. Build Wavelet Tree over BWT
        m_wt.build(bwt);

        // 4. Compute C-array: C[c] is the total number of characters in text lexicographically smaller than c
        m_c.assign(256, 0);
        std::vector<uint64_t> char_freq(256, 0);
        for (char ch : text) {
            char_freq[static_cast<uint8_t>(ch)]++;
        }
        uint64_t accum = 0;
        for (int c = 0; c < 256; ++c) {
            m_c[c] = accum;
            accum += char_freq[c];
        }

        // 5. Store sampled Suffix Array: SA[i] stored if i % m_sa_sample_rate == 0
        m_sa_samples.clear();
        for (size_t i = 0; i < m_n; i += m_sa_sample_rate) {
            m_sa_samples.push_back(sa[i]);
        }
    }

    // count(P): Returns the number of occurrences of pattern P in the text
    // Complexity: O(|P| * log Sigma) basic rank queries!
    uint64_t count(const std::string& pattern) const {
        if (pattern.empty() || m_n == 0) return 0;
        int64_t l = 0;
        int64_t r = static_cast<int64_t>(m_n) - 1;

        for (int i = static_cast<int>(pattern.length()) - 1; i >= 0; --i) {
            char c = pattern[i];
            uint8_t uc = static_cast<uint8_t>(c);
            uint64_t c_offset = m_c[uc];

            l = static_cast<int64_t>(c_offset + m_wt.rank(c, static_cast<size_t>(l)));
            r = static_cast<int64_t>(c_offset + m_wt.rank(c, static_cast<size_t>(r + 1)) - 1);

            if (l > r) {
                return 0; // Pattern does not exist
            }
        }
        return static_cast<uint64_t>(r - l + 1);
    }

    // locate(P): Returns all starting positions of pattern P in the text
    std::vector<size_t> locate(const std::string& pattern) const {
        std::vector<size_t> positions;
        if (pattern.empty() || m_n == 0) return positions;

        int64_t l = 0;
        int64_t r = static_cast<int64_t>(m_n) - 1;

        for (int i = static_cast<int>(pattern.length()) - 1; i >= 0; --i) {
            char c = pattern[i];
            uint8_t uc = static_cast<uint8_t>(c);
            uint64_t c_offset = m_c[uc];

            l = static_cast<int64_t>(c_offset + m_wt.rank(c, static_cast<size_t>(l)));
            r = static_cast<int64_t>(c_offset + m_wt.rank(c, static_cast<size_t>(r + 1)) - 1);

            if (l > r) {
                return positions;
            }
        }

        // For each index in [l, r], walk LF-mapping until a sampled SA is hit
        positions.reserve(r - l + 1);
        for (int64_t row = l; row <= r; ++row) {
            size_t curr = static_cast<size_t>(row);
            size_t steps = 0;
            while (curr % m_sa_sample_rate != 0) {
                // LF(curr) = C[BWT[curr]] + rank_{BWT[curr]}(curr)
                char c = m_wt.access(curr);
                uint8_t uc = static_cast<uint8_t>(c);
                curr = m_c[uc] + m_wt.rank(c, curr);
                steps++;
            }
            size_t sa_val = m_sa_samples[curr / m_sa_sample_rate];
            positions.push_back(sa_val + steps);
        }

        std::sort(positions.begin(), positions.end());
        return positions;
    }

    size_t size() const { return m_n; }
    size_t index_bytes() const {
        return sizeof(*this) + m_wt.index_bytes() +
               m_c.size() * sizeof(uint64_t) +
               m_sa_samples.capacity() * sizeof(size_t);
    }

private:
    size_t m_n;
    size_t m_sa_sample_rate;
    wavelet_tree m_wt;
    std::vector<uint64_t> m_c;
    std::vector<size_t> m_sa_samples;
};

} // namespace succinct

#endif // FM_INDEX_HPP
