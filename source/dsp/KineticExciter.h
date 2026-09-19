#pragma once

#include "DspMath.h"
#include "VactrolGate.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace braun::mr16 {

enum class MicrotonalScale : int {
    TwelveTet       = 0,
    JustIntonation  = 1,
    Pythagorean     = 2,
    Slendro         = 3,
    Pelog           = 4,
    WendyCarlosAlpha= 5,
    BohlenPierce    = 6,
    HarmonicSeries  = 7
};

/**
 * KineticExciter: Physical contact mechanics, stick-slip friction,
 * stochastic trigger generator, and microtonal chime exciter engine.
 *
 * Implements:
 * 1. Hunt-Crossley non-linear viscoelastic mass-spring collision strike.
 * 2. Karnopp stick-slip friction model with Stribeck velocity curves for bowed glass/metal.
 * 3. Buchla 292 optical vactrol dynamic pluck shaping.
 * 4. External audio input with 15 Hz DC blocker and direct resonator matrix injection.
 * 5. Autonomous Poisson rain stochastic trigger generator (Delta t = -ln(1-U)/lambda).
 * 6. Euclidean polyrhythm generator E(k, n) via Bjorklund's algorithm.
 * 7. 16-key microtonal chime synthesizer with AS-42 scale snap and tactile strike buttons.
 */
class KineticExciter {
public:
    static constexpr size_t kNumChimeKeys = 16;
    static constexpr size_t kNumStrikeButtons = 4;

    KineticExciter() noexcept;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    // Direct performance triggers
    void triggerStrike(float velocity, float hardness = 0.65f) noexcept;
    void triggerStrikeButton(int buttonIndex, float velocity = 1.0f) noexcept;
    void triggerChimeKey(int keyIndex, float velocity = 1.0f) noexcept;

    // Continuous friction / bow excitation
    void setFriction(float bowVelocity, float bowPressure) noexcept;

    // Autonomous trigger engines
    void setPoissonRain(bool enable, float epm, float humanize = 0.5f) noexcept;
    void setEuclidean(bool enable, int pulses, int steps, float bpm = 120.0f) noexcept;

    // External audio injection
    void setExternalInput(bool enable, float sensitivity, float directMix = 0.35f) noexcept;

    // Microtonal scale selection
    void setMicrotonalScale(MicrotonalScale scale) noexcept;
    void setChimeRootHz(float rootHz) noexcept;

    // Real-time audio processing: returns instantaneous exciter bus sample
    [[nodiscard]] float processSample(float externalAudioIn = 0.0f, float bodyVelocity = 0.0f) noexcept;

    // Telemetry and status queries
    [[nodiscard]] float getLastTriggeredHz() const noexcept { return mLastChimeHz; }
    [[nodiscard]] bool hasNewChimeTrigger(float& outHz) noexcept;
    [[nodiscard]] bool isMalletInContact() const noexcept { return mInContact; }
    [[nodiscard]] float getExciterActivity() const noexcept { return mExciterActivity; }

private:
    void updatePoissonCountdown() noexcept;
    void updateEuclideanRhythm() noexcept;
    void computeScaleRatios() noexcept;

    float mSampleRate { 48000.0f };
    FastPRNG mPrng { 0x9e3779b97f4a7c15ULL };

    // 1. Hunt-Crossley Mass-Spring Strike Collision
    float mHammerPos { 0.0f };       // Mallet position (meters)
    float mHammerVel { 0.0f };       // Mallet velocity (m/s)
    float mHammerAcc { 0.0f };       // Mallet acceleration (m/s^2)
    float mHammerForce { 0.0f };     // Contact force (Newtons)
    float mHammerMass { 0.012f };    // Mallet mass (kg)
    float mContactStiffness { 5.0e6f }; // kc (N/m^alpha)
    float mNonLinExponent { 2.2f };  // alpha (1.5 to 2.8)
    float mViscousDamping { 30.0f }; // lambda_c
    bool  mInContact { false };

    // 2. Karnopp Stick-Slip Friction (Bowed Metal / Glass)
    OnePoleSmoother mBowVelocitySmoother;
    OnePoleSmoother mBowPressureSmoother;
    float mFrictionState { 0.0f };

    // 3. Buchla 292 Optical Vactrol Dynamic Pluck
    VactrolGate mVactrolPluck;

    // 4. External Audio Input Direct Resonator Injection
    bool  mExtEnable { false };
    float mExtSensitivity { 1.0f };
    float mExtDirectMix { 0.35f };
    OnePoleSmoother mExtGainSmoother;
    float mDcStateX { 0.0f };
    float mDcStateY { 0.0f };
    float mDcR { 0.998f };
    float mExtPulseEnv { 0.0f };
    float mExtPulseDecayCoeff { 0.995f };

    // 5. Poisson Rain Stochastic Generator
    bool  mPoissonEnable { false };
    float mPoissonEpm { 18.0f };     // Events per minute
    float mPoissonHumanize { 0.50f };
    int   mPoissonCountdown { 0 };

    // 6. Euclidean Polyrhythm Ring E(k, n)
    bool  mEuclideanEnable { false };
    int   mEuclideanPulses { 4 };
    int   mEuclideanSteps { 16 };
    float mEuclideanBpm { 120.0f };
    int   mEuclideanClockSamples { 6000 };
    int   mEuclideanClockCounter { 0 };
    int   mEuclideanCurrentStep { 0 };

    // 7. Microtonal Chime Synthesizer
    MicrotonalScale mScale { MicrotonalScale::TwelveTet };
    float mChimeRootHz { 220.0f };
    float mLastChimeHz { 220.0f };
    bool  mNewChimeTriggered { false };
    std::array<float, kNumChimeKeys> mScaleRatios { 1.0f };

    // Output activity indicator
    float mExciterActivity { 0.0f };
};

} // namespace braun::mr16
