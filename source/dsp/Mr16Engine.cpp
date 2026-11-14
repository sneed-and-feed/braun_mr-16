#include "Mr16Engine.h"

namespace braun::mr16 {

Mr16Engine::Mr16Engine() noexcept {
    applyParametersToDsp();
}

std::vector<PresetDefinition> Mr16Engine::getFactoryPresets() {
    std::vector<PresetDefinition> presets;
    presets.reserve(16);

    auto makePreset = [](const char* id, const char* name, const char* category, const char* desc,
                         int exciterType, float strikeHard, float strikeVel, float fricForce, float fricSpeed,
                         float vactrolSag, float poissonHz, bool euclOn, int euclPulses, int euclSteps,
                         ManifoldType manifold, float f0Hz, float dampingSec, MaterialType material,
                         float coupling, float spread, float modalQ,
                         float lorenzRate, float lorenzChaos, float lorenzFreqModPct, float lorenzQModPct,
                         bool chorusOn, float chorusRate, float chorusDepthMs, float chorusDim, float chorusMixPct,
                         float panSpreadPct, float lpgCutoffHz, float drivePct, float trimDb, float dryWetPct,
                         int displayMode) -> PresetDefinition {
        Mr16Parameters p;
        p.exciterMode           = exciterType;
        p.exciterStrikeHardness = strikeHard;
        p.exciterStrikeVelocity = strikeVel;
        p.exciterBowPressure    = (exciterType == 1) ? fricForce : 0.0f;
        p.exciterBowVelocity    = (exciterType == 1) ? fricSpeed : 0.0f;
        p.uiFrictionForce       = fricForce;
        p.uiFrictionSpeed       = fricSpeed;
        p.vactrolSagEnable      = (vactrolSag > 0.02f);
        p.vactrolSagAmount      = vactrolSag;
        p.poissonEnable         = (poissonHz > 0.01f);
        p.poissonEpm            = poissonHz * 60.0f;
        p.euclideanEnable       = euclOn;
        p.euclideanPulses       = euclPulses;
        p.euclideanSteps        = euclSteps;

        p.manifold              = manifold;
        p.manifoldMorph         = 0.0f;
        p.fundamentalHz         = f0Hz;
        p.decayScale            = dampingSec;
        p.material              = material;
        p.couplingDepth         = coupling;
        p.overtoneSpread        = spread;
        p.modalQScale           = modalQ / 85.0f;

        p.chaosRateHz           = lorenzRate;
        p.chaosDepth            = lorenzChaos;
        p.chaosDetuneCents      = (lorenzFreqModPct * 0.01f) * 300.0f;
        p.chaosQMod             = lorenzQModPct * 0.01f;
        p.lorenzMorphSteer      = 0.0f;

        p.chorusDimensionMode   = DimensionMode::Manual;
        p.chorusEnable          = chorusOn;
        p.chorusRateHz          = chorusRate;
        p.chorusDepthMs         = chorusDepthMs;
        p.chorusDimension       = chorusDim;
        p.chorusMix             = chorusMixPct * 0.01f;

        p.stereoWidth           = panSpreadPct * 0.01f;
        p.vactrolLpgCutoff      = lpgCutoffHz;
        p.driveSaturation       = drivePct * 0.01f;
        p.masterVolumeDb        = trimDb;
        p.dryWetMix             = dryWetPct * 0.01f;
        p.softLimiterEnable     = true;
        p.outputMute            = false;
        p.displayMode           = displayMode;

        return { id, name, category, desc, p };
    };

    // 1. CALIBRATED DEFAULT
    presets.push_back(makePreset(
        "DEFAULT_CONCERT_CHLADNI", "CALIBRATED DEFAULT", "Studio Benchmark",
        "Calibrated 2D square steel plate struck by a medium-density felt mallet with balanced Householder inter-modal coupling and subtle BBD spatial depth.",
        0, 0.65f, 0.80f, 0.50f, 0.40f, 0.35f, 0.0f, false, 4, 16,
        ManifoldType::ChladniPlate, 220.0f, 1.80f, MaterialType::Steel, 0.40f, 1.00f, 85.0f,
        0.85f, 0.50f, 15.0f, 20.0f,
        true, 0.65f, 1.40f, 0.75f, 45.0f,
        85.0f, 14000.0f, 25.0f, 0.0f, 65.0f, 0));

    // 2. MARIMBA ROSEWOOD BAR
    presets.push_back(makePreset(
        "MARIMBA_ROSEWOOD_BAR", "MARIMBA ROSEWOOD BAR", "Mallet & Wood",
        "Stiff wooden beam resonator with Euler-Bernoulli overtone dispersion, soft rubber mallet attack, and warm acoustic absorption.",
        0, 0.35f, 0.85f, 0.30f, 0.20f, 0.25f, 0.0f, false, 4, 16,
        ManifoldType::StiffBeam, 174.61f, 0.90f, MaterialType::Wood, 0.20f, 1.00f, 45.0f,
        0.40f, 0.30f, 5.0f, 10.0f,
        true, 0.40f, 0.80f, 0.50f, 25.0f,
        70.0f, 8500.0f, 15.0f, 0.0f, 70.0f, 2));

    // 3. CRYSTAL WINE GLASS
    presets.push_back(makePreset(
        "CRYSTAL_WINE_GLASS", "CRYSTAL WINE GLASS", "Friction & Glass",
        "High-Q borosilicate glass rim excited by continuous stick-slip friction, producing a pure ethereal crystal tone.",
        1, 0.70f, 0.60f, 0.62f, 0.55f, 0.40f, 0.0f, false, 2, 16,
        ManifoldType::ChladniPlate, 587.33f, 4.50f, MaterialType::Glass, 0.35f, 0.85f, 220.0f,
        0.30f, 0.40f, 8.0f, 15.0f,
        true, 0.50f, 1.20f, 0.80f, 40.0f,
        90.0f, 16000.0f, 10.0f, -1.0f, 85.0f, 0));

    // 4. TIBETAN BRONZE BELL
    presets.push_back(makePreset(
        "TIBETAN_BRONZE_BELL", "TIBETAN BRONZE BELL", "Metallic Plates",
        "Dense bronze bell plate excited by heavy metal striker, rich non-linear Householder energy exchange, and long shimmer ring-down.",
        0, 0.85f, 0.95f, 0.50f, 0.40f, 0.50f, 0.0f, false, 3, 16,
        ManifoldType::ChladniPlate, 130.81f, 5.20f, MaterialType::Brass, 0.78f, 1.25f, 160.0f,
        0.60f, 0.65f, 18.0f, 35.0f,
        true, 0.35f, 2.20f, 0.90f, 50.0f,
        95.0f, 12000.0f, 35.0f, -2.0f, 80.0f, 0));

    // 5. CATHEDRAL TUBE DRONE
    presets.push_back(makePreset(
        "CATHEDRAL_TUBE_DRONE", "CATHEDRAL TUBE DRONE", "Acoustic Cavity",
        "Negative-curvature Poincaré horn resonator driven by air jet exciter with optical vactrol sag, creating a deep cathedral organ resonance.",
        2, 0.40f, 0.75f, 0.40f, 0.30f, 0.70f, 0.0f, false, 2, 16,
        ManifoldType::PoincareHorn, 65.41f, 6.00f, MaterialType::Brass, 0.50f, 1.10f, 110.0f,
        0.25f, 0.45f, 10.0f, 25.0f,
        true, 0.28f, 1.80f, 0.85f, 45.0f,
        85.0f, 4500.0f, 20.0f, 0.0f, 90.0f, 2));

    // 6. VOCAL FORMANT CHOIR
    presets.push_back(makePreset(
        "VOCAL_FORMANT_CHOIR", "VOCAL FORMANT CHOIR", "Vocal Tract",
        "16-pole vocal tract cavity morphing between open vowels [a, o, u] modulated by subtle chaotic Lorenz drift and Dimension chorus.",
        2, 0.50f, 0.80f, 0.45f, 0.40f, 0.55f, 0.0f, false, 4, 16,
        ManifoldType::VocalFormant, 110.0f, 3.20f, MaterialType::Nylon, 0.30f, 1.00f, 70.0f,
        0.70f, 0.55f, 25.0f, 30.0f,
        true, 0.55f, 2.00f, 0.85f, 55.0f,
        80.0f, 7500.0f, 15.0f, 0.0f, 85.0f, 1));

    // 7. POISSON ZINC ROOF
    presets.push_back(makePreset(
        "POISSON_ZINC_ROOF", "POISSON ZINC ROOF", "Kinetic Stochastic",
        "Stochastic Poisson rain impulses impacting a thin biharmonic sheet metal plate, spatialized across the golden stereo panorama.",
        0, 0.75f, 0.70f, 0.20f, 0.20f, 0.20f, 14.5f, false, 4, 16,
        ManifoldType::ChladniPlate, 330.0f, 1.20f, MaterialType::Steel, 0.45f, 1.15f, 95.0f,
        0.90f, 0.40f, 12.0f, 15.0f,
        true, 0.45f, 1.00f, 0.60f, 30.0f,
        95.0f, 15000.0f, 20.0f, 0.0f, 75.0f, 0));

    // 8. EUCLIDEAN METALLOPHONE
    presets.push_back(makePreset(
        "EUCLIDEAN_METALLOPHONE", "EUCLIDEAN METALLOPHONE", "Polyrhythmic",
        "Clocked 5/16 Euclidean strike pattern driving interlocking brass chime modes, producing hypnotic generative gamelan metallophone cycles.",
        0, 0.80f, 0.85f, 0.30f, 0.30f, 0.30f, 0.0f, true, 5, 16,
        ManifoldType::StiffBeam, 329.63f, 2.40f, MaterialType::Brass, 0.55f, 1.05f, 125.0f,
        0.50f, 0.35f, 10.0f, 15.0f,
        true, 0.60f, 1.50f, 0.70f, 35.0f,
        85.0f, 13500.0f, 22.0f, 0.0f, 70.0f, 2));

    // 9. DIMENSION CELESTE
    presets.push_back(makePreset(
        "DIMENSION_CELESTE", "DIMENSION CELESTE", "Spatial Chorus",
        "Pure acoustic glockenspiel bells routed through the Roland Dimension D tri-phase chorus with ultra-wide decorrelation.",
        0, 0.90f, 0.85f, 0.20f, 0.20f, 0.25f, 0.0f, false, 4, 16,
        ManifoldType::StiffBeam, 659.25f, 3.80f, MaterialType::Steel, 0.35f, 1.00f, 180.0f,
        0.45f, 0.30f, 8.0f, 12.0f,
        true, 0.45f, 2.40f, 1.00f, 65.0f,
        90.0f, 18000.0f, 12.0f, -1.0f, 75.0f, 0));

    // 10. LORENZ CHAOTIC ORBIT
    presets.push_back(makePreset(
        "LORENZ_CHAOTIC_ORBIT", "LORENZ CHAOTIC ORBIT", "Chaotic Morph",
        "Metallic plate gong with center frequencies continuously warped along the 3D Lorenz attractor butterfly trajectory.",
        0, 0.70f, 0.90f, 0.40f, 0.40f, 0.40f, 0.0f, false, 3, 16,
        ManifoldType::ChladniPlate, 196.0f, 4.00f, MaterialType::Steel, 0.60f, 1.20f, 140.0f,
        2.40f, 0.85f, 45.0f, 60.0f,
        true, 0.70f, 1.60f, 0.80f, 40.0f,
        90.0f, 12500.0f, 30.0f, 0.0f, 85.0f, 1));

    // 11. BOWED TITANIUM SHEET
    presets.push_back(makePreset(
        "BOWED_TITANIUM_SHEET", "BOWED TITANIUM SHEET", "Bowed Friction",
        "Continuous Karnopp friction bow exciting an ultra-stiff titanium plate, creating dark metallic drone textures.",
        1, 0.60f, 0.70f, 0.75f, 0.65f, 0.50f, 0.0f, false, 2, 16,
        ManifoldType::ChladniPlate, 146.83f, 3.80f, MaterialType::Steel, 0.50f, 1.10f, 150.0f,
        0.60f, 0.50f, 15.0f, 25.0f,
        true, 0.35f, 1.80f, 0.75f, 45.0f,
        85.0f, 9500.0f, 25.0f, -1.0f, 90.0f, 0));

    // 12. HYPERBOLIC SUB RESONATOR
    presets.push_back(makePreset(
        "HYPERBOLIC_SUB_RESONATOR", "HYPERBOLIC SUB RESONATOR", "Sub Bass",
        "Sub-bass physical horn cavity tuned to 43.6 Hz (F1) with tight Buchla 292 LPG dynamics for clean, deep low-end punch.",
        0, 0.30f, 0.90f, 0.20f, 0.20f, 0.45f, 0.0f, false, 4, 16,
        ManifoldType::PoincareHorn, 43.65f, 2.50f, MaterialType::Wood, 0.30f, 0.90f, 55.0f,
        0.30f, 0.30f, 5.0f, 10.0f,
        false, 0.30f, 1.00f, 0.40f, 0.0f,
        40.0f, 450.0f, 15.0f, 1.0f, 70.0f, 2));

    // 13. AFRICAN BALAFON
    presets.push_back(makePreset(
        "AFRICAN_BALAFON", "AFRICAN BALAFON", "World Acoustic",
        "Traditional wooden slat marimba with simulated buzz membrane overtones and natural gourd cavity damping.",
        0, 0.75f, 0.85f, 0.30f, 0.30f, 0.30f, 0.0f, false, 5, 16,
        ManifoldType::StiffBeam, 196.0f, 1.40f, MaterialType::Wood, 0.45f, 1.15f, 50.0f,
        0.50f, 0.40f, 12.0f, 20.0f,
        true, 0.50f, 1.20f, 0.65f, 30.0f,
        75.0f, 11000.0f, 28.0f, 0.0f, 75.0f, 2));

    // 14. SOLAR WIND RESONANCE
    presets.push_back(makePreset(
        "SOLAR_WIND_RESONANCE", "SOLAR WIND RESONANCE", "Ambient Drone",
        "High-density Poisson micro-transients driving a complex network of lightly damped nylon and brass resonators.",
        2, 0.45f, 0.65f, 0.40f, 0.40f, 0.65f, 32.0f, false, 3, 16,
        ManifoldType::PoincareHorn, 220.0f, 4.20f, MaterialType::Nylon, 0.40f, 1.10f, 90.0f,
        0.35f, 0.55f, 18.0f, 30.0f,
        true, 0.40f, 2.00f, 0.85f, 50.0f,
        90.0f, 8500.0f, 18.0f, 0.0f, 80.0f, 1));

    // 15. FROSTED GLASS ARPEGGIO
    presets.push_back(makePreset(
        "FROSTED_GLASS_ARPEGGIO", "FROSTED GLASS ARPEGGIO", "Generative",
        "Interlocking 7/12 Euclidean strike rhythm exciting high-frequency crystal glass modes with spatial BBD chorus wash.",
        0, 0.85f, 0.80f, 0.30f, 0.30f, 0.30f, 0.0f, true, 7, 12,
        ManifoldType::ChladniPlate, 880.0f, 3.60f, MaterialType::Glass, 0.40f, 0.90f, 210.0f,
        0.65f, 0.45f, 14.0f, 20.0f,
        true, 0.75f, 1.50f, 0.80f, 50.0f,
        90.0f, 17000.0f, 14.0f, -1.0f, 75.0f, 0));

    // 16. INFINITE MODAL SINGULARITY
    presets.push_back(makePreset(
        "INFINITE_MODAL_SINGULARITY", "INFINITE MODAL SINGULARITY", "Extreme Self-Oscillation",
        "Maximum Householder feedback coupling (100%) and zero damping, held in bounded, singing self-oscillation by the Hermite saturator.",
        0, 0.70f, 0.80f, 0.50f, 0.50f, 0.40f, 0.0f, false, 1, 16,
        ManifoldType::ChladniPlate, 220.0f, 10.0f, MaterialType::Steel, 0.98f, 1.00f, 250.0f,
        1.10f, 0.75f, 20.0f, 40.0f,
        true, 0.40f, 2.20f, 0.95f, 50.0f,
        95.0f, 14000.0f, 65.0f, -3.0f, 95.0f, 1));

    return presets;
}

void Mr16Engine::prepare(double sampleRate, int maxBlockSize) noexcept {
    mSampleRate = (isFinite(sampleRate) && sampleRate > 100.0) ? static_cast<float>(sampleRate) : 48000.0f;
    mMaxBlockSize = std::max(1, maxBlockSize);

    mExciter.prepare(sampleRate);
    mModalMatrix.prepare(sampleRate);
    mLorenz.prepare(sampleRate);
    mChorus.prepare(sampleRate);
    mVactrolGate.prepare(sampleRate);
    mVactrolGate.setMode(VactrolMode::FilterOnly);
    mVactrolGate.setCutoffRange(40.0f, mParams.vactrolLpgCutoff);
    mVactrolGate.setControlVoltage(1.0f);

    mVolumeSmoother.setSampleRate(mSampleRate);
    mVolumeSmoother.setTimeConstant(0.020f); // 20 ms click-free volume slewing
    mBowBodyFilter.setCutoff(mSampleRate, 900.0f);

    applyParametersToDsp();
    reset();
}

void Mr16Engine::reset() noexcept {
    mExciter.reset();
    mModalMatrix.reset();
    mLastParamFundamentalHz = mParams.fundamentalHz;
    mModalMatrix.setFundamentalHz(mParams.fundamentalHz);
    mLorenz.reset();
    mChorus.reset();
    mVactrolGate.reset();
    mVactrolGate.setMode(VactrolMode::FilterOnly);
    mVactrolGate.setCutoffRange(40.0f, mParams.vactrolLpgCutoff);
    mVactrolGate.setControlVoltage(1.0f);

    const float targetGain = mParams.outputMute ? 0.0f : dbToGain(mParams.masterVolumeDb);
    mVolumeSmoother.reset(targetGain);
    mBowBodyFilter.reset();
    mBowBodyVelocity = 0.0f;

    mQueueWriteHead.store(0, std::memory_order_relaxed);
    mQueueReadHead = 0;

    mScopeRingL.fill(0.0f);
    mScopeRingR.fill(0.0f);
    mScopeWritePos = 0;

    mInputRmsL = 0.0f;
    mInputRmsR = 0.0f;
    mOutputRmsL = 0.0f;
    mOutputRmsR = 0.0f;

    mVisualizerTelemetry = VisualizerFrame {};
}

void Mr16Engine::setParameters(const Mr16Parameters& params) noexcept {
    mParams = params;
    applyParametersToDsp();
}

void Mr16Engine::applyParametersToDsp() noexcept {
    // Deck 01: Exciter
    mExciter.setFriction(mParams.exciterBowVelocity, mParams.exciterBowPressure);
    mExciter.setPoissonRain(mParams.poissonEnable, mParams.poissonEpm, mParams.poissonHumanize);
    mExciter.setEuclidean(mParams.euclideanEnable, mParams.euclideanPulses, mParams.euclideanSteps, mParams.euclideanBpm);
    mExciter.setExternalInput(mParams.externalAudioEnable, mParams.externalSensitivity, mParams.externalDirectMix);
    mExciter.setMicrotonalScale(mParams.microtonalScale);
    mExciter.setChimeRootHz(mParams.chimeRootHz);

    // Deck 02: Modal Resonator Matrix
    // Only re-tune from mParams.fundamentalHz when the base parameter/pitch-bend actually changes,
    // preserving active Chime Strip / MIDI Note-On / Euclidean step fundamentals across audio blocks.
    if (std::abs(mParams.fundamentalHz - mLastParamFundamentalHz) > 1.0e-4f) {
        mLastParamFundamentalHz = mParams.fundamentalHz;
        mModalMatrix.setFundamentalHz(mParams.fundamentalHz);
    }
    mModalMatrix.setManifold(mParams.manifold, mParams.manifoldMorph);
    mModalMatrix.setMaterial(mParams.material);
    mModalMatrix.setDecayScale(mParams.decayScale);
    mModalMatrix.setCouplingDepth(mParams.couplingDepth);
    mModalMatrix.setStereoWidth(mParams.stereoWidth);
    mModalMatrix.setQScale(mParams.modalQScale);
    mModalMatrix.setOvertoneSpread(mParams.overtoneSpread);
    mModalMatrix.setBipolarSpread(mParams.bipolarSpread);

    // Deck 03: 3D Lorenz Attractor
    mLorenz.setRateHz(mParams.chaosRateHz);
    mLorenz.setDepth(mParams.chaosDepth);
    mLorenz.setDetuneMaxCents(mParams.chaosDetuneCents);
    mLorenz.setQModDepth(mParams.chaosQMod);

    // Deck 04: Spatial Chorus
    mChorus.setEnabled(mParams.chorusEnable);
    if (mParams.chorusDimensionMode == DimensionMode::Manual) {
        mChorus.setParameters(mParams.chorusRateHz, mParams.chorusDepthMs, mParams.chorusMix);
    } else {
        mChorus.setMode(mParams.chorusDimensionMode);
    }
    mChorus.setDimensionSpread(mParams.chorusDimension);

    // Deck 05: Dynamics & LPG (Tone shaper only, avoiding amplitude choke gating)
    mVactrolGate.setMode(VactrolMode::FilterOnly);
    mVactrolGate.setCutoffRange(40.0f, mParams.vactrolLpgCutoff);
    mVactrolGate.setControlVoltage(1.0f);
    mVactrolGate.setDynamicSag(mParams.vactrolSagAmount);
    mVactrolGate.setDecayTime(mParams.vactrolDecaySec);
    mSaturator.setKneeAndCeiling(mParams.saturatorKnee, mParams.saturatorCeiling);

    // Deck 07: Master
    const float targetGain = mParams.outputMute ? 0.0f : dbToGain(mParams.masterVolumeDb);
    mVolumeSmoother.setTarget(targetGain);
}

bool Mr16Engine::enqueueTriggerStrike(float velocity, float hardness) noexcept {
    const size_t writeIdx = mQueueWriteHead.load(std::memory_order_relaxed);
    const size_t nextIdx = (writeIdx + 1) & kEventQueueMask;
    if (nextIdx == mQueueReadHead) {
        return false; // Queue full
    }
    mEventQueue[writeIdx] = { TriggerEvent::Type::Strike, 0, velocity, hardness, 0.0f };
    mQueueWriteHead.store(nextIdx, std::memory_order_release);
    return true;
}

bool Mr16Engine::enqueueTriggerButton(int buttonIndex, float velocity) noexcept {
    const size_t writeIdx = mQueueWriteHead.load(std::memory_order_relaxed);
    const size_t nextIdx = (writeIdx + 1) & kEventQueueMask;
    if (nextIdx == mQueueReadHead) {
        return false;
    }
    mEventQueue[writeIdx] = { TriggerEvent::Type::StrikeButton, buttonIndex, velocity, 0.65f, 0.0f };
    mQueueWriteHead.store(nextIdx, std::memory_order_release);
    return true;
}

bool Mr16Engine::enqueueTriggerChime(int keyIndex, float velocity, float freqHz) noexcept {
    const size_t writeIdx = mQueueWriteHead.load(std::memory_order_relaxed);
    const size_t nextIdx = (writeIdx + 1) & kEventQueueMask;
    if (nextIdx == mQueueReadHead) {
        return false;
    }
    mEventQueue[writeIdx] = { TriggerEvent::Type::ChimeKey, keyIndex, velocity, 0.65f, freqHz };
    mQueueWriteHead.store(nextIdx, std::memory_order_release);
    return true;
}

bool Mr16Engine::enqueueMidiNoteOn(int midiNote, float velocity) noexcept {
    const size_t writeIdx = mQueueWriteHead.load(std::memory_order_relaxed);
    const size_t nextIdx = (writeIdx + 1) & kEventQueueMask;
    if (nextIdx == mQueueReadHead) {
        return false;
    }
    const int clampedNote = std::clamp(midiNote, 0, 127);
    const float freq = midiNoteToHz(static_cast<float>(clampedNote));
    mEventQueue[writeIdx] = { TriggerEvent::Type::NoteOn, clampedNote, velocity, 0.65f, freq };
    mQueueWriteHead.store(nextIdx, std::memory_order_release);
    return true;
}

void Mr16Engine::drainEventQueue() noexcept {
    const size_t writeIdx = mQueueWriteHead.load(std::memory_order_acquire);
    while (mQueueReadHead != writeIdx) {
        const auto& ev = mEventQueue[mQueueReadHead];
        switch (ev.type) {
            case TriggerEvent::Type::Strike:
                mExciter.triggerStrike(ev.velocity, ev.hardness);
                mVactrolGate.trigger(ev.velocity);
                break;
            case TriggerEvent::Type::StrikeButton:
                mExciter.triggerStrikeButton(ev.index, ev.velocity);
                mVactrolGate.trigger(ev.velocity);
                break;
            case TriggerEvent::Type::ChimeKey:
                mExciter.triggerChimeKey(ev.index, ev.velocity, ev.freqHz);
                mVactrolGate.trigger(ev.velocity);
                break;
            case TriggerEvent::Type::NoteOn:
                mModalMatrix.setFundamentalHz(ev.freqHz);
                mExciter.triggerStrike(ev.velocity, 0.50f + 0.40f * ev.velocity);
                mVactrolGate.trigger(ev.velocity);
                break;
        }
        mQueueReadHead = (mQueueReadHead + 1) & kEventQueueMask;
    }
}

void Mr16Engine::processBlock(const float* inL, const float* inR,
                              float* outL, float* outR, int numSamples) noexcept {
    if (outL == nullptr || outR == nullptr || numSamples <= 0) {
        return;
    }

    // MANDATORY AUDIT DIRECTIVE: Instantiate ScopedNoDenormals EXCLUSIVELY ONCE at the outer block entry point!
    // Never instantiate inside per-sample loops.
    ScopedNoDenormals noDenormals;

    // Drain lock-free trigger events from GUI / MIDI
    drainEventQueue();

    // Check for acoustic freeze hold
    if (mParams.freezeHold) {
        mModalMatrix.setDecayScale(20.0f);
    } else {
        mModalMatrix.setDecayScale(mParams.decayScale);
    }

    float blockInRmsL = 0.0f;
    float blockInRmsR = 0.0f;
    float blockOutRmsL = 0.0f;
    float blockOutRmsR = 0.0f;
    float effectiveManifoldMorph = mParams.manifoldMorph;

    for (int i = 0; i < numSamples; ++i) {
        const float sampleInL = (inL != nullptr) ? flushDenormal(inL[i]) : 0.0f;
        const float sampleInR = (inR != nullptr) ? flushDenormal(inR[i]) : 0.0f;
        const float monoExtIn = 0.5f * (sampleInL + sampleInR);

        blockInRmsL += sampleInL * sampleInL;
        blockInRmsR += sampleInR * sampleInR;

        // --------------------------------------------------------------------
        // Step 1: Advance 3D Lorenz Attractor (RK4) & Modulate Modal Matrix
        // --------------------------------------------------------------------
        mLorenz.step();
        if (mParams.lorenzMorphSteer > 1.0e-4f && (i & 15) == 0) {
            const float steerDelta = 0.5f * (0.50f * mLorenz.getNormalizedX()
                                           + 0.35f * mLorenz.getNormalizedY()
                                           + 0.15f * mLorenz.getNormalizedZ());
            effectiveManifoldMorph = std::clamp(mParams.manifoldMorph + mParams.lorenzMorphSteer * steerDelta, 0.0f, 1.0f);
            mModalMatrix.setManifold(mParams.manifold, effectiveManifoldMorph);
        }
        mModalMatrix.applyModulation(
            mLorenz.getFrequencyMultipliers(),
            mLorenz.getQSpreadFactor(),
            mLorenz.getSpatialPanOffset()
        );

        // Check if an exciter chime or sequencer trigger tuned the matrix fundamental
        float newChimeHz = 0.0f;
        if (mExciter.hasNewChimeTrigger(newChimeHz)) {
            mModalMatrix.setFundamentalHz(newChimeHz);
        }

        // --------------------------------------------------------------------
        // Step 2: Kinetic Exciter Engine with Resonator Body Velocity Feedback (Deck 01)
        // --------------------------------------------------------------------
        const float exciterOut = mExciter.processSample(monoExtIn, mBowBodyVelocity);

        // --------------------------------------------------------------------
        // Step 3: 16-Pole Modal Resonator Matrix with Householder Scattering (Deck 02)
        // --------------------------------------------------------------------
        float resL = 0.0f;
        float resR = 0.0f;
        mModalMatrix.processSample(exciterOut, resL, resR);
        mBowBodyVelocity = flushDenormal(std::clamp(mBowBodyFilter.process(0.5f * (resL + resR)) * 0.12f, -0.25f, 0.25f));

        // --------------------------------------------------------------------
        // Step 4: Buchla 292 Dynamic Vactrol LPG Resonant Sag (Deck 05)
        // --------------------------------------------------------------------
        float lpgL = resL;
        float lpgR = resR;
        if (mParams.vactrolSagEnable) {
            mVactrolGate.processStereo(resL, resR, lpgL, lpgR);
        }

        // --------------------------------------------------------------------
        // Step 5: Master Volume Slewing & Bounded C1 Hermite Soft Saturator (Deck 05/07)
        // --------------------------------------------------------------------
        const float currentGain = mVolumeSmoother.next();
        const float driveGain = 1.0f + mParams.driveSaturation * 0.8f;
        const float scaledL = lpgL * currentGain * driveGain;
        const float scaledR = lpgR * currentGain * driveGain;

        const float satL = mParams.softLimiterEnable
            ? mSaturator.processSample(scaledL)
            : (isFinite(scaledL) ? flushDenormal(scaledL) : 0.0f);
        const float satR = mParams.softLimiterEnable
            ? mSaturator.processSample(scaledR)
            : (isFinite(scaledR) ? flushDenormal(scaledR) : 0.0f);

        // --------------------------------------------------------------------
        // Step 6: Tri-Phase Spatial BBD Chorus & Master Bus Bounds (Deck 04/05/07)
        // --------------------------------------------------------------------
        float finalL = satL;
        float finalR = satR;
        if (mParams.chorusEnable) {
            mChorus.process(satL, satR, finalL, finalR);
        }

        // Master physical resonance saturation boundary check (bypassable soft knee; brickwall safety clamp remains active below)
        if (mParams.softLimiterEnable) {
            finalL = mSaturator.processSample(finalL);
            finalR = mSaturator.processSample(finalR);
        } else {
            finalL = isFinite(finalL) ? flushDenormal(finalL) : 0.0f;
            finalR = isFinite(finalR) ? flushDenormal(finalR) : 0.0f;
        }

        // Deck 05: Master Dry / Wet Mix
        if (mParams.externalAudioEnable && (inL != nullptr || inR != nullptr)) {
            const float dryMix = std::clamp(1.0f - mParams.dryWetMix, 0.0f, 1.0f);
            const float wetMix = std::clamp(mParams.dryWetMix, 0.0f, 1.0f);
            finalL = dryMix * sampleInL + wetMix * finalL;
            finalR = dryMix * sampleInR + wetMix * finalR;
        }

        // Master hard brickwall safety clamp guarding the master output bus (ceiling = 1.00 / 0.0 dBFS)
        finalL = std::clamp(finalL, -mParams.saturatorCeiling, +mParams.saturatorCeiling);
        finalR = std::clamp(finalR, -mParams.saturatorCeiling, +mParams.saturatorCeiling);

        outL[i] = finalL;
        outR[i] = finalR;

        mScopeRingL[mScopeWritePos] = finalL;
        mScopeRingR[mScopeWritePos] = finalR;
        mScopeWritePos = (mScopeWritePos + 1) & 255u;

        blockOutRmsL += finalL * finalL;
        blockOutRmsR += finalR * finalR;
    }

    // Update RMS and telemetry
    const float invN = 1.0f / static_cast<float>(numSamples);
    mInputRmsL  = flushDenormal(std::sqrt(std::max(0.0f, blockInRmsL * invN)));
    mInputRmsR  = flushDenormal(std::sqrt(std::max(0.0f, blockInRmsR * invN)));
    mOutputRmsL = flushDenormal(std::sqrt(std::max(0.0f, blockOutRmsL * invN)));
    mOutputRmsR = flushDenormal(std::sqrt(std::max(0.0f, blockOutRmsR * invN)));

    // Populate Visualizer Telemetry for Deck 06 Phosphor CRT Scope
    mVisualizerTelemetry.modalEnergies        = mModalMatrix.getModalEnergies();
    mVisualizerTelemetry.modalFrequencies     = mModalMatrix.getCurrentFrequencies();
    mVisualizerTelemetry.lorenzX              = mLorenz.getX();
    mVisualizerTelemetry.lorenzY              = mLorenz.getY();
    mVisualizerTelemetry.lorenzZ              = mLorenz.getZ();
    mVisualizerTelemetry.lorenzNormX          = mLorenz.getNormalizedX();
    mVisualizerTelemetry.lorenzNormY          = mLorenz.getNormalizedY();
    mVisualizerTelemetry.lorenzNormZ          = mLorenz.getNormalizedZ();
    mVisualizerTelemetry.chladniFundamentalHz = mModalMatrix.getFundamentalHz();
    mVisualizerTelemetry.manifoldMorph        = effectiveManifoldMorph;
    mVisualizerTelemetry.manifoldType         = static_cast<int>(mParams.manifold);
    mVisualizerTelemetry.inputRmsL            = mInputRmsL;
    mVisualizerTelemetry.inputRmsR            = mInputRmsR;
    mVisualizerTelemetry.outputRmsL           = mOutputRmsL;
    mVisualizerTelemetry.outputRmsR           = mOutputRmsR;
    mVisualizerTelemetry.exciterActivity      = mExciter.getExciterActivity();
    mVisualizerTelemetry.inContact            = mExciter.isMalletInContact();
    for (size_t k = 0; k < 256; ++k) {
        const size_t idx = (mScopeWritePos + k) & 255u;
        mVisualizerTelemetry.scopeSamplesL[k] = mScopeRingL[idx];
        mVisualizerTelemetry.scopeSamplesR[k] = mScopeRingR[idx];
    }
}

void Mr16Engine::processBlock(float* bufferL, float* bufferR, int numSamples) noexcept {
    processBlock(bufferL, bufferR, bufferL, bufferR, numSamples);
}

VisualizerFrame Mr16Engine::getVisualizerFrame() const noexcept {
    return mVisualizerTelemetry;
}

} // namespace braun::mr16
