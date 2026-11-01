#!/usr/bin/env bash
set -e

echo "========================================================================"
echo "   ADSA PROJECT: SUCCINCT RANK & SELECT COMPLETE PIPELINE RUNNER        "
echo "   Abhishek Nagar (206126002) & Kishan Lambariya (206126017)           "
echo "========================================================================"

echo ""
echo "[Step 1/5] Building all C++ targets with clang++ -O3..."
make -j4

echo ""
echo "[Step 2/5] Running Correctness Test Suite (100% Agreement Check)..."
./bin/correctness_tests

echo ""
echo "[Step 3/5] Running Interactive Demonstrations..."
./bin/main_demo

echo ""
echo "[Step 4/5] Running Real-World Text Indexing (FM-Index / Wavelet Tree)..."
./bin/text_index_demo

echo ""
echo "[Step 5/5] Running Benchmarking Engine & Plotting Publication Figures..."
./bin/benchmark_runner
MPLCONFIGDIR=/tmp/matplotlib .venv/bin/python3 scripts/plot_benchmarks.py

echo ""
echo "========================================================================"
echo "   PIPELINE COMPLETE! ALL TESTS PASSED & FIGURES GENERATED.             "
echo "   View results in results/figures/ or open visualization/interactive_visualizer.html"
echo "========================================================================"
