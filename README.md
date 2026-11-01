# Implementation and Performance Analysis of Succinct Data Structures for Fast Rank and Select Queries

**Course**: Advanced Data Structures and Algorithms (ADSA Project)  
**Authors**:  
- **Abhishek Nagar** (Roll No: `206126002`)  
- **Kishan Lambariya** (Roll No: `206126017`)  
**Primary Reference**: Simon Gog & Matthias Petri (2014). *"Optimized succinct data structures for massive data"*, Software: Practice and Experience, 44(11), 1287–1314.

---

## 📌 Executive Summary

Succinct data structures represent binary sequences close to the information-theoretic lower bound while supporting constant-time $O(1)$ queries without full decompression. The fundamental primitives underlying modern text indexing (FM-indexes, Wavelet Trees, Compressed Suffix Arrays) are:
- $\mathbf{rank}_1(i)$: Counts the number of 1-bits in prefix $B[0 \dots i-1]$.
- $\mathbf{select}_1(k)$: Returns the 0-based bit index of the $k$-th set bit (1-based).

This project implements, validates, and empirically evaluates high-performance succinct rank and select implementations in modern **C++17**, alongside comprehensive benchmark analysis and visualization in **Python/Matplotlib**.

```
                           PROPOSED SYSTEM ARCHITECTURE
                           
 ┌──────────────┐     ┌──────────────┐     ┌──────────────────────────────────┐     ┌──────────────┐     ┌──────────────┐
 │ Input Setup  │     │Data Structure│     │       Rank & Select Models       │     │  Execution   │     │   Metrics    │
 │              │────▶│              │────▶│ 1. Naive Baseline (Bit Scan)     │────▶│ Test Harness │────▶│ Latency (ns) │
 │ - Synthetic  │     │ 64-bit Words │     │ 2. Block-Based Rank (Rs + Rb)    │     │              │     │ QPS / Mops   │
 │   n, d, seed │     │ Packed Vector│     │ 3. Interleaved RANK-1L (1-Cache) │     │ - Correctness│     │ Memory (B)   │
 │ - Real Text  │     │ Cache-Aligned│     │ 4. Sampled Select (s-samples)    │     │   Suite 100% │     │ Overhead (%) │
 │   DNA/English│     │              │     │ 5. Clark Select (SEL-C 2-Level)  │     │ - Benchmarks │     │ Trade-offs   │
 └──────────────┘     └──────────────┘     │ 6. Binary Search Select (SEL-BS) │     └──────────────┘     └──────────────┘
                                           └──────────────────────────────────┘
```

---

## 🚀 Key Features & Highlights

1. **Complete C++ Implementation**:
   - **Bitvector**: 64-bit word packed representation (`uint64_t`), bitwise get/set/clear/toggle, and word-level memory tracking.
   - **Low-level Primitives**: Complete comparative implementation of 64-bit `popcount` and `select64` across **TBL** (table lookup), **BW** (broadword parallel arithmetic), and **BLT** (hardware intrinsics + Gog & Petri Listing 1).
   - **Rank Data Structures**:
     - *Naive Scan*: Baseline $O(i)$ linear scan.
     - *Block-Based Rank*: Two-level Jacobson/Vigna structure with 64-bit superblocks ($R_s$) and 16-bit blocks ($R_b$).
     - *Interleaved Rank (RANK-1L)*: Single-level cache-aligned representation interleaving cumulative rank with data words, guaranteeing single cache line fetch.
   - **Select Data Structures**:
     - *Naive Scan*: Baseline $O(n)$ search.
     - *Sampled Select*: Stores positions of every $s$-th 1 bit with word-level forward scan and hardware `select64`.
     - *Binary Search Select (SEL-BS)*: Zero-memory-overhead select via binary search over rank superblocks.
     - *Clark Select (SEL-C)*: Practical two-level multi-inventory select with relative sub-block offsets.
   - **End-to-End Application**: Full **Wavelet Tree** and **FM-Index** demonstrating exact pattern matching (`count(P)` and `locate(P)`) on real genomic DNA text.

2. **100% Agreement Correctness Suite**:
   - Exhaustive tests covering empty bitvectors, all-zeros, all-ones, single-bit vectors, internal word boundaries, out-of-bounds parameters, and randomized vectors.
   - Passes all 98 automated assertions with 100% agreement against the naive ground truth.

3. **Publication-Ready Empirical Analysis**:
   - High-precision benchmarks with pre-generated query workloads cycling over $2 \times 10^5$ to $10^7$ queries.
   - Automated Python plotting pipeline generating 6 high-resolution figures (PNG and SVG) in `results/figures/`.
   - Standalone **Interactive HTML Visualizer** in `visualization/interactive_visualizer.html`.

---

## 📁 Repository Structure

```
.
├── CMakeLists.txt                 # Modern CMake build configuration
├── Makefile                       # Fast build targets for clang++/g++
├── README.md                      # Quickstart and overview guide
├── PROJECT_REPORT.md              # Comprehensive academic M.Tech report
├── include/                       # High-performance C++ header library
│   ├── bit_vector.hpp             # 64-bit word packed bitvector
│   ├── primitives.hpp             # 64-bit popcount & select64 (TBL, BW, BLT)
│   ├── naive_rank_select.hpp      # Baseline O(n) linear scanning algorithms
│   ├── block_rank.hpp             # Two-level Jacobson / Vigna block rank
│   ├── interleaved_rank.hpp       # Gog & Petri RANK-1L interleaved structure
│   ├── sampled_select.hpp         # Sampled select with configurable interval s
│   ├── binary_search_select.hpp   # Zero-overhead select via rank binary search
│   ├── clark_select.hpp           # Two-level practical Clark select (SEL-C)
│   ├── wavelet_tree.hpp           # Wavelet Tree over arbitrary byte alphabets
│   ├── fm_index.hpp               # FM-Index pattern matching (count/locate)
│   └── dataset_generator.hpp      # Synthetic & real-world workload generator
├── src/
│   ├── primitives.cpp             # Lookup tables initialization & primitives
│   ├── bit_vector.cpp             # Bitvector implementation & I/O
│   ├── main_demo.cpp              # Interactive CLI demo with visual outputs
│   ├── correctness_tests.cpp      # Comprehensive 100% agreement test suite
│   ├── benchmark_runner.cpp       # Nanosecond benchmarking engine
│   └── text_index_demo.cpp        # Real-world FM-Index & Wavelet Tree demo
├── scripts/
│   ├── run_all.sh                 # Master script: compile, test, bench, plot
│   └── plot_benchmarks.py         # Python publication figure generator
├── results/
│   ├── benchmark_primitives.csv   # Raw popcount/select64 timings
│   ├── benchmark_rank.csv         # Rank latencies & space overheads
│   ├── benchmark_select.csv       # Select latencies & space overheads
│   ├── benchmark_tradeoff.csv     # Time-Space Pareto frontier data
│   └── figures/                   # Generated PNG & SVG charts
└── visualization/
    └── interactive_visualizer.html# Browser-based interactive UI visualizer
```

---

## ⚡ Quickstart & Compilation

### Requirements
- **C++ Compiler**: `clang++` (Apple Clang 14+) or `g++` (GCC 9+) with C++17 support.
- **Python**: Python 3.9+ with `matplotlib`, `pandas`, and `numpy` (already configured in `.venv`).

### 1. Run Everything in One Step
```bash
./scripts/run_all.sh
```

### 2. Manual Make Targets
```bash
# Build all executables into bin/
make -j4

# Run 100% agreement correctness tests
make test

# Run interactive CLI demonstration
make demo

# Run real-world text indexing (FM-Index / Wavelet Tree) demo
make fmi_demo

# Run full nanosecond benchmark suite
make benchmark

# Generate publication plots from benchmark results
make plots
```

---

## 🖥️ Interactive Web Visualizer

Open `visualization/interactive_visualizer.html` in any web browser:
```bash
open visualization/interactive_visualizer.html
```
- Click bits to toggle them in real-time.
- Trace `Rank₁(i)` step-by-step with prefix highlighting.
- Trace `Select₁(k)` with sample jumps and word scans.
- View embedded benchmark charts and Pareto trade-off curves.

---

## 📊 Summary of Experimental Findings

| Structure | Latency (ns) | Throughput | Space Overhead | Key Advantage |
| :--- | :---: | :---: | :---: | :--- |
| **Naive Scan** | ~5,800 ns | 0.17 Mops | 0.0 % | Baseline linear search; excessive latency on large vectors. |
| **BlockRank (512, 64)** | **1.08 ns** | **929 Mops** | 38.3 % | Fastest rank; two-level cache-aware index. |
| **Interleaved RANK-1L (256)**| **4.53 ns** | **220 Mops** | **25.0 %** | Single memory fetch; optimal for massive data sets. |
| **Sampled Select ($s=64$)** | **11.85 ns** | **84 Mops** | 25.4 % | Ultra-fast select via bounded forward scan. |
| **Clark Select (SEL-C)** | **16.81 ns** | **59 Mops** | **5.5 %** | Sublinear two-level inventory with minimal memory footprint. |
| **Binary Search (SEL-BS)** | 44.58 ns | 22 Mops | **0.0 %** | Reuses rank index; zero extra memory required. |

For detailed algorithmic proofs, literature survey, and full mathematical analysis, refer to [`PROJECT_REPORT.md`](file:///Users/abhishek/Downloads/M.Tech/ds/PROJECT_REPORT.md).
