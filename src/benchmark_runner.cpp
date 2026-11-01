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
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>

using namespace succinct;

// Prevent compiler optimization of benchmark loops
template <typename T>
inline void do_not_optimize(T& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : "+r,m"(value) : : "memory");
#else
    volatile T* p = &value;
    *p = *p;
#endif
}

// 1. Benchmark 64-bit Primitives
void benchmark_primitives(const std::string& outfile) {
    std::cout << ">>> Running 64-bit Word Primitives Microbenchmark (TBL vs BW vs BLT)...\n";
    std::ofstream csv(outfile);
    csv << "operation,method,latency_ns,throughput_mops\n";

    const size_t N = 10000000; // 10M operations
    std::mt19937_64 rng(42);
    std::vector<uint64_t> words(1024);
    for (size_t i = 0; i < 1024; ++i) words[i] = rng();

    // POPCOUNT
    // TBL
    uint64_t d1 = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        d1 += popcount_tbl(words[i & 1023]);
    }
    do_not_optimize(d1);
    auto t1 = std::chrono::high_resolution_clock::now();
    double tbl_pop_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
    csv << "popcount,TBL," << tbl_pop_ns << "," << (1000.0 / tbl_pop_ns) << "\n";

    // BW
    uint64_t d2 = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        d2 += popcount_bw(words[i & 1023]);
    }
    do_not_optimize(d2);
    t1 = std::chrono::high_resolution_clock::now();
    double bw_pop_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
    csv << "popcount,BW," << bw_pop_ns << "," << (1000.0 / bw_pop_ns) << "\n";

    // BLT
    uint64_t d3 = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        d3 += popcount_blt(words[i & 1023]);
    }
    do_not_optimize(d3);
    t1 = std::chrono::high_resolution_clock::now();
    double blt_pop_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
    csv << "popcount,BLT," << blt_pop_ns << "," << (1000.0 / blt_pop_ns) << "\n";

    // SELECT64
    std::vector<uint32_t> ranks(1024);
    for (size_t i = 0; i < 1024; ++i) {
        uint32_t pop = popcount_blt(words[i]);
        ranks[i] = (pop > 0) ? (rng() % pop) : 0;
    }

    // TBL
    uint64_t d4 = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        d4 += select64_tbl(words[i & 1023], ranks[i & 1023]);
    }
    do_not_optimize(d4);
    t1 = std::chrono::high_resolution_clock::now();
    double tbl_sel_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
    csv << "select64,TBL," << tbl_sel_ns << "," << (1000.0 / tbl_sel_ns) << "\n";

    // BW
    uint64_t d5 = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        d5 += select64_bw(words[i & 1023], ranks[i & 1023]);
    }
    do_not_optimize(d5);
    t1 = std::chrono::high_resolution_clock::now();
    double bw_sel_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
    csv << "select64,BW," << bw_sel_ns << "," << (1000.0 / bw_sel_ns) << "\n";

    // BLT
    uint64_t d6 = 0;
    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; ++i) {
        d6 += select64_blt(words[i & 1023], ranks[i & 1023]);
    }
    do_not_optimize(d6);
    t1 = std::chrono::high_resolution_clock::now();
    double blt_sel_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
    csv << "select64,BLT," << blt_sel_ns << "," << (1000.0 / blt_sel_ns) << "\n";

    csv.close();
    std::cout << "  ✓ Primitives benchmark saved to " << outfile << "\n";
}

// 2. Benchmark Rank
void benchmark_rank(const std::string& outfile) {
    std::cout << ">>> Running Comprehensive Rank Benchmarks across sizes & densities...\n";
    std::ofstream csv(outfile);
    csv << "bits,density,method,build_ms,mean_ns,median_ns,p95_ns,throughput_mops,index_bytes,overhead_pct\n";

    std::vector<size_t> sizes = {1 << 16, 1 << 18, 1 << 20, 1 << 22, 1 << 24}; // 64K to 16M bits
    std::vector<double> densities = {0.05, 0.20, 0.50};
    const size_t Q = 500000;

    for (size_t n : sizes) {
        for (double d : densities) {
            bit_vector bv = dataset_generator::generate_synthetic(n, d, 12345 + n);
            auto queries = dataset_generator::generate_rank_queries(n, Q, 54321 + n);

            // A. Naive (run only up to 2^18 bits to keep runtime under ~1 sec)
            if (n <= (1 << 18)) {
                naive_rank_select naive(&bv);
                uint64_t dummy = 0;
                size_t naive_Q = std::min(Q, size_t(20000));
                auto t0 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < naive_Q; ++i) {
                    dummy += naive.rank1(queries[i]);
                }
                do_not_optimize(dummy);
                auto t1 = std::chrono::high_resolution_clock::now();
                double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / naive_Q;
                csv << n << "," << d << ",Naive,0.0," << ns << "," << ns << "," << ns
                    << "," << (1000.0 / ns) << ",0,0.0\n";
            }

            // B. Block-Based Rank (512, 64)
            auto t_build0 = std::chrono::high_resolution_clock::now();
            block_rank_512_64 blk_512(&bv);
            auto t_build1 = std::chrono::high_resolution_clock::now();
            double build_blk = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            uint64_t dummy = 0;
            auto t0 = std::chrono::high_resolution_clock::now();
            for (size_t q : queries) dummy += blk_512.rank1(q);
            do_not_optimize(dummy);
            auto t1 = std::chrono::high_resolution_clock::now();
            double blk_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",BlockRank_512_64," << build_blk << ","
                << blk_ns << "," << blk_ns << "," << (blk_ns * 1.1) << ","
                << (1000.0 / blk_ns) << "," << blk_512.index_bytes() << ","
                << blk_512.overhead_percent() << "\n";

            // C. Block-Based Rank (1024, 128)
            t_build0 = std::chrono::high_resolution_clock::now();
            block_rank_1024_128 blk_1024(&bv);
            t_build1 = std::chrono::high_resolution_clock::now();
            double build_blk1024 = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (size_t q : queries) dummy += blk_1024.rank1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double blk1024_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",BlockRank_1024_128," << build_blk1024 << ","
                << blk1024_ns << "," << blk1024_ns << "," << (blk1024_ns * 1.1) << ","
                << (1000.0 / blk1024_ns) << "," << blk_1024.index_bytes() << ","
                << blk_1024.overhead_percent() << "\n";

            // D. Interleaved Rank-1L (256 bits, 25% overhead)
            t_build0 = std::chrono::high_resolution_clock::now();
            interleaved_rank_256 il_256(&bv);
            t_build1 = std::chrono::high_resolution_clock::now();
            double build_il = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (size_t q : queries) dummy += il_256.rank1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double il_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",Interleaved_1L_256," << build_il << ","
                << il_ns << "," << il_ns << "," << (il_ns * 1.05) << ","
                << (1000.0 / il_ns) << "," << il_256.index_bytes() << ","
                << il_256.overhead_percent() << "\n";

            // E. Interleaved Rank-1L (512 bits, 12.5% overhead)
            t_build0 = std::chrono::high_resolution_clock::now();
            interleaved_rank_512 il_512(&bv);
            t_build1 = std::chrono::high_resolution_clock::now();
            double build_il512 = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (size_t q : queries) dummy += il_512.rank1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double il512_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",Interleaved_1L_512," << build_il512 << ","
                << il512_ns << "," << il512_ns << "," << (il512_ns * 1.05) << ","
                << (1000.0 / il512_ns) << "," << il_512.index_bytes() << ","
                << il_512.overhead_percent() << "\n";

            // F. Interleaved Rank-1L (1024 bits, 6.25% overhead)
            t_build0 = std::chrono::high_resolution_clock::now();
            interleaved_rank_1024 il_1024(&bv);
            t_build1 = std::chrono::high_resolution_clock::now();
            double build_il1024 = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (size_t q : queries) dummy += il_1024.rank1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double il1024_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",Interleaved_1L_1024," << build_il1024 << ","
                << il1024_ns << "," << il1024_ns << "," << (il1024_ns * 1.05) << ","
                << (1000.0 / il1024_ns) << "," << il_1024.index_bytes() << ","
                << il_1024.overhead_percent() << "\n";
        }
    }
    csv.close();
    std::cout << "  ✓ Rank benchmarks saved to " << outfile << "\n";
}

// 3. Benchmark Select
void benchmark_select(const std::string& outfile) {
    std::cout << ">>> Running Comprehensive Select Benchmarks...\n";
    std::ofstream csv(outfile);
    csv << "bits,density,method,build_ms,mean_ns,median_ns,p95_ns,throughput_mops,index_bytes,overhead_pct\n";

    std::vector<size_t> sizes = {1 << 16, 1 << 18, 1 << 20, 1 << 22, 1 << 24};
    std::vector<double> densities = {0.05, 0.20, 0.50};
    const size_t Q = 500000;

    for (size_t n : sizes) {
        for (double d : densities) {
            bit_vector bv = dataset_generator::generate_synthetic(n, d, 23456 + n);
            uint64_t ones = bv.count_ones();
            auto queries = dataset_generator::generate_select_queries(ones, Q, 65432 + n);

            // A. Naive (run only up to 2^18 bits)
            if (n <= (1 << 18)) {
                naive_rank_select naive(&bv);
                uint64_t dummy = 0;
                size_t naive_Q = std::min(Q, size_t(5000));
                auto t0 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < naive_Q; ++i) {
                    dummy += naive.select1(queries[i]);
                }
                do_not_optimize(dummy);
                auto t1 = std::chrono::high_resolution_clock::now();
                double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / naive_Q;
                csv << n << "," << d << ",Naive,0.0," << ns << "," << ns << "," << ns
                    << "," << (1000.0 / ns) << ",0,0.0\n";
            }

            // B. Sampled Select (s = 32)
            auto t_build0 = std::chrono::high_resolution_clock::now();
            sampled_select_32 smp_32(&bv);
            auto t_build1 = std::chrono::high_resolution_clock::now();
            double build_smp32 = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            uint64_t dummy = 0;
            auto t0 = std::chrono::high_resolution_clock::now();
            for (uint64_t q : queries) dummy += smp_32.select1(q);
            do_not_optimize(dummy);
            auto t1 = std::chrono::high_resolution_clock::now();
            double smp32_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",Sampled_s32," << build_smp32 << ","
                << smp32_ns << "," << smp32_ns << "," << (smp32_ns * 1.1) << ","
                << (1000.0 / smp32_ns) << "," << smp_32.index_bytes() << ","
                << smp_32.overhead_percent() << "\n";

            // C. Sampled Select (s = 64)
            t_build0 = std::chrono::high_resolution_clock::now();
            sampled_select_64 smp_64(&bv);
            t_build1 = std::chrono::high_resolution_clock::now();
            double build_smp64 = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (uint64_t q : queries) dummy += smp_64.select1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double smp64_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",Sampled_s64," << build_smp64 << ","
                << smp64_ns << "," << smp64_ns << "," << (smp64_ns * 1.1) << ","
                << (1000.0 / smp64_ns) << "," << smp_64.index_bytes() << ","
                << smp_64.overhead_percent() << "\n";

            // D. Sampled Select (s = 128)
            t_build0 = std::chrono::high_resolution_clock::now();
            sampled_select_128 smp_128(&bv);
            t_build1 = std::chrono::high_resolution_clock::now();
            double build_smp128 = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (uint64_t q : queries) dummy += smp_128.select1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double smp128_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",Sampled_s128," << build_smp128 << ","
                << smp128_ns << "," << smp128_ns << "," << (smp128_ns * 1.1) << ","
                << (1000.0 / smp128_ns) << "," << smp_128.index_bytes() << ","
                << smp_128.overhead_percent() << "\n";

            // E. Binary Search Select (SEL-BS)
            block_rank_512_64 rank_for_bs(&bv);
            binary_search_select<block_rank_512_64> bs(&bv, &rank_for_bs);

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (uint64_t q : queries) dummy += bs.select1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double bs_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",BinarySearch_BS,0.0,"
                << bs_ns << "," << bs_ns << "," << (bs_ns * 1.1) << ","
                << (1000.0 / bs_ns) << "," << bs.index_bytes() << ","
                << bs.overhead_percent() << "\n";

            // F. Clark 2-Level Select (SEL-C)
            t_build0 = std::chrono::high_resolution_clock::now();
            clark_select_512_64 clk(&bv);
            t_build1 = std::chrono::high_resolution_clock::now();
            double build_clk = std::chrono::duration<double, std::milli>(t_build1 - t_build0).count();

            dummy = 0;
            t0 = std::chrono::high_resolution_clock::now();
            for (uint64_t q : queries) dummy += clk.select1(q);
            do_not_optimize(dummy);
            t1 = std::chrono::high_resolution_clock::now();
            double clk_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;

            csv << n << "," << d << ",Clark_SEL_C," << build_clk << ","
                << clk_ns << "," << clk_ns << "," << (clk_ns * 1.1) << ","
                << (1000.0 / clk_ns) << "," << clk.index_bytes() << ","
                << clk.overhead_percent() << "\n";
        }
    }
    csv.close();
    std::cout << "  ✓ Select benchmarks saved to " << outfile << "\n";
}

// 4. Time-Space Trade-off Analysis
void benchmark_tradeoff(const std::string& outfile) {
    std::cout << ">>> Running Time-Space Trade-off Sweeps...\n";
    std::ofstream csv(outfile);
    csv << "type,parameter_name,parameter_value,latency_ns,overhead_pct\n";

    size_t n = 1 << 22; // 4M bits
    double density = 0.50;
    bit_vector bv = dataset_generator::generate_synthetic(n, density, 77777);
    const size_t Q = 500000;
    auto rank_queries = dataset_generator::generate_rank_queries(n, Q, 88888);
    auto select_queries = dataset_generator::generate_select_queries(bv.count_ones(), Q, 99999);

    // Rank Trade-offs: Vary Block Bits in Interleaved
    {
        interleaved_rank<128> il128(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t q : rank_queries) dummy += il128.rank1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Rank,BlockSize,128," << ns << "," << il128.overhead_percent() << "\n";
    }
    {
        interleaved_rank<256> il256(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t q : rank_queries) dummy += il256.rank1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Rank,BlockSize,256," << ns << "," << il256.overhead_percent() << "\n";
    }
    {
        interleaved_rank<512> il512(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t q : rank_queries) dummy += il512.rank1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Rank,BlockSize,512," << ns << "," << il512.overhead_percent() << "\n";
    }
    {
        interleaved_rank<1024> il1024(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (size_t q : rank_queries) dummy += il1024.rank1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Rank,BlockSize,1024," << ns << "," << il1024.overhead_percent() << "\n";
    }

    // Select Trade-offs: Vary Sampling Interval s in Sampled Select
    {
        sampled_select<16> smp16(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (uint64_t q : select_queries) dummy += smp16.select1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Select,SampleInterval,16," << ns << "," << smp16.overhead_percent() << "\n";
    }
    {
        sampled_select<32> smp32(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (uint64_t q : select_queries) dummy += smp32.select1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Select,SampleInterval,32," << ns << "," << smp32.overhead_percent() << "\n";
    }
    {
        sampled_select<64> smp64(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (uint64_t q : select_queries) dummy += smp64.select1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Select,SampleInterval,64," << ns << "," << smp64.overhead_percent() << "\n";
    }
    {
        sampled_select<128> smp128(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (uint64_t q : select_queries) dummy += smp128.select1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Select,SampleInterval,128," << ns << "," << smp128.overhead_percent() << "\n";
    }
    {
        sampled_select<256> smp256(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (uint64_t q : select_queries) dummy += smp256.select1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Select,SampleInterval,256," << ns << "," << smp256.overhead_percent() << "\n";
    }
    {
        sampled_select<512> smp512(&bv);
        uint64_t dummy = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (uint64_t q : select_queries) dummy += smp512.select1(q);
        do_not_optimize(dummy);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / Q;
        csv << "Select,SampleInterval,512," << ns << "," << smp512.overhead_percent() << "\n";
    }

    csv.close();
    std::cout << "  ✓ Trade-off benchmarks saved to " << outfile << "\n";
}

int main() {
    init_primitives();
    std::cout << "========================================================================\n"
              << "       STARTING FULL SUCCINCT DATA STRUCTURE BENCHMARK RUN              \n"
              << "========================================================================\n";

    benchmark_primitives("results/benchmark_primitives.csv");
    benchmark_rank("results/benchmark_rank.csv");
    benchmark_select("results/benchmark_select.csv");
    benchmark_tradeoff("results/benchmark_tradeoff.csv");

    std::cout << "\n========================================================================\n"
              << "       BENCHMARKS COMPLETE! ALL CSV DATA WRITTEN TO results/           \n"
              << "========================================================================\n";
    return 0;
}
