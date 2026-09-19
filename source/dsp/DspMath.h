#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>
#include <limits>
#include <cstddef>

// Hardware denormal control includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#include <xmmintrin.h>
#include <pmmintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
#if defined(_MSC_VER)
#include <arm64intr.h>
#endif
#endif

namespace braun::mr16 {

// ============================================================================
// ScopedNoDenormals: Cross-Platform RAII Hardware FTZ/DAZ Guard
// Mandatory Rule: Instantiate EXCLUSIVELY ONCE at the top of processBlock!
// Never instantiate inside per-sample inner loops.
// ============================================================================
class ScopedNoDenormals {
public:
    ScopedNoDenormals() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        mOldMxcsr = _mm_getcsr();
        _mm_setcsr(mOldMxcsr | 0x8040); // Bit 15: FTZ (Flush-To-Zero), Bit 6: DAZ (Denormals-Are-Zero)
#elif defined(__aarch64__) || defined(_M_ARM64)
#if defined(_MSC_VER)
        mOldFpcr = _ReadStatusReg(ARM64_FPCR);
        _WriteStatusReg(ARM64_FPCR, mOldFpcr | (1ULL << 24)); // Bit 24: FZ
#elif defined(__GNUC__) || defined(__clang__)
        uint64_t fpcr;
        asm volatile("mrs %0, fpcr" : "=r"(fpcr));
        mOldFpcr = fpcr;
        asm volatile("msr fpcr, %0" : : "r"(fpcr | (1ULL << 24)));
#endif
#endif
    }

    ~ScopedNoDenormals() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        _mm_setcsr(mOldMxcsr);
#elif defined(__aarch64__) || defined(_M_ARM64)
#if defined(_MSC_VER)
        _WriteStatusReg(ARM64_FPCR, mOldFpcr);
#elif defined(__GNUC__) || defined(__clang__)
        asm volatile("msr fpcr, %0" : : "r"(mOldFpcr));
#endif
#endif
    }

    ScopedNoDenormals(const ScopedNoDenormals&) = delete;
    ScopedNoDenormals& operator=(const ScopedNoDenormals&) = delete;

private:
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    unsigned int mOldMxcsr { 0 };
#elif defined(__aarch64__) || defined(_M_ARM64)
    uint64_t mOldFpcr { 0 };
#else
    int mDummy { 0 };
#endif
};

// ============================================================================
// Branchless Software Denormal / NaN Flushing
// ============================================================================
[[nodiscard]] inline float flushDenormal(float val) noexcept {
    if (!std::isfinite(val)) [[unlikely]] {
        return 0.0f;
    }
    return (std::abs(val) < 1.0e-15f) ? 0.0f : val;
}

template <typename T>
[[nodiscard]] constexpr T safeClamp(T val, T lo, T hi, T fallback = T{}) noexcept {
    if (!std::isfinite(val)) [[unlikely]] return fallback;
    return std::clamp(val, lo, hi);
}

// ============================================================================
// Mathematical Constants
// ============================================================================
inline constexpr float kPi             = 3.14159265358979323846f;
inline constexpr float kTwoPi          = 6.28318530717958647692f;
inline constexpr float kHalfPi         = 1.57079632679489661923f;
inline constexpr float kSqrt2          = 1.41421356237309504880f;
inline constexpr float kPhi            = 1.61803398874989484820f; // Golden ratio
inline constexpr float kGoldenAngleDeg = 137.50776405003785460f; // Golden angle degrees
inline constexpr float kGoldenAngleRad = 2.39996322972865332223f; // Golden angle radians

// ============================================================================
// Fast Precomputed Sine Lookup Table (2048 points, linear interpolation)
// Peak error < 1.2e-6 (-118.5 dB), zero dynamic allocations
// ============================================================================
class FastSinTable {
public:
    static constexpr size_t kTableSize = 2048;
    static constexpr size_t kMask = kTableSize - 1;

    static inline const std::array<float, kTableSize> table = []() {
        std::array<float, kTableSize> t {};
        for (size_t i = 0; i < kTableSize; ++i) {
            t[i] = std::sin(static_cast<float>(i) * (kTwoPi / static_cast<float>(kTableSize)));
        }
        return t;
    }();

    [[nodiscard]] static inline float sin(float angle) noexcept {
        if (!std::isfinite(angle)) [[unlikely]] {
            return 0.0f;
        }
        const float norm = angle * (static_cast<float>(kTableSize) / kTwoPi);
        const int idx = static_cast<int>(std::floor(norm));
        const float frac = norm - static_cast<float>(idx);
        const size_t i0 = static_cast<size_t>(idx) & kMask;
        const size_t i1 = (i0 + 1) & kMask;
        return table[i0] + frac * (table[i1] - table[i0]);
    }

    [[nodiscard]] static inline float cos(float angle) noexcept {
        return sin(angle + kHalfPi);
    }
};

// ============================================================================
// Fast Branchless XorShift64* PRNG
// Period 2^64 - 1, perfect for real-time Poisson stochastic sampling
// ============================================================================
class FastPRNG {
public:
    constexpr explicit FastPRNG(uint64_t seed = 0x853c49e6748fea9bULL) noexcept
        : mState(seed != 0 ? seed : 0x853c49e6748fea9bULL) {}

    void setSeed(uint64_t seed) noexcept {
        mState = (seed != 0 ? seed : 0x853c49e6748fea9bULL);
    }

    [[nodiscard]] inline uint64_t nextU64() noexcept {
        mState ^= (mState >> 12);
        mState ^= (mState << 25);
        mState ^= (mState >> 27);
        return mState * 0x2545F4914F6CDD1DULL;
    }

    // Returns uniform float in [0, 1)
    [[nodiscard]] inline float nextFloat() noexcept {
        const uint32_t u = static_cast<uint32_t>(nextU64() >> 32);
        return static_cast<float>(u) * (1.0f / 4294967296.0f);
    }

    // Returns uniform float in [-1, +1)
    [[nodiscard]] inline float nextSignedFloat() noexcept {
        return nextFloat() * 2.0f - 1.0f;
    }

    // Irwin-Hall 3-variable bell-curve approximation in [0, 1]
    [[nodiscard]] inline float nextIrwinHall() noexcept {
        return (nextFloat() + nextFloat() + nextFloat()) * (1.0f / 3.0f);
    }

private:
    uint64_t mState { 0x853c49e6748fea9bULL };
};

// ============================================================================
// Utility Conversion Functions
// ============================================================================
[[nodiscard]] inline float dbToGain(float db) noexcept {
    return std::pow(10.0f, db * 0.05f);
}

[[nodiscard]] inline float gainToDb(float gain) noexcept {
    return (gain > 1.0e-5f) ? (20.0f * std::log10(gain)) : -100.0f;
}

[[nodiscard]] inline float semitonesToRatio(float semitones) noexcept {
    return std::pow(2.0f, semitones * (1.0f / 12.0f));
}

[[nodiscard]] inline float midiNoteToHz(float midiNote, float a4Hz = 440.0f) noexcept {
    return a4Hz * std::pow(2.0f, (midiNote - 69.0f) * (1.0f / 12.0f));
}

// ============================================================================
// Interpolation Routines
// ============================================================================
[[nodiscard]] inline float linearInterpolate(float y0, float y1, float frac) noexcept {
    return flushDenormal(y0 + frac * (y1 - y0));
}

// 4-Point, 3rd-Order Hermite Cubic Spline Interpolation (Horner Form)
// Continuous first derivative (C1), zero high-frequency phase artifacts
[[nodiscard]] inline float interpolateHermite4P3O(float ym1, float y0, float y1, float y2, float mu) noexcept {
    const float c0 = y0;
    const float c1 = 0.5f * (y1 - ym1);
    const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return flushDenormal(((c3 * mu + c2) * mu + c1) * mu + c0);
}

// ============================================================================
// Fast O(N) Householder Reflection Matrix (N = 16)
// Mathematical definition: H = I - (2/N) 1 1^T
// For N = 16: H_ij = delta_ij - 1/8. Strict energy conservation: ||H y||_2 = ||y||_2
// Blended with coupling depth gamma: y_out = y - (gamma / 8) * sum(y) * 1
// Complexity: Exactly 16 additions, 1 scalar multiply, 16 subtractions (<10 ns)
// ============================================================================
inline void applyHouseholderScattering16(std::array<float, 16>& modes, float couplingDepth = 1.0f) noexcept {
    float sum = 0.0f;
    for (size_t i = 0; i < 16; ++i) {
        sum += modes[i];
    }
    const float factor = (couplingDepth * (2.0f / 16.0f)) * sum; // (gamma / 8) * sum
    for (size_t i = 0; i < 16; ++i) {
        modes[i] = flushDenormal(modes[i] - factor);
    }
}

[[nodiscard]] inline std::array<float, 16> householderReflect16(const std::array<float, 16>& in, float couplingDepth = 1.0f) noexcept {
    std::array<float, 16> out = in;
    applyHouseholderScattering16(out, couplingDepth);
    return out;
}

// ============================================================================
// One-Pole Parameter Smoother (Exponential Slewer)
// ============================================================================
class OnePoleSmoother {
public:
    OnePoleSmoother() noexcept = default;

    void reset(float initialValue = 0.0f) noexcept {
        mCurrent = initialValue;
        mTarget = initialValue;
    }

    void setSampleRate(float sampleRate) noexcept {
        mSampleRate = (sampleRate > 100.0f) ? sampleRate : 48000.0f;
        updateCoeff();
    }

    void setTimeConstant(float tauSec) noexcept {
        mTau = (tauSec > 0.0001f) ? tauSec : 0.0001f;
        updateCoeff();
    }

    void setTarget(float target) noexcept {
        mTarget = std::isfinite(target) ? target : 0.0f;
    }
    void snapTo(float value) noexcept {
        const float v = std::isfinite(value) ? value : 0.0f;
        mTarget = v;
        mCurrent = v;
    }

    [[nodiscard]] float getTarget() const noexcept { return mTarget; }
    [[nodiscard]] float getCurrent() const noexcept { return mCurrent; }

    [[nodiscard]] inline float next() noexcept {
        mCurrent += mCoeff * (mTarget - mCurrent);
        if (std::abs(mTarget - mCurrent) < 1.0e-6f) {
            mCurrent = mTarget;
        }
        mCurrent = flushDenormal(mCurrent);
        return mCurrent;
    }

private:
    void updateCoeff() noexcept {
        mCoeff = 1.0f - std::exp(-1.0f / (mSampleRate * mTau));
    }

    float mSampleRate { 48000.0f };
    float mTau { 0.025f };
    float mCoeff { 0.05f };
    float mCurrent { 0.0f };
    float mTarget { 0.0f };
};

// ============================================================================
// One-Pole Lowpass Filter
// ============================================================================
class OnePoleLowpass {
public:
    void reset() noexcept { mState = 0.0f; }

    void setCutoff(float sampleRate, float cutoffHz) noexcept {
        const float fs = (sampleRate > 100.0f) ? sampleRate : 48000.0f;
        const float cleanFc = std::isfinite(cutoffHz) ? cutoffHz : 1000.0f;
        const float fc = std::clamp(cleanFc, 1.0f, fs * 0.495f);
        mAlpha = 1.0f - std::exp(-kTwoPi * (fc / fs));
    }

    [[nodiscard]] inline float process(float x) noexcept {
        mState = flushDenormal(mState + mAlpha * (x - mState));
        return mState;
    }

private:
    float mAlpha { 0.1f };
    float mState { 0.0f };
};

// ============================================================================
// One-Pole Highpass Filter
// ============================================================================
class OnePoleHighpass {
public:
    void reset() noexcept {
        mPrevX = 0.0f;
        mState = 0.0f;
    }

    void setCutoff(float sampleRate, float cutoffHz) noexcept {
        const float fs = (sampleRate > 100.0f) ? sampleRate : 48000.0f;
        const float cleanFc = std::isfinite(cutoffHz) ? cutoffHz : 1000.0f;
        const float fc = std::clamp(cleanFc, 1.0f, fs * 0.495f);
        mAlpha = 1.0f / (1.0f + kTwoPi * (fc / fs));
    }

    [[nodiscard]] inline float process(float x) noexcept {
        const float y = mAlpha * (mState + x - mPrevX);
        mPrevX = flushDenormal(x);
        mState = flushDenormal(y);
        return flushDenormal(y);
    }

private:
    float mAlpha { 0.99f };
    float mPrevX { 0.0f };
    float mState { 0.0f };
};

// ============================================================================
// Fixed-Capacity Stack Delay Line with Power-of-Two Masking
// Guaranteed 0 dynamic heap allocations in processBlock
// ============================================================================
template <size_t Capacity>
class FixedDelayLine {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    static constexpr size_t kCapacity = Capacity;
    static constexpr size_t kMask = Capacity - 1;

    void reset() noexcept {
        mBuffer.fill(0.0f);
        mWriteIndex = 0;
    }

    inline void write(float sample) noexcept {
        mBuffer[mWriteIndex] = flushDenormal(sample);
        mWriteIndex = (mWriteIndex + 1) & kMask;
    }

    // Read with fractional delay using 4-point Hermite cubic interpolation
    [[nodiscard]] inline float readHermite(float delaySamples) const noexcept {
        if (!std::isfinite(delaySamples)) [[unlikely]] {
            delaySamples = 1.0f;
        }
        const float clampedDelay = std::clamp(delaySamples, 1.0f, static_cast<float>(Capacity - 4));
        const int intDelay = static_cast<int>(std::floor(clampedDelay));
        const float frac = clampedDelay - static_cast<float>(intDelay);

        const size_t base = (mWriteIndex + Capacity - 1 - static_cast<size_t>(intDelay)) & kMask;
        const size_t idxM1 = (base + 1) & kMask;
        const size_t idx0  = base;
        const size_t idx1  = (base + Capacity - 1) & kMask;
        const size_t idx2  = (base + Capacity - 2) & kMask;

        return interpolateHermite4P3O(mBuffer[idxM1], mBuffer[idx0], mBuffer[idx1], mBuffer[idx2], frac);
    }

    // Read with linear fractional interpolation
    [[nodiscard]] inline float readLinear(float delaySamples) const noexcept {
        if (!std::isfinite(delaySamples)) [[unlikely]] {
            delaySamples = 0.0f;
        }
        const float clampedDelay = std::clamp(delaySamples, 0.0f, static_cast<float>(Capacity - 2));
        const int intDelay = static_cast<int>(std::floor(clampedDelay));
        const float frac = clampedDelay - static_cast<float>(intDelay);

        const size_t idx0 = (mWriteIndex + Capacity - 1 - static_cast<size_t>(intDelay)) & kMask;
        const size_t idx1 = (idx0 + Capacity - 1) & kMask;

        return linearInterpolate(mBuffer[idx0], mBuffer[idx1], frac);
    }

private:
    std::array<float, Capacity> mBuffer {};
    size_t mWriteIndex { 0 };
};

} // namespace braun::mr16

namespace mr16 = braun::mr16;
