#ifndef WAVELET_TREE_HPP
#define WAVELET_TREE_HPP

#include "bit_vector.hpp"
#include "interleaved_rank.hpp"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace succinct {

// Succinct Wavelet Tree over byte alphabet [0 ... 255]
// Maps character rank and select queries to O(log Sigma) bitvector rank/select queries.
class wavelet_tree {
public:
    wavelet_tree() : m_size(0), m_alphabet_min(0), m_alphabet_max(0) {}

    explicit wavelet_tree(const std::string& text) {
        if (!text.empty()) {
            build(text);
        }
    }

    void build(const std::string& text) {
        m_size = text.length();
        if (m_size == 0) return;

        uint8_t min_c = 255, max_c = 0;
        for (char ch : text) {
            uint8_t c = static_cast<uint8_t>(ch);
            min_c = std::min(min_c, c);
            max_c = std::max(max_c, c);
        }
        m_alphabet_min = min_c;
        m_alphabet_max = max_c;

        m_root = build_node(text.begin(), text.end(), min_c, max_c);
    }

    // Rank of character c in prefix [0 ... i-1]
    uint64_t rank(char ch, size_t i) const {
        uint8_t c = static_cast<uint8_t>(ch);
        if (c < m_alphabet_min || c > m_alphabet_max || i == 0 || !m_root) {
            return 0;
        }
        return rank_node(m_root.get(), c, std::min(i, m_size));
    }

    // Access character at position i
    char access(size_t i) const {
        if (!m_root || i >= m_size) {
            throw std::out_of_range("wavelet_tree::access out of range");
        }
        return static_cast<char>(access_node(m_root.get(), i));
    }

    size_t size() const { return m_size; }

    size_t index_bytes() const {
        return sizeof(*this) + count_node_bytes(m_root.get());
    }

private:
    struct Node {
        uint8_t low_char;
        uint8_t high_char;
        bit_vector bv;
        interleaved_rank_256 rank_idx;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;

        Node(uint8_t low, uint8_t high) : low_char(low), high_char(high) {}
    };

    template <typename It>
    std::unique_ptr<Node> build_node(It begin, It end, uint8_t low, uint8_t high) {
        if (begin >= end || low >= high) {
            return nullptr;
        }

        auto node = std::make_unique<Node>(low, high);
        uint8_t mid = low + (high - low) / 2;
        size_t len = std::distance(begin, end);

        node->bv.resize(len, false);
        std::string left_str, right_str;
        left_str.reserve(len);
        right_str.reserve(len);

        size_t idx = 0;
        for (It it = begin; it != end; ++it, ++idx) {
            uint8_t c = static_cast<uint8_t>(*it);
            if (c > mid) {
                node->bv.set(idx);
                right_str.push_back(static_cast<char>(c));
            } else {
                left_str.push_back(static_cast<char>(c));
            }
        }

        node->rank_idx.build(node->bv);

        if (low < mid) {
            node->left = build_node(left_str.begin(), left_str.end(), low, mid);
        }
        if (mid + 1 < high) {
            node->right = build_node(right_str.begin(), right_str.end(), mid + 1, high);
        }

        return node;
    }

    uint64_t rank_node(const Node* node, uint8_t c, size_t i) const {
        if (!node || i == 0) return 0;
        uint8_t mid = node->low_char + (node->high_char - node->low_char) / 2;

        if (c > mid) {
            uint64_t ones_up_to_i = node->rank_idx.rank1(i);
            if (node->low_char == node->high_char || !node->right) {
                return ones_up_to_i;
            }
            return rank_node(node->right.get(), c, static_cast<size_t>(ones_up_to_i));
        } else {
            uint64_t zeros_up_to_i = node->rank_idx.rank0(i);
            if (node->low_char == node->high_char || !node->left) {
                return zeros_up_to_i;
            }
            return rank_node(node->left.get(), c, static_cast<size_t>(zeros_up_to_i));
        }
    }

    uint8_t access_node(const Node* node, size_t i) const {
        if (node->low_char == node->high_char) {
            return node->low_char;
        }
        uint8_t mid = node->low_char + (node->high_char - node->low_char) / 2;
        bool bit = node->bv.get(i);
        if (bit) {
            uint64_t next_i = node->rank_idx.rank1(i);
            if (!node->right) return node->high_char;
            return access_node(node->right.get(), static_cast<size_t>(next_i));
        } else {
            uint64_t next_i = node->rank_idx.rank0(i);
            if (!node->left) return node->low_char;
            return access_node(node->left.get(), static_cast<size_t>(next_i));
        }
    }

    size_t count_node_bytes(const Node* node) const {
        if (!node) return 0;
        size_t bytes = sizeof(Node) + node->bv.allocated_bytes() + node->rank_idx.total_allocated_bytes();
        bytes += count_node_bytes(node->left.get());
        bytes += count_node_bytes(node->right.get());
        return bytes;
    }

    size_t m_size;
    uint8_t m_alphabet_min;
    uint8_t m_alphabet_max;
    std::unique_ptr<Node> m_root;
};

} // namespace succinct

#endif // WAVELET_TREE_HPP
