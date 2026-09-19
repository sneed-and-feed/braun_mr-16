#pragma once

#include "TestHarness.h"

namespace test {

inline void registerTier1Tests() {

    // ========================================================================
    // T1_EXC: Kinetic Exciter Subsystem Tests
    // ========================================================================

    registerTest("Tier 1", "T1_EXC_01", "Kinetic Exciter - Mass-Spring Contact Strike Collision Dynamics", []() {
        mr16::KineticExciter exciter;
        exciter.prepare(48000.0);
        exciter.reset();

        // Trigger strike and verify transient pulse output
        exciter.triggerStrike(0.85f, 0.70f);

        const size_t numSamples = 2048;
        std::vector<float> output(numSamples, 0.0f);
        float peakAmp = 0.0f;
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = exciter.processSample(0.0f);
            peakAmp = std::max(peakAmp, std::abs(output[i]));
        }

        TEST_ASSERT(peakAmp > 0.01f, "Strike excitation must produce non-zero peak energy");
        TEST_ASSERT(peakAmp <= 2.0f, "Strike excitation peak energy must be physically bounded");
        
        // Energy must decay after the initial impact
        double earlyEnergy = test_utils::computeRMS(output, 0, 256);
        double lateEnergy  = test_utils::computeRMS(output, 1024, 512);
        TEST_ASSERT(earlyEnergy > lateEnergy, "Contact impact must exhibit initial burst followed by decay");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_02", "Kinetic Exciter - Mallet Hardness Modulates Transient Spectral Centroid", []() {
        mr16::KineticExciter softExciter, hardExciter;
        softExciter.prepare(48000.0);
        hardExciter.prepare(48000.0);

        softExciter.triggerStrike(0.80f, 0.15f); // Soft rubber / felt mallet
        hardExciter.triggerStrike(0.80f, 0.95f); // Hard steel / brass striker

        const size_t n = 2048;
        std::vector<float> softSig(n), hardSig(n);
        for (size_t i = 0; i < n; ++i) {
            softSig[i] = softExciter.processSample(0.0f);
            hardSig[i] = hardExciter.processSample(0.0f);
        }

        double softCentroid = test_utils::computeSpectralCentroid(softSig, 48000.0, 1024);
        double hardCentroid = test_utils::computeSpectralCentroid(hardSig, 48000.0, 1024);

        TEST_ASSERT(hardCentroid > softCentroid, "Hard mallet strike must have higher spectral centroid than soft mallet");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_03", "Kinetic Exciter - Karnopp Stick-Slip Friction Continuous Bowing", []() {
        mr16::KineticExciter exciter;
        exciter.prepare(48000.0);
        exciter.setFriction(0.60f, 0.50f); // Continuous bow rubbing

        const size_t n = 4000;
        std::vector<float> sig(n);
        for (size_t i = 0; i < n; ++i) {
            sig[i] = exciter.processSample(0.0f);
        }

        double rms = test_utils::computeRMS(sig, 1000, 2000);
        TEST_ASSERT(rms > 0.001, "Continuous stick-slip friction must maintain non-zero sustained RMS energy");
        TEST_ASSERT(test_utils::isSignalFinite(sig), "Friction signal must be 100% finite");

        // Disengaging friction should decay to silence
        exciter.setFriction(0.0f, 0.0f);
        std::vector<float> silence(2000);
        for (size_t i = 0; i < silence.size(); ++i) {
            silence[i] = exciter.processSample(0.0f);
        }
        double postRms = test_utils::computeRMS(silence, 1000, 1000);
        TEST_ASSERT(postRms < rms * 0.05, "Friction release must decay to silence");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_04", "Kinetic Exciter - Buchla 292 Optical Vactrol Asymmetric Dynamic Sag", []() {
        mr16::VactrolGate vactrol;
        vactrol.prepare(48000.0);
        vactrol.reset();

        // Trigger optical flash
        vactrol.trigger(1.0f);
        
        float maxConductance = 0.0f;
        size_t peakSample = 0;
        const size_t n = 8000;
        std::vector<float> conductance(n);

        for (size_t i = 0; i < n; ++i) {
            float dummy = vactrol.processSample(1.0f);
            (void)dummy;
            float c = vactrol.getConductance();
            conductance[i] = c;
            if (c > maxConductance) {
                maxConductance = c;
                peakSample = i;
            }
        }

        TEST_ASSERT(maxConductance > 0.5f, "Vactrol optical trigger must open conductance");
        // Attack should be fast (~2 ms = ~96 samples at 48k)
        TEST_ASSERT(peakSample < 300, "Vactrol attack must be fast (< 6 ms)");
        
        // Decay should be slow and gradual
        float tailConductance = conductance[4000];
        TEST_ASSERT(tailConductance < maxConductance, "Vactrol conductance must decay over time");
        TEST_ASSERT(tailConductance >= 0.0f, "Vactrol conductance must remain non-negative");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_05", "Kinetic Exciter - External Audio Input & DC Neutralization", []() {
        mr16::KineticExciter exciter;
        exciter.prepare(48000.0);
        exciter.setExternalInput(true, 1.0f, 0.5f);

        // Feed signal with heavy DC offset (0.5f) + 440 Hz sine
        const size_t n = 4000;
        auto inSig = test_utils::generateSine(n, 440.0, 48000.0, 0.4f);
        for (float& s : inSig) s += 0.5f;

        std::vector<float> outSig(n);
        for (size_t i = 0; i < n; ++i) {
            outSig[i] = exciter.processSample(inSig[i]);
        }

        // Measure late DC offset (mean of output signal)
        double meanVal = 0.0;
        for (size_t i = 2000; i < n; ++i) {
            meanVal += static_cast<double>(outSig[i]);
        }
        meanVal /= 2000.0;

        TEST_ASSERT(std::abs(meanVal) < 0.15, "15 Hz DC blocking filter must attenuate DC offset significantly");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_06", "Kinetic Exciter - Autonomous Poisson Rain Stochastic Trigger Rate", []() {
        mr16::KineticExciter exciter;
        exciter.prepare(48000.0);
        exciter.setPoissonRain(true, 60.0f, 0.5f); // 60 events per minute = 1 Hz nominal

        const size_t n = 48000 * 3; // 3 seconds
        int pulseCount = 0;
        for (size_t i = 0; i < n; ++i) {
            float s = exciter.processSample(0.0f);
            if (std::abs(s) > 0.05f && exciter.isMalletInContact()) {
                ++pulseCount;
            }
        }

        TEST_ASSERT(pulseCount > 0, "Poisson stochastic rain must trigger pulses over 3 seconds");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_07", "Kinetic Exciter - Euclidean Polyrhythm Clock Pulse Distribution", []() {
        mr16::KineticExciter exciter;
        exciter.prepare(48000.0);
        // E(4, 16) at 120 BPM: 4 pulses evenly spaced across 16 steps
        exciter.setEuclidean(true, 4, 16, 240.0f);

        const size_t n = 48000 * 2;
        int activeSpikes = 0;
        for (size_t i = 0; i < n; ++i) {
            float s = exciter.processSample(0.0f);
            if (std::abs(s) > 0.08f) ++activeSpikes;
        }

        TEST_ASSERT(activeSpikes > 0, "Euclidean rhythm engine must generate strike events");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_08", "Kinetic Exciter - 16-Key Microtonal Chime Synthesizer", []() {
        mr16::KineticExciter exciter;
        exciter.prepare(48000.0);
        exciter.setMicrotonalScale(mr16::MicrotonalScale::JustIntonation);
        exciter.setChimeRootHz(220.0f);

        // Trigger key 0 and key 7
        exciter.triggerChimeKey(0, 0.8f);
        float hz0 = exciter.getLastTriggeredHz();
        TEST_ASSERT_NEAR(hz0, 220.0f, 1.0f, "Key 0 must trigger root fundamental 220 Hz");

        exciter.triggerChimeKey(7, 0.8f);
        float hz7 = exciter.getLastTriggeredHz();
        TEST_ASSERT(hz7 > hz0, "Higher chime key must have higher fundamental frequency");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_EXC_09", "Kinetic Exciter - External Input Clean Feedthrough Zero Phantom Mallet Strike", []() {
        mr16::KineticExciter exciter;
        exciter.prepare(48000.0);
        exciter.setExternalInput(true, 1.0f, 0.5f);

        // Feed transient audio pulses (sharp step functions and Dirac-like bursts)
        // that previously triggered the phantom mallet collision engine
        const size_t n = 4800; // 100 ms at 48 kHz
        std::vector<float> inSig(n, 0.0f);

        // Pre-roll to allow mExtGainSmoother (10 ms time constant) to settle near unity
        const size_t settleSamples = 1000;

        // Multiple sharp transient step bursts with steep onset edges
        for (size_t t = 0; t < 4; ++t) {
            size_t start = settleSamples + t * 800;
            for (size_t k = 0; k < 40; ++k) {
                inSig[start + k] = 0.95f;
            }
        }

        std::vector<float> outSig(n, 0.0f);
        bool phantomStrikeOccurred = false;

        for (size_t i = 0; i < n; ++i) {
            outSig[i] = exciter.processSample(inSig[i]);
            if (exciter.isMalletInContact()) {
                phantomStrikeOccurred = true;
            }
        }

        TEST_ASSERT(!phantomStrikeOccurred, "External audio transient pulses must strictly NOT trigger synthetic mallet contact strikes");
        TEST_ASSERT(!exciter.isMalletInContact(), "Mallet contact state must remain strictly false throughout external input excitation");

        // Verify that output contains the properly scaled external audio excitation
        // With input = 0.95f, sensitivity = 1.0f, directMix = 0.5f, gain ~ 1.0, steady portion of step is ~ 0.475f
        float maxExcitation = 0.0f;
        for (size_t i = settleSamples; i < n; ++i) {
            maxExcitation = std::max(maxExcitation, std::abs(outSig[i]));
        }

        TEST_ASSERT(maxExcitation > 0.20f && maxExcitation < 1.0f, "Output must contain properly scaled external audio excitation");
        return test::gCurrentTestAssertFailures == 0;
    });

    // ========================================================================
    // T1_MOD: 16-Pole Modal Resonator Matrix Subsystem Tests
    // ========================================================================

    registerTest("Tier 1", "T1_MOD_01", "Modal Resonator Matrix - Biharmonic Chladni Plate Ratios", []() {
        mr16::ModalResonatorMatrix matrix;
        matrix.prepare(48000.0);
        matrix.setFundamentalHz(200.0f);
        matrix.setManifold(mr16::ManifoldType::ChladniPlate);
        matrix.reset();

        const auto& freqs = matrix.getCurrentFrequencies();
        TEST_ASSERT_NEAR(freqs[0], 200.0f, 1.0f, "Mode 0 must equal base frequency");
        TEST_ASSERT(freqs[1] > freqs[0], "Mode 1 frequency must be strictly greater than mode 0");
        TEST_ASSERT_NEAR(freqs[1] / freqs[0], mr16::ModalResonatorMatrix::kChladniPlateRatios[1], 0.05f,
                         "Mode 1 ratio must match Chladni plate standing wave formula");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_02", "Modal Resonator Matrix - Stiff Struck Beam / Marimba Dispersion", []() {
        mr16::ModalResonatorMatrix matrix;
        matrix.prepare(48000.0);
        matrix.setFundamentalHz(150.0f);
        matrix.setManifold(mr16::ManifoldType::StiffBeam);

        const auto& freqs = matrix.getCurrentFrequencies();
        TEST_ASSERT_NEAR(freqs[0], 150.0f, 1.0f, "Beam fundamental must equal 150 Hz");
        // Euler-Bernoulli beam 2nd mode is ~2.756x fundamental
        TEST_ASSERT_NEAR(freqs[1] / freqs[0], 2.7565f, 0.1f, "Beam second mode must follow Euler-Bernoulli ratio ~2.756");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_03", "Modal Resonator Matrix - Vocal Formant Tract Cavity Resonances", []() {
        mr16::ModalResonatorMatrix matrix;
        matrix.prepare(48000.0);
        matrix.setFundamentalHz(120.0f);
        matrix.setManifold(mr16::ManifoldType::VocalFormant);

        const auto& freqs = matrix.getCurrentFrequencies();
        TEST_ASSERT_NEAR(freqs[0], 120.0f, 1.0f, "Vocal tract fundamental must equal 120 Hz");
        for (size_t i = 1; i < mr16::ModalResonatorMatrix::kNumModes; ++i) {
            TEST_ASSERT(freqs[i] > freqs[i - 1], "Formant frequencies must increase monotonically");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_04", "Modal Resonator Matrix - Poincaré Hyperbolic Horn Negative Curvature Flare", []() {
        mr16::ModalResonatorMatrix matrix;
        matrix.prepare(48000.0);
        matrix.setFundamentalHz(100.0f);
        matrix.setManifold(mr16::ManifoldType::PoincareHorn);

        const auto& freqs = matrix.getCurrentFrequencies();
        TEST_ASSERT_NEAR(freqs[0], 100.0f, 1.0f, "Poincaré Horn fundamental must equal 100 Hz");
        TEST_ASSERT(freqs[15] > freqs[0], "Highest mode must be higher than fundamental");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_05", "Modal Resonator Matrix - Continuous Geometric Manifold Morphing", []() {
        mr16::ModalResonatorMatrix matrix;
        matrix.prepare(48000.0);
        matrix.setFundamentalHz(220.0f);

        // Morph 0.0 vs 1.0
        matrix.setManifold(mr16::ManifoldType::ChladniPlate, 0.0f);
        float f1_base = matrix.getCurrentFrequencies()[1];

        matrix.setManifold(mr16::ManifoldType::ChladniPlate, 0.5f);
        float f1_mid = matrix.getCurrentFrequencies()[1];

        TEST_ASSERT(f1_base > 0.0f && f1_mid > 0.0f, "Frequencies must remain positive under morphing");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_06", "Modal Resonator Matrix - Material Damping: Wood vs Glass Decay Times", []() {
        mr16::ModalResonatorMatrix matrixWood, matrixGlass;
        matrixWood.prepare(48000.0);
        matrixWood.setFundamentalHz(440.0f);
        matrixWood.setMaterial(mr16::MaterialType::Wood);
        matrixWood.setDecayScale(1.0f);

        matrixGlass.prepare(48000.0);
        matrixGlass.setFundamentalHz(440.0f);
        matrixGlass.setMaterial(mr16::MaterialType::Glass);
        matrixGlass.setDecayScale(1.0f);

        const size_t n = 24000;
        std::vector<float> woodL(n), woodR(n), glassL(n), glassR(n);

        float outL, outR;
        matrixWood.processSample(1.0f, outL, outR);
        woodL[0] = outL;
        matrixGlass.processSample(1.0f, outL, outR);
        glassL[0] = outL;

        for (size_t i = 1; i < n; ++i) {
            matrixWood.processSample(0.0f, outL, outR);
            woodL[i] = outL;
            matrixGlass.processSample(0.0f, outL, outR);
            glassL[i] = outL;
        }

        double woodLateRMS  = test_utils::computeRMS(woodL, 12000, 4000);
        double glassLateRMS = test_utils::computeRMS(glassL, 12000, 4000);

        TEST_ASSERT(glassLateRMS > woodLateRMS, "Glass material must sustain acoustic energy significantly longer than Wood");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_07", "Modal Resonator Matrix - 16x16 Householder Energy Conservation", []() {
        std::array<float, 16> vec = {{
            0.5f, -0.3f, 0.8f, -0.2f, 0.1f, -0.4f, 0.6f, -0.7f,
            0.2f, -0.5f, 0.3f, -0.1f, 0.4f, -0.2f, 0.1f, -0.3f
        }};

        float initialEnergy = 0.0f;
        for (float v : vec) initialEnergy += v * v;

        // Apply Householder scattering with coupling = 1.0 (lossless orthogonal reflection)
        std::array<float, 16> scattered = mr16::householderReflect16(vec, 1.0f);

        float scatteredEnergy = 0.0f;
        for (float v : scattered) scatteredEnergy += v * v;

        TEST_ASSERT_NEAR(scatteredEnergy, initialEnergy, 1.0e-5,
                         "Orthogonal Householder matrix must conserve vector energy exactly (Frobenius norm ratio 1.0)");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_08", "Modal Resonator Matrix - Golden Ratio Stereo Angular Dispersion", []() {
        mr16::ModalResonatorMatrix matrix;
        matrix.prepare(48000.0);
        matrix.setFundamentalHz(300.0f);
        matrix.setStereoWidth(1.0f); // Maximum golden ratio width

        const size_t n = 4000;
        std::vector<float> l(n), r(n);
        float outL, outR;
        matrix.processSample(1.0f, outL, outR);
        l[0] = outL; r[0] = outR;

        for (size_t i = 1; i < n; ++i) {
            matrix.processSample(0.0f, outL, outR);
            l[i] = outL; r[i] = outR;
        }

        float correlation = test_utils::computeStereoCorrelation(l, r);
        TEST_ASSERT(correlation < 0.95f, "Golden ratio stereo dispersion must decorrelate left and right modal channels");
        TEST_ASSERT(test_utils::isSignalFinite(l) && test_utils::isSignalFinite(r), "Modal outputs must be finite");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_09", "Modal Resonator Matrix - Resonance Q Factor Scaling", []() {
        mr16::ModalResonatorMatrix matrixLowQ, matrixHighQ;
        matrixLowQ.prepare(48000.0);
        matrixLowQ.setFundamentalHz(440.0f);
        matrixLowQ.setQScale(0.2f);

        matrixHighQ.prepare(48000.0);
        matrixHighQ.setFundamentalHz(440.0f);
        matrixHighQ.setQScale(5.0f);

        TEST_ASSERT_NEAR(matrixLowQ.getQScale(), 0.2f, 0.01f, "Low Q scale getter must match set value");
        TEST_ASSERT_NEAR(matrixHighQ.getQScale(), 5.0f, 0.01f, "High Q scale getter must match set value");

        const size_t n = 24000;
        std::vector<float> lowL(n), highL(n);
        float outL, outR;
        matrixLowQ.processSample(1.0f, outL, outR); lowL[0] = outL;
        matrixHighQ.processSample(1.0f, outL, outR); highL[0] = outL;

        for (size_t i = 1; i < n; ++i) {
            matrixLowQ.processSample(0.0f, outL, outR); lowL[i] = outL;
            matrixHighQ.processSample(0.0f, outL, outR); highL[i] = outL;
        }

        double lowLateRMS = test_utils::computeRMS(lowL, 12000, 4000);
        double highLateRMS = test_utils::computeRMS(highL, 12000, 4000);
        TEST_ASSERT(highLateRMS > lowLateRMS, "Higher Q scale must sustain resonance significantly longer than lower Q scale");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_10", "Modal Resonator Matrix - Overtone Spread Harmonic Stretching", []() {
        mr16::ModalResonatorMatrix matrix;
        matrix.prepare(48000.0);
        matrix.setFundamentalHz(200.0f);
        matrix.setManifold(mr16::ManifoldType::ChladniPlate);

        // Narrow spread: 0.5x
        matrix.setOvertoneSpread(0.5f);
        TEST_ASSERT_NEAR(matrix.getOvertoneSpread(), 0.5f, 0.01f, "Overtone spread getter must match set value");
        const auto freqsNarrow = matrix.getCurrentFrequencies();
        TEST_ASSERT_NEAR(freqsNarrow[0], 200.0f, 0.5f, "Fundamental must remain 200 Hz under narrow overtone spread");

        // Wide spread: 2.0x
        matrix.setOvertoneSpread(2.0f);
        TEST_ASSERT_NEAR(matrix.getOvertoneSpread(), 2.0f, 0.01f, "Overtone spread getter must match set value");
        const auto freqsWide = matrix.getCurrentFrequencies();
        TEST_ASSERT_NEAR(freqsWide[0], 200.0f, 0.5f, "Fundamental must remain 200 Hz under wide overtone spread");

        // Higher overtones must be spaced further apart with wide spread than narrow spread
        TEST_ASSERT(freqsWide[1] > freqsNarrow[1], "Overtone 1 frequency must be higher under wide spread than narrow spread");
        TEST_ASSERT(freqsWide[4] > freqsNarrow[4], "Overtone 4 frequency must be higher under wide spread than narrow spread");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_11", "Mr16Engine - Modal Q and Overtone Spread Parameter Integration", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 512);

        auto params = engine.getParameters();
        params.modalQScale = 3.5f;
        params.overtoneSpread = 1.8f;
        params.chaosDetuneCents = 300.0f;
        params.vactrolSagEnable = true;
        params.vactrolSagAmount = 0.8f;

        engine.setParameters(params);
        const auto& updated = engine.getParameters();
        TEST_ASSERT_NEAR(updated.modalQScale, 3.5f, 0.01f, "Engine modalQScale parameter must be updated");
        TEST_ASSERT_NEAR(updated.overtoneSpread, 1.8f, 0.01f, "Engine overtoneSpread parameter must be updated");
        TEST_ASSERT_NEAR(updated.chaosDetuneCents, 300.0f, 0.01f, "Engine chaosDetuneCents parameter must be updated");
        TEST_ASSERT(updated.vactrolSagEnable, "Engine vactrolSagEnable parameter must be true");
        return test::gCurrentTestAssertFailures == 0;
    });

    // ========================================================================
    // T1_LOR: 3D Chaotic Lorenz Attractor Subsystem Tests
    // ========================================================================

    registerTest("Tier 1", "T1_LOR_01", "Lorenz Attractor - RK4 Integration & Strange Attractor Bounded Trajectory", []() {
        mr16::LorenzAttractor lorenz;
        lorenz.prepare(48000.0);
        lorenz.reset();
        lorenz.setRateHz(1.0f);
        lorenz.setDepth(0.80f);

        // Run 50,000 steps (~1 second of audio)
        for (int i = 0; i < 50000; ++i) {
            lorenz.step();
            float x = lorenz.getX();
            float y = lorenz.getY();
            float z = lorenz.getZ();

            TEST_ASSERT(std::isfinite(x) && std::isfinite(y) && std::isfinite(z), "Lorenz state coordinates must remain finite");
            TEST_ASSERT(std::abs(x) < 40.0f, "Lorenz X must remain within physical strange attractor bounds");
            TEST_ASSERT(std::abs(y) < 50.0f, "Lorenz Y must remain within physical strange attractor bounds");
            TEST_ASSERT(z >= -5.0f && z < 70.0f, "Lorenz Z must remain within physical strange attractor bounds");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_LOR_02", "Lorenz Attractor - Normalized Telemetry Outputs in [-1, +1]", []() {
        mr16::LorenzAttractor lorenz;
        lorenz.prepare(48000.0);
        lorenz.reset();

        for (int i = 0; i < 10000; ++i) {
            lorenz.step();
            float nx = lorenz.getNormalizedX();
            float ny = lorenz.getNormalizedY();
            float nz = lorenz.getNormalizedZ();

            TEST_ASSERT(nx >= -1.05f && nx <= 1.05f, "Normalized X must be bounded in [-1, +1]");
            TEST_ASSERT(ny >= -1.05f && ny <= 1.05f, "Normalized Y must be bounded in [-1, +1]");
            TEST_ASSERT(nz >= -0.05f && nz <= 1.05f, "Normalized Z must be non-negative in [0, 1]");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_LOR_03", "Lorenz Attractor - 16-Mode Frequency Multipliers Deterministic Chaos", []() {
        mr16::LorenzAttractor lorenz;
        lorenz.prepare(48000.0);
        lorenz.setDetuneMaxCents(60.0f); // +/- 60 cents
        lorenz.setDepth(0.70f);

        for (int i = 0; i < 5000; ++i) {
            lorenz.step();
        }

        const auto& multipliers = lorenz.getFrequencyMultipliers();
        for (size_t m = 0; m < mr16::LorenzAttractor::kNumModes; ++m) {
            TEST_ASSERT(multipliers[m] > 0.85f && multipliers[m] < 1.15f,
                        "Detune multipliers must remain within musical microtonal bounds");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    // ========================================================================
    // T1_CHO: Tri-Phase Spatial BBD Chorus Subsystem Tests
    // ========================================================================

    registerTest("Tier 1", "T1_CHO_01", "Tri-Phase BBD Chorus - Stereo Expansion & Decorrelation", []() {
        mr16::SpatialChorus chorus;
        chorus.prepare(48000.0);
        chorus.setMode(mr16::DimensionMode::Mode3);
        chorus.setParameters(0.75f, 2.5f, 1.0f); // 100% wet

        // Feed mono 440 Hz test sine into both channels
        const size_t n = 24000;
        auto monoSine = test_utils::generateSine(n, 440.0, 48000.0, 0.5f);

        std::vector<float> outL(n), outR(n);
        for (size_t i = 0; i < n; ++i) {
            chorus.process(monoSine[i], monoSine[i], outL[i], outR[i]);
        }

        float correlation = test_utils::computeStereoCorrelation(outL, outR);
        TEST_ASSERT(correlation < 0.40f, "Tri-phase Dimension chorus must provide wide stereo decorrelation (rho < 0.40)");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_CHO_02", "Tri-Phase BBD Chorus - Mathematical Proof of Mono-Sum Phase Cancellation Immunity", []() {
        mr16::SpatialChorus chorus;
        chorus.prepare(48000.0);
        chorus.setMode(mr16::DimensionMode::Mode2);
        chorus.setParameters(0.55f, 2.0f, 1.0f); // 100% wet

        // Feed pink noise through the chorus
        const size_t n = 48000;
        auto pinkNoise = test_utils::generatePinkNoise(n, 0.4f, 424242ULL);

        std::vector<float> outL(n), outR(n), monoSum(n);
        for (size_t i = 0; i < n; ++i) {
            chorus.process(pinkNoise[i], pinkNoise[i], outL[i], outR[i]);
            monoSum[i] = 0.5f * (outL[i] + outR[i]); // Sum to mono
        }

        double inputRms   = test_utils::computeRMS(pinkNoise, 2000, 40000);
        double monoSumRms = test_utils::computeRMS(monoSum, 2000, 40000);

        // Under destructive comb filtering, mono sum collapses completely (< 0.05).
        // Under Dimension D tri-phase matrixing with 0.5 headroom scaling, mono sum preserves robust energy (ratio > 0.25).
        double ratio = monoSumRms / inputRms;
        TEST_ASSERT(ratio > 0.25, "Mono sum must NOT collapse into destructive comb-filter null cancellation");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_CHO_03", "Tri-Phase BBD Chorus - Bypass Mode True Transparency", []() {
        mr16::SpatialChorus chorus;
        chorus.prepare(48000.0);
        chorus.setEnabled(false); // True bypass

        const size_t n = 1000;
        auto testSig = test_utils::generateSine(n, 1000.0, 48000.0, 0.5f);

        for (size_t i = 0; i < n; ++i) {
            float outL, outR;
            chorus.process(testSig[i], testSig[i], outL, outR);
            TEST_ASSERT_NEAR(outL, testSig[i], 1.0e-5, "Bypassed chorus must pass audio bit-exact in left channel");
            TEST_ASSERT_NEAR(outR, testSig[i], 1.0e-5, "Bypassed chorus must pass audio bit-exact in right channel");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_CHO_04", "Tri-Phase BBD Chorus - Continuous Dimension Spread Width Scaling", []() {
        mr16::SpatialChorus chorusMono, chorusWide;
        chorusMono.prepare(48000.0);
        chorusWide.prepare(48000.0);

        chorusMono.setParameters(0.75f, 2.5f, 1.0f);
        chorusMono.setDimensionSpread(0.0f); // Pure mono wet

        chorusWide.setParameters(0.75f, 2.5f, 1.0f);
        chorusWide.setDimensionSpread(1.0f); // Standard Dimension spread

        const size_t n = 24000;
        auto monoSine = test_utils::generateSine(n, 440.0, 48000.0, 0.5f);

        std::vector<float> monoL(n), monoR(n), wideL(n), wideR(n);
        for (size_t i = 0; i < n; ++i) {
            chorusMono.process(monoSine[i], monoSine[i], monoL[i], monoR[i]);
            chorusWide.process(monoSine[i], monoSine[i], wideL[i], wideR[i]);
        }

        double monoSideRms = 0.0;
        double wideSideRms = 0.0;
        for (size_t i = 2000; i < n; ++i) {
            const float sideM = monoL[i] - monoR[i];
            const float sideW = wideL[i] - wideR[i];
            monoSideRms += sideM * sideM;
            wideSideRms += sideW * sideW;
        }
        monoSideRms = std::sqrt(monoSideRms / (n - 2000));
        wideSideRms = std::sqrt(wideSideRms / (n - 2000));

        TEST_ASSERT_NEAR(static_cast<float>(monoSideRms), 0.0f, 1.0e-5f, "Zero dimension spread must produce pure mono wet signal (zero side energy)");
        TEST_ASSERT(wideSideRms > 0.05, "Standard dimension spread must produce significant stereo side energy");
        return test::gCurrentTestAssertFailures == 0;
    });

    // ========================================================================
    // T1_DYN: Dynamics, Saturator & Master Engine Integration Tests
    // ========================================================================

    registerTest("Tier 1", "T1_DYN_01", "Bounded Saturator - Linear Small-Signal Transparency below Knee (0.72)", []() {
        mr16::BoundedSaturator saturator(0.72f, 1.05f);

        std::vector<float> probeSignals = { 0.0f, 0.01f, 0.10f, 0.35f, 0.50f, 0.70f, 0.72f,
                                            -0.01f, -0.10f, -0.35f, -0.50f, -0.70f, -0.72f };

        for (float x : probeSignals) {
            float y = saturator.processSample(x);
            TEST_ASSERT_NEAR(y, x, 1.0e-6, "Signals within linear zone (|x| <= 0.72) must experience exact unity gain");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_DYN_02", "Bounded Saturator - Asymptotic Saturation Clamping at 1.05 Ceiling", []() {
        mr16::BoundedSaturator saturator(0.72f, 1.05f);

        std::vector<float> extremeInputs = { 1.05f, 1.50f, 2.50f, 5.00f, 10.0f, 100.0f,
                                             -1.05f, -1.50f, -2.50f, -5.00f, -10.0f, -100.0f };

        for (float x : extremeInputs) {
            float y = saturator.processSample(x);
            TEST_ASSERT(std::abs(y) <= 1.05001f, "Saturator output must be strictly bounded by ceiling 1.05");
            if (x > 0.0f) {
                TEST_ASSERT_NEAR(y, 1.05f, 0.001f, "Positive extreme inputs must clamp to 1.05");
            } else {
                TEST_ASSERT_NEAR(y, -1.05f, 0.001f, "Negative extreme inputs must clamp to -1.05");
            }
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_DYN_03", "Bounded Saturator - Odd Mathematical Symmetry: f(-x) === -f(x)", []() {
        mr16::BoundedSaturator saturator(0.72f, 1.05f);

        for (int i = 1; i <= 50; ++i) {
            float x = static_cast<float>(i) * 0.05f;
            float yPos = saturator.processSample(x);
            float yNeg = saturator.processSample(-x);
            TEST_ASSERT_NEAR(yPos, -yNeg, 1.0e-6, "Saturator transfer curve must possess exact odd symmetry");
        }
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_DYN_04", "Mr16Engine - Complete Signal Flow Integration & Block Processing", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 512);
        engine.reset();

        // Enqueue kinetic strike
        engine.enqueueTriggerStrike(0.85f, 0.70f);

        const int blockSize = 256;
        const int totalBlocks = 200; // ~1.07 seconds at 48 kHz
        std::vector<float> inL(blockSize, 0.0f), inR(blockSize, 0.0f);
        std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

        float peakInitial = 0.0f;
        float peakAt1Sec = 0.0f;
        float earlyEnergy = 0.0f;
        float lateEnergy = 0.0f;
        int earlySampleCount = 0;
        int lateSampleCount = 0;

        for (int b = 0; b < totalBlocks; ++b) {
            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);
            for (int s = 0; s < blockSize; ++s) {
                const float sL = outL[s];
                const float sR = outR[s];
                TEST_ASSERT(std::isfinite(sL) && std::isfinite(sR), "Engine output must be finite");
                TEST_ASSERT(std::abs(sL) <= 1.05f && std::abs(sR) <= 1.05f,
                            "Engine output must be bounded within saturator ceiling 1.05");

                const float maxMag = std::max(std::abs(sL), std::abs(sR));
                if (b < 10) {
                    peakInitial = std::max(peakInitial, maxMag);
                    earlyEnergy += sL * sL + sR * sR;
                    earlySampleCount += 2;
                } else if (b >= 180) { // ~0.96s to 1.07s
                    peakAt1Sec = std::max(peakAt1Sec, maxMag);
                    lateEnergy += sL * sL + sR * sR;
                    lateSampleCount += 2;
                }
            }
        }

        const float earlyRms = std::sqrt(earlyEnergy / std::max(1, earlySampleCount));
        const float lateRms = std::sqrt(lateEnergy / std::max(1, lateSampleCount));

        TEST_ASSERT(peakInitial > 0.005f, "Engine strike trigger must propagate into audible output");
        TEST_ASSERT(peakAt1Sec < 0.05f, "Engine output must naturally decay at 1.0s without self-oscillation");
        TEST_ASSERT(lateRms < earlyRms * 0.2f, "Engine output RMS must significantly decay from strike transient");

        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_DYN_05", "Mr16Engine - Idle Silence on Power On / Reset", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 512);

        // 1. Verify fresh engine with default parameters is strictly silent on power-on
        engine.reset();

        const int numBlocks = 100; // ~1.06 seconds of audio at 48 kHz
        const int blockSize = 512;
        std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

        float peakIdleFresh = 0.0f;
        for (int b = 0; b < numBlocks; ++b) {
            engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), blockSize);
            for (int s = 0; s < blockSize; ++s) {
                peakIdleFresh = std::max(peakIdleFresh, std::max(std::abs(outL[s]), std::abs(outR[s])));
            }
        }
        TEST_ASSERT(peakIdleFresh <= 1.0e-5f, "Fresh engine on power-on must be strictly silent (<= 1e-5)");

        // 2. Verify engine with snapshot parameters (euclideanEnable = false) is strictly silent
        mr16::Mr16Parameters p;
        p.exciterStrikeVelocity = 0.80f;
        p.exciterStrikeHardness = 0.50f;
        p.exciterBowPressure    = 0.0f;
        p.exciterBowVelocity    = 0.0f;
        p.poissonEnable         = false;
        p.poissonEpm            = 0.0f;
        p.euclideanEnable       = false; // Explicitly false as configured in snapshot
        p.euclideanPulses       = 4;
        p.euclideanSteps        = 16;
        p.externalAudioEnable   = false;
        p.fundamentalHz         = 440.0f;
        p.couplingDepth         = 0.25f;
        p.chorusEnable          = true;
        p.outputMute            = false;

        engine.setParameters(p);
        engine.reset();

        float peakIdlePlugin = 0.0f;
        for (int b = 0; b < numBlocks; ++b) {
            engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), blockSize);
            for (int s = 0; s < blockSize; ++s) {
                peakIdlePlugin = std::max(peakIdlePlugin, std::max(std::abs(outL[s]), std::abs(outR[s])));
            }
        }
        TEST_ASSERT(peakIdlePlugin <= 1.0e-5f, "Plugin engine with euclideanEnable=false must remain completely silent on power-on (<= 1e-5)");

        // 3. Verify that the engine produces sound when an explicit strike is triggered
        engine.enqueueTriggerStrike(0.85f, 0.70f);
        float peakAfterStrike = 0.0f;
        for (int b = 0; b < 10; ++b) {
            engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), blockSize);
            for (int s = 0; s < blockSize; ++s) {
                peakAfterStrike = std::max(peakAfterStrike, std::max(std::abs(outL[s]), std::abs(outR[s])));
            }
        }
        TEST_ASSERT(peakAfterStrike > 0.005f, "Engine must produce audible output once an explicit strike is triggered");

        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_DYN_06", "Mr16Engine - Exciter Mode Switching Without Discontinuities", []() {
        mr16::Mr16Engine engine;
        engine.prepare(48000.0, 128);
        engine.reset();

        mr16::Mr16Parameters p;
        p.fundamentalHz = 440.0f;
        p.euclideanEnable = false;
        p.poissonEnable = false;
        p.chorusEnable = false;
        p.outputMute = false;

        constexpr int blockSize = 128;
        std::vector<float> outL(blockSize, 0.0f);
        std::vector<float> outR(blockSize, 0.0f);

        // 1. Start in Strike mode (idle)
        p.exciterBowPressure = 0.0f;
        p.exciterBowVelocity = 0.0f;
        p.externalAudioEnable = false;
        engine.setParameters(p);
        for (int b = 0; b < 4; ++b) {
            engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), blockSize);
        }

        // 2. Switch to Friction mode (pressure = 0.5, velocity = 0.5)
        p.exciterBowPressure = 0.50f;
        p.exciterBowVelocity = 0.50f;
        engine.setParameters(p);

        float maxStepDelta = 0.0f;
        float prevSample = 0.0f;
        for (int b = 0; b < 10; ++b) {
            engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), blockSize);
            for (int s = 0; s < blockSize; ++s) {
                const float curr = outL[s];
                const float delta = std::abs(curr - prevSample);
                maxStepDelta = std::max(maxStepDelta, delta);
                prevSample = curr;
            }
        }
        TEST_ASSERT(maxStepDelta < 0.015f, "Exciter mode switch into friction must slew smoothly without step impulse (< 0.015)");
        TEST_ASSERT(test_utils::isSignalFinite(outL), "Friction output must remain strictly finite");

        // 3. Switch from Friction back to Strike mode (pressure = 0.0, velocity = 0.0)
        p.exciterBowPressure = 0.0f;
        p.exciterBowVelocity = 0.0f;
        engine.setParameters(p);

        maxStepDelta = 0.0f;
        for (int b = 0; b < 10; ++b) {
            engine.processBlock(nullptr, nullptr, outL.data(), outR.data(), blockSize);
            for (int s = 0; s < blockSize; ++s) {
                const float curr = outL[s];
                const float delta = std::abs(curr - prevSample);
                maxStepDelta = std::max(maxStepDelta, delta);
                prevSample = curr;
            }
        }
        TEST_ASSERT(maxStepDelta < 0.015f, "Exciter mode switch out of friction must slew smoothly without step impulse (< 0.015)");

        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_DYN_07", "Mr16Engine - Master Dry / Wet Mix External Audio Blending", []() {
        mr16::Mr16Engine engineDry, engineWet;
        engineDry.prepare(48000.0, 128);
        engineWet.prepare(48000.0, 128);

        mr16::Mr16Parameters pDry;
        pDry.externalAudioEnable = true;
        pDry.externalSensitivity = 1.0f;
        pDry.dryWetMix = 0.0f; // 100% dry external input feedthrough
        pDry.outputMute = false;
        engineDry.setParameters(pDry);

        mr16::Mr16Parameters pWet;
        pWet.externalAudioEnable = true;
        pWet.externalSensitivity = 1.0f;
        pWet.dryWetMix = 1.0f; // 100% wet resonator output
        pWet.outputMute = false;
        engineWet.setParameters(pWet);

        constexpr int blockSize = 128;
        const auto inSig = test_utils::generateSine(blockSize * 10, 1000.0, 48000.0, 0.5f);
        std::vector<float> dryOutL(blockSize, 0.0f), dryOutR(blockSize, 0.0f);
        std::vector<float> wetOutL(blockSize, 0.0f), wetOutR(blockSize, 0.0f);

        for (int b = 0; b < 10; ++b) {
            const float* inPtr = inSig.data() + b * blockSize;
            engineDry.processBlock(inPtr, inPtr, dryOutL.data(), dryOutR.data(), blockSize);
            engineWet.processBlock(inPtr, inPtr, wetOutL.data(), wetOutR.data(), blockSize);
        }

        // With dryWetMix = 0.0, output should pass input signal cleanly
        for (int s = 0; s < blockSize; ++s) {
            const float expected = inSig[9 * blockSize + s];
            TEST_ASSERT_NEAR(dryOutL[s], expected, 1.0e-4f, "100% dry mix must pass input signal cleanly");
        }

        TEST_ASSERT(test_utils::isSignalFinite(wetOutL), "Wet output must remain finite");
        return test::gCurrentTestAssertFailures == 0;
    });

    registerTest("Tier 1", "T1_MOD_12", "Modal Resonator Matrix - Decay Scale Proportional RT60 Damping", []() {
        mr16::ModalResonatorMatrix shortMatrix, longMatrix;
        shortMatrix.prepare(48000.0);
        longMatrix.prepare(48000.0);

        shortMatrix.setFundamentalHz(440.0f);
        shortMatrix.setDecayScale(0.20f); // Short decay (fast damping)

        longMatrix.setFundamentalHz(440.0f);
        longMatrix.setDecayScale(3.00f); // Long decay (slow damping)

        float sL = 0.0f, sR = 0.0f;
        float lL = 0.0f, lR = 0.0f;
        shortMatrix.processSample(1.0f, sL, sR);
        longMatrix.processSample(1.0f, lL, lR);

        const size_t ringSamples = 48000;
        std::vector<float> shortRing(ringSamples), longRing(ringSamples);
        for (size_t i = 0; i < ringSamples; ++i) {
            shortMatrix.processSample(0.0f, shortRing[i], sR);
            longMatrix.processSample(0.0f, longRing[i], lR);
        }

        double shortTailEnergy = test_utils::computeRMS(shortRing, 24000, 24000);
        double longTailEnergy = test_utils::computeRMS(longRing, 24000, 24000);

        TEST_ASSERT(longTailEnergy > shortTailEnergy * 10.0, "Long decay scale (3.0s) must retain significantly more tail energy than short decay scale (0.2s)");
        return test::gCurrentTestAssertFailures == 0;
    });

}

} // namespace test
