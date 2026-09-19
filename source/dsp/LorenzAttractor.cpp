#include "LorenzAttractor.h"

namespace braun::mr16 {

void LorenzAttractor::prepare(double sampleRate) noexcept {
    mSampleRate = (sampleRate > 100.0) ? static_cast<float>(sampleRate) : 48000.0f;
    setRateHz(mRateHz);
    reset();
}

void LorenzAttractor::reset() noexcept {
    // Canonical strange attractor initial seed
    mX = 0.10f;
    mY = 0.0f;
    mZ = 0.0f;

    mNormX = 0.0f;
    mNormY = 0.0f;
    mNormZ = 0.0f;

    mFreqMultipliers.fill(1.0f);
    mQSpread = 1.0f;
    mPanOffsetRad = 0.0f;

    updateNormalizedOutputs();
}

void LorenzAttractor::setRateHz(float rateHz) noexcept {
    const float cleanRate = std::isfinite(rateHz) ? rateHz : 1.0f;
    mRateHz = std::clamp(cleanRate, 0.01f, 50.0f);
    // Scale dimensionless step size to audio sample rate
    // Nominal attractor speed corresponds to roughly 10 dimensionless time units/second
    mDt = std::clamp(mRateHz * (10.0f / mSampleRate), 0.0001f, 0.025f);
}

void LorenzAttractor::setDepth(float depth) noexcept {
    const float cleanDepth = std::isfinite(depth) ? depth : 0.5f;
    mDepth = std::clamp(cleanDepth, 0.0f, 1.0f);
    updateNormalizedOutputs();
}

void LorenzAttractor::setDetuneMaxCents(float cents) noexcept {
    const float cleanCents = std::isfinite(cents) ? cents : 50.0f;
    mMaxDetuneCents = std::clamp(cleanCents, 0.0f, 200.0f);
    updateNormalizedOutputs();
}

void LorenzAttractor::step() noexcept {
    const float h = mDt;

    // Classical 4th-Order Runge-Kutta (RK4)
    const StateDeriv k1 = evaluateDeriv(mX, mY, mZ);

    const float x2 = mX + 0.5f * h * k1.dx;
    const float y2 = mY + 0.5f * h * k1.dy;
    const float z2 = mZ + 0.5f * h * k1.dz;
    const StateDeriv k2 = evaluateDeriv(x2, y2, z2);

    const float x3 = mX + 0.5f * h * k2.dx;
    const float y3 = mY + 0.5f * h * k2.dy;
    const float z3 = mZ + 0.5f * h * k2.dz;
    const StateDeriv k3 = evaluateDeriv(x3, y3, z3);

    const float x4 = mX + h * k3.dx;
    const float y4 = mY + h * k3.dy;
    const float z4 = mZ + h * k3.dz;
    const StateDeriv k4 = evaluateDeriv(x4, y4, z4);

    mX += (h / 6.0f) * (k1.dx + 2.0f * k2.dx + 2.0f * k3.dx + k4.dx);
    mY += (h / 6.0f) * (k1.dy + 2.0f * k2.dy + 2.0f * k3.dy + k4.dy);
    mZ += (h / 6.0f) * (k1.dz + 2.0f * k2.dz + 2.0f * k3.dz + k4.dz);

    // Anti-blowup safety guard: check radius ||u|| > 120.0 (norm squared > 14400)
    const float r2 = mX * mX + mY * mY + mZ * mZ;
    if (!std::isfinite(r2) || r2 > 14400.0f) [[unlikely]] {
        mX = 0.10f;
        mY = 0.0f;
        mZ = 0.0f;
    }

    updateNormalizedOutputs();
}

void LorenzAttractor::updateNormalizedOutputs() noexcept {
    // Lorenz coordinates bounded inside [-20, 20], [-28, 28], [0, 48]
    mNormX = std::clamp(mX * (1.0f / 20.0f), -1.0f, 1.0f);
    mNormY = std::clamp(mY * (1.0f / 28.0f), -1.0f, 1.0f);
    mNormZ = std::clamp(mZ * (1.0f / 48.0f),  0.0f, 1.0f);

    // Frequency detune across 16 modes: odd modes pitch up, even modes pitch down
    const float baseDetuneCents = mNormX * mDepth * mMaxDetuneCents;
    for (size_t i = 0; i < kNumModes; ++i) {
        const float sign = (i % 2 == 1) ? 1.0f : -1.0f;
        // Mode-dependent dispersion coefficient
        const float modeSpread = 1.0f + 0.03f * static_cast<float>(i);
        const float cents = sign * baseDetuneCents * modeSpread;
        // 2^(cents / 1200)
        mFreqMultipliers[i] = std::pow(2.0f, cents * (1.0f / 1200.0f));
    }

    // Modal Q-factor spread (skews damping between low and high modes)
    mQSpread = 1.0f + mNormY * mDepth * 0.65f;

    // Spatial panning wobble offset in radians
    mPanOffsetRad = mNormX * mDepth * 0.35f;
}

} // namespace braun::mr16
