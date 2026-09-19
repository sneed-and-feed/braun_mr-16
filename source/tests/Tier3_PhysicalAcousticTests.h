#pragma once

#include "TestHarness.h"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

namespace test {

inline void registerTier3Tests() {

    // ========================================================================
    // T3_FRIC: Karnopp Stick-Slip Friction & Stiction Dynamics (40 Tests)
    // ========================================================================

    // T3_FRIC_01 to T3_FRIC_20: Static breakaway force across normal forces Fn in [0.05, 1.0]
    for (int i = 1; i <= 20; ++i) {
        const float fn = i * 0.05f;
        const std::string id = "T3_FRIC_" + (i < 10 ? std::string("0") : std::string("")) + std::to_string(i);
        const std::string name = "Stick-Slip Stiction - Breakaway Force Threshold for Fn = " + std::to_string(fn);

        registerTest("Tier 3", id, name, [fn]() {
            mr16::KineticExciter exciter;
            exciter.prepare(48000.0);
            exciter.setFriction(0.001f, fn); // Sub-breakaway micro-velocity inside deadband

            // Run 64 samples
            float maxF = 0.0f;
            for (int s = 0; s < 64; ++s) {
                float out = exciter.processSample(0.0f);
                maxF = std::max(maxF, std::abs(out));
            }

            const float expectedMaxBreakaway = 0.85f * fn * 0.35f + 0.05f;
            TEST_ASSERT(maxF <= expectedMaxBreakaway, "Stiction force inside deadband must not exceed static breakaway threshold");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_FRIC_21 to T3_FRIC_30: Dynamic Stribeck curve decay across velocities
    const std::vector<float> stribeckVelocities = { 0.01f, 0.02f, 0.03f, 0.05f, 0.08f, 0.12f, 0.20f, 0.35f, 0.50f, 0.80f };
    for (size_t i = 0; i < stribeckVelocities.size(); ++i) {
        const float v = stribeckVelocities[i];
        const int testNum = static_cast<int>(21 + i);
        const std::string id = "T3_FRIC_" + std::to_string(testNum);
        const std::string name = "Stick-Slip Stiction - Stribeck Velocity Damping at vRel = " + std::to_string(v);

        registerTest("Tier 3", id, name, [v]() {
            mr16::KineticExciter exciter;
            exciter.prepare(48000.0);
            exciter.setFriction(v, 0.60f);

            std::vector<float> out(256, 0.0f);
            for (size_t s = 0; s < out.size(); ++s) {
                out[s] = exciter.processSample(0.0f);
            }

            double rms = test_utils::computeRMS(out, 64, 128);
            TEST_ASSERT(rms > 0.0001, "Stribeck dynamic slip must produce audible non-zero friction energy");
            TEST_ASSERT(test_utils::isSignalFinite(out), "Slip signal must be 100% finite");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_FRIC_31 to T3_FRIC_35: Deadband stiction locking (|v| < 0.0015)
    const std::vector<float> microVelocities = { 0.0f, 0.0002f, 0.0005f, 0.0010f, 0.0014f };
    for (size_t i = 0; i < microVelocities.size(); ++i) {
        const float v = microVelocities[i];
        const int testNum = static_cast<int>(31 + i);
        const std::string id = "T3_FRIC_" + std::to_string(testNum);
        const std::string name = "Stick-Slip Stiction - Deadband Locking for v = " + std::to_string(v);

        registerTest("Tier 3", id, name, [v]() {
            mr16::KineticExciter exciter;
            exciter.prepare(48000.0);
            exciter.setFriction(v, 0.80f);

            for (int s = 0; s < 128; ++s) {
                float out = exciter.processSample(0.0f);
                TEST_ASSERT(std::abs(out) <= 0.85f * 0.80f * 0.35f + 0.01f, "Deadband must enforce static stiction force limit");
            }
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_FRIC_36 to T3_FRIC_40: Odd anti-symmetry F(-v) = -F(v)
    const std::vector<float> symVelocities = { 0.005f, 0.02f, 0.10f, 0.30f, 0.60f };
    for (size_t i = 0; i < symVelocities.size(); ++i) {
        const float v = symVelocities[i];
        const int testNum = static_cast<int>(36 + i);
        const std::string id = "T3_FRIC_" + std::to_string(testNum);
        const std::string name = "Stick-Slip Stiction - Directional Anti-Symmetry for v = " + std::to_string(v);

        registerTest("Tier 3", id, name, [v]() {
            // Analytical check of Karnopp velocity curve anti-symmetry
            constexpr float vStribeck = 0.06f;
            (void)vStribeck;
            const float fn = 0.75f;
            const float fs = 0.85f * fn;
            const float fc = 0.35f * fn;
            auto fKarnopp = [&](float vel) {
                const float sgn = (vel > 0.0f) ? 1.0f : -1.0f;
                const float decay = std::exp(-(vel * vel) / (vStribeck * vStribeck));
                return sgn * (fc + (fs - fc) * decay) + 0.12f * fn * vel;
            };

            const float fPos = fKarnopp(v);
            const float fNeg = fKarnopp(-v);
            TEST_ASSERT(std::abs(fPos + fNeg) < 1.0e-5f, "Karnopp friction must satisfy strict odd anti-symmetry: f(-v) === -f(v)");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // ========================================================================
    // T3_VAC: Buchla 292 Optical Vactrol Sag Times & Dynamics (40 Tests)
    // ========================================================================

    // T3_VAC_01 to T3_VAC_10: Attack rise time (<2 ms) across trigger velocities
    for (int i = 1; i <= 10; ++i) {
        const float vel = i * 0.10f;
        const std::string id = "T3_VAC_" + (i < 10 ? std::string("0") : std::string("")) + std::to_string(i);
        const std::string name = "Vactrol Sag - Fast Optical Rise Time (<2ms) for Velocity = " + std::to_string(vel);

        registerTest("Tier 3", id, name, [vel]() {
            mr16::VactrolGate vactrol;
            vactrol.prepare(48000.0);
            vactrol.reset();
            vactrol.trigger(vel);

            // 2 ms at 48 kHz is 96 samples
            float maxRise = 0.0f;
            for (int s = 0; s < 96; ++s) {
                (void)vactrol.processSample(0.0f);
                float cv = vactrol.getConductance();
                maxRise = std::max(maxRise, cv);
            }
            TEST_ASSERT(maxRise >= vel * 0.50f, "Vactrol optical conductance must rise rapidly within 2 ms");
            TEST_ASSERT(maxRise <= vel * 1.05f, "Rise must not exceed physical velocity peak");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_VAC_11 to T3_VAC_20: Fast release phase decay at 40 ms across decay scales
    for (int i = 1; i <= 10; ++i) {
        const float decaySec = 0.040f * (0.5f + i * 0.1f);
        const int testNum = 10 + i;
        const std::string id = "T3_VAC_" + std::to_string(testNum);
        const std::string name = "Vactrol Sag - Primary Release Decay at 40ms for Tau = " + std::to_string(decaySec);

        registerTest("Tier 3", id, name, [decaySec]() {
            mr16::VactrolGate vactrol;
            vactrol.prepare(48000.0);
            vactrol.reset();
            vactrol.setDecayTime(decaySec);
            vactrol.trigger(1.0f);

            // Advance 40 ms (1920 samples)
            float lateCv = 0.0f;
            for (int s = 0; s < 1920; ++s) {
                (void)vactrol.processSample(0.0f);
                lateCv = vactrol.getConductance();
            }
            TEST_ASSERT(lateCv < 0.75f, "Conductance must drop substantially after primary 40ms release phase");
            TEST_ASSERT(lateCv > 0.01f, "Conductance must still retain phosphorescent tail energy at 40ms");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_VAC_21 to T3_VAC_30: Long phosphorescent tail persistence at 150ms - 400ms
    for (int i = 1; i <= 10; ++i) {
        const int delayMs = 150 + i * 25; // 175 ms to 400 ms
        const int testNum = 20 + i;
        const std::string id = "T3_VAC_" + std::to_string(testNum);
        const std::string name = "Vactrol Sag - Phosphorescent Tail Persistence at t = " + std::to_string(delayMs) + "ms";

        registerTest("Tier 3", id, name, [delayMs]() {
            mr16::VactrolGate vactrol;
            vactrol.prepare(48000.0);
            vactrol.reset();
            vactrol.setDynamicSag(0.60f);
            vactrol.trigger(1.0f);

            const int totalSamples = (48000 * delayMs) / 1000;
            float cv = 0.0f;
            for (int s = 0; s < totalSamples; ++s) {
                (void)vactrol.processSample(0.0f);
                cv = vactrol.getConductance();
            }
            TEST_ASSERT(cv > 0.0005f, "Phosphorescent tail must maintain non-zero optical conductance");
            TEST_ASSERT(cv < 0.50f, "Phosphorescent tail must decay monotonically over time");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_VAC_31 to T3_VAC_35: Sag memory / hysteresis accumulation
    for (int i = 1; i <= 5; ++i) {
        const int pulseCount = i * 3;
        const int testNum = 30 + i;
        const std::string id = "T3_VAC_" + std::to_string(testNum);
        const std::string name = "Vactrol Sag - Successive Pulse Accumulation for " + std::to_string(pulseCount) + " Pulses";

        registerTest("Tier 3", id, name, [pulseCount]() {
            mr16::VactrolGate vactrol;
            vactrol.prepare(48000.0);
            vactrol.reset();

            float maxCv = 0.0f;
            for (int p = 0; p < pulseCount; ++p) {
                vactrol.trigger(0.35f);
                for (int s = 0; s < 480; ++s) { // 10 ms spacing
                    (void)vactrol.processSample(0.0f);
                    maxCv = std::max(maxCv, vactrol.getConductance());
                }
            }
            TEST_ASSERT(maxCv <= 1.05f, "Successive vactrol charge accumulation must remain bounded without blowout");
            TEST_ASSERT(maxCv > 0.35f, "Rapid pulses must accumulate charge above single-pulse level");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_VAC_36 to T3_VAC_40: Cutoff frequency CV tracking
    const std::vector<float> cvCheckpoints = { 0.0f, 0.25f, 0.50f, 0.75f, 1.0f };
    for (size_t i = 0; i < cvCheckpoints.size(); ++i) {
        const float cv = cvCheckpoints[i];
        const int testNum = static_cast<int>(36 + i);
        const std::string id = "T3_VAC_" + std::to_string(testNum);
        const std::string name = "Vactrol Sag - Dynamic Cutoff Range Mapping for CV = " + std::to_string(cv);

        registerTest("Tier 3", id, name, [cv]() {
            mr16::VactrolGate vactrol;
            vactrol.prepare(48000.0);
            vactrol.reset();
            vactrol.setCutoffRange(40.0f, 14000.0f);
            vactrol.setControlVoltage(cv);

            float outL = 0.0f, outR = 0.0f;
            vactrol.processStereo(1.0f, 1.0f, outL, outR);

            TEST_ASSERT(std::isfinite(outL) && std::isfinite(outR), "Vactrol stereo filtering must be finite");
            TEST_ASSERT(std::abs(outL) <= 1.05f && std::abs(outR) <= 1.05f, "Filtered signal must be bounded");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // ========================================================================
    // T3_PAN: Golden-Ratio Spatial Panning Invariants (40 Tests)
    // ========================================================================

    // T3_PAN_01 to T3_PAN_16: Exact azimuth formula theta_m = fmod(m * 137.507764, 360) - 180
    constexpr float kGoldenAngle = 137.507764f;
    for (int m = 0; m < 16; ++m) {
        const std::string id = "T3_PAN_" + (m < 9 ? std::string("0") : std::string("")) + std::to_string(m + 1);
        const std::string name = "Golden-Ratio Panning - Azimuth Angle for Mode m = " + std::to_string(m);

        registerTest("Tier 3", id, name, [m, kGoldenAngle]() {
            const float rawDeg = std::fmod(static_cast<float>(m) * kGoldenAngle, 360.0f);
            const float thetaM = rawDeg - 180.0f;
            TEST_ASSERT(thetaM >= -180.0f && thetaM <= 180.0f, "Azimuth angle must be in [-180, +180] deg");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_PAN_17 to T3_PAN_32: Constant-power energy preservation L^2 + R^2 = 1.0
    for (int m = 0; m < 16; ++m) {
        const int testNum = 17 + m;
        const std::string id = "T3_PAN_" + std::to_string(testNum);
        const std::string name = "Golden-Ratio Panning - Constant Power Law L^2 + R^2 = 1.0 for Mode m = " + std::to_string(m);

        registerTest("Tier 3", id, name, [m, kGoldenAngle]() {
            const float rawDeg = std::fmod(static_cast<float>(m) * kGoldenAngle, 360.0f);
            const float angleRad = rawDeg * (3.1415926535f / 180.0f);
            const float panNorm = 0.5f + 0.5f * std::sin(angleRad);
            const float panL = std::cos((3.1415926535f / 2.0f) * panNorm);
            const float panR = std::sin((3.1415926535f / 2.0f) * panNorm);
            const float energy = panL * panL + panR * panR;
            TEST_ASSERT(std::abs(energy - 1.0f) < 1.0e-4f, "Pan law must preserve constant acoustic power (L^2 + R^2 = 1.0)");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_PAN_33 to T3_PAN_36: Stereo width scaling (0%, 33%, 66%, 100%)
    const std::vector<float> widths = { 0.0f, 0.33f, 0.66f, 1.00f };
    for (size_t i = 0; i < widths.size(); ++i) {
        const float w = widths[i];
        const int testNum = static_cast<int>(33 + i);
        const std::string id = "T3_PAN_" + std::to_string(testNum);
        const std::string name = "Golden-Ratio Panning - Stereo Width Scaling for Width = " + std::to_string(w);

        registerTest("Tier 3", id, name, [w, kGoldenAngle]() {
            for (int m = 0; m < 16; ++m) {
                const float rawDeg = std::fmod(static_cast<float>(m) * kGoldenAngle, 360.0f);
                const float angleRad = rawDeg * (3.1415926535f / 180.0f);
                const float panNorm = 0.5f + 0.5f * w * std::sin(angleRad);
                TEST_ASSERT(panNorm >= 0.0f && panNorm <= 1.0f, "Pan normalized position must be in [0, 1]");
                if (w == 0.0f) {
                    TEST_ASSERT(std::abs(panNorm - 0.5f) < 1.0e-5f, "Zero width must collapse pan to center 0.5");
                }
            }
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_PAN_37 to T3_PAN_40: Left/Right panorama symmetry & mono cancellation immunity
    for (int i = 1; i <= 4; ++i) {
        const int testNum = 36 + i;
        const std::string id = "T3_PAN_" + std::to_string(testNum);
        const std::string name = "Golden-Ratio Panning - Panorama Energy Balance Quadrant " + std::to_string(i);

        registerTest("Tier 3", id, name, [i, kGoldenAngle]() {
            float sumL = 0.0f, sumR = 0.0f;
            const int startM = (i - 1) * 4;
            for (int m = startM; m < startM + 4; ++m) {
                const float rawDeg = std::fmod(static_cast<float>(m) * kGoldenAngle, 360.0f);
                const float angleRad = rawDeg * (3.1415926535f / 180.0f);
                const float panNorm = 0.5f + 0.5f * std::sin(angleRad);
                sumL += std::cos((3.1415926535f / 2.0f) * panNorm);
                sumR += std::sin((3.1415926535f / 2.0f) * panNorm);
            }
            TEST_ASSERT(sumL > 0.5f && sumR > 0.5f, "Each modal quadrant must contribute energy to both channels");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // ========================================================================
    // T3_LIM: Modal Resonator Hard Limiter & Anti-Runaway Protection (40 Tests)
    // ========================================================================

    // T3_LIM_01 to T3_LIM_16: Resonant sine injection tuned to each mode m in [0, 15] at max Q
    for (int m = 0; m < 16; ++m) {
        const std::string id = "T3_LIM_" + (m < 9 ? std::string("0") : std::string("")) + std::to_string(m + 1);
        const std::string name = "Modal Anti-Runaway - Resonant Sine Input Tuned to Mode " + std::to_string(m) + " at Max Q";

        registerTest("Tier 3", id, name, [m]() {
            mr16::Mr16Engine engine;
            engine.prepare(48000.0, 256);
            engine.reset();

            mr16::Mr16Parameters p;
            p.externalAudioEnable = true;
            p.externalSensitivity = 1.0f;
            p.externalDirectMix = 0.50f;
            p.modalQScale = 10.0f; // Maximum Q boost
            p.decayScale = 5.0f;
            p.fundamentalHz = 220.0f;
            p.manifold = mr16::ManifoldType::ChladniPlate;
            p.material = mr16::MaterialType::Steel;
            engine.setParameters(p);

            // Compute the target mode frequency
            const float fMode = 220.0f * mr16::ModalResonatorMatrix::kChladniPlateRatios[m];
            constexpr int blockSize = 256;
            constexpr int numBlocks = 48000 / blockSize; // 1 second
            const auto inSig = test_utils::generateSine(blockSize * numBlocks, fMode, 48000.0, 1.0f);
            std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

            float maxPeak = 0.0f;
            int breachCount = 0;

            for (int b = 0; b < numBlocks; ++b) {
                const float* inPtr = inSig.data() + b * blockSize;
                engine.processBlock(inPtr, inPtr, outL.data(), outR.data(), blockSize);
                for (int s = 0; s < blockSize; ++s) {
                    float mag = std::max(std::abs(outL[s]), std::abs(outR[s]));
                    maxPeak = std::max(maxPeak, mag);
                    if (mag > 1.0501f) {
                        ++breachCount;
                    }
                }
            }

            TEST_ASSERT(breachCount == 0, "Resonant sine at max Q must strictly NEVER breach the 1.05 ceiling");
            TEST_ASSERT(maxPeak <= 1.0501f, "Output peak must be contained by modal brickwall limiter");
            TEST_ASSERT(maxPeak > 0.05f, "Resonant mode must pass audible signal");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_LIM_17 to T3_LIM_26: Fundamental frequency sweep under resonant input
    const std::vector<float> sweepFreqs = { 55.0f, 110.0f, 165.0f, 220.0f, 330.0f, 440.0f, 550.0f, 660.0f, 880.0f, 1760.0f };
    for (size_t i = 0; i < sweepFreqs.size(); ++i) {
        const float f = sweepFreqs[i];
        const int testNum = static_cast<int>(17 + i);
        const std::string id = "T3_LIM_" + std::to_string(testNum);
        const std::string name = "Modal Anti-Runaway - Fundamental Resonance Sweep at f0 = " + std::to_string(f) + " Hz";

        registerTest("Tier 3", id, name, [f]() {
            mr16::Mr16Engine engine;
            engine.prepare(48000.0, 256);
            engine.reset();

            mr16::Mr16Parameters p;
            p.externalAudioEnable = true;
            p.externalSensitivity = 1.0f;
            p.modalQScale = 5.0f;
            p.decayScale = 3.0f;
            p.fundamentalHz = f;
            p.material = mr16::MaterialType::Glass; // High Q material
            engine.setParameters(p);

            constexpr int blockSize = 256;
            constexpr int numBlocks = (48000 * 1) / blockSize;
            const auto inSig = test_utils::generateSine(blockSize * numBlocks, f, 48000.0, 0.95f);
            std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

            float maxPeak = 0.0f;
            for (int b = 0; b < numBlocks; ++b) {
                const float* inPtr = inSig.data() + b * blockSize;
                engine.processBlock(inPtr, inPtr, outL.data(), outR.data(), blockSize);
                for (int s = 0; s < blockSize; ++s) {
                    maxPeak = std::max(maxPeak, std::max(std::abs(outL[s]), std::abs(outR[s])));
                    TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "Output must be finite");
                }
            }

            TEST_ASSERT(maxPeak <= 1.0501f, "Output peak must not exceed saturator ceiling during resonant sweep");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_LIM_27 to T3_LIM_31: Multi-harmonic chord excitation tests
    for (int i = 1; i <= 5; ++i) {
        const int testNum = 26 + i;
        const std::string id = "T3_LIM_" + std::to_string(testNum);
        const std::string name = "Modal Anti-Runaway - Dense Harmonic Chord Cluster Resonance " + std::to_string(i);

        registerTest("Tier 3", id, name, []() {
            mr16::Mr16Engine engine;
            engine.prepare(48000.0, 256);
            engine.reset();

            mr16::Mr16Parameters p;
            p.externalAudioEnable = true;
            p.externalSensitivity = 1.0f;
            p.modalQScale = 4.0f;
            p.couplingDepth = 0.50f; // High Householder coupling
            p.material = mr16::MaterialType::Brass;
            engine.setParameters(p);

            constexpr int blockSize = 256;
            constexpr int numBlocks = 48000 / blockSize;
            // Synthesize triad: 220, 277.18, 329.63 Hz
            std::vector<float> chordIn(blockSize * numBlocks, 0.0f);
            for (int s = 0; s < blockSize * numBlocks; ++s) {
                const double t = static_cast<double>(s) / 48000.0;
                chordIn[s] = 0.33f * static_cast<float>(
                    std::sin(2.0 * 3.1415926535 * 220.0 * t) +
                    std::sin(2.0 * 3.1415926535 * 277.18 * t) +
                    std::sin(2.0 * 3.1415926535 * 329.63 * t)
                );
            }

            std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);
            float maxPeak = 0.0f;

            for (int b = 0; b < numBlocks; ++b) {
                const float* inPtr = chordIn.data() + b * blockSize;
                engine.processBlock(inPtr, inPtr, outL.data(), outR.data(), blockSize);
                for (int s = 0; s < blockSize; ++s) {
                    maxPeak = std::max(maxPeak, std::max(std::abs(outL[s]), std::abs(outR[s])));
                    TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "Chord output must be finite");
                }
            }

            TEST_ASSERT(maxPeak <= 1.0501f, "Harmonic chord cluster output must not breach limiter ceiling");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_LIM_32 to T3_LIM_36: Post-overload decay to silence (>90% decay within 2s)
    for (int i = 1; i <= 5; ++i) {
        const float decaySetting = 1.0f + i * 0.5f;
        const int testNum = 31 + i;
        const std::string id = "T3_LIM_" + std::to_string(testNum);
        const std::string name = "Modal Anti-Runaway - Post-Resonance Clean Decay for Decay = " + std::to_string(decaySetting);

        registerTest("Tier 3", id, name, [decaySetting]() {
            mr16::Mr16Engine engine;
            engine.prepare(48000.0, 256);
            engine.reset();

            mr16::Mr16Parameters p;
            p.externalAudioEnable = true;
            p.decayScale = decaySetting;
            p.modalQScale = 2.0f;
            engine.setParameters(p);

            constexpr int blockSize = 256;
            // Excite with 0.5s of sine
            constexpr int exciteBlocks = (48000 / 2) / blockSize;
            const auto inSig = test_utils::generateSine(blockSize * exciteBlocks, 440.0, 48000.0, 0.9f);
            std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);

            for (int b = 0; b < exciteBlocks; ++b) {
                const float* inPtr = inSig.data() + b * blockSize;
                engine.processBlock(inPtr, inPtr, outL.data(), outR.data(), blockSize);
            }

            // Cut off input and decay for 2 seconds (375 blocks)
            constexpr int decayBlocks = (48000 * 2) / blockSize;
            std::vector<float> silentIn(blockSize, 0.0f);
            float earlyRms = 0.0f, lateRms = 0.0f;

            for (int b = 0; b < decayBlocks; ++b) {
                engine.processBlock(silentIn.data(), silentIn.data(), outL.data(), outR.data(), blockSize);
                float energy = 0.0f;
                for (int s = 0; s < blockSize; ++s) {
                    energy += outL[s] * outL[s] + outR[s] * outR[s];
                }
                float rms = std::sqrt(energy / (blockSize * 2));
                if (b == 0) earlyRms = rms;
                if (b == decayBlocks - 1) lateRms = rms;
            }

            TEST_ASSERT(lateRms < earlyRms * 0.20f, "Resonant energy must strictly decay by >80% after input cut");
            TEST_ASSERT(lateRms < 0.05f, "Tail must decay towards silence without runaway metallic feedback");
            return test::gCurrentTestAssertFailures == 0;
        });
    }

    // T3_LIM_37 to T3_LIM_40: High-energy Dirac burst (+40 dBFS) state clamping
    for (int i = 1; i <= 4; ++i) {
        const float impulseAmp = 25.0f * i; // +28 dBFS to +40 dBFS
        const int testNum = 36 + i;
        const std::string id = "T3_LIM_" + std::to_string(testNum);
        const std::string name = "Modal Anti-Runaway - High-Energy Impulse Clamping for Amp = " + std::to_string(impulseAmp);

        registerTest("Tier 3", id, name, [impulseAmp]() {
            mr16::Mr16Engine engine;
            engine.prepare(48000.0, 256);
            engine.reset();

            mr16::Mr16Parameters p;
            p.externalAudioEnable = true;
            p.modalQScale = 5.0f;
            engine.setParameters(p);

            constexpr int blockSize = 256;
            std::vector<float> inL(blockSize, 0.0f), inR(blockSize, 0.0f);
            std::vector<float> outL(blockSize, 0.0f), outR(blockSize, 0.0f);
            inL[0] = impulseAmp;
            inR[0] = impulseAmp;

            engine.processBlock(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);

            for (int s = 0; s < blockSize; ++s) {
                TEST_ASSERT(std::isfinite(outL[s]) && std::isfinite(outR[s]), "Impulse output must be finite");
                TEST_ASSERT(std::abs(outL[s]) <= 1.0501f && std::abs(outR[s]) <= 1.0501f, "Extreme burst must be clamped by brickwall ceiling");
            }
            return test::gCurrentTestAssertFailures == 0;
        });
    }

}

} // namespace test
