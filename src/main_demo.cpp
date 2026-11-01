#include "bit_vector.hpp"
#include "naive_rank_select.hpp"
#include "block_rank.hpp"
#include "interleaved_rank.hpp"
#include "sampled_select.hpp"
#include "binary_search_select.hpp"
#include "clark_select.hpp"
#include "dataset_generator.hpp"
#include "primitives.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <vector>

using namespace succinct;

#define ANSI_GREEN  "\033[32m"
#define ANSI_RED    "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE   "\033[34m"
#define ANSI_CYAN   "\033[36m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_BOLD   "\033[1m"
#define ANSI_RESET  "\033[0m"

void print_header() {
    std::cout << ANSI_BOLD ANSI_CYAN
              << "╔═══════════════════════════════════════════════════════════════════════════╗\n"
              << "║       ADVANCED DATA STRUCTURES: SUCCINCT RANK & SELECT SYSTEM             ║\n"
              << "║       Implementation & Empirical Analysis (Gog & Petri 2014, Vigna 2008)  ║\n"
              << "║       Abhishek Nagar (206126002)  |  Kishan Lambariya (206126017)         ║\n"
              << "╚═══════════════════════════════════════════════════════════════════════════╝\n"
              << ANSI_RESET << "\n";
}

void print_bitvector_info(const bit_vector& bv, const std::string& label) {
    std::cout << ANSI_BOLD ANSI_YELLOW << "─── [Bitvector: " << label << "] ───" << ANSI_RESET << "\n";
    std::cout << "  • Length (n):        " << bv.size() << " bits\n";
    std::cout << "  • 64-bit Words:      " << bv.word_count() << " words (" << bv.word_count() * 8 << " bytes)\n";
    std::cout << "  • Number of 1s (m):  " << bv.count_ones() << "\n";
    std::cout << "  • Bit Density:       " << std::fixed << std::setprecision(2)
              << (bv.size() > 0 ? (double(bv.count_ones()) / bv.size() * 100.0) : 0.0) << " %\n";
    std::cout << "  • Memory Footprint:  " << bv.allocated_bytes() << " bytes\n";
    std::cout << "  • Content (Visual):  " << ANSI_GREEN;
    size_t display_len = std::min(bv.size(), size_t(64));
    for (size_t i = 0; i < display_len; ++i) {
        if (i % 8 == 0 && i > 0) std::cout << " ";
        std::cout << (bv.get(i) ? '1' : '0');
    }
    if (bv.size() > 64) std::cout << " ... (" << bv.size() << " bits total)";
    std::cout << ANSI_RESET << "\n\n";
}

void demo_rank(const bit_vector& bv, size_t query_i) {
    naive_rank_select naive(&bv);
    block_rank_512_64 blk(&bv);
    interleaved_rank_256 il(&bv);

    std::cout << ANSI_BOLD ANSI_MAGENTA << ">>> [QUERY: Rank1(" << query_i << ") - Number of 1s in prefix B[0 ... " << (query_i > 0 ? query_i - 1 : 0) << "]]" << ANSI_RESET << "\n";

    // Visual prefix representation
    std::cout << "  Visual: [";
    size_t limit = std::min(bv.size(), size_t(32));
    for (size_t j = 0; j < limit; ++j) {
        if (j < query_i) {
            std::cout << ANSI_GREEN << (bv.get(j) ? '1' : '0') << ANSI_RESET;
        } else if (j == query_i) {
            std::cout << ANSI_RED ANSI_BOLD << "|" << (bv.get(j) ? '1' : '0') << ANSI_RESET;
        } else {
            std::cout << "\033[90m" << (bv.get(j) ? '1' : '0') << ANSI_RESET;
        }
    }
    if (bv.size() > 32) std::cout << "...";
    std::cout << "] (Green = Counted Prefix, Red = Query Position " << query_i << ")\n";

    uint64_t r_naive = naive.rank1(query_i);
    uint64_t r_blk = blk.rank1(query_i);
    uint64_t r_il = il.rank1(query_i);

    std::cout << "  ┌───────────────────────┬────────────┬─────────────────────────────┐\n";
    std::cout << "  │ Implementation        │ Result     │ Algorithmic Step            │\n";
    std::cout << "  ├───────────────────────┼────────────┼─────────────────────────────┤\n";
    std::cout << "  │ Naive Baseline        │ " << std::setw(10) << r_naive << " │ Linear Bit Scan O(i)        │\n";
    std::cout << "  │ Block-Based (512, 64) │ " << std::setw(10) << r_blk   << " │ Rs[" << (query_i/512) << "] + Rb[" << (query_i/64) << "] + Popcnt   │\n";
    std::cout << "  │ Interleaved RANK-1L   │ " << std::setw(10) << r_il    << " │ 1-Cache Hit (Superblock+Blk)│\n";
    std::cout << "  └───────────────────────┴────────────┴─────────────────────────────┘\n";
    std::cout << "  • Agreement: " << ((r_naive == r_blk && r_blk == r_il) ? ANSI_GREEN "PERFECT (100% MATCH)" : ANSI_RED "MISMATCH")
              << ANSI_RESET << " | rank0(" << query_i << ") = " << (query_i - r_naive) << "\n\n";
}

void demo_select(const bit_vector& bv, uint64_t query_k) {
    naive_rank_select naive(&bv);
    block_rank_512_64 blk(&bv);
    sampled_select_64 smp(&bv);
    binary_search_select<block_rank_512_64> bs(&bv, &blk);
    clark_select_512_64 clk(&bv);

    std::cout << ANSI_BOLD ANSI_MAGENTA << ">>> [QUERY: Select1(" << query_k << ") - Position of the " << query_k << "-th 1 (1-based)]" << ANSI_RESET << "\n";

    uint64_t pos_naive = naive.select1(query_k);
    uint64_t pos_smp = smp.select1(query_k);
    uint64_t pos_bs = bs.select1(query_k);
    uint64_t pos_clk = clk.select1(query_k);

    std::cout << "  ┌───────────────────────┬────────────┬─────────────────────────────┐\n";
    std::cout << "  │ Implementation        │ Bit Index  │ Query Strategy              │\n";
    std::cout << "  ├───────────────────────┼────────────┼─────────────────────────────┤\n";
    std::cout << "  │ Naive Baseline        │ ";
    if (pos_naive == NOT_FOUND) std::cout << " NOT_FOUND │ Linear Scan O(n)            │\n";
    else std::cout << std::setw(10) << pos_naive << " │ Linear Scan O(n)            │\n";

    std::cout << "  │ Sampled Select (s=64) │ ";
    if (pos_smp == NOT_FOUND) std::cout << " NOT_FOUND │ Sample[" << (query_k > 0 ? (query_k-1)/64 : 0) << "] + Word Popcnt │\n";
    else std::cout << std::setw(10) << pos_smp << " │ Sample[" << (query_k > 0 ? (query_k-1)/64 : 0) << "] + Word Popcnt │\n";

    std::cout << "  │ Binary Search (SEL-BS)│ ";
    if (pos_bs == NOT_FOUND) std::cout << " NOT_FOUND │ Binary Search over Rank Rs  │\n";
    else std::cout << std::setw(10) << pos_bs << " │ Binary Search over Rank Rs  │\n";

    std::cout << "  │ Clark Select (SEL-C)  │ ";
    if (pos_clk == NOT_FOUND) std::cout << " NOT_FOUND │ 2-Level Multi-Inventory     │\n";
    else std::cout << std::setw(10) << pos_clk << " │ 2-Level Multi-Inventory     │\n";
    std::cout << "  └───────────────────────┴────────────┴─────────────────────────────┘\n";

    if (pos_smp != NOT_FOUND) {
        std::cout << "  • Verification: Bit at index " << pos_smp << " is "
                  << (bv.get(pos_smp) ? ANSI_GREEN "1 (CORRECT)" : ANSI_RED "0 (ERROR)")
                  << ANSI_RESET << " | Preceding 1s = " << naive.rank1(pos_smp)
                  << " (Expected " << (query_k - 1) << ")\n\n";
    } else {
        std::cout << "  • Result: NOT_FOUND (k is out of range: valid range is [1, " << bv.count_ones() << "])\n\n";
    }
}

void run_quick_benchmark() {
    std::cout << ANSI_BOLD ANSI_CYAN << "═══════════════════════════════════════════════════════════════════════════\n"
              << "          MICRO-BENCHMARK: 1,000,000 QUERIES PERFORMANCE TEST             \n"
              << "═══════════════════════════════════════════════════════════════════════════\n"
              << ANSI_RESET;

    size_t n = 1 << 20; // 1M bits
    double density = 0.30;
    std::cout << "Generating synthetic bitvector (n = 1,048,576 bits, density = 30%)...\n";
    bit_vector bv = dataset_generator::generate_synthetic(n, density, 9999);

    naive_rank_select naive(&bv);
    block_rank_512_64 blk(&bv);
    interleaved_rank_256 il(&bv);
    sampled_select_64 smp(&bv);
    binary_search_select<block_rank_512_64> bs(&bv, &blk);
    clark_select_512_64 clk(&bv);

    size_t Q = 200000;
    auto rank_queries = dataset_generator::generate_rank_queries(n, Q, 1111);
    auto select_queries = dataset_generator::generate_select_queries(bv.count_ones(), Q, 2222);

    std::cout << "Benchmarking " << Q << " queries per implementation...\n\n";

    // 1. Block Rank
    auto t0 = std::chrono::high_resolution_clock::now();
    uint64_t dummy = 0;
    for (size_t q : rank_queries) dummy += blk.rank1(q);
    auto t1 = std::chrono::high_resolution_clock::now();
    double blk_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

    // 2. Interleaved Rank
    t0 = std::chrono::high_resolution_clock::now();
    for (size_t q : rank_queries) dummy += il.rank1(q);
    t1 = std::chrono::high_resolution_clock::now();
    double il_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

    // 3. Sampled Select
    t0 = std::chrono::high_resolution_clock::now();
    for (uint64_t q : select_queries) dummy += smp.select1(q);
    t1 = std::chrono::high_resolution_clock::now();
    double smp_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

    // 4. Binary Search Select
    t0 = std::chrono::high_resolution_clock::now();
    for (uint64_t q : select_queries) dummy += bs.select1(q);
    t1 = std::chrono::high_resolution_clock::now();
    double bs_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

    // 5. Clark Select
    t0 = std::chrono::high_resolution_clock::now();
    for (uint64_t q : select_queries) dummy += clk.select1(q);
    t1 = std::chrono::high_resolution_clock::now();
    double clk_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

    std::cout << "  ┌─────────────────────────────┬──────────────┬──────────────┬───────────────┐\n";
    std::cout << "  │ Data Structure              │ Latency (ns) │ Throughput   │ Space Overhead│\n";
    std::cout << "  ├─────────────────────────────┼──────────────┼──────────────┼───────────────┤\n";
    std::cout << "  │ Block-Based Rank (512, 64)  │ " << std::setw(10) << std::fixed << std::setprecision(2) << blk_ns
              << " ns │ " << std::setw(8) << std::setprecision(1) << (1000.0 / blk_ns) << " Mops │ "
              << std::setw(11) << std::setprecision(2) << blk.overhead_percent() << " % │\n";

    std::cout << "  │ Interleaved RANK-1L (256)   │ " << std::setw(10) << std::fixed << std::setprecision(2) << il_ns
              << " ns │ " << std::setw(8) << std::setprecision(1) << (1000.0 / il_ns) << " Mops │ "
              << std::setw(11) << std::setprecision(2) << il.overhead_percent() << " % │\n";

    std::cout << "  │ Sampled Select (s = 64)     │ " << std::setw(10) << std::fixed << std::setprecision(2) << smp_ns
              << " ns │ " << std::setw(8) << std::setprecision(1) << (1000.0 / smp_ns) << " Mops │ "
              << std::setw(11) << std::setprecision(2) << smp.overhead_percent() << " % │\n";

    std::cout << "  │ Binary Search Select (BS)   │ " << std::setw(10) << std::fixed << std::setprecision(2) << bs_ns
              << " ns │ " << std::setw(8) << std::setprecision(1) << (1000.0 / bs_ns) << " Mops │ "
              << std::setw(11) << std::setprecision(2) << bs.overhead_percent() << " % │\n";

    std::cout << "  │ Clark 2-Level Select (SEL-C)│ " << std::setw(10) << std::fixed << std::setprecision(2) << clk_ns
              << " ns │ " << std::setw(8) << std::setprecision(1) << (1000.0 / clk_ns) << " Mops │ "
              << std::setw(11) << std::setprecision(2) << clk.overhead_percent() << " % │\n";
    std::cout << "  └─────────────────────────────┴──────────────┴──────────────┴───────────────┘\n";
    std::cout << "  (Dummy accumulator: " << dummy << " to prevent compiler optimization)\n\n";
}

int main(int argc, char* argv[]) {
    init_primitives();
    print_header();

    // 1. Example 1 from Slide 2: B = 101101, rank1(4) = 3, select1(3) = pos 3
    std::cout << ANSI_BOLD ANSI_BLUE "=== [DEMO 1: Slide 2 Presentation Example] ===" ANSI_RESET "\n";
    bit_vector slide_example("101101");
    print_bitvector_info(slide_example, "Slide 2 Example (B = 101101)");
    demo_rank(slide_example, 4);
    demo_select(slide_example, 3);

    // 2. Example 2: Structured 64-bit vector
    std::cout << ANSI_BOLD ANSI_BLUE "=== [DEMO 2: 64-bit Word Representation] ===" ANSI_RESET "\n";
    bit_vector demo_bv("1100101011110000101010101111111100001111010101011001100111110000");
    print_bitvector_info(demo_bv, "Custom 64-bit Vector");
    demo_rank(demo_bv, 16);
    demo_rank(demo_bv, 32);
    demo_select(demo_bv, 1);
    demo_select(demo_bv, 10);
    demo_select(demo_bv, demo_bv.count_ones());
    demo_select(demo_bv, demo_bv.count_ones() + 1); // Edge case

    // 3. Quick Performance Benchmark
    run_quick_benchmark();

    std::cout << ANSI_BOLD ANSI_GREEN << "Demo finished successfully. Ready for professor review!\n" << ANSI_RESET;
    return 0;
}
