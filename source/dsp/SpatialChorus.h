#pragma once

#include "DspMath.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace braun::mr16 {

enum class DimensionMode : int {
    Manual = 0,
    Mode1 = 1, // Subtle spatial expansion (rate 0.40 Hz, depth 1.5 ms)
    Mode2 = 2, // Gentle acoustic warmth (rate 0.55 Hz, depth 2.2 ms)
    Mode3 = 3, // Deep physical ensemble (rate 0.75 Hz, depth 3.2 ms)
    Mode4 = 4  // All-buttons dynamic swirl (rate 1.10 Hz, depth 4.5 ms)
};

/**
 * SpatialChorus: Tri-Phase Analog BBD Dimension Spatial Chorus.
 *
 * Implements:
 * 1. 3-Phase BBD delay modulation (0 deg, 120 deg, 240 deg).
 * 2. 4-point, 3rd-order Hermite fractional delay interpolation.
 * 3. NE570 analog compander emulation:
 *    - Pre-emphasis high-shelf filter (1.8 kHz +6 dB boost)
 *    - 9.5 kHz Butterworth anti-aliasing reconstruction lowpass
 *    - De-emphasis filter (-6 dB attenuation above 1.8 kHz)
 * 4. Roland Dimension D spatial matrixing:
 *    - Strictly positive mono sum coefficients for ZERO comb-filtering cancellation.
 * 5. Authentic SDD-320 Dimension button presets (1..4) and continuous manual mode.
 */
class SpatialChorus {
public:
    static constexpr size_t kDelayCapacity = 4096;

    SpatialChorus() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setMode(DimensionMode mode) noexcept;
    void setParameters(float rateHz, float depthMs, float mix) noexcept;
    void setEnabled(bool enabled) noexcept { mEnabled = enabled; }

    [[nodiscard]] bool isEnabled() const noexcept { return mEnabled; }
    [[nodiscard]] DimensionMode getMode() const noexcept { return mDimensionMode; }
    [[nodiscard]] float getRateHz() const noexcept { return mRateHz; }
    [[nodiscard]] float getDepthMs() const noexcept { return mDepthMs; }
    [[nodiscard]] float getMix() const noexcept { return mMix; }

    // Process a single stereo sample pair
    void process(float inL, float inR, float& outL, float& outR) noexcept;

    // Process stereo buffer in-place
    void processBlock(float* bufferL, float* bufferR, int numSamples) noexcept;

private:
    void updateCompanderCoefficients() noexcept;
    void updateLfoIncrement() noexcept;

    // 1-pole high-shelf pre-emphasis filter
    struct CompanderShelf {
        float b0 { 1.0f };
        float b1 { 0.0f };
        float a1 { 0.0f };
        float state { 0.0f };

        void reset() noexcept { state = 0.0f; }

        [[nodiscard]] inline float process(float x) noexcept {
            const float y = b0 * x + state;
            state = flushDenormal(b1 * x - a1 * y);
            return flushDenormal(y);
        }
    };

    // 2-pole Butterworth lowpass filter for reconstruction
    struct ReconstructionLowpass {
        float b0 { 1.0f };
        float b1 { 0.0f };
        float b2 { 0.0f };
        float a1 { 0.0f };
        float a2 { 0.0f };
        float s1 { 0.0f };
        float s2 { 0.0f };

        void reset() noexcept { s1 = 0.0f; s2 = 0.0f; }

        [[nodiscard]] inline float process(float x) noexcept {
            const float y = b0 * x + s1;
            s1 = flushDenormal(b1 * x - a1 * y + s2);
            s2 = flushDenormal(b2 * x - a2 * y);
            return flushDenormal(y);
        }
    };

    float mSampleRate { 48000.0f };
    bool mEnabled { true };
    DimensionMode mDimensionMode { DimensionMode::Mode2 };

    float mRateHz { 0.55f };
    float mDepthMs { 2.20f };
    float mMix { 0.45f };
    float mBaseDelayMs { 5.50f };

    // LFO phase accumulator
    float mLfoPhase { 0.0f };
    float mLfoInc { 0.0f };

    // 3 Tri-phase BBD delay lines
    FixedDelayLine<kDelayCapacity> mDelay1;
    FixedDelayLine<kDelayCapacity> mDelay2;
    FixedDelayLine<kDelayCapacity> mDelay3;

    // Compander filters
    CompanderShelf mPreEmphasis;
    CompanderShelf mDeEmphasis1;
    CompanderShelf mDeEmphasis2;
    CompanderShelf mDeEmphasis3;

    ReconstructionLowpass mReconLp1;
    ReconstructionLowpass mReconLp2;
    ReconstructionLowpass mReconLp3;
};

} // namespace braun::mr16
