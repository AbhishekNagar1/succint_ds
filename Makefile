CXX := clang++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -Iinclude
SRC_DIR := src
BIN_DIR := bin
RES_DIR := results/figures

SRCS_COMMON := $(SRC_DIR)/primitives.cpp $(SRC_DIR)/bit_vector.cpp

TARGETS := $(BIN_DIR)/main_demo $(BIN_DIR)/correctness_tests $(BIN_DIR)/benchmark_runner $(BIN_DIR)/text_index_demo

.PHONY: all clean test demo fmi_demo benchmark plots run_all

all: $(TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/main_demo: $(SRC_DIR)/main_demo.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/correctness_tests: $(SRC_DIR)/correctness_tests.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/benchmark_runner: $(SRC_DIR)/benchmark_runner.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/text_index_demo: $(SRC_DIR)/text_index_demo.cpp $(SRCS_COMMON) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(BIN_DIR)/correctness_tests
	./$(BIN_DIR)/correctness_tests

demo: $(BIN_DIR)/main_demo
	./$(BIN_DIR)/main_demo

fmi_demo: $(BIN_DIR)/text_index_demo
	./$(BIN_DIR)/text_index_demo

benchmark: $(BIN_DIR)/benchmark_runner
	./$(BIN_DIR)/benchmark_runner

plots:
	MPLCONFIGDIR=/tmp/matplotlib .venv/bin/python3 scripts/plot_benchmarks.py

run_all: test demo fmi_demo benchmark plots
	@echo "All stages executed successfully!"

clean:
	rm -rf $(BIN_DIR)
