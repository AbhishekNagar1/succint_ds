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
#include <cassert>
#include <string>
#include <vector>
#include <iomanip>

using namespace succinct;

// Color styling for terminal outputs
#define ANSI_GREEN  "\033[32m"
#define ANSI_RED    "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE   "\033[34m"
#define ANSI_BOLD   "\033[1m"
#define ANSI_RESET  "\033[0m"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

void verify_condition(bool condition, const std::string& test_name, const std::string& details = "") {
    if (condition) {
        g_tests_passed++;
        std::cout << "  [" ANSI_GREEN "PASS" ANSI_RESET "] " << test_name << "\n";
    } else {
        g_tests_failed++;
        std::cerr << "  [" ANSI_RED "FAIL" ANSI_RESET "] " << test_name;
        if (!details.empty()) {
            std::cerr << " -> " << details;
        }
        std::cerr << "\n";
    }
}

// 1. Test 64-bit Word Primitives
void test_primitives() {
    std::cout << "\n" ANSI_BOLD ANSI_BLUE "=== [Test Suite 1: 64-bit Word Primitives (popcount & select64)] ===" ANSI_RESET "\n";

    // Exhaustive test on all 1-byte patterns
    for (uint64_t val = 0; val < 256; ++val) {
        uint64_t blt = popcount_blt(val);
        uint64_t tbl = popcount_tbl(val);
        uint64_t bw  = popcount_bw(val);
        if (blt != tbl || blt != bw) {
            verify_condition(false, "popcount byte exhaustive", "val=" + std::to_string(val));
            return;
        }
    }
    verify_condition(true, "All 8-bit popcount patterns match between TBL, BW, and BLT");

    // Test select64 on various fixed patterns
    uint64_t test_words[] = {
        0x0ULL,
        0x1ULL,
        0x8000000000000000ULL,
        0xAAAAAAAAAAAAAAAAULL,
        0x5555555555555555ULL,
        0xFFFFFFFFFFFFFFFFULL,
        0x0123456789ABCDEFULL,
        0xFEDCBA9876543210ULL
    };

    bool select_ok = true;
    for (uint64_t w : test_words) {
        uint32_t ones = popcount_blt(w);
        for (uint32_t i = 0; i < ones; ++i) {
            uint32_t lin = select64_linear(w, i);
            uint32_t tbl = select64_tbl(w, i);
            uint32_t bw  = select64_bw(w, i);
            uint32_t blt = select64_blt(w, i);
            if (lin != tbl || lin != bw || lin != blt) {
                select_ok = false;
                std::cerr << "Mismatch on word 0x" << std::hex << w << std::dec
                          << " i=" << i << " lin=" << lin << " tbl=" << tbl << " bw=" << bw << " blt=" << blt << "\n";
                break;
            }
        }
        if (!select_ok) break;
    }
    verify_condition(select_ok, "Fixed 64-bit select64 matches across Linear, TBL, BW, and BLT (Gog & Petri Listing 1)");
}

// 2. Comprehensive Exhaustive Comparison between Naive and Succinct Implementations
void test_bitvector_exhaustive(const bit_vector& bv, const std::string& desc) {
    naive_rank_select naive(&bv);
    block_rank_512_64 blk_rank(&bv);
    interleaved_rank_256 il_rank(&bv);
    sampled_select_64 smp_sel(&bv);
    binary_search_select<block_rank_512_64> bs_sel(&bv, &blk_rank);
    clark_select_512_64 clk_sel(&bv);

    size_t n = bv.size();
    uint64_t total_ones = bv.count_ones();

    // Verify Rank across every single position [0 ... n]
    bool rank_match = true;
    for (size_t i = 0; i <= n; ++i) {
        uint64_t r_naive = naive.rank1(i);
        uint64_t r_blk = blk_rank.rank1(i);
        uint64_t r_il = il_rank.rank1(i);

        if (r_naive != r_blk || r_naive != r_il) {
            rank_match = false;
            std::cerr << "Rank mismatch at i=" << i << " in " << desc
                      << " naive=" << r_naive << " blk=" << r_blk << " il=" << r_il << "\n";
            break;
        }
    }
    verify_condition(rank_match, desc + ": Rank1 matches 100% with naive baseline across all i in [0, n]");

    // Verify Select across all valid k in [1 ... total_ones]
    bool select_match = true;
    for (uint64_t k = 1; k <= total_ones; ++k) {
        uint64_t s_naive = naive.select1(k);
        uint64_t s_smp = smp_sel.select1(k);
        uint64_t s_bs = bs_sel.select1(k);
        uint64_t s_clk = clk_sel.select1(k);

        if (s_naive != s_smp || s_naive != s_bs || s_naive != s_clk) {
            select_match = false;
            std::cerr << "Select mismatch at k=" << k << " in " << desc
                      << " naive=" << s_naive << " smp=" << s_smp << " bs=" << s_bs << " clk=" << s_clk << "\n";
            break;
        }
    }
    verify_condition(select_match, desc + ": Select1 matches 100% with naive baseline for all k in [1, ones]");

    // Verify Out-of-bounds Select (k=0 and k > total_ones)
    bool oob_ok = (naive.select1(0) == NOT_FOUND) &&
                  (smp_sel.select1(0) == NOT_FOUND) &&
                  (bs_sel.select1(0) == NOT_FOUND) &&
                  (clk_sel.select1(0) == NOT_FOUND) &&
                  (naive.select1(total_ones + 1) == NOT_FOUND) &&
                  (smp_sel.select1(total_ones + 1) == NOT_FOUND) &&
                  (bs_sel.select1(total_ones + 1) == NOT_FOUND) &&
                  (clk_sel.select1(total_ones + 1) == NOT_FOUND);
    verify_condition(oob_ok, desc + ": Select out-of-bounds (k=0, k > ones) returns NOT_FOUND across all models");
}

// 3. Test Extreme Edge Cases from Slide 12
void test_extreme_edge_cases() {
    std::cout << "\n" ANSI_BOLD ANSI_BLUE "=== [Test Suite 2: Extreme Edge Cases (Slide 12)] ===" ANSI_RESET "\n";

    // Case 1: Empty bitvector
    bit_vector empty_bv(0);
    test_bitvector_exhaustive(empty_bv, "Empty bitvector (n = 0)");

    // Case 2: All-Zero vector (n = 1024)
    bit_vector all_zero(1024, false);
    test_bitvector_exhaustive(all_zero, "All-Zero vector (n = 1024)");

    // Case 3: All-One vector (n = 1024)
    bit_vector all_one(1024, true);
    test_bitvector_exhaustive(all_one, "All-One vector (n = 1024)");

    // Case 4: Single Bit vectors ([0] and [1])
    bit_vector single_zero("0");
    test_bitvector_exhaustive(single_zero, "Single-Bit vector [0]");

    bit_vector single_one("1");
    test_bitvector_exhaustive(single_one, "Single-Bit vector [1]");

    // Case 5: Word Boundary tests (lengths around 63, 64, 65, 127, 128, 129, 255, 256, 511, 512, 513)
    size_t boundary_lens[] = {63, 64, 65, 127, 128, 129, 255, 256, 257, 511, 512, 513, 1023, 1024, 1025};
    for (size_t blen : boundary_lens) {
        bit_vector bv = dataset_generator::generate_synthetic(blen, 0.35, 1000 + blen);
        test_bitvector_exhaustive(bv, "Boundary length n = " + std::to_string(blen));
    }
}

// 4. Test Randomized Vectors with Varying Densities (5%, 20%, 50%, 80%)
void test_randomized_verification() {
    std::cout << "\n" ANSI_BOLD ANSI_BLUE "=== [Test Suite 3: Randomized Verification with Fixed Seeds] ===" ANSI_RESET "\n";

    double densities[] = {0.05, 0.20, 0.50, 0.80};
    size_t sizes[] = {500, 2048, 8192};

    for (double d : densities) {
        for (size_t sz : sizes) {
            std::string desc = "Random Vector n=" + std::to_string(sz) + " (density=" + std::to_string(int(d * 100)) + "%)";
            bit_vector bv = dataset_generator::generate_synthetic(sz, d, 4242 + sz);
            test_bitvector_exhaustive(bv, desc);
        }
    }
}

int main() {
    init_primitives();

    std::cout << ANSI_BOLD ANSI_YELLOW
              << "========================================================================\n"
              << "       CORRECTNESS TEST SUITE FOR SUCCINCT RANK AND SELECT              \n"
              << "       M.Tech ADSA Project - Abhishek Nagar & Kishan Lambariya          \n"
              << "========================================================================"
              << ANSI_RESET "\n";

    test_primitives();
    test_extreme_edge_cases();
    test_randomized_verification();

    std::cout << "\n" ANSI_BOLD << "------------------------------------------------------------------------\n";
    if (g_tests_failed == 0) {
        std::cout << ANSI_GREEN "ALL " << g_tests_passed << " TESTS PASSED! 100% AGREEMENT VERIFIED." ANSI_RESET "\n";
    } else {
        std::cout << ANSI_RED << g_tests_failed << " TESTS FAILED out of "
                  << (g_tests_passed + g_tests_failed) << " tests." ANSI_RESET "\n";
    }
    std::cout << "------------------------------------------------------------------------\n" ANSI_RESET;

    return g_tests_failed == 0 ? 0 : 1;
}
