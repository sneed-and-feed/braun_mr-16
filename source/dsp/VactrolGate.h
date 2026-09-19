#pragma once

#include "DspMath.h"
#include <cmath>
#include <algorithm>

namespace braun::mr16 {

enum class VactrolMode : int {
    Combo = 0, // Simultaneous lowpass filter + VCA (Buchla 292 classic)
    FilterOnly = 1, // Lowpass filter only
    VcaOnly = 2     // VCA envelope only
};

/**
 * VactrolGate: Emulation of the Buchla 292 Optical Lowpass Gate (LPG).
 *
 * Implements:
 * 1. Asymmetric photocarrier trapping differential equation:
 *    - Rapid turn-on response (tau_attack = 2 ms)
 *    - Prolonged multi-exponential decay tail with dynamic sag expansion
 * 2. Dual non-linear coupling to lowpass cutoff (fc) and VCA passband gain.
 * 3. Dynamic audio envelope follower for automatic resonant sag response.
 * 4. TPT 1-pole / 2-pole topology-preserving filter with zero-delay stability.
 */
class VactrolGate {
public:
    VactrolGate() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setMode(VactrolMode mode) noexcept { mMode = mode; }
    void setControlVoltage(float cv) noexcept;
    void setDynamicSag(float sagAmount) noexcept;
    void setCutoffRange(float minHz, float maxHz) noexcept;
    void setDecayTime(float decaySec) noexcept;

    // Trigger an instantaneous optocoupler LED flash (e.g. from an exciter strike)
    void trigger(float velocity = 1.0f) noexcept;

    // Process single sample
    [[nodiscard]] float processSample(float input) noexcept;

    // Process stereo pair
    void processStereo(float inL, float inR, float& outL, float& outR) noexcept;

    // Block processing (in-place stereo)
    void processBlock(float* bufferL, float* bufferR, int numSamples) noexcept;

    [[nodiscard]] float getConductance() const noexcept { return mConductance; }
    [[nodiscard]] float getCurrentCutoffHz() const noexcept { return mCurrentCutoff; }

private:
    void updateConductance(float targetCv) noexcept;

    float mSampleRate { 48000.0f };
    VactrolMode mMode { VactrolMode::Combo };

    // Vactrol physical parameters
    float mConductance { 0.0f };
    float mTargetCv { 0.0f };
    float mDynamicSag { 0.50f };
    float mTauAttack { 0.002f };  // 2 ms fast attack
    float mTauDecay { 0.080f };   // 80 ms nominal decay
    float mSagKappa { 4.5f };     // Sag expansion non-linearity
    float mMinCutoff { 40.0f };
    float mMaxCutoff { 18000.0f };
    float mCurrentCutoff { 40.0f };

    // Dynamic envelope follower for self-sagging
    float mEnvFollower { 0.0f };

    // TPT filter states (Stereo 2-pole cascade)
    float mStateL1 { 0.0f };
    float mStateL2 { 0.0f };
    float mStateR1 { 0.0f };
    float mStateR2 { 0.0f };
};

} // namespace braun::mr16
