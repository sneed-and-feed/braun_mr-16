#include "VactrolGate.h"

namespace braun::mr16 {

void VactrolGate::prepare(double sampleRate) noexcept {
    mSampleRate = (sampleRate > 100.0) ? static_cast<float>(sampleRate) : 48000.0f;
    reset();
}

void VactrolGate::reset() noexcept {
    mConductance = 0.0f;
    mEnvFollower = 0.0f;
    mCurrentCutoff = mMinCutoff;
    mStateL1 = 0.0f;
    mStateL2 = 0.0f;
    mStateR1 = 0.0f;
    mStateR2 = 0.0f;
}

void VactrolGate::setControlVoltage(float cv) noexcept {
    mTargetCv = std::clamp(cv, 0.0f, 1.0f);
}

void VactrolGate::setDynamicSag(float sagAmount) noexcept {
    mDynamicSag = std::clamp(sagAmount, 0.0f, 1.0f);
}

void VactrolGate::setCutoffRange(float minHz, float maxHz) noexcept {
    mMinCutoff = std::clamp(minHz, 20.0f, 1000.0f);
    mMaxCutoff = std::clamp(maxHz, mMinCutoff + 100.0f, mSampleRate * 0.495f);
}

void VactrolGate::setDecayTime(float decaySec) noexcept {
    mTauDecay = std::clamp(decaySec, 0.010f, 2.500f);
}

void VactrolGate::trigger(float velocity) noexcept {
    const float vel = std::clamp(velocity, 0.0f, 1.0f);
    // Sudden excitation jump in LED control voltage
    mConductance = std::min(1.0f, mConductance + vel);
}

void VactrolGate::updateConductance(float targetCv) noexcept {
    const float dt = 1.0f / mSampleRate;
    const float u = std::clamp(targetCv, 0.0f, 1.0f);

    float tau;
    if (u >= mConductance) {
        // Fast LED excitation phase
        tau = mTauAttack;
    } else {
        // Slow CdS semiconductor recombination phase with non-linear sag expansion
        const float sagCoeff = 1.0f + mSagKappa * (1.0f - mConductance) * (1.0f - mConductance);
        tau = mTauDecay * sagCoeff;
    }

    const float alpha = 1.0f - std::exp(-dt / std::max(1.0e-5f, tau));
    mConductance += alpha * (u - mConductance);
    mConductance = flushDenormal(std::clamp(mConductance, 0.0f, 1.0f));

    // Non-linear perceptual mapping to cutoff frequency
    const float gNonLin = std::pow(mConductance, 1.8f);
    mCurrentCutoff = mMinCutoff * std::pow(mMaxCutoff / mMinCutoff, gNonLin);
    mCurrentCutoff = std::clamp(mCurrentCutoff, 20.0f, mSampleRate * 0.495f);
}

float VactrolGate::processSample(float input) noexcept {
    if (!std::isfinite(input)) [[unlikely]] {
        return 0.0f;
    }

    // Dynamic envelope follower for self-sagging
    const float absIn = std::abs(input);
    const float envAlpha = (absIn > mEnvFollower) ? 0.02f : 0.0005f;
    mEnvFollower += envAlpha * (absIn - mEnvFollower);
    mEnvFollower = flushDenormal(mEnvFollower);

    // Compute effective control voltage
    const float effectiveCv = std::clamp(mTargetCv + mDynamicSag * mEnvFollower, 0.0f, 1.0f);
    updateConductance(effectiveCv);

    // VCA passband gain
    const float vcaGain = mConductance * mConductance;

    if (mMode == VactrolMode::VcaOnly) {
        return flushDenormal(input * vcaGain);
    }

    // TPT 2-pole lowpass filter
    const float gFilter = std::tan(kPi * (mCurrentCutoff / mSampleRate));
    const float h = gFilter / (1.0f + gFilter);

    // Pole 1
    const float v1 = (input - mStateL1) * h;
    const float y1 = v1 + mStateL1;
    mStateL1 = flushDenormal(2.0f * v1 + mStateL1);

    // Pole 2
    const float v2 = (y1 - mStateL2) * h;
    const float y2 = v2 + mStateL2;
    mStateL2 = flushDenormal(2.0f * v2 + mStateL2);

    if (mMode == VactrolMode::FilterOnly) {
        return flushDenormal(y2);
    }

    // Combo mode: filtered signal scaled by VCA gain
    return flushDenormal(y2 * vcaGain);
}

void VactrolGate::processStereo(float inL, float inR, float& outL, float& outR) noexcept {
    if (!std::isfinite(inL)) inL = 0.0f;
    if (!std::isfinite(inR)) inR = 0.0f;

    // Combined input magnitude for envelope tracking
    const float maxMag = std::max(std::abs(inL), std::abs(inR));
    const float envAlpha = (maxMag > mEnvFollower) ? 0.02f : 0.0005f;
    mEnvFollower += envAlpha * (maxMag - mEnvFollower);
    mEnvFollower = flushDenormal(mEnvFollower);

    const float effectiveCv = std::clamp(mTargetCv + mDynamicSag * mEnvFollower, 0.0f, 1.0f);
    updateConductance(effectiveCv);

    const float vcaGain = mConductance * mConductance;

    if (mMode == VactrolMode::VcaOnly) {
        outL = flushDenormal(inL * vcaGain);
        outR = flushDenormal(inR * vcaGain);
        return;
    }

    // TPT 2-pole lowpass filter
    const float gFilter = std::tan(kPi * (mCurrentCutoff / mSampleRate));
    const float h = gFilter / (1.0f + gFilter);

    // Left channel
    const float vL1 = (inL - mStateL1) * h;
    const float yL1 = vL1 + mStateL1;
    mStateL1 = flushDenormal(2.0f * vL1 + mStateL1);

    const float vL2 = (yL1 - mStateL2) * h;
    const float yL2 = vL2 + mStateL2;
    mStateL2 = flushDenormal(2.0f * vL2 + mStateL2);

    // Right channel
    const float vR1 = (inR - mStateR1) * h;
    const float yR1 = vR1 + mStateR1;
    mStateR1 = flushDenormal(2.0f * vR1 + mStateR1);

    const float vR2 = (yR1 - mStateR2) * h;
    const float yR2 = vR2 + mStateR2;
    mStateR2 = flushDenormal(2.0f * vR2 + mStateR2);

    if (mMode == VactrolMode::FilterOnly) {
        outL = flushDenormal(yL2);
        outR = flushDenormal(yR2);
        return;
    }

    // Combo mode
    outL = flushDenormal(yL2 * vcaGain);
    outR = flushDenormal(yR2 * vcaGain);
}

void VactrolGate::processBlock(float* bufferL, float* bufferR, int numSamples) noexcept {
    if (bufferL == nullptr || bufferR == nullptr || numSamples <= 0) {
        return;
    }
    for (int i = 0; i < numSamples; ++i) {
        processStereo(bufferL[i], bufferR[i], bufferL[i], bufferR[i]);
    }
}

} // namespace braun::mr16
