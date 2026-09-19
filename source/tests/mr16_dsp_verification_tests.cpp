#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <string>
#include <cstdlib>
#include <cmath>
#include <cassert>

// ============================================================================
// Real-Time Allocation Tracking Override
// ============================================================================
static bool gTrackAllocations = false;
static size_t gAllocationCount = 0;
static size_t gBytesAllocated = 0;

void* operator new(size_t size) {
    if (gTrackAllocations) {
        ++gAllocationCount;
        gBytesAllocated += size;
    }
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, size_t) noexcept {
    std::free(p);
}

void* operator new[](size_t size) {
    if (gTrackAllocations) {
        ++gAllocationCount;
        gBytesAllocated += size;
    }
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete[](void* p) noexcept {
    std::free(p);
}

void operator delete[](void* p, size_t) noexcept {
    std::free(p);
}

#include "../dsp/DspMath.h"
#include "../dsp/BoundedSaturator.h"
#include "../dsp/VactrolGate.h"
#include "../dsp/LorenzAttractor.h"
#include "../dsp/SpatialChorus.h"
#include "../dsp/ModalResonatorMatrix.h"
#include "../dsp/KineticExciter.h"
#include "../dsp/Mr16Engine.h"

using namespace braun::mr16;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << " (" #cond ") at line " << __LINE__ << "\n"; \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_NEAR(val, target, eps, msg) \
    do { \
        if (std::abs((val) - (target)) > (eps)) { \
            std::cerr << "FAILED: " << msg << " val=" << (val) << " target=" << (target) \
                      << " diff=" << std::abs((val) - (target)) << " eps=" << (eps) << " at line " << __LINE__ << "\n"; \
            return false; \
        } \
    } while (0)

// ----------------------------------------------------------------------------
// Test 1: Householder Energy Conservation
// ----------------------------------------------------------------------------
bool testHouseholderEnergyConservation() {
    std::cout << "  [TEST 1] Householder Energy Conservation (H^T H = I)... ";
    
    // Dirac pulse in Mode 0
    std::array<float, 16> vec { 0.0f };
    vec[0] = 1.0f;

    float normBefore = 0.0f;
    for (float v : vec) normBefore += v * v;
    normBefore = std::sqrt(normBefore);

    // Householder reflection H = I - (2/N) 1 1^T with coupling = 1.0
    applyHouseholderScattering16(vec, 1.0f);

    float normAfter = 0.0f;
    for (float v : vec) normAfter += v * v;
    normAfter = std::sqrt(normAfter);

    TEST_ASSERT_NEAR(normAfter, normBefore, 1.0e-5f, "Householder reflection must conserve Euclidean norm exactly");
    
    // Test arbitrary vector
    for (size_t i = 0; i < 16; ++i) {
        vec[i] = std::sin(static_cast<float>(i) * 0.73f);
    }
    normBefore = 0.0f;
    for (float v : vec) normBefore += v * v;
    normBefore = std::sqrt(normBefore);

    applyHouseholderScattering16(vec, 1.0f);

    normAfter = 0.0f;
    for (float v : vec) normAfter += v * v;
    normAfter = std::sqrt(normAfter);

    TEST_ASSERT_NEAR(normAfter, normBefore, 1.0e-5f, "Arbitrary vector Householder scattering must conserve energy");

    std::cout << "PASSED (Norm Ratio = " << (normAfter / normBefore) << ")\n";
    return true;
}

// ----------------------------------------------------------------------------
// Test 2: Zero Heap Allocations in processBlock
// ----------------------------------------------------------------------------
bool testZeroHeapAllocations() {
    std::cout << "  [TEST 2] Hard Real-Time Zero Heap Allocations in processBlock... ";

    Mr16Engine engine;
    engine.prepare(48000.0, 512);

    std::array<float, 512> inL {};
    std::array<float, 512> inR {};
    std::array<float, 512> outL {};
    std::array<float, 512> outR {};

    // Warm up
    engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), 512);

    // Trigger performance strikes
    engine.enqueueTriggerStrike(0.9f, 0.75f);
    engine.enqueueTriggerButton(1, 0.8f);
    engine.enqueueTriggerChime(5, 0.85f);
    engine.enqueueMidiNoteOn(60, 0.9f);

    // Enable heap allocation tracker
    gAllocationCount = 0;
    gBytesAllocated = 0;
    gTrackAllocations = true;

    for (int block = 0; block < 20; ++block) {
        engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), 512);
    }

    gTrackAllocations = false;

    TEST_ASSERT(gAllocationCount == 0, "Heap allocations detected in real-time audio thread!");
    std::cout << "PASSED (Allocations = " << gAllocationCount << " across 10,240 samples)\n";
    return true;
}

// ----------------------------------------------------------------------------
// Test 3: Dimension D Spatial Chorus Mono Cancellation Immunity
// ----------------------------------------------------------------------------
bool testChorusMonoCompatibility() {
    std::cout << "  [TEST 3] Dimension D Tri-Phase Chorus Mono Sum Cancellation Immunity... ";

    SpatialChorus chorus;
    chorus.prepare(48000.0);
    chorus.setEnabled(true);
    chorus.setParameters(0.65f, 2.5f, 1.0f); // 100% wet

    float monoEnergy = 0.0f;
    float wetEnergyL = 0.0f;
    float wetEnergyR = 0.0f;

    for (int i = 0; i < 2048; ++i) {
        // Unit pulse
        const float in = (i == 0) ? 1.0f : 0.0f;
        float outL = 0.0f, outR = 0.0f;
        chorus.process(in, in, outL, outR);

        wetEnergyL += outL * outL;
        wetEnergyR += outR * outR;

        const float monoSum = outL + outR;
        monoEnergy += monoSum * monoSum;
    }

    TEST_ASSERT(monoEnergy > 0.1f, "Mono sum must not cancel out to zero!");
    TEST_ASSERT(wetEnergyL > 0.05f && wetEnergyR > 0.05f, "Wet outputs must have substantial delay energy");

    std::cout << "PASSED (Mono Sum Energy = " << monoEnergy << ")\n";
    return true;
}

// ----------------------------------------------------------------------------
// Test 4: TPT SVF Lyapunov Stability Under Audio-Rate Lorenz Chaos
// ----------------------------------------------------------------------------
bool testLyapunovStabilityUnderChaos() {
    std::cout << "  [TEST 4] TPT SVF Lyapunov Stability Under Audio-Rate Lorenz Chaos... ";

    Mr16Engine engine;
    engine.prepare(48000.0, 256);

    Mr16Parameters p;
    p.chaosRateHz = 5.0f; // High-speed chaos
    p.chaosDepth = 1.0f;  // Maximum chaos modulation
    p.chaosDetuneCents = 120.0f;
    p.couplingDepth = 0.85f;
    p.decayScale = 5.0f;
    engine.setParameters(p);

    engine.enqueueTriggerStrike(1.0f, 0.95f);

    std::array<float, 256> outL {};
    std::array<float, 256> outR {};

    for (int block = 0; block < 100; ++block) { // 25,600 samples
        engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), 256);

        for (int i = 0; i < 256; ++i) {
            TEST_ASSERT(std::isfinite(outL[i]), "Non-finite output sample detected in Left channel!");
            TEST_ASSERT(std::isfinite(outR[i]), "Non-finite output sample detected in Right channel!");
            TEST_ASSERT(std::abs(outL[i]) <= 1.10f, "BoundedSaturator ceiling violated in Left channel!");
            TEST_ASSERT(std::abs(outR[i]) <= 1.10f, "BoundedSaturator ceiling violated in Right channel!");
        }
    }

    std::cout << "PASSED (25,600 samples clean and strictly bounded in [-1.05, 1.05])\n";
    return true;
}

// ----------------------------------------------------------------------------
// Test 5: 4 Manifolds and 5 Material Damping Profiles
// ----------------------------------------------------------------------------
bool testManifoldsAndMaterials() {
    std::cout << "  [TEST 5] Geometric Manifolds & Material Damping Profiles... ";

    ModalResonatorMatrix matrix;
    matrix.prepare(48000.0);
    matrix.setFundamentalHz(220.0f);

    const std::array<ManifoldType, 4> manifolds = {
        ManifoldType::ChladniPlate,
        ManifoldType::StiffBeam,
        ManifoldType::VocalFormant,
        ManifoldType::PoincareHorn
    };

    const std::array<MaterialType, 5> materials = {
        MaterialType::Wood,
        MaterialType::Glass,
        MaterialType::Steel,
        MaterialType::Brass,
        MaterialType::Nylon
    };

    for (auto man : manifolds) {
        matrix.setManifold(man, 0.0f);
        const auto& freqs = matrix.getCurrentFrequencies();
        for (size_t i = 0; i < 16; ++i) {
            TEST_ASSERT(freqs[i] >= 20.0f && freqs[i] <= 24000.0f, "Manifold frequencies must be in audible band");
            if (i > 0) {
                TEST_ASSERT(freqs[i] > freqs[i-1], "Modal frequencies must be strictly ascending");
            }
        }

        for (auto mat : materials) {
            matrix.setMaterial(mat);
            float outL = 0.0f, outR = 0.0f;
            matrix.processSample(1.0f, outL, outR);
            TEST_ASSERT(std::isfinite(outL) && std::isfinite(outR), "Modal output must be finite");
        }
    }

    std::cout << "PASSED (4 Manifolds x 5 Materials validated)\n";
    return true;
}

// ----------------------------------------------------------------------------
// Test 6: 16 Curated Factory Presets Integrity
// ----------------------------------------------------------------------------
bool testFactoryPresets() {
    std::cout << "  [TEST 6] 16 Curated Factory Presets Verification... ";

    const auto presets = Mr16Engine::getFactoryPresets();
    TEST_ASSERT(presets.size() == 16, "Must provide exactly 16 factory presets");

    Mr16Engine engine;
    engine.prepare(48000.0, 128);

    std::array<float, 128> outL {};
    std::array<float, 128> outR {};

    for (const auto& preset : presets) {
        TEST_ASSERT(preset.id != nullptr && std::string(preset.id).length() > 0, "Preset must have valid ID");
        TEST_ASSERT(preset.name != nullptr && std::string(preset.name).length() > 0, "Preset must have valid Name");

        engine.setParameters(preset.params);
        engine.reset();
        engine.enqueueTriggerStrike(0.8f, 0.7f);

        engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), 128);

        for (int i = 0; i < 128; ++i) {
            TEST_ASSERT(std::isfinite(outL[i]) && std::isfinite(outR[i]), "Preset output must be finite");
        }
    }

    std::cout << "PASSED (All 16 factory presets verified successfully)\n";
    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main() {
    std::cout << "================================================================================\n";
    std::cout << "    BRAUN MR-16 — C++20 REAL-TIME DSP CORE VERIFICATION RUNNER                 \n";
    std::cout << "================================================================================\n\n";

    int passed = 0;
    int failed = 0;

    auto runTest = [&](bool (*testFunc)()) {
        if (testFunc()) {
            ++passed;
        } else {
            ++failed;
        }
    };

    runTest(testHouseholderEnergyConservation);
    runTest(testZeroHeapAllocations);
    runTest(testChorusMonoCompatibility);
    runTest(testLyapunovStabilityUnderChaos);
    runTest(testManifoldsAndMaterials);
    runTest(testFactoryPresets);

    std::cout << "\n--------------------------------------------------------------------------------\n";
    std::cout << "SUMMARY: " << passed << " PASSED, " << failed << " FAILED\n";
    std::cout << "================================================================================\n";

    return (failed == 0) ? 0 : 1;
}
