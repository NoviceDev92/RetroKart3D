#include "test_framework.h"
#include "test_tier1_features.h"
#include "test_tier2_boundaries.h"
#include "test_tier3_interactions.h"
#include "test_tier4_realworld.h"
#include <QCoreApplication>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    bool runT1 = true;
    bool runT2 = true;
    bool runT3 = true;
    bool runT4 = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--tier=1") {
            runT1 = true; runT2 = false; runT3 = false; runT4 = false;
        } else if (arg == "--tier=2") {
            runT1 = false; runT2 = true; runT3 = false; runT4 = false;
        } else if (arg == "--tier=3") {
            runT1 = false; runT2 = false; runT3 = true; runT4 = false;
        } else if (arg == "--tier=4") {
            runT1 = false; runT2 = false; runT3 = false; runT4 = true;
        } else if (arg == "--all") {
            runT1 = true; runT2 = true; runT3 = true; runT4 = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "RetroKart3D Automated E2E Test Runner\n";
            std::cout << "Usage: tests.exe [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --all       Run all test tiers (default)\n";
            std::cout << "  --tier=1    Run Tier 1: Feature Coverage (F1 to F16)\n";
            std::cout << "  --tier=2    Run Tier 2: Boundary & Corner Cases\n";
            std::cout << "  --tier=3    Run Tier 3: Cross-Feature Interactions\n";
            std::cout << "  --tier=4    Run Tier 4: Real-World Race Simulations\n";
            std::cout << "  --help      Show this help message\n";
            return 0;
        }
    }

    std::cout << "=======================================================\n";
    std::cout << "    RETROKART 3D COMPREHENSIVE E2E AUTOMATED RUNNER    \n";
    std::cout << "  Desktop Qt 6.11.1 MinGW 64-bit | Requirements R1-R5  \n";
    std::cout << "=======================================================\n";

    if (runT1) runTier1FeatureTests();
    if (runT2) runTier2BoundaryTests();
    if (runT3) runTier3InteractionTests();
    if (runT4) runTier4RealWorldTests();

    TestContext::instance().printSummary();

    return (TestContext::instance().failedTests() == 0) ? 0 : 1;
}
