#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <complex>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <string>
#include <functional>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <numeric>
#include <random>

// Core DSP Headers
#include "DspMath.h"
#include "BoundedSaturator.h"
#include "VactrolGate.h"
#include "SpatialChorus.h"
#include "LorenzAttractor.h"
#include "ModalResonatorMatrix.h"
#include "KineticExciter.h"
#include "Mr16Engine.h"

// ============================================================================
// Global Heap Allocation Tracker for Real-Time Safety Verification
// ============================================================================
extern bool gTrackAllocations;
extern size_t gAllocationCount;
extern size_t gBytesAllocated;

// ============================================================================
// Test Framework Infrastructure
// ============================================================================
namespace test {

inline int gCurrentTestAssertFailures = 0;

#if defined(_MSC_VER)
#define TEST_ASSERT(cond, msg) do { \
    __pragma(warning(push)) \
    __pragma(warning(disable: 4127)) \
    if (!(cond)) { \
        std::cerr << "      ASSERTION FAILED [" << __FILE__ << ":" << __LINE__ << "]: " << (msg) << "\n"; \
        ++test::gCurrentTestAssertFailures; \
    } \
    __pragma(warning(pop)) \
} while (0)
#else
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::cerr << "      ASSERTION FAILED [" << __FILE__ << ":" << __LINE__ << "]: " << (msg) << "\n"; \
        ++test::gCurrentTestAssertFailures; \
    } \
} while (0)
#endif

#define TEST_ASSERT_NEAR(val, expected, tol, msg) do { \
    double v_ = static_cast<double>(val); \
    double exp_ = static_cast<double>(expected); \
    double diff_ = std::abs(v_ - exp_); \
    if (diff_ > (tol)) { \
        std::cerr << "      ASSERTION FAILED [" << __FILE__ << ":" << __LINE__ << "]: " << (msg) \
                  << " (actual: " << v_ << ", expected: " << exp_ << ", diff: " << diff_ << ", tol: " << (tol) << ")\n"; \
        ++test::gCurrentTestAssertFailures; \
    } \
} while (0)

struct TestCase {
    std::string tier;
    std::string id;
    std::string name;
    std::function<bool()> run;
};

inline std::vector<TestCase>& getTestRegistry() {
    static std::vector<TestCase> registry;
    return registry;
}

inline void registerTest(const std::string& tier, const std::string& id, const std::string& name, std::function<bool()> fn) {
    getTestRegistry().push_back({tier, id, name, std::move(fn)});
}

} // namespace test

// ============================================================================
// Test Utilities: FFT, Signal Generation, Metrics & Analysis
// ============================================================================
namespace test_utils {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 6.28318530717958647692;

// In-Place Cooley-Tukey Radix-2 FFT
inline void fft(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        double ang = -2.0 * kPi / static_cast<double>(len);
        std::complex<double> wlen(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<double> u = a[i + j];
                std::complex<double> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

// Exact Peak Frequency Detector using 4-term Blackman-Harris window and quadratic interpolation
inline double findPeakFrequency(const std::vector<float>& signal, double sampleRate, size_t startSample, size_t numSamples, double minF, double maxF) {
    size_t fftSize = 1;
    while (fftSize * 2 <= numSamples) fftSize *= 2;
    if (fftSize < 256) fftSize = 256;
    if (startSample + fftSize > signal.size()) {
        if (signal.size() <= fftSize) startSample = 0;
        else startSample = signal.size() - fftSize;
    }

    std::vector<std::complex<double>> buffer(fftSize);
    for (size_t i = 0; i < fftSize; ++i) {
        const double a0 = 0.35875, a1 = 0.48829, a2 = 0.14128, a3 = 0.01168;
        const double t = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(fftSize - 1);
        const double win = a0 - a1 * std::cos(t) + a2 * std::cos(2.0 * t) - a3 * std::cos(3.0 * t);
        const size_t idx = startSample + i;
        const float val = (idx < signal.size()) ? signal[idx] : 0.0f;
        buffer[i] = static_cast<double>(val) * win;
    }

    fft(buffer);

    const double binWidth = sampleRate / static_cast<double>(fftSize);
    const size_t minBin = std::max(size_t{2}, static_cast<size_t>(minF / binWidth));
    const size_t maxBin = std::min(fftSize / 2 - 2, static_cast<size_t>(maxF / binWidth) + 1);

    size_t bestBin = minBin;
    double maxMag = 0.0;
    for (size_t k = minBin; k <= maxBin; ++k) {
        double mag = std::abs(buffer[k]);
        if (mag > maxMag) {
            maxMag = mag;
            bestBin = k;
        }
    }

    const double alpha = std::abs(buffer[bestBin - 1]);
    const double beta  = std::abs(buffer[bestBin]);
    const double gamma = std::abs(buffer[bestBin + 1]);

    double p = 0.0;
    double denom = (alpha - 2.0 * beta + gamma);
    if (std::abs(denom) > 1.0e-12) {
        p = 0.5 * (alpha - gamma) / denom;
    }

    return (static_cast<double>(bestBin) + p) * binWidth;
}

inline double computeRMS(const std::vector<float>& sig, size_t start = 0, size_t count = 0) {
    if (sig.empty()) return 0.0;
    if (count == 0 || start + count > sig.size()) count = sig.size() - start;
    if (count == 0) return 0.0;

    double sum = 0.0;
    for (size_t i = start; i < start + count; ++i) {
        sum += static_cast<double>(sig[i]) * static_cast<double>(sig[i]);
    }
    return std::sqrt(sum / static_cast<double>(count));
}

inline float computePeak(const std::vector<float>& sig, size_t start = 0, size_t count = 0) {
    if (sig.empty()) return 0.0f;
    if (count == 0 || start + count > sig.size()) count = sig.size() - start;
    if (count == 0) return 0.0f;

    float peak = 0.0f;
    for (size_t i = start; i < start + count; ++i) {
        peak = std::max(peak, std::abs(sig[i]));
    }
    return peak;
}

inline float computeStereoCorrelation(const std::vector<float>& l, const std::vector<float>& r) {
    const size_t n = std::min(l.size(), r.size());
    if (n == 0) return 1.0f;

    double sumL = 0.0, sumR = 0.0, sumLR = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sumL += static_cast<double>(l[i]) * static_cast<double>(l[i]);
        sumR += static_cast<double>(r[i]) * static_cast<double>(r[i]);
        sumLR += static_cast<double>(l[i]) * static_cast<double>(r[i]);
    }
    double denom = std::sqrt(sumL * sumR);
    if (denom < 1.0e-12) return 1.0f;
    return static_cast<float>(std::clamp(sumLR / denom, -1.0, 1.0));
}

inline double computeSpectralCentroid(const std::vector<float>& signal, double sampleRate, size_t fftSize = 1024) {
    if (signal.size() < fftSize) return 0.0;
    std::vector<std::complex<double>> buffer(fftSize);
    for (size_t i = 0; i < fftSize; ++i) {
        buffer[i] = static_cast<double>(signal[i]);
    }
    fft(buffer);

    const double binWidth = sampleRate / static_cast<double>(fftSize);
    double num = 0.0, den = 0.0;
    for (size_t k = 0; k < fftSize / 2; ++k) {
        double mag = std::abs(buffer[k]);
        double freq = static_cast<double>(k) * binWidth;
        num += freq * mag;
        den += mag;
    }
    return (den > 1.0e-9) ? (num / den) : 0.0;
}

inline std::vector<float> computeAutocorrelation(const std::vector<float>& signal, size_t maxLag = 256) {
    if (signal.empty()) return {};
    const size_t n = signal.size();
    const size_t lags = std::min(maxLag, n);
    std::vector<float> ac(lags, 0.0f);

    double e0 = 0.0;
    for (size_t i = 0; i < n; ++i) {
        e0 += static_cast<double>(signal[i]) * static_cast<double>(signal[i]);
    }
    if (e0 < 1.0e-12) {
        ac[0] = 1.0f;
        return ac;
    }

    for (size_t lag = 0; lag < lags; ++lag) {
        double sum = 0.0;
        for (size_t i = 0; i + lag < n; ++i) {
            sum += static_cast<double>(signal[i]) * static_cast<double>(signal[i + lag]);
        }
        ac[lag] = static_cast<float>(sum / e0);
    }
    return ac;
}

inline bool hasNaNOrInf(const std::vector<float>& signal) {
    for (float v : signal) {
        if (!std::isfinite(v)) return true;
    }
    return false;
}

inline bool isSignalFinite(const std::vector<float>& signal) {
    return !hasNaNOrInf(signal);
}

// Stimulus Generators
inline std::vector<float> generateSine(size_t numSamples, double freq, double sampleRate, float amp = 1.0f) {
    std::vector<float> sig(numSamples);
    for (size_t i = 0; i < numSamples; ++i) {
        sig[i] = amp * static_cast<float>(std::sin(kTwoPi * freq * static_cast<double>(i) / sampleRate));
    }
    return sig;
}

inline std::vector<float> generateImpulse(size_t numSamples, float amp = 1.0f) {
    std::vector<float> sig(numSamples, 0.0f);
    if (numSamples > 0) sig[0] = amp;
    return sig;
}

inline std::vector<float> generateStep(size_t numSamples, size_t stepIndex, float initialVal = 0.0f, float finalVal = 1.0f) {
    std::vector<float> sig(numSamples, initialVal);
    for (size_t i = stepIndex; i < numSamples; ++i) {
        sig[i] = finalVal;
    }
    return sig;
}

inline std::vector<float> generatePinkNoise(size_t numSamples, float amp = 1.0f, uint64_t seed = 12345ULL) {
    std::vector<float> sig(numSamples);
    mr16::FastPRNG prng(seed);
    // 3-pole pinking filter approximation (Paul Kellet's filter)
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        float white = prng.nextSignedFloat();
        b0 = 0.99765f * b0 + white * 0.0990460f;
        b1 = 0.96300f * b1 + white * 0.2965164f;
        b2 = 0.57000f * b2 + white * 1.0526913f;
        float pink = b0 + b1 + b2 + white * 0.1848f;
        sig[i] = amp * pink * 0.2f;
    }
    return sig;
}

} // namespace test_utils
