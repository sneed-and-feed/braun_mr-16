#pragma once

#include "DspMath.h"
#include <cmath>
#include <algorithm>
#include <cstddef>

namespace braun::mr16 {

/**
 * BoundedSaturator: Warm analog soft-knee saturator and True Brickwall limiter.
 *
 * Adheres to Dieter Rams' "Weniger, aber besser" philosophy and C++20 real-time standards:
 * 1. Warm analog soft-saturation curve (Germanium diode / analog tape style) with C1 continuous
 *    transition below the clipping threshold.
 * 2. Strict mathematical True Brickwall ceiling clamp at exactly 1.00 (0.0 dBFS) or configured ceiling
 *    with C1 smooth transition, guaranteeing zero signal overshoots (|y| <= ceiling) even under
 *    extreme feedback, high-Q resonance, or input overdrive.
 * 3. Exact linear small-signal transparency (|x| <= knee, unity gain slope = 1.0).
 * 4. Zero derivative at |x| = ceiling, preventing harsh clipping corners and aliasing hash.
 * 5. Branchless IEEE 754 NaN / denormal protection, zero heap allocations.
 */
class BoundedSaturator {
public:
    static constexpr float kDefaultKnee = 0.72f;
    static constexpr float kDefaultCeiling = 1.00f; // Exactly 0.0 dBFS True Brickwall

    constexpr BoundedSaturator() noexcept
        : mKnee(kDefaultKnee), mCeiling(kDefaultCeiling),
          mDelta(kDefaultCeiling - kDefaultKnee),
          mInvDelta(1.0f / (kDefaultCeiling - kDefaultKnee)) {}

    constexpr BoundedSaturator(float knee, float ceiling) noexcept
        : mKnee(knee), mCeiling(ceiling),
          mDelta(ceiling - knee),
          mInvDelta((ceiling > knee) ? (1.0f / (ceiling - knee)) : 1.0f) {}

    void setKneeAndCeiling(float knee, float ceiling) noexcept {
        const float cleanKnee = std::isfinite(knee) ? knee : kDefaultKnee;
        const float cleanCeiling = std::isfinite(ceiling) ? ceiling : kDefaultCeiling;
        mKnee = std::clamp(cleanKnee, 0.10f, 0.99f);
        mCeiling = std::max(mKnee + 0.01f, cleanCeiling);
        mDelta = mCeiling - mKnee;
        mInvDelta = (mDelta > 1.0e-5f) ? (1.0f / mDelta) : 1.0f;
    }

    [[nodiscard]] float getKnee() const noexcept { return mKnee; }
    [[nodiscard]] float getCeiling() const noexcept { return mCeiling; }

    /**
     * Process a single audio sample (inlined for hard real-time performance).
     */
    [[nodiscard]] inline float processSample(float x) const noexcept {
        if (!std::isfinite(x)) [[unlikely]] {
            return 0.0f;
        }

        const float absX = std::abs(x);
        if (absX < 1.0e-15f) [[unlikely]] {
            return 0.0f;
        }

        // Region 1: Linear unity-gain transparency below knee (|x| <= mKnee)
        if (absX <= mKnee) [[likely]] {
            return x;
        }

        const float sign = (x > 0.0f) ? 1.0f : -1.0f;

        // Region 3: Clamped saturation ceiling (Strict True Brickwall clamp at configured ceiling)
        if (absX >= mCeiling) [[unlikely]] {
            return sign * mCeiling;
        }

        // Region 2: C1 Hermite warm analog soft-saturation curve (Germanium diode / tape style)
        // Satisfies C1 continuity:
        //   poly(0) = 0, poly'(0) = 1 (smooth unity-gain derivative match at knee threshold)
        //   poly(1) = 1, poly'(1) = 0 (smooth horizontal derivative match at ceiling)
        const float u = (absX - mKnee) * mInvDelta;
        // Horner evaluation of u + u^2 - u^3 = u * (1.0 + u * (1.0 - u))
        const float poly = u * (1.0f + u * (1.0f - u));
        const float y = sign * (mKnee + mDelta * poly);
        // Guaranteed zero overshoot: strictly clamp output to [-mCeiling, +mCeiling]
        return flushDenormal(std::clamp(y, -mCeiling, mCeiling));
    }

    /**
     * Block processing for contiguous channels.
     */
    void processBlock(const float* input, float* output, int numSamples) const noexcept;

    /**
     * In-place block processing.
     */
    void processBlock(float* buffer, int numSamples) const noexcept;

private:
    float mKnee { kDefaultKnee };
    float mCeiling { kDefaultCeiling };
    float mDelta { kDefaultCeiling - kDefaultKnee };
    float mInvDelta { 1.0f / (kDefaultCeiling - kDefaultKnee) };
};

} // namespace braun::mr16
