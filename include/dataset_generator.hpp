#ifndef DATASET_GENERATOR_HPP
#define DATASET_GENERATOR_HPP

#include "bit_vector.hpp"
#include <random>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <iostream>

namespace succinct {

class dataset_generator {
public:
    // Generate synthetic bitvector with length n, density d (0.0 to 1.0), and reproducible seed
    static bit_vector generate_synthetic(size_t n, double density, uint64_t seed = 42) {
        std::mt19937_64 rng(seed);
        std::bernoulli_distribution dist(density);

        bit_vector bv(n, false);
        for (size_t i = 0; i < n; ++i) {
            if (dist(rng)) {
                bv.set(i);
            }
        }
        return bv;
    }

    // Convert raw text/binary file into bitvector (ASCII 8-bit stream)
    static bit_vector from_file_ascii(const std::string& filepath, size_t max_bytes = 0) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }

        std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        size_t byte_count = buffer.size();
        if (max_bytes > 0 && max_bytes < byte_count) {
            byte_count = max_bytes;
        }

        bit_vector bv(byte_count * 8, false);
        for (size_t i = 0; i < byte_count; ++i) {
            uint8_t byte_val = static_cast<uint8_t>(buffer[i]);
            for (size_t b = 0; b < 8; ++b) {
                if ((byte_val >> b) & 1) {
                    bv.set(i * 8 + b);
                }
            }
        }
        return bv;
    }

    // Convert text string into bitvector (DNA / text indicator vector for character c)
    static bit_vector character_mask_bitvector(const std::string& text, char target_char) {
        bit_vector bv(text.length(), false);
        for (size_t i = 0; i < text.length(); ++i) {
            if (text[i] == target_char) {
                bv.set(i);
            }
        }
        return bv;
    }

    // Pre-generate random query workloads for Rank: i in [0, n]
    static std::vector<size_t> generate_rank_queries(size_t n, size_t count, uint64_t seed = 1337) {
        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, n);
        std::vector<size_t> queries(count);
        for (size_t i = 0; i < count; ++i) {
            queries[i] = dist(rng);
        }
        return queries;
    }

    // Pre-generate random query workloads for Select:
    // 95% valid k in [1, total_ones], 5% boundary/invalid (k=0 or k > total_ones)
    static std::vector<uint64_t> generate_select_queries(uint64_t total_ones, size_t count, uint64_t seed = 2026) {
        std::mt19937_64 rng(seed);
        std::vector<uint64_t> queries(count);
        if (total_ones == 0) {
            queries.assign(count, 1);
            return queries;
        }

        std::uniform_int_distribution<uint64_t> valid_dist(1, total_ones);
        std::uniform_real_distribution<double> coin(0.0, 1.0);

        for (size_t i = 0; i < count; ++i) {
            if (coin(rng) < 0.95) {
                queries[i] = valid_dist(rng);
            } else {
                // Edge cases: 0 or total_ones + 1
                queries[i] = (coin(rng) < 0.5) ? 0 : (total_ones + 1);
            }
        }
        return queries;
    }
};

} // namespace succinct

#endif // DATASET_GENERATOR_HPP
