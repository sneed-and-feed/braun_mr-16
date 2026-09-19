#pragma once

#include "TestHarness.h"

namespace test {

inline void registerTier2Tests() {

    // ========================================================================
    // T2_RT: Hard Real-Time Heap Safety Verification
    // ========================================================================

    registerTest("Tier 2", "T2_RT_01", "Real-Time Safety - Zero Heap Allocations During processBlock()", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 512);
        engine.reset();

        const int blockSize = 256;
        std::vector<float> inL(blockSize, 0.2f), inR(blockSize, 0.2f);
        std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

        // Warm up cache and state
        engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);

        // Arm heap allocation trap
        gAllocationCount = 0;
        gBytesAllocated = 0;
        gTrackAllocations = true;

        for (int block = 0; block < 100; ++block) {
            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);
        }

        gTrackAllocations = false;

        TEST_ASSERT(gAllocationCount == 0, "Audio rendering MUST make bit-exact ZERO heap allocations!");
        TEST_ASSERT(gBytesAllocated == 0, "Audio rendering MUST allocate ZERO bytes of memory!");
        return test::gCurrentTestAssertFailures == 0;
    });

    // ========================================================================
    // T2_BND: Extreme Parameter Sweeps & Numerical Bounds
    // ========================================================================

    registerTest("Tier 2", "T2_BND_01", "Boundary Stress - Fundamental Frequency Boundary Sweeps (20 Hz - 2000 Hz)", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 512);

        auto params = engine.getParameters();
        const std::vector<float> freqProbes = { 20.0f, 25.0f, 40.0f, 100.0f, 440.0f, 1000.0f, 1999.0f, 2000.0f };

        const int blockSize = 128;
        std::vector<float> inL(blockSize, 0.0f), inR(blockSize, 0.0f);
        std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

        for (float f0 : freqProbes) {
            params.fundamentalHz = f0;
            engine.setParameters(params);
            engine.enqueueTriggerStrike(0.8f, 0.7f);

            for (int b = 0; b < 10; ++b) {
                engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);
                for (int s = 0; s < blockSize; ++s) {
                    TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "Output must be finite at extreme fundamental");
                    TEST_ASSERT(std::abs(outL[s]) <= 1.05f && std::abs(outR[s]) <= 1.05f, "Output must be bounded <= 1.05");
                }
            }
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 2", "T2_BND_02", "Boundary Stress - 100% Maximum Feedback Coupling Singularity & Infinite Hold", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 512);

        auto params = engine.getParameters();
        params.couplingDepth = 1.00f; // 100% full Householder scattering coupling
        params.decayScale = 10.0f;    // Maximum sustain
        params.material = mr16::MaterialType::Steel;
        engine.setParameters(params);

        // Inject initial high-energy impulse
        engine.enqueueTriggerStrike(1.0f, 0.9f);

        const int blockSize = 256;
        std::vector<float> inL(blockSize, 0.0f), inR(blockSize, 0.0f);
        std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

        // Run for 150 blocks (~38,400 samples)
        for (int b = 0; b < 150; ++b) {
            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);
            for (int s = 0; s < blockSize; ++s) {
                TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "Infinite hold output must remain finite");
                TEST_ASSERT(std::abs(outL[s]) <= 1.05f && std::abs(outR[s]) <= 1.05f,
                            "Maximum feedback must be strictly contained by the Hermite saturator ceiling <= 1.05");
            }
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 2", "T2_BND_03", "Boundary Stress - High-Energy Dirac Impulse Overload (+40 dBFS Burst)", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 512);
        engine.reset();

        const int blockSize = 256;
        std::vector<float> inL(blockSize, 0.0f), inR(blockSize, 0.0f);
        std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

        // Inject +40 dBFS Dirac impulse: amp = 100.0f
        inL[0] = 100.0f;
        inR[0] = 100.0f;

        engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);

        for (int s = 0; s < blockSize; ++s) {
            TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "+40 dBFS burst output must be finite");
            TEST_ASSERT(std::abs(outL[s]) <= 1.0501f && std::abs(outR[s]) <= 1.0501f,
                        "+40 dBFS burst output must be clamped within saturator ceiling 1.05");
        }

        // Subsequent blocks must decay cleanly back into linear zone without DC latching
        inL[0] = 0.0f; inR[0] = 0.0f;
        for (int b = 0; b < 20; ++b) {
            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);
        }
        double lateRms = test_utils::computeRMS(outL, 0, blockSize);
        TEST_ASSERT(lateRms < 0.2, "Post-overload recovery must decay cleanly");
        return test::gCurrentTestAssertFailures == 0;
    });

    // ========================================================================
    // T2_TOX: Denormal, NaN, and Infinity Toxic Neutralization
    // ========================================================================

    registerTest("Tier 2", "T2_TOX_01", "Toxic Immunity - Software flushDenormal() Neutralizes Non-Finite Values", []() {
        TEST_ASSERT(mr16::flushDenormal(std::numeric_limits<float>::quiet_NaN()) == 0.0f, "NaN must flush to 0.0f");
        TEST_ASSERT(mr16::flushDenormal(std::numeric_limits<float>::infinity()) == 0.0f, "+Inf must flush to 0.0f");
        TEST_ASSERT(mr16::flushDenormal(-std::numeric_limits<float>::infinity()) == 0.0f, "-Inf must flush to 0.0f");
        TEST_ASSERT(mr16::flushDenormal(1.0e-16f) == 0.0f, "1.0e-16 subnormal must flush to 0.0f");
        TEST_ASSERT(mr16::flushDenormal(-1.0e-16f) == 0.0f, "-1.0e-16 subnormal must flush to 0.0f");
        TEST_ASSERT(mr16::flushDenormal(1.0e-38f) == 0.0f, "1.0e-38 IEEE subnormal must flush to 0.0f");
        TEST_ASSERT(mr16::flushDenormal(-1.0e-45f) == 0.0f, "-1.0e-45 IEEE denormal must flush to 0.0f");

        // Normal values must pass unaltered
        TEST_ASSERT(mr16::flushDenormal(1.0f) == 1.0f, "1.0 must pass unchanged");
        TEST_ASSERT(mr16::flushDenormal(-0.5f) == -0.5f, "-0.5 must pass unchanged");
        TEST_ASSERT(mr16::flushDenormal(1.0e-6f) == 1.0e-6f, "1e-6 must pass unchanged");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 2", "T2_TOX_02", "Toxic Immunity - Ingestion of Toxic NaN and Infinite Buffers into Mr16Engine", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 256);

        const int blockSize = 128;
        std::vector<float> toxicInL(blockSize), toxicInR(blockSize);
        std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

        // Fill inputs with poison values
        for (int i = 0; i < blockSize; ++i) {
            toxicInL[i] = (i % 3 == 0) ? std::numeric_limits<float>::quiet_NaN()
                        : (i % 3 == 1) ? std::numeric_limits<float>::infinity()
                        : 1.0e-38f;
            toxicInR[i] = (i % 2 == 0) ? -std::numeric_limits<float>::infinity() : -1.0e-40f;
        }

        engine.processBlock(toxicInL.data(), toxicInR.data(), outL.data(), outR.data(), blockSize);

        for (int s = 0; s < blockSize; ++s) {
            TEST_ASSERT(std::isfinite(outL[s]), "Output Left must neutralize toxic inputs to finite values");
            TEST_ASSERT(std::isfinite(outR[s]), "Output Right must neutralize toxic inputs to finite values");
            TEST_ASSERT(std::abs(outL[s]) <= 1.05f && std::abs(outR[s]) <= 1.05f, "Output must remain bounded");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    // ========================================================================
    // T2_RATE: Multi-Rate Sample Rate & Block Size Invariance
    // ========================================================================

    registerTest("Tier 2", "T2_RATE_01", "Multi-Rate Scaling - Preparation and Execution across 44.1k to 192k", []() {
        const std::vector<double> sampleRates = { 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 };

        for (double fs : sampleRates) {
            mr16::Mr16Engine engine;
            engine.prepare(fs, 512);
            engine.reset();
            engine.enqueueTriggerStrike(0.75f, 0.60f);

            const int blockSize = 256;
            std::vector<float> inL(blockSize, 0.0f), inR(blockSize, 0.0f);
            std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);

            for (int s = 0; s < blockSize; ++s) {
                TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "Output must be finite at fs");
                TEST_ASSERT(std::abs(outL[s]) <= 1.05f && std::abs(outR[s]) <= 1.05f, "Output must be bounded at fs");
            }
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 2", "T2_RATE_02", "Block Size Scaling - Arbitrary and Prime Block Sizes (1 to 2048)", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 2048);
        engine.reset();

        const std::vector<int> testBlockSizes = { 1, 2, 7, 13, 31, 64, 128, 256, 512, 1024, 2048 };

        for (int bs : testBlockSizes) {
            std::vector<float> inL(bs, 0.0f), inR(bs, 0.0f);
            std::vector<float> outL(bs, 0.0f), outR(bs, 0.0f);

            engine.enqueueTriggerStrike(0.5f, 0.5f);
            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), bs);

            for (int s = 0; s < bs; ++s) {
                TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "Block size processing must produce finite samples");
            }
        }
        return test::gCurrentTestAssertFailures == 0;
    });

}

} // namespace test
