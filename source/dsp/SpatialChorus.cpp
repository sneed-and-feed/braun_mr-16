#include "SpatialChorus.h"

namespace braun::mr16 {

void SpatialChorus::prepare(double sampleRate) noexcept {
    mSampleRate = (sampleRate > 100.0) ? static_cast<float>(sampleRate) : 48000.0f;
    updateCompanderCoefficients();
    updateLfoIncrement();
    reset();
}

void SpatialChorus::reset() noexcept {
    mLfoPhase = 0.0f;
    mDelay1.reset();
    mDelay2.reset();
    mDelay3.reset();
    mPreEmphasis.reset();
    mDeEmphasis1.reset();
    mDeEmphasis2.reset();
    mDeEmphasis3.reset();
    mReconLp1.reset();
    mReconLp2.reset();
    mReconLp3.reset();
}

void SpatialChorus::setMode(DimensionMode mode) noexcept {
    mDimensionMode = mode;
    switch (mode) {
        case DimensionMode::Mode1:
            mRateHz = 0.40f;
            mDepthMs = 1.50f;
            mMix = 0.35f;
            break;
        case DimensionMode::Mode2:
            mRateHz = 0.55f;
            mDepthMs = 2.20f;
            mMix = 0.45f;
            break;
        case DimensionMode::Mode3:
            mRateHz = 0.75f;
            mDepthMs = 3.20f;
            mMix = 0.55f;
            break;
        case DimensionMode::Mode4:
            mRateHz = 1.10f;
            mDepthMs = 4.50f;
            mMix = 0.65f;
            break;
        case DimensionMode::Manual:
        default:
            break;
    }
    updateLfoIncrement();
}

void SpatialChorus::setParameters(float rateHz, float depthMs, float mix) noexcept {
    mDimensionMode = DimensionMode::Manual;
    mRateHz = std::clamp(std::isfinite(rateHz) ? rateHz : 0.55f, 0.05f, 5.0f);
    mDepthMs = std::clamp(std::isfinite(depthMs) ? depthMs : 2.20f, 0.1f, 5.0f);
    mMix = std::clamp(std::isfinite(mix) ? mix : 0.45f, 0.0f, 1.0f);
    updateLfoIncrement();
}

void SpatialChorus::setDimensionSpread(float spread) noexcept {
    mDimensionSpread = std::clamp(std::isfinite(spread) ? spread : 1.0f, 0.0f, 2.0f);
}

void SpatialChorus::updateLfoIncrement() noexcept {
    mLfoInc = kTwoPi * (mRateHz / mSampleRate);
}

void SpatialChorus::updateCompanderCoefficients() noexcept {
    // 1. Pre-emphasis: 1-pole high-shelf (+6 dB boost above 1.8 kHz)
    // Analog s-domain: H_pre(s) = (omega2 / omega1) * (s + omega1) / (s + omega2)
    // f1 = 1800 Hz, f2 = 3600 Hz (+6 dB asymptote)
    const float f1 = 1800.0f;
    const float f2 = 3600.0f;
    const float w1 = kTwoPi * f1;
    const float w2 = kTwoPi * f2;
    const float K = 2.0f * mSampleRate;

    const float preRatio = w2 / w1; // 2.0
    const float preDenom = K + w2;
    mPreEmphasis.b0 = preRatio * (K + w1) / preDenom;
    mPreEmphasis.b1 = preRatio * (w1 - K) / preDenom;
    mPreEmphasis.a1 = (w2 - K) / preDenom;

    // 2. De-emphasis: exact inverse of pre-emphasis
    const float deRatio = w1 / w2; // 0.5
    const float deDenom = K + w1;
    const float deB0 = deRatio * (K + w2) / deDenom;
    const float deB1 = deRatio * (w2 - K) / deDenom;
    const float deA1 = (w1 - K) / deDenom;

    mDeEmphasis1.b0 = deB0; mDeEmphasis1.b1 = deB1; mDeEmphasis1.a1 = deA1;
    mDeEmphasis2.b0 = deB0; mDeEmphasis2.b1 = deB1; mDeEmphasis2.a1 = deA1;
    mDeEmphasis3.b0 = deB0; mDeEmphasis3.b1 = deB1; mDeEmphasis3.a1 = deA1;

    // 3. 2-pole Butterworth reconstruction lowpass at 9.5 kHz
    const float fcRecon = std::clamp(9500.0f, 1000.0f, mSampleRate * 0.45f);
    const float omegaRecon = std::tan(kPi * (fcRecon / mSampleRate));
    const float omegaSq = omegaRecon * omegaRecon;
    const float sqrt2Omega = kSqrt2 * omegaRecon;
    const float lpDenom = 1.0f + sqrt2Omega + omegaSq;

    const float lpB0 = omegaSq / lpDenom;
    const float lpB1 = 2.0f * lpB0;
    const float lpB2 = lpB0;
    const float lpA1 = 2.0f * (omegaSq - 1.0f) / lpDenom;
    const float lpA2 = (1.0f - sqrt2Omega + omegaSq) / lpDenom;

    auto setupLp = [&](ReconstructionLowpass& lp) {
        lp.b0 = lpB0; lp.b1 = lpB1; lp.b2 = lpB2;
        lp.a1 = lpA1; lp.a2 = lpA2;
    };
    setupLp(mReconLp1);
    setupLp(mReconLp2);
    setupLp(mReconLp3);
}

void SpatialChorus::process(float inL, float inR, float& outL, float& outR) noexcept {
    inL = flushDenormal(inL);
    inR = flushDenormal(inR);

    if (!mEnabled || mMix <= 1.0e-4f) {
        outL = inL;
        outR = inR;
        return;
    }

    if (!std::isfinite(mLfoPhase)) [[unlikely]] {
        mLfoPhase = 0.0f;
    }

    // Advance tri-phase LFO
    mLfoPhase += mLfoInc;
    if (mLfoPhase >= kTwoPi) {
        mLfoPhase -= kTwoPi;
    }

    // 3 LFO phases: 0 deg, 120 deg, 240 deg
    constexpr float kPhase120 = kTwoPi * (1.0f / 3.0f);
    constexpr float kPhase240 = kTwoPi * (2.0f / 3.0f);

    const float lfo1 = FastSinTable::sin(mLfoPhase);
    const float lfo2 = FastSinTable::sin(mLfoPhase + kPhase120);
    const float lfo3 = FastSinTable::sin(mLfoPhase + kPhase240);

    // Compute modulated delay times in samples
    const float baseSamples = mBaseDelayMs * 0.001f * mSampleRate;
    const float depthSamples = mDepthMs * 0.001f * mSampleRate;

    const float delay1 = baseSamples + depthSamples * lfo1;
    const float delay2 = baseSamples + depthSamples * lfo2;
    const float delay3 = baseSamples + depthSamples * lfo3;

    // Sum stereo input for BBD excitation and apply compander pre-emphasis
    const float monoIn = 0.5f * (inL + inR);
    const float bbdIn = mPreEmphasis.process(monoIn);

    // Write to the 3 BBD delay lines
    mDelay1.write(bbdIn);
    mDelay2.write(bbdIn);
    mDelay3.write(bbdIn);

    // Read fractional Hermite delay taps
    float tap1 = mDelay1.readHermite(delay1);
    float tap2 = mDelay2.readHermite(delay2);
    float tap3 = mDelay3.readHermite(delay3);

    // Reconstruction filtering (clock feedthrough removal)
    tap1 = mReconLp1.process(tap1);
    tap2 = mReconLp2.process(tap2);
    tap3 = mReconLp3.process(tap3);

    // De-emphasis post-filtering
    tap1 = mDeEmphasis1.process(tap1);
    tap2 = mDeEmphasis2.process(tap2);
    tap3 = mDeEmphasis3.process(tap3);

    // Roland Dimension D Spatial Matrix (SDD-320 normalized):
    // Peak sum: (+d1 - 0.5*d2 + 0.5*d3) has max excursion of 2.0.
    // Normalized with factor 0.5f ensures wet signal maintains exact unity gain headroom.
    // Mono sum: 0.5 * (0.5*d1 + 0.5*d2 + 1.0*d3) (Strictly positive, ZERO comb cancellation!)
    const float rawWetL = 0.5f * (tap1 - 0.5f * tap2 + 0.5f * tap3);
    const float rawWetR = 0.5f * (-0.5f * tap1 + tap2 + 0.5f * tap3);
    const float mid = 0.5f * (rawWetL + rawWetR);
    const float side = 0.5f * (rawWetL - rawWetR);
    const float wetL = mid + mDimensionSpread * side;
    const float wetR = mid - mDimensionSpread * side;

    // Blend dry/wet
    const float dryGain = 1.0f - mMix;
    outL = flushDenormal(dryGain * inL + mMix * wetL);
    outR = flushDenormal(dryGain * inR + mMix * wetR);
}

void SpatialChorus::processBlock(float* bufferL, float* bufferR, int numSamples) noexcept {
    if (bufferL == nullptr || bufferR == nullptr || numSamples <= 0) {
        return;
    }
    if (!mEnabled || mMix <= 1.0e-4f) {
        return;
    }
    for (int i = 0; i < numSamples; ++i) {
        process(bufferL[i], bufferR[i], bufferL[i], bufferR[i]);
    }
}

} // namespace braun::mr16
