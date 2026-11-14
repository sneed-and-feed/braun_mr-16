#include "LorenzAttractor.h"

namespace braun::mr16 {

void LorenzAttractor::prepare(double sampleRate) noexcept {
    mSampleRate = (isFinite(sampleRate) && sampleRate > 100.0) ? sampleRate : 48000.0;
    setRateHz(mRateHz);
    reset();
}

void LorenzAttractor::reset() noexcept {
    // Canonical strange attractor initial seed
    mX = 0.10;
    mY = 0.0;
    mZ = 0.0;

    mNormX = 0.0f;
    mNormY = 0.0f;
    mNormZ = 0.0f;

    mFreqMultipliers.fill(1.0f);
    mQSpread = 1.0f;
    mPanOffsetRad = 0.0f;

    updateNormalizedOutputs();
}

void LorenzAttractor::setRateHz(float rateHz) noexcept {
    const float cleanRate = isFinite(rateHz) ? rateHz : 1.0f;
    mRateHz = std::clamp(cleanRate, 0.01f, 50.0f);
    // Scale dimensionless step size cleanly across all sample rates (44.1 kHz to 192 kHz+)
    // Double-precision integration avoids the former 0.0001f floor that froze [0.01, 1.92] Hz at 192 kHz
    mDt = std::clamp(static_cast<double>(mRateHz) * (10.0 / mSampleRate), 1.0e-9, 0.025);
}

void LorenzAttractor::setDepth(float depth) noexcept {
    const float clamped = safeClamp(depth, 0.0f, 1.0f, 0.5f);
    if (std::abs(clamped - mDepth) < 1.0e-6f) {
        return;
    }
    mDepth = clamped;
    updateNormalizedOutputs();
}

void LorenzAttractor::setDetuneMaxCents(float cents) noexcept {
    const float clamped = safeClamp(cents, 0.0f, 300.0f, 50.0f);
    if (std::abs(clamped - mMaxDetuneCents) < 1.0e-5f) {
        return;
    }
    mMaxDetuneCents = clamped;
    updateNormalizedOutputs();
}

void LorenzAttractor::setQModDepth(float qMod) noexcept {
    const float clamped = safeClamp(qMod, 0.0f, 1.0f, 0.65f);
    if (std::abs(clamped - mQModDepth) < 1.0e-6f) {
        return;
    }
    mQModDepth = clamped;
    updateNormalizedOutputs();
}

void LorenzAttractor::step() noexcept {
    const double h = mDt;

    // Classical 4th-Order Runge-Kutta (RK4) in IEEE-754 double precision
    const StateDeriv k1 = evaluateDeriv(mX, mY, mZ);

    const double x2 = mX + 0.5 * h * k1.dx;
    const double y2 = mY + 0.5 * h * k1.dy;
    const double z2 = mZ + 0.5 * h * k1.dz;
    const StateDeriv k2 = evaluateDeriv(x2, y2, z2);

    const double x3 = mX + 0.5 * h * k2.dx;
    const double y3 = mY + 0.5 * h * k2.dy;
    const double z3 = mZ + 0.5 * h * k2.dz;
    const StateDeriv k3 = evaluateDeriv(x3, y3, z3);

    const double x4 = mX + h * k3.dx;
    const double y4 = mY + h * k3.dy;
    const double z4 = mZ + h * k3.dz;
    const StateDeriv k4 = evaluateDeriv(x4, y4, z4);

    mX += (h / 6.0) * (k1.dx + 2.0 * k2.dx + 2.0 * k3.dx + k4.dx);
    mY += (h / 6.0) * (k1.dy + 2.0 * k2.dy + 2.0 * k3.dy + k4.dy);
    mZ += (h / 6.0) * (k1.dz + 2.0 * k2.dz + 2.0 * k3.dz + k4.dz);

    // Anti-blowup safety guard: check radius ||u|| > 120.0 (norm squared > 14400)
    const double r2 = mX * mX + mY * mY + mZ * mZ;
    if (!isFinite(r2) || r2 > 14400.0) [[unlikely]] {
        mX = 0.10;
        mY = 0.0;
        mZ = 0.0;
    }

    updateNormalizedOutputs();
}

void LorenzAttractor::updateNormalizedOutputs() noexcept {
    // Lorenz coordinates bounded inside [-20, 20], [-28, 28], [0, 48]
    mNormX = std::clamp(static_cast<float>(mX * (1.0 / 20.0)), -1.0f, 1.0f);
    mNormY = std::clamp(static_cast<float>(mY * (1.0 / 28.0)), -1.0f, 1.0f);
    mNormZ = std::clamp(static_cast<float>(mZ * (1.0 / 48.0)),  0.0f, 1.0f);

    // Frequency detune across 16 modes: odd modes pitch up, even modes pitch down
    const float baseDetuneCents = mNormX * mDepth * mMaxDetuneCents;
    if (std::abs(baseDetuneCents) <= 1.0e-5f) {
        mFreqMultipliers.fill(1.0f);
    } else {
        for (size_t i = 0; i < kNumModes; ++i) {
            const float sign = (i % 2 == 1) ? 1.0f : -1.0f;
            // Mode-dependent dispersion coefficient
            const float modeSpread = 1.0f + 0.03f * static_cast<float>(i);
            const float cents = sign * baseDetuneCents * modeSpread;
            // 2^(cents / 1200)
            mFreqMultipliers[i] = std::pow(2.0f, cents * (1.0f / 1200.0f));
        }
    }

    // Modal Q-factor spread (skews damping between low and high modes)
    mQSpread = 1.0f + mNormY * mDepth * mQModDepth;

    // Spatial panning wobble offset in radians
    mPanOffsetRad = mNormX * mDepth * 0.35f;
}

} // namespace braun::mr16
