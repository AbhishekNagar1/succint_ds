#include "bit_vector.hpp"
#include <sstream>

namespace succinct {

bit_vector::bit_vector() : m_size(0) {}

bit_vector::bit_vector(size_t num_bits, bool init_val) : m_size(num_bits) {
    size_t words = (num_bits + 63) / 64;
    m_data.resize(words, init_val ? ~0ULL : 0ULL);
    clean_padding();
}

bit_vector::bit_vector(const std::string& bit_str) : m_size(bit_str.length()) {
    size_t words = (m_size + 63) / 64;
    m_data.resize(words, 0ULL);
    for (size_t i = 0; i < m_size; ++i) {
        if (bit_str[i] == '1') {
            set(i);
        } else if (bit_str[i] != '0') {
            throw std::invalid_argument("bit_vector string must contain only '0' and '1'");
        }
    }
}

bit_vector::bit_vector(const std::vector<uint64_t>& raw_words, size_t num_bits)
    : m_size(num_bits), m_data(raw_words) {
    size_t expected_words = (num_bits + 63) / 64;
    if (m_data.size() < expected_words) {
        m_data.resize(expected_words, 0ULL);
    }
    clean_padding();
}

void bit_vector::clean_padding() {
    if (m_size == 0) return;
    size_t rem = m_size & 63;
    if (rem != 0 && !m_data.empty()) {
        uint64_t mask = (1ULL << rem) - 1ULL;
        m_data.back() &= mask;
    }
}

void bit_vector::resize(size_t num_bits, bool init_val) {
    size_t old_size = m_size;
    m_size = num_bits;
    size_t words = (num_bits + 63) / 64;
    m_data.resize(words, init_val ? ~0ULL : 0ULL);
    if (num_bits > old_size && init_val) {
        for (size_t i = old_size; i < num_bits; ++i) {
            set(i);
        }
    }
    clean_padding();
}

uint64_t bit_vector::count_ones() const {
    uint64_t total = 0;
    for (uint64_t w : m_data) {
        total += popcount64(w);
    }
    return total;
}

std::string bit_vector::to_string(size_t max_bits) const {
    std::ostringstream oss;
    size_t limit = (m_size < max_bits) ? m_size : max_bits;
    for (size_t i = 0; i < limit; ++i) {
        oss << (get(i) ? '1' : '0');
        if ((i + 1) % 8 == 0 && (i + 1) < limit) {
            oss << ' ';
        }
    }
    if (m_size > max_bits) {
        oss << "... (" << m_size << " bits total)";
    }
    return oss.str();
}

void bit_vector::save(std::ostream& os) const {
    os.write(reinterpret_cast<const char*>(&m_size), sizeof(m_size));
    size_t words = m_data.size();
    os.write(reinterpret_cast<const char*>(&words), sizeof(words));
    if (words > 0) {
        os.write(reinterpret_cast<const char*>(m_data.data()), words * sizeof(uint64_t));
    }
}

void bit_vector::load(std::istream& is) {
    is.read(reinterpret_cast<char*>(&m_size), sizeof(m_size));
    size_t words = 0;
    is.read(reinterpret_cast<char*>(&words), sizeof(words));
    m_data.resize(words);
    if (words > 0) {
        is.read(reinterpret_cast<char*>(m_data.data()), words * sizeof(uint64_t));
    }
}

} // namespace succinct
