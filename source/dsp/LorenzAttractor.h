#pragma once

#include "DspMath.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace braun::mr16 {

/**
 * LorenzAttractor: 3D Continuous Chaotic Dynamical System Engine.
 *
 * Implements:
 * 1. Lorenz coupled differential equations:
 *    dx/dt = sigma * (y - x)
 *    dy/dt = x * (rho - z) - y
 *    dz/dt = x * y - beta * z
 * 2. Classical 4th-Order Runge-Kutta (RK4) integration with bounded step size.
 * 3. Anti-blowup safety guard with automatic canonical re-seeding.
 * 4. Real-time modulation generators for 16-mode frequency detune, Q spread,
 *    and spatial panning / manifold morphing.
 * 5. Telemetry interface for Deck 06 Phosphor CRT 3D Attractor projection.
 */
class LorenzAttractor {
public:
    static constexpr size_t kNumModes = 16;
    static constexpr float kSigma = 10.0f;
    static constexpr float kRho   = 28.0f;
    static constexpr float kBeta  = 8.0f / 3.0f; // 2.666667

    LorenzAttractor() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setRateHz(float rateHz) noexcept;
    void setDepth(float depth) noexcept; // 0.0 to 1.0 overall chaos depth
    void setDetuneMaxCents(float cents) noexcept; // Typically 0 to 120 cents

    // Advances the chaotic attractor state by one sample
    void step() noexcept;

    // Direct state coordinates
    [[nodiscard]] float getX() const noexcept { return mX; }
    [[nodiscard]] float getY() const noexcept { return mY; }
    [[nodiscard]] float getZ() const noexcept { return mZ; }

    // Normalized coordinates in [-1, +1] (or [0, 1] for Z)
    [[nodiscard]] float getNormalizedX() const noexcept { return mNormX; }
    [[nodiscard]] float getNormalizedY() const noexcept { return mNormY; }
    [[nodiscard]] float getNormalizedZ() const noexcept { return mNormZ; }

    // 16-mode frequency ratio multipliers based on twin-lobe flipping
    [[nodiscard]] const std::array<float, kNumModes>& getFrequencyMultipliers() const noexcept {
        return mFreqMultipliers;
    }

    // Modal Q-factor spread modulation factor
    [[nodiscard]] float getQSpreadFactor() const noexcept { return mQSpread; }

    // Spatial panning angle offset in radians
    [[nodiscard]] float getSpatialPanOffset() const noexcept { return mPanOffsetRad; }

private:
    struct StateDeriv {
        float dx { 0.0f };
        float dy { 0.0f };
        float dz { 0.0f };
    };

    [[nodiscard]] static inline StateDeriv evaluateDeriv(float x, float y, float z) noexcept {
        return {
            kSigma * (y - x),
            x * (kRho - z) - y,
            x * y - kBeta * z
        };
    }

    void updateNormalizedOutputs() noexcept;

    float mSampleRate { 48000.0f };
    float mRateHz { 1.0f };
    float mDt { 0.005f };
    float mDepth { 0.50f };
    float mMaxDetuneCents { 50.0f };

    // Attractor physical state
    float mX { 0.10f };
    float mY { 0.0f };
    float mZ { 0.0f };

    // Normalized outputs
    float mNormX { 0.0f };
    float mNormY { 0.0f };
    float mNormZ { 0.0f };

    // Modulations
    std::array<float, kNumModes> mFreqMultipliers { 1.0f };
    float mQSpread { 1.0f };
    float mPanOffsetRad { 0.0f };
};

} // namespace braun::mr16
