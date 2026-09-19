#pragma once

#include "DspMath.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace braun::mr16 {

enum class ManifoldType : int {
    ChladniPlate = 0, // Biharmonic 2D square free plate (Nabla^4)
    StiffBeam    = 1, // Euler-Bernoulli bar with undercut arch
    VocalFormant = 2, // Fant tube acoustic vowel formant series
    PoincareHorn = 3  // Negative-curvature hyperbolic horn flare
};

enum class MaterialType : int {
    Wood  = 0, // Spruce: steep high-frequency acoustic loss
    Glass = 1, // Crystalline, sustained upper overtones
    Steel = 2, // High elasticity, extreme metallic ring-down
    Brass = 3, // Rich harmonic bloom, balanced damping
    Nylon = 4  // Viscoelastic, highly muted high frequencies
};

/**
 * ModalResonatorMatrix: 16-Pole Physical Acoustic Modal Resonator.
 *
 * Implements:
 * 1. 16 parallel Zavalishin Topology-Preserving Transform (TPT) State Variable Filters (SVF)
 *    with unconditional Lyapunov stability under audio-rate modulation.
 * 2. 4 Geometric Acoustic Manifolds with continuous geometric morphing.
 * 3. 5 Calibrated physical material damping profiles.
 * 4. Fast O(N) Orthogonal Householder scattering matrix (<10 ns/sample).
 * 5. Golden-ratio constant-power stereo panning (Phi = 137.5 deg).
 * 6. Real-time modal energy tracking for Deck 06 Phosphor CRT Scope telemetry.
 */
class ModalResonatorMatrix {
public:
    static constexpr size_t kNumModes = 16;

    // Manifold frequency ratios calibrated to physical boundary mechanics
    static inline constexpr std::array<float, kNumModes> kChladniPlateRatios = {
        1.0000f, 1.5882f, 2.0588f, 2.7647f, 
        3.1765f, 4.0588f, 4.4118f, 5.3529f, 
        5.8824f, 6.7647f, 7.4118f, 8.5294f, 
        9.3529f, 10.2941f, 11.6471f, 13.0000f
    };

    static inline constexpr std::array<float, kNumModes> kStiffBeamRatios = {
        1.0000f,  2.7565f,  4.0000f,  5.4040f, 
        7.1200f,  8.9330f, 10.0000f, 11.5000f, 
        13.3450f, 15.6000f, 17.8000f, 20.2500f, 
        23.1000f, 26.3000f, 29.8000f, 33.6000f
    };

    static inline constexpr std::array<float, kNumModes> kVocalFormantRatios = {
        1.0000f,  1.4500f,  2.1000f,  2.8500f, 
        3.7500f,  4.6000f,  5.5500f,  6.6000f, 
        7.7500f,  9.0000f, 10.3500f, 11.8000f, 
        13.4000f, 15.1000f, 17.0000f, 19.1000f
    };

    static inline constexpr std::array<float, kNumModes> kPoincareHornRatios = {
        1.0000f, 1.1926f, 1.4866f, 1.8493f, 
        2.2561f, 2.6926f, 3.1496f, 3.6210f, 
        4.1024f, 4.5913f, 5.0858f, 5.5846f, 
        6.0867f, 6.5914f, 7.0980f, 7.6062f
    };

    ModalResonatorMatrix() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setFundamentalHz(float f0Hz) noexcept;
    void setManifold(ManifoldType type, float morph = 0.0f) noexcept;
    void setMaterial(MaterialType material) noexcept;
    void setDecayScale(float decayScale) noexcept;
    void setCouplingDepth(float coupling) noexcept;
    void setStereoWidth(float width) noexcept;

    // Real-time chaotic modulation inputs
    void applyModulation(const std::array<float, kNumModes>& freqMultipliers,
                         float qSpreadFactor,
                         float panOffsetRad) noexcept;

    // Process single sample: takes exciter bus input and returns panned stereo
    void processSample(float exciterInput, float& outL, float& outR) noexcept;

    // Block processing
    void processBlock(const float* exciterBuffer, float* outL, float* outR, int numSamples) noexcept;

    // Telemetry getters for Phosphor CRT Scope
    [[nodiscard]] const std::array<float, kNumModes>& getModalEnergies() const noexcept {
        return mModalEnergies;
    }

    [[nodiscard]] const std::array<float, kNumModes>& getCurrentFrequencies() const noexcept {
        return mCurrentFrequencies;
    }

    [[nodiscard]] float getFundamentalHz() const noexcept { return mFundamentalHz; }
    [[nodiscard]] ManifoldType getManifoldType() const noexcept { return mManifold; }
    [[nodiscard]] MaterialType getMaterialType() const noexcept { return mMaterial; }
    [[nodiscard]] float getStiffnessB() const noexcept { return mStiffnessB; }
    [[nodiscard]] float getClusterDetune() const noexcept { return mClusterDetune; }
    [[nodiscard]] const std::array<float, kNumModes>& getModeWeights() const noexcept { return mModeWeights; }

private:
    void updateMaterialParameters() noexcept;
    void updateFilterCoefficients() noexcept;
    void calculateDampingAndQ() noexcept;
    void calculateSpatialPanning() noexcept;

    float mSampleRate { 48000.0f };
    float mFundamentalHz { 220.0f };
    float mDecayScale { 1.0f };
    float mCouplingDepth { 0.25f };
    float mStereoWidth { 0.85f };
    float mManifoldMorph { 0.0f };

    ManifoldType mManifold { ManifoldType::ChladniPlate };
    MaterialType mMaterial { MaterialType::Wood };

    // Physical Acoustic Material Properties
    float mStiffnessB { 0.0005f };
    float mClusterDetune { 0.0f };
    std::array<float, kNumModes> mModeWeights {{
        1.50f, 1.50f, 1.50f, 1.50f, 1.00f, 0.75f, 0.55f, 0.40f,
        0.30f, 0.22f, 0.16f, 0.12f, 0.09f, 0.07f, 0.05f, 0.04f
    }};

    // TPT SVF State Variables
    std::array<float, kNumModes> mS1 { 0.0f };
    std::array<float, kNumModes> mS2 { 0.0f };
    std::array<float, kNumModes> mG { 0.0f };
    std::array<float, kNumModes> mK { 0.0f };
    std::array<float, kNumModes> mA1 { 0.0f };

    // Feedback coupling states
    std::array<float, kNumModes> mFeedbackModes { 0.0f };

    // Frequencies & Qs
    std::array<float, kNumModes> mBaseRatios { 1.0f };
    std::array<float, kNumModes> mBaseQ { 50.0f };
    std::array<float, kNumModes> mCurrentFrequencies { 220.0f };

    // Modulation states
    std::array<float, kNumModes> mModFreqMultipliers { 1.0f };
    float mModQSpread { 1.0f };
    float mModPanOffset { 0.0f };

    // Panning gains
    std::array<float, kNumModes> mPanL { 0.7071f };
    std::array<float, kNumModes> mPanR { 0.7071f };

    // CRT Telemetry Energy Trackers
    std::array<float, kNumModes> mModalEnergies { 0.0f };
};

} // namespace braun::mr16
