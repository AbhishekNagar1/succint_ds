#ifndef BIT_VECTOR_HPP
#define BIT_VECTOR_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>
#include <iostream>
#include <stdexcept>
#include "primitives.hpp"

namespace succinct {

class bit_vector {
public:
    bit_vector();
    explicit bit_vector(size_t num_bits, bool init_val = false);
    explicit bit_vector(const std::string& bit_str);
    bit_vector(const std::vector<uint64_t>& raw_words, size_t num_bits);

    // Core bitwise accessors
    inline bool get(size_t i) const {
        if (i >= m_size) {
            throw std::out_of_range("bit_vector::get index out of range: " + std::to_string(i));
        }
        return (m_data[i >> 6] >> (i & 63)) & 1ULL;
    }

    inline bool operator[](size_t i) const {
        return (m_data[i >> 6] >> (i & 63)) & 1ULL;
    }

    inline void set(size_t i) {
        if (i >= m_size) {
            throw std::out_of_range("bit_vector::set index out of range");
        }
        m_data[i >> 6] |= (1ULL << (i & 63));
    }

    inline void clear(size_t i) {
        if (i >= m_size) {
            throw std::out_of_range("bit_vector::clear index out of range");
        }
        m_data[i >> 6] &= ~(1ULL << (i & 63));
    }

    inline void toggle(size_t i) {
        if (i >= m_size) {
            throw std::out_of_range("bit_vector::toggle index out of range");
        }
        m_data[i >> 6] ^= (1ULL << (i & 63));
    }

    // Word-level accessors
    inline uint64_t get_word(size_t w) const {
        if (w >= m_data.size()) {
            throw std::out_of_range("bit_vector::get_word word index out of range");
        }
        return m_data[w];
    }

    inline void set_word(size_t w, uint64_t val) {
        if (w >= m_data.size()) {
            throw std::out_of_range("bit_vector::set_word word index out of range");
        }
        m_data[w] = val;
        clean_padding();
    }

    // Capacity and sizes
    inline size_t size() const { return m_size; }
    inline size_t word_count() const { return m_data.size(); }
    inline const uint64_t* data() const { return m_data.data(); }
    inline uint64_t* data() { return m_data.data(); }

    // Memory footprint calculation in bytes
    size_t allocated_bytes() const {
        return sizeof(*this) + m_data.capacity() * sizeof(uint64_t);
    }

    // Total number of 1s in [0 ... size-1]
    uint64_t count_ones() const;

    // Formatting for display and I/O
    std::string to_string(size_t max_bits = 64) const;

    // Resize
    void resize(size_t num_bits, bool init_val = false);

    // Serialization
    void save(std::ostream& os) const;
    void load(std::istream& is);

private:
    void clean_padding();

    size_t m_size;
    std::vector<uint64_t> m_data;
};

} // namespace succinct

#endif // BIT_VECTOR_HPP
