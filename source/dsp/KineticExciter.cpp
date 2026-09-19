#include "KineticExciter.h"

namespace braun::mr16 {

KineticExciter::KineticExciter() noexcept {
    computeScaleRatios();
}

void KineticExciter::prepare(double sampleRate) noexcept {
    mSampleRate = (sampleRate > 100.0) ? static_cast<float>(sampleRate) : 48000.0f;
    mVactrolPluck.prepare(sampleRate);
    mVactrolPluck.setCutoffRange(50.0f, 16000.0f);
    mVactrolPluck.setDecayTime(0.065f);

    // Friction velocity and pressure slewing (5 ms de-zippering, click-free)
    mBowVelocitySmoother.setSampleRate(mSampleRate);
    mBowVelocitySmoother.setTimeConstant(0.005f);
    mBowPressureSmoother.setSampleRate(mSampleRate);
    mBowPressureSmoother.setTimeConstant(0.005f);

    // External audio input gain slewing (10 ms crossfade, click-free)
    mExtGainSmoother.setSampleRate(mSampleRate);
    mExtGainSmoother.setTimeConstant(0.010f);

    // Sample-rate invariant DC blocking (15 Hz) filter coefficient
    mDcR = 1.0f - (kTwoPi * 15.0f / mSampleRate);
    mExtPulseDecayCoeff = std::exp(-1.0f / (mSampleRate * 0.025f));

    reset();
}

void KineticExciter::reset() noexcept {
    mHammerPos = 0.0f;
    mHammerVel = 0.0f;
    mHammerAcc = 0.0f;
    mHammerForce = 0.0f;
    mInContact = false;

    mBowVelocitySmoother.reset(0.0f);
    mBowPressureSmoother.reset(0.0f);
    mFrictionState = 0.0f;

    mVactrolPluck.reset();

    mExtGainSmoother.reset(mExtEnable ? 1.0f : 0.0f);
    mDcStateX = 0.0f;
    mDcStateY = 0.0f;
    mExtPulseEnv = 0.0f;

    mPoissonCountdown = 1000;
    mEuclideanClockCounter = 1000;
    mEuclideanCurrentStep = 0;

    computeScaleRatios();
    updatePoissonCountdown();
    updateEuclideanRhythm();

    // Verify exciter activity and all exciter outputs start completely silent on reset()
    mNewChimeTriggered = false;
    mExciterActivity = 0.0f;
}

void KineticExciter::triggerStrike(float velocity, float hardness) noexcept {
    const float cleanVel = std::isfinite(velocity) ? velocity : 0.7f;
    const float cleanHard = std::isfinite(hardness) ? hardness : 0.65f;
    const float vel = std::clamp(cleanVel, 0.01f, 1.0f);
    const float hard = std::clamp(cleanHard, 0.0f, 1.0f);

    // Hardness modulates Hertzian contact stiffness and non-linear exponent
    // Soft mallet: alpha = 2.6, lower stiffness, prolonged contact duration
    // Hard steel mallet: alpha = 1.6, high stiffness, sharp impulse
    mContactStiffness = 1.0e6f * std::pow(10.0f, hard * 2.8f);
    mNonLinExponent = 2.6f - hard * 1.0f;
    mViscousDamping = 40.0f * (1.0f - hard * 0.4f);

    // Initial strike velocity and penetration entry
    mHammerVel = vel * 4.5f; // Initial collision velocity in m/s
    mHammerPos = 0.0001f;    // Initial penetration contact point
    mHammerAcc = 0.0f;
    mHammerForce = 0.0f;
    mInContact = true;

    // Trigger vactrol optical excitation LED
    mVactrolPluck.trigger(vel);
}

void KineticExciter::triggerStrikeButton(int buttonIndex, float velocity) noexcept {
    const int idx = std::clamp(buttonIndex, 0, static_cast<int>(kNumStrikeButtons - 1));
    // 4 tactile strike buttons mapped to fundamental, minor third, fifth, octave
    constexpr std::array<int, kNumStrikeButtons> kButtonKeyMap = { 0, 3, 7, 12 };
    triggerChimeKey(kButtonKeyMap[static_cast<size_t>(idx)], velocity);
}

void KineticExciter::triggerChimeKey(int keyIndex, float velocity) noexcept {
    const int idx = std::clamp(keyIndex, 0, static_cast<int>(kNumChimeKeys - 1));
    const float noteHz = mChimeRootHz * mScaleRatios[static_cast<size_t>(idx)];

    mLastChimeHz = noteHz;
    mNewChimeTriggered = true;

    // Key velocity modulates strike hardness slightly
    const float hardness = 0.50f + 0.35f * velocity;
    triggerStrike(velocity, hardness);
}

bool KineticExciter::hasNewChimeTrigger(float& outHz) noexcept {
    if (mNewChimeTriggered) {
        outHz = mLastChimeHz;
        mNewChimeTriggered = false;
        return true;
    }
    return false;
}

void KineticExciter::setFriction(float bowVelocity, float bowPressure) noexcept {
    mBowVelocitySmoother.setTarget(std::clamp(bowVelocity, 0.0f, 2.0f));
    mBowPressureSmoother.setTarget(std::clamp(bowPressure, 0.0f, 2.0f));
}

void KineticExciter::setPoissonRain(bool enable, float epm, float humanize) noexcept {
    mPoissonEnable = enable;
    mPoissonEpm = std::clamp(epm, 1.0f, 1200.0f);
    mPoissonHumanize = std::clamp(humanize, 0.0f, 1.0f);
    updatePoissonCountdown();
}

void KineticExciter::setEuclidean(bool enable, int pulses, int steps, float bpm) noexcept {
    mEuclideanEnable = enable;
    mEuclideanSteps = std::clamp(steps, 2, 32);
    mEuclideanPulses = std::clamp(pulses, 0, mEuclideanSteps);
    mEuclideanBpm = std::clamp(bpm, 20.0f, 300.0f);
    updateEuclideanRhythm();
}

void KineticExciter::setExternalInput(bool enable, float sensitivity, float directMix) noexcept {
    mExtEnable = enable;
    mExtSensitivity = std::clamp(sensitivity, 0.0f, 5.0f);
    mExtDirectMix = std::clamp(directMix, 0.0f, 1.0f);
    mExtGainSmoother.setTarget(enable ? 1.0f : 0.0f);
}

void KineticExciter::setMicrotonalScale(MicrotonalScale scale) noexcept {
    mScale = scale;
    computeScaleRatios();
}

void KineticExciter::setChimeRootHz(float rootHz) noexcept {
    mChimeRootHz = std::clamp(rootHz, 20.0f, 2000.0f);
}

void KineticExciter::computeScaleRatios() noexcept {
    switch (mScale) {
        case MicrotonalScale::TwelveTet:
            for (size_t i = 0; i < kNumChimeKeys; ++i) {
                mScaleRatios[i] = std::pow(2.0f, static_cast<float>(i) * (1.0f / 12.0f));
            }
            break;

        case MicrotonalScale::JustIntonation: {
            constexpr std::array<float, kNumChimeKeys> kJiRatios = {
                1.0f,           16.0f / 15.0f,  9.0f / 8.0f,   6.0f / 5.0f,
                5.0f / 4.0f,    4.0f / 3.0f,    45.0f / 32.0f, 3.0f / 2.0f,
                8.0f / 5.0f,    5.0f / 3.0f,    9.0f / 5.0f,   15.0f / 8.0f,
                2.0f,           32.0f / 15.0f,  9.0f / 4.0f,   12.0f / 5.0f
            };
            mScaleRatios = kJiRatios;
            break;
        }

        case MicrotonalScale::Pythagorean: {
            constexpr std::array<float, kNumChimeKeys> kPythRatios = {
                1.0f,            256.0f / 243.0f, 9.0f / 8.0f,    32.0f / 27.0f,
                81.0f / 64.0f,   4.0f / 3.0f,     729.0f / 512.0f, 3.0f / 2.0f,
                128.0f / 81.0f,  27.0f / 16.0f,   16.0f / 9.0f,   243.0f / 128.0f,
                2.0f,            512.0f / 243.0f, 9.0f / 4.0f,    64.0f / 27.0f
            };
            mScaleRatios = kPythRatios;
            break;
        }

        case MicrotonalScale::Slendro:
            for (size_t i = 0; i < kNumChimeKeys; ++i) {
                mScaleRatios[i] = std::pow(2.0f, static_cast<float>(i) * (1.0f / 5.0f));
            }
            break;

        case MicrotonalScale::Pelog:
            for (size_t i = 0; i < kNumChimeKeys; ++i) {
                mScaleRatios[i] = std::pow(2.0f, static_cast<float>(i) * (1.0f / 7.0f));
            }
            break;

        case MicrotonalScale::WendyCarlosAlpha:
            for (size_t i = 0; i < kNumChimeKeys; ++i) {
                // 77.965 cents per step (15.385 steps per octave)
                mScaleRatios[i] = std::pow(2.0f, (static_cast<float>(i) * 77.965f) * (1.0f / 1200.0f));
            }
            break;

        case MicrotonalScale::BohlenPierce:
            for (size_t i = 0; i < kNumChimeKeys; ++i) {
                // 13 steps per tritave (3:1)
                mScaleRatios[i] = std::pow(3.0f, static_cast<float>(i) * (1.0f / 13.0f));
            }
            break;

        case MicrotonalScale::HarmonicSeries:
            for (size_t i = 0; i < kNumChimeKeys; ++i) {
                mScaleRatios[i] = static_cast<float>(i + 1);
            }
            break;
    }
}

void KineticExciter::updatePoissonCountdown() noexcept {
    const float lambda = mPoissonEpm * (1.0f / 60.0f); // Events per second
    const float u = std::clamp(mPrng.nextFloat(), 1.0e-5f, 0.99999f);
    // Exponential inter-arrival: Delta t = -ln(1 - U) / lambda
    const float dt = -std::log(1.0f - u) / std::max(0.01f, lambda);
    mPoissonCountdown = std::max(1, static_cast<int>(dt * mSampleRate));
}

void KineticExciter::updateEuclideanRhythm() noexcept {
    // 16th note clock at specified BPM: 4 steps per beat
    const float stepsPerSec = (mEuclideanBpm * (1.0f / 60.0f)) * 4.0f;
    mEuclideanClockSamples = std::max(10, static_cast<int>(mSampleRate / std::max(0.1f, stepsPerSec)));
}

float KineticExciter::processSample(float externalAudioIn, float bodyVelocity) noexcept {
    float exciterSum = 0.0f;

    // ------------------------------------------------------------------------
    // 1. Autonomous Trigger Engines (Poisson Rain & Euclidean)
    // ------------------------------------------------------------------------
    if (mPoissonEnable) {
        if (--mPoissonCountdown <= 0) {
            updatePoissonCountdown();
            // Irwin-Hall 3-variable bell-curve droplet mass
            const float dropletMass = mPrng.nextIrwinHall();
            const float jitter = 1.0f + (mPrng.nextFloat() - 0.5f) * mPoissonHumanize;
            const float strikeVel = std::clamp(dropletMass * jitter, 0.05f, 1.0f);
            const float strikeHard = 0.55f + 0.30f * mPrng.nextFloat();
            if (mExtEnable) {
                // In external audio mode: Poisson acts as a stochastic dynamic particle gate / granular chopper
                mExtPulseEnv = 1.0f;
            } else {
                triggerStrike(strikeVel, strikeHard);
            }
        }
    }

    if (mEuclideanEnable) {
        if (--mEuclideanClockCounter <= 0) {
            mEuclideanClockCounter = mEuclideanClockSamples;
            const int steps = (mEuclideanSteps > 0) ? mEuclideanSteps : 16;
            const int step = mEuclideanCurrentStep;
            mEuclideanCurrentStep = (mEuclideanCurrentStep + 1) % steps;

            // Bjorklund pulse condition: (step * k) % n < k
            if ((step * mEuclideanPulses) % steps < mEuclideanPulses) {
                if (mExtEnable) {
                    mExtPulseEnv = 1.0f;
                } else {
                    const int key = step % static_cast<int>(kNumChimeKeys);
                    triggerChimeKey(key, 0.75f);
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // 2. Hunt-Crossley Non-Linear Contact Collision (Symplectic Verlet)
    // ------------------------------------------------------------------------
    float hammerOut = 0.0f;
    if (mInContact) {
        const float dt = 1.0f / mSampleRate;

        // Position predictor: x_{n+1} = x_n + v_n dt + 0.5 a_n dt^2
        const float xNext = mHammerPos + mHammerVel * dt + 0.5f * mHammerAcc * dt * dt;

        if (xNext > 0.0f) {
            const float xDot = (xNext - mHammerPos) / dt;
            const float xPow = std::pow(xNext, mNonLinExponent);

            // Viscoelastic contact force: F = max(0, kc * x^alpha + lambda * x^alpha * x_dot)
            const float fNext = std::max(0.0f, mContactStiffness * xPow + mViscousDamping * xPow * xDot);
            const float aNext = -fNext / mHammerMass;

            // Velocity update: v_{n+1} = v_n + 0.5 (a_n + a_{n+1}) dt
            mHammerVel += 0.5f * (mHammerAcc + aNext) * dt;
            mHammerPos = xNext;
            mHammerAcc = aNext;
            mHammerForce = fNext;

            // Normalized force output scaled for audio bus injection
            hammerOut = mHammerForce * 0.0015f;
        } else {
            // Mallet bounced off resonant surface
            mHammerPos = 0.0f;
            mHammerVel = 0.0f;
            mHammerAcc = 0.0f;
            mHammerForce = 0.0f;
            mInContact = false;
        }
    }

    // ------------------------------------------------------------------------
    // 3. Buchla 292 Optical Vactrol Dynamic Pluck Shaping
    // ------------------------------------------------------------------------
    const float vactrolOut = mVactrolPluck.processSample(hammerOut);
    exciterSum += vactrolOut;

    // ------------------------------------------------------------------------
    // 4. Karnopp Stick-Slip Friction Dynamics (Bowed Metal / Glass)
    // ------------------------------------------------------------------------
    const float currentBowVelocity = mBowVelocitySmoother.next();
    const float currentBowPressure = mBowPressureSmoother.next();

    float frictionOut = 0.0f;
    if (currentBowPressure > 1.0e-5f && std::abs(currentBowVelocity) > 1.0e-5f) {
        const float vRel = currentBowVelocity - bodyVelocity;
        constexpr float kDeadband = 0.0015f;
        const float fn = currentBowPressure;
        const float fs = 0.85f * fn;
        const float fc = 0.35f * fn;
        constexpr float vStribeck = 0.06f;

        float frictionForce = 0.0f;
        if (std::abs(vRel) < kDeadband) {
            // Stick phase: locked contact balancing external shear force
            frictionForce = std::clamp(vRel * 400.0f, -fs, +fs);
        } else {
            // Slip phase: Stribeck velocity curve with normal-force-scaled viscous damping
            const float sgn = (vRel > 0.0f) ? 1.0f : -1.0f;
            const float decay = std::exp(-(vRel * vRel) / (vStribeck * vStribeck));
            frictionForce = sgn * (fc + (fs - fc) * decay) + 0.12f * fn * vRel;
        }

        frictionOut = frictionForce * 0.35f;
    }
    exciterSum += frictionOut;

    // ------------------------------------------------------------------------
    // 5. External Audio Direct Resonator Injection
    // ------------------------------------------------------------------------
    // Continuous 15 Hz DC blocking filter maintains primed state to prevent step jumps
    const float cleanIn = flushDenormal(externalAudioIn);
    const float dcY = cleanIn - mDcStateX + mDcR * mDcStateY;
    mDcStateX = cleanIn;
    mDcStateY = flushDenormal(dcY);

    mExtPulseEnv = flushDenormal(mExtPulseEnv * mExtPulseDecayCoeff);
    const float currentExtGain = mExtGainSmoother.next();
    if (currentExtGain > 1.0e-5f) {
        const float extScale = mExtSensitivity * currentExtGain;
        const float continuousExt = dcY * (extScale * mExtDirectMix);
        const float pulseExt = dcY * (mExtPulseEnv * extScale * 0.20f);
        exciterSum += continuousExt + pulseExt;
    }

    // Soft limiter / ceiling on exciter bus output to prevent extreme velocity strikes
    // from injecting numerical overload into the modal matrix
    constexpr float kExciterLinearCeiling = 0.75f;
    const float absSum = std::abs(exciterSum);
    if (absSum > kExciterLinearCeiling) {
        const float sign = (exciterSum > 0.0f) ? 1.0f : -1.0f;
        const float excess = absSum - kExciterLinearCeiling;
        exciterSum = sign * (kExciterLinearCeiling + 0.25f * (excess / (1.0f + excess)));
    }

    exciterSum = flushDenormal(exciterSum);
    mExciterActivity = flushDenormal(0.992f * mExciterActivity + 0.008f * std::abs(exciterSum));
    return exciterSum;
}

} // namespace braun::mr16
