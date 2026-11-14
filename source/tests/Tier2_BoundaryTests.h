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
                TEST_ASSERT(mr16::isFinite(outL[s]) && mr16::isFinite(outR[s]), "Block size processing must produce finite samples");
            }
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 2", "T2_TOX_03", "Toxic Immunity - Bitwise IEEE-754 isFinite & safeClamp Under /fp:fast", []() {
        volatile uint32_t rawQNaN32 = 0x7FC00001u;
        volatile uint32_t rawSNaN32 = 0x7F800001u;
        volatile uint32_t rawPosInf32 = 0x7F800000u;
        volatile uint32_t rawNegInf32 = 0xFF800000u;

        float qnan32 = 0.0f, snan32 = 0.0f, posInf32 = 0.0f, negInf32 = 0.0f;
        uint32_t tmp32 = rawQNaN32;   std::memcpy(&qnan32, &tmp32, sizeof(float));
        tmp32 = rawSNaN32;            std::memcpy(&snan32, &tmp32, sizeof(float));
        tmp32 = rawPosInf32;          std::memcpy(&posInf32, &tmp32, sizeof(float));
        tmp32 = rawNegInf32;          std::memcpy(&negInf32, &tmp32, sizeof(float));

        TEST_ASSERT(!mr16::isFinite(qnan32), "Bitwise float qNaN must be detected under /fp:fast");
        TEST_ASSERT(!mr16::isFinite(snan32), "Bitwise float sNaN must be detected under /fp:fast");
        TEST_ASSERT(!mr16::isFinite(posInf32), "Bitwise float +Inf must be detected under /fp:fast");
        TEST_ASSERT(!mr16::isFinite(negInf32), "Bitwise float -Inf must be detected under /fp:fast");
        TEST_ASSERT(mr16::isFinite(0.0f) && mr16::isFinite(-1.0f) && mr16::isFinite(12345.67f),
                    "Finite float values must return true");

        volatile uint64_t rawQNaN64 = 0x7FF8000000000001ULL;
        volatile uint64_t rawPosInf64 = 0x7FF0000000000000ULL;
        double qnan64 = 0.0, posInf64 = 0.0;
        uint64_t tmp64 = rawQNaN64;   std::memcpy(&qnan64, &tmp64, sizeof(double));
        tmp64 = rawPosInf64;          std::memcpy(&posInf64, &tmp64, sizeof(double));

        TEST_ASSERT(!mr16::isFinite(qnan64), "Bitwise double qNaN must be detected under /fp:fast");
        TEST_ASSERT(!mr16::isFinite(posInf64), "Bitwise double +Inf must be detected under /fp:fast");
        TEST_ASSERT(mr16::isFinite(48000.0) && mr16::isFinite(-0.001), "Finite double values must return true");

        const float clampedNaN = mr16::safeClamp(qnan32, -1.0f, 1.0f);
        const float clampedInf = mr16::safeClamp(posInf32, -1.0f, 1.0f);
        TEST_ASSERT(mr16::isFinite(clampedNaN) && clampedNaN >= -1.0f && clampedNaN <= 1.0f,
                    "safeClamp(NaN) must neutralize to finite fallback within [lo, hi]");
        TEST_ASSERT(mr16::isFinite(clampedInf) && clampedInf >= -1.0f && clampedInf <= 1.0f,
                    "safeClamp(+Inf) must neutralize to finite fallback within [lo, hi]");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 2", "T2_RATE_03", "Block Size Edge Cases (0, 1, 4096) Zero-Alloc Safety & Live 256-Sample Scope Telemetry", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 4096);
        engine.reset();

        std::vector<float> inL(4096, 0.0f), inR(4096, 0.0f);
        std::vector<float> outL(4096, 0.0f), outR(4096, 0.0f);

        // Warm up
        engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), 64);
        engine.enqueueTriggerStrike(0.85f, 0.75f);

        gAllocationCount = 0;
        gBytesAllocated = 0;
        gTrackAllocations = true;

        // 0-sample degenerate block
        engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), 0);

        // 1-sample ultra-short blocks
        for (int i = 0; i < 64; ++i) {
            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), 1);
        }

        // 4096-sample large host buffer (> 2048 legacy cap)
        engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), 4096);

        gTrackAllocations = false;

        TEST_ASSERT(gAllocationCount == 0, "Edge-case block sizes (0, 1, 4096) must make ZERO heap allocations");
        TEST_ASSERT(gBytesAllocated == 0, "Edge-case block sizes (0, 1, 4096) must allocate ZERO bytes");
        TEST_ASSERT(test_utils::isSignalFinite(outL) && test_utils::isSignalFinite(outR),
                    "4096-sample block output must be 100% finite");

        const auto viz = engine.getVisualizerFrame();
        TEST_ASSERT(viz.scopeSamplesL.size() == 256 && viz.scopeSamplesR.size() == 256,
                    "VisualizerFrame must publish full 256-sample scope window");
        float scopePeak = 0.0f;
        bool scopeFinite = true;
        for (size_t i = 0; i < 256; ++i) {
            if (!mr16::isFinite(viz.scopeSamplesL[i]) || !mr16::isFinite(viz.scopeSamplesR[i])) {
                scopeFinite = false;
            }
            scopePeak = std::max(scopePeak, std::max(std::abs(viz.scopeSamplesL[i]), std::abs(viz.scopeSamplesR[i])));
        }
        TEST_ASSERT(scopeFinite, "VisualizerFrame scope ring samples must be strictly finite");
        TEST_ASSERT(scopePeak > 1.0e-5f, "VisualizerFrame scope ring must capture non-zero live audio waveform");
        return test::gCurrentTestAssertFailures == 0;
    });

}

} // namespace test
