#include "ModalResonatorMatrix.h"

namespace braun::mr16 {

void ModalResonatorMatrix::prepare(double sampleRate) noexcept {
    mSampleRate = (sampleRate > 100.0) ? static_cast<float>(sampleRate) : 48000.0f;
    reset();
}

void ModalResonatorMatrix::reset() noexcept {
    mS1.fill(0.0f);
    mS2.fill(0.0f);
    mFeedbackModes.fill(0.0f);
    mModalEnergies.fill(0.0f);
    mModFreqMultipliers.fill(1.0f);
    mModQSpread = 1.0f;
    mModPanOffset = 0.0f;
    mQScale = 1.0f;
    mOvertoneSpread = 1.0f;

    updateMaterialParameters();
    setManifold(mManifold, mManifoldMorph);
    calculateDampingAndQ();
    updateFilterCoefficients();
    calculateSpatialPanning();
}

void ModalResonatorMatrix::updateMaterialParameters() noexcept {
    switch (mMaterial) {
        case MaterialType::Wood:
            mStiffnessB = 0.0005f;
            mClusterDetune = 0.0f;
            mModeWeights = {{
                1.50f, 1.50f, 1.50f, 1.50f, 1.00f, 0.75f, 0.55f, 0.40f,
                0.30f, 0.22f, 0.16f, 0.12f, 0.09f, 0.07f, 0.05f, 0.04f
            }};
            break;
        case MaterialType::Glass:
            mStiffnessB = 0.0002f;
            mClusterDetune = 0.0f;
            mModeWeights = {{
                0.85f, 0.90f, 0.95f, 1.00f, 1.00f, 1.35f, 1.38f, 1.41f,
                1.44f, 1.47f, 1.50f, 1.53f, 1.56f, 1.59f, 1.62f, 1.65f
            }};
            break;
        case MaterialType::Steel:
            mStiffnessB = 0.0240f;
            mClusterDetune = 0.0f;
            mModeWeights = {{
                1.00f, 1.35f, 1.35f, 1.35f, 1.35f, 1.35f, 1.35f, 1.15f,
                1.00f, 0.90f, 0.85f, 0.80f, 0.80f, 0.80f, 0.80f, 0.80f
            }};
            break;
        case MaterialType::Brass:
            mStiffnessB = 0.0080f;
            mClusterDetune = 0.0180f;
            mModeWeights = {{
                1.35f, 1.35f, 1.35f, 1.35f, 1.35f, 1.20f, 1.20f, 1.20f,
                1.20f, 1.20f, 1.20f, 1.05f, 0.95f, 0.85f, 0.75f, 0.70f
            }};
            break;
        case MaterialType::Nylon:
            mStiffnessB = 0.0010f;
            mClusterDetune = 0.0f;
            mModeWeights = {{
                1.40f, 1.00f, 0.70f, 0.45f, 0.30f, 0.20f, 0.14f, 0.10f,
                0.07f, 0.05f, 0.04f, 0.03f, 0.02f, 0.02f, 0.01f, 0.01f
            }};
            break;
    }
}

void ModalResonatorMatrix::setFundamentalHz(float f0Hz) noexcept {
    mFundamentalHz = std::clamp(f0Hz, 20.0f, 4000.0f);
    calculateDampingAndQ();
    updateFilterCoefficients();
}

void ModalResonatorMatrix::setManifold(ManifoldType type, float morph) noexcept {
    mManifold = type;
    mManifoldMorph = std::clamp(morph, 0.0f, 1.0f);

    const auto& currentRatios = [&]() -> const std::array<float, kNumModes>& {
        switch (type) {
            case ManifoldType::ChladniPlate: return kChladniPlateRatios;
            case ManifoldType::StiffBeam:    return kStiffBeamRatios;
            case ManifoldType::VocalFormant: return kVocalFormantRatios;
            case ManifoldType::PoincareHorn: return kPoincareHornRatios;
            default:                         return kChladniPlateRatios;
        }
    }();

    const auto& nextRatios = [&]() -> const std::array<float, kNumModes>& {
        switch (type) {
            case ManifoldType::ChladniPlate: return kStiffBeamRatios;
            case ManifoldType::StiffBeam:    return kVocalFormantRatios;
            case ManifoldType::VocalFormant: return kPoincareHornRatios;
            case ManifoldType::PoincareHorn: return kChladniPlateRatios;
            default:                         return kStiffBeamRatios;
        }
    }();

    for (size_t i = 0; i < kNumModes; ++i) {
        mBaseRatios[i] = (1.0f - mManifoldMorph) * currentRatios[i] + mManifoldMorph * nextRatios[i];
    }

    calculateDampingAndQ();
    updateFilterCoefficients();
}

void ModalResonatorMatrix::setMaterial(MaterialType material) noexcept {
    mMaterial = material;
    updateMaterialParameters();
    calculateDampingAndQ();
    updateFilterCoefficients();
}

void ModalResonatorMatrix::setDecayScale(float decayScale) noexcept {
    mDecayScale = std::clamp(decayScale, 0.05f, 20.0f);
    calculateDampingAndQ();
    updateFilterCoefficients();
}

void ModalResonatorMatrix::setQScale(float qScale) noexcept {
    mQScale = std::clamp(qScale, 0.05f, 20.0f);
    calculateDampingAndQ();
    updateFilterCoefficients();
}

void ModalResonatorMatrix::setOvertoneSpread(float spread) noexcept {
    mOvertoneSpread = std::clamp(spread, 0.2f, 3.0f);
    updateFilterCoefficients();
}

void ModalResonatorMatrix::setCouplingDepth(float coupling) noexcept {
    mCouplingDepth = std::clamp(coupling, 0.0f, 1.0f);
}

void ModalResonatorMatrix::setStereoWidth(float width) noexcept {
    mStereoWidth = std::clamp(width, 0.0f, 1.5f);
    calculateSpatialPanning();
}

void ModalResonatorMatrix::calculateDampingAndQ() noexcept {
    // Calibrated material parameters: eta(f) = eta0 + eta1 * (f / 1000)^mu
    float eta0 = 0.0080f;
    float eta1 = 0.0350f;
    float mu   = 1.40f;

    switch (mMaterial) {
        case MaterialType::Wood:
            eta0 = 0.0080f; eta1 = 0.0350f; mu = 1.40f;
            break;
        case MaterialType::Glass:
            eta0 = 0.0012f; eta1 = 0.0030f; mu = 0.80f;
            break;
        case MaterialType::Steel:
            eta0 = 0.0004f; eta1 = 0.0010f; mu = 0.60f;
            break;
        case MaterialType::Brass:
            eta0 = 0.0018f; eta1 = 0.0080f; mu = 1.00f;
            break;
        case MaterialType::Nylon:
            eta0 = 0.0350f; eta1 = 0.0900f; mu = 1.80f;
            break;
    }

    const float nyquist = mSampleRate * 0.495f;

    for (size_t i = 0; i < kNumModes; ++i) {
        const float m = static_cast<float>(i);
        const float dispersion = std::sqrt(1.0f + mStiffnessB * m * m);
        const float beating = 1.0f + mClusterDetune * std::sin(m * kPi / 2.5f);
        const float effectiveRatio = mBaseRatios[i] * dispersion * beating;

        const float cleanF0 = std::isfinite(mFundamentalHz) ? mFundamentalHz : 220.0f;
        const float f = std::clamp(cleanF0 * effectiveRatio, 20.0f, nyquist);
        const float fKhz = f * 0.001f;
        const float loss = eta0 + eta1 * std::pow(fKhz, mu);
        const float baseQ = (loss > 1.0e-5f) ? (1.0f / loss) : 1000.0f;
        const float cleanBaseQ = std::isfinite(baseQ) ? baseQ : 50.0f;
        const float cleanDecay = std::isfinite(mDecayScale) ? mDecayScale : 1.0f;
        mBaseQ[i] = std::clamp(cleanBaseQ * cleanDecay * mQScale, 0.5f, 4000.0f);
    }
}

void ModalResonatorMatrix::updateFilterCoefficients() noexcept {
    const float nyquist = mSampleRate * 0.495f;
    const float spreadFactor = std::clamp(mOvertoneSpread, 0.2f, 3.0f);

    for (size_t i = 0; i < kNumModes; ++i) {
        const float m = static_cast<float>(i);
        const float dispersion = std::sqrt(1.0f + mStiffnessB * m * m);
        const float beating = 1.0f + mClusterDetune * std::sin(m * kPi / 2.5f);
        const float effectiveRatio = 1.0f + (mBaseRatios[i] * dispersion * beating - 1.0f) * spreadFactor;

        // Effective modulated center frequency
        const float mult = std::isfinite(mModFreqMultipliers[i]) ? mModFreqMultipliers[i] : 1.0f;
        const float cleanF0 = std::isfinite(mFundamentalHz) ? mFundamentalHz : 220.0f;
        const float rawF = cleanF0 * effectiveRatio * mult;
        const float f = std::clamp(std::isfinite(rawF) ? rawF : 220.0f, 10.0f, nyquist);
        mCurrentFrequencies[i] = f;

        // Effective modulated Q-factor with modal spread
        const float cleanSpread = std::isfinite(mModQSpread) ? mModQSpread : 1.0f;
        const float qSpread = (i >= 8) ? cleanSpread : (1.0f / std::max(0.1f, cleanSpread));
        const float cleanBaseQ = std::isfinite(mBaseQ[i]) ? mBaseQ[i] : 50.0f;
        const float rawQ = cleanBaseQ * qSpread;
        const float q = std::clamp(std::isfinite(rawQ) ? rawQ : 50.0f, 0.5f, 4000.0f);

        // Zavalishin TPT State Variable Filter coefficients
        const float g = std::tan(kPi * (f / mSampleRate));
        const float k = 1.0f / q;
        const float a1 = 1.0f / (1.0f + g * (g + k));

        mG[i] = g;
        mK[i] = k;
        mA1[i] = a1;
    }
}

void ModalResonatorMatrix::calculateSpatialPanning() noexcept {
    for (size_t i = 0; i < kNumModes; ++i) {
        // Golden angle distribution: theta_k = (k * 137.507764 deg) mod 360 deg
        const float angleDeg = std::fmod(static_cast<float>(i) * kGoldenAngleDeg, 360.0f);
        const float angleRad = angleDeg * (kPi / 180.0f) + mModPanOffset;

        // Constant-power sine/cosine panning
        const float p = 0.5f + 0.5f * mStereoWidth * std::sin(angleRad);
        const float cleanP = std::isfinite(p) ? p : 0.5f;
        const float panNorm = std::clamp(cleanP, 0.0f, 1.0f);

        mPanL[i] = std::cos(kHalfPi * panNorm);
        mPanR[i] = std::sin(kHalfPi * panNorm);
    }
}

void ModalResonatorMatrix::applyModulation(const std::array<float, kNumModes>& freqMultipliers,
                                          float qSpreadFactor,
                                          float panOffsetRad) noexcept {
    mModFreqMultipliers = freqMultipliers;
    mModQSpread = qSpreadFactor;
    mModPanOffset = panOffsetRad;

    updateFilterCoefficients();
    calculateSpatialPanning();
}

void ModalResonatorMatrix::processSample(float exciterInput, float& outL, float& outR) noexcept {
    exciterInput = flushDenormal(exciterInput);
    std::array<float, kNumModes> modeOutputs;

    // 1. Parallel TPT SVF integration with inter-modal feedback
    for (size_t i = 0; i < kNumModes; ++i) {
        // Excite each mode with injected excitation + scattered modal feedback
        const float x = exciterInput + mFeedbackModes[i] * (0.28f * mCouplingDepth);

        // TPT SVF Bandpass Step (Zero-Delay Instantaneous Resolvent)
        const float vHp = mA1[i] * (x - (mK[i] + mG[i]) * mS1[i] - mS2[i]);
        const float vBp = mG[i] * vHp + mS1[i];
        const float vLp = mG[i] * vBp + mS2[i];

        // State update preserving physical kinetic / potential energy
        mS1[i] = flushDenormal(2.0f * vBp - mS1[i]);
        mS2[i] = flushDenormal(2.0f * vLp - mS2[i]);

        modeOutputs[i] = flushDenormal(vBp * mModeWeights[i]);
    }

    // 2. Orthogonal Householder Scattering Matrix: H = I - (2/N) 1 1^T
    // Evaluated in O(N) operations (<10 ns)
    float sumModes = 0.0f;
    for (size_t i = 0; i < kNumModes; ++i) {
        sumModes += modeOutputs[i] * mK[i];
    }
    const float householderFactor = (mCouplingDepth * (2.0f / 16.0f)) * sumModes;

    std::array<float, kNumModes> scattered;
    for (size_t i = 0; i < kNumModes; ++i) {
        scattered[i] = flushDenormal((modeOutputs[i] * mK[i]) - householderFactor);
        // Store scattered state for next-sample inter-modal energy exchange
        mFeedbackModes[i] = scattered[i];

        // Real-time modal energy envelope tracking for Phosphor CRT Scope
        const float absVal = std::abs(modeOutputs[i]);
        mModalEnergies[i] = flushDenormal(0.992f * mModalEnergies[i] + 0.008f * absVal);
    }

    // 3. Golden-Ratio Angular Stereo Summation
    float sumL = 0.0f;
    float sumR = 0.0f;
    for (size_t i = 0; i < kNumModes; ++i) {
        sumL += modeOutputs[i] * mPanL[i];
        sumR += modeOutputs[i] * mPanR[i];
    }

    outL = flushDenormal(sumL);
    outR = flushDenormal(sumR);
}

void ModalResonatorMatrix::processBlock(const float* exciterBuffer, float* outL, float* outR, int numSamples) noexcept {
    if (exciterBuffer == nullptr || outL == nullptr || outR == nullptr || numSamples <= 0) {
        return;
    }
    for (int i = 0; i < numSamples; ++i) {
        processSample(exciterBuffer[i], outL[i], outR[i]);
    }
}

} // namespace braun::mr16
