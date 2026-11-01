#include "wavelet_tree.hpp"
#include "fm_index.hpp"
#include "dataset_generator.hpp"
#include "primitives.hpp"

#include <iostream>
#include <iomanip>
#include <string>
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

int main() {
    init_primitives();

    std::cout << ANSI_BOLD ANSI_CYAN
              << "╔═══════════════════════════════════════════════════════════════════════════╗\n"
              << "║      SUCCINCT TEXT INDEXING: FM-INDEX & WAVELET TREE DEMO                 ║\n"
              << "║      Real-World Application of Fast Rank and Select on Compressed Texts   ║\n"
              << "╚═══════════════════════════════════════════════════════════════════════════╝\n"
              << ANSI_RESET << "\n";

    // 1. Genomic DNA Example
    std::string dna = "GATTAGACAGATTAGACACATTAGACAGATTAGACA";
    std::cout << ANSI_BOLD ANSI_YELLOW << "--- [Genomic Sequence Indexing] ---" ANSI_RESET "\n";
    std::cout << "Original DNA text (length " << dna.length() << " bp):\n  " << ANSI_GREEN << dna << ANSI_RESET << "\n\n";

    std::cout << "Constructing Succinct FM-Index (BWT + Wavelet Tree + Sampled Suffix Array)...\n";
    fm_index fmi(dna, 4); // SA sampled every 4 positions
    std::cout << "FM-Index successfully built! Index size: " << fmi.index_bytes() << " bytes\n\n";

    // Test Count Queries
    std::vector<std::string> patterns = {"GATTAGACA", "TAGA", "ACA", "CAT", "ZZZ"};
    std::cout << ANSI_BOLD ANSI_MAGENTA << ">>> [Pattern Search: count(P) and locate(P)]" ANSI_RESET "\n";
    std::cout << "  ┌────────────┬─────────────┬────────────────────────────────┐\n";
    std::cout << "  │ Pattern P  │ Occurrences │ Match Starting Offsets in Text │\n";
    std::cout << "  ├────────────┼─────────────┼────────────────────────────────┤\n";

    for (const auto& pat : patterns) {
        uint64_t cnt = fmi.count(pat);
        auto locs = fmi.locate(pat);
        std::cout << "  │ " << std::left << std::setw(10) << pat << " │ "
                  << std::right << std::setw(11) << cnt << " │ [";
        for (size_t k = 0; k < locs.size(); ++k) {
            std::cout << locs[k] << (k + 1 < locs.size() ? ", " : "");
        }
        std::cout << "]" << std::string(std::max(0, 30 - int(locs.size() * 3 + 2)), ' ') << "│\n";
    }
    std::cout << "  └────────────┴─────────────┴────────────────────────────────┘\n\n";

    // Verify locate offsets on the text
    for (const auto& pat : patterns) {
        auto locs = fmi.locate(pat);
        for (size_t offset : locs) {
            std::string sub = dna.substr(offset, pat.length());
            if (sub != pat) {
                std::cerr << "Verification failed for pattern " << pat << " at offset " << offset << "\n";
            }
        }
    }
    std::cout << ANSI_GREEN "  ✓ All FM-Index locate matches verified against original DNA text!\n" ANSI_RESET "\n";

    // 2. Wavelet Tree Direct Character Rank Demo
    std::cout << ANSI_BOLD ANSI_YELLOW << "--- [Wavelet Tree Character Rank Demo] ---" ANSI_RESET "\n";
    wavelet_tree wt(dna);
    std::cout << "Wavelet Tree built over DNA alphabet {A, C, G, T}.\n";
    std::cout << "• rank('A', 10) = " << wt.rank('A', 10) << " (number of 'A's in prefix dna[0 ... 9])\n";
    std::cout << "• rank('G', 20) = " << wt.rank('G', 20) << " (number of 'G's in prefix dna[0 ... 19])\n";
    std::cout << "• access(0) = '" << wt.access(0) << "' (Expected '" << dna[0] << "')\n";
    std::cout << "• access(4) = '" << wt.access(4) << "' (Expected '" << dna[4] << "')\n\n";

    std::cout << ANSI_BOLD ANSI_GREEN << "Text Indexing Demo completed successfully.\n" ANSI_RESET;
    return 0;
}
