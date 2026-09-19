#include "Mr16Engine.h"

namespace braun::mr16 {

Mr16Engine::Mr16Engine() noexcept {
    applyParametersToDsp();
}

std::vector<PresetDefinition> Mr16Engine::getFactoryPresets() {
    std::vector<PresetDefinition> presets;
    presets.reserve(16);

    // 1. Chladni Zinc Plate
    {
        Mr16Parameters p;
        p.fundamentalHz = 220.0f;
        p.manifold = ManifoldType::ChladniPlate;
        p.material = MaterialType::Steel;
        p.decayScale = 1.25f;
        p.couplingDepth = 0.28f;
        p.stereoWidth = 0.85f;
        p.exciterStrikeVelocity = 0.80f;
        p.exciterStrikeHardness = 0.70f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode1;
        presets.push_back({
            "chladni_zinc_plate",
            "Chladni Zinc Plate",
            "Metallic",
            "Crisp biharmonic 2D standing waves on square zinc plate with felt striker",
            p
        });
    }

    // 2. Deep Marimba Bar
    {
        Mr16Parameters p;
        p.fundamentalHz = 110.0f;
        p.manifold = ManifoldType::StiffBeam;
        p.material = MaterialType::Wood;
        p.decayScale = 0.85f;
        p.couplingDepth = 0.15f;
        p.stereoWidth = 0.70f;
        p.exciterStrikeVelocity = 0.85f;
        p.exciterStrikeHardness = 0.40f;
        p.chorusEnable = false;
        presets.push_back({
            "deep_marimba_bar",
            "Deep Marimba Bar",
            "Acoustic",
            "Warm rosewood marimba bar with 4th and 10th harmonic arch undercut",
            p
        });
    }

    // 3. Hyperbolic Bell Flare
    {
        Mr16Parameters p;
        p.fundamentalHz = 330.0f;
        p.manifold = ManifoldType::PoincareHorn;
        p.material = MaterialType::Brass;
        p.decayScale = 2.40f;
        p.couplingDepth = 0.45f;
        p.stereoWidth = 1.0f;
        p.chaosRateHz = 0.35f;
        p.chaosDepth = 0.40f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode2;
        presets.push_back({
            "hyperbolic_bell_flare",
            "Hyperbolic Bell Flare",
            "Atmospheric",
            "Negative-curvature Poincare horn flare with airy high-dispersion ring",
            p
        });
    }

    // 4. Vocal Formant Choir
    {
        Mr16Parameters p;
        p.fundamentalHz = 146.83f; // D3
        p.manifold = ManifoldType::VocalFormant;
        p.material = MaterialType::Nylon;
        p.decayScale = 1.50f;
        p.couplingDepth = 0.35f;
        p.stereoWidth = 0.90f;
        p.chaosRateHz = 0.80f;
        p.chaosDepth = 0.60f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode3;
        presets.push_back({
            "vocal_formant_choir",
            "Vocal Formant Choir",
            "Vocalic",
            "Fant acoustic tube formant series with choral vowel resonance",
            p
        });
    }

    // 5. Monsoon Zinc Roof
    {
        Mr16Parameters p;
        p.fundamentalHz = 185.0f;
        p.manifold = ManifoldType::ChladniPlate;
        p.material = MaterialType::Steel;
        p.decayScale = 0.90f;
        p.couplingDepth = 0.20f;
        p.poissonEnable = true;
        p.poissonEpm = 85.0f;
        p.poissonHumanize = 0.70f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode1;
        presets.push_back({
            "monsoon_zinc_roof",
            "Monsoon Zinc Roof",
            "Generative",
            "Stochastic Poisson droplets striking a broad steel plate",
            p
        });
    }

    // 6. Euclidean Glass Chime
    {
        Mr16Parameters p;
        p.fundamentalHz = 523.25f; // C5
        p.manifold = ManifoldType::ChladniPlate;
        p.material = MaterialType::Glass;
        p.decayScale = 2.0f;
        p.couplingDepth = 0.30f;
        p.euclideanEnable = true;
        p.euclideanPulses = 5;
        p.euclideanSteps = 16;
        p.euclideanBpm = 112.0f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode2;
        presets.push_back({
            "euclidean_glass_chime",
            "Euclidean Glass Chime",
            "Rhythmic",
            "Geometric interlocking polyrhythms ringing crystalline glass modes",
            p
        });
    }

    // 7. Bowed Crystal Rod
    {
        Mr16Parameters p;
        p.fundamentalHz = 440.0f;
        p.manifold = ManifoldType::StiffBeam;
        p.material = MaterialType::Glass;
        p.decayScale = 3.20f;
        p.couplingDepth = 0.38f;
        p.exciterBowVelocity = 0.75f;
        p.exciterBowPressure = 0.85f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode2;
        presets.push_back({
            "bowed_crystal_rod",
            "Bowed Crystal Rod",
            "Continuous",
            "Sustained Karnopp friction bow exciting high-Q glass overtones",
            p
        });
    }

    // 8. Dimension Cathedral
    {
        Mr16Parameters p;
        p.fundamentalHz = 164.81f; // E3
        p.manifold = ManifoldType::PoincareHorn;
        p.material = MaterialType::Brass;
        p.decayScale = 4.50f;
        p.couplingDepth = 0.50f;
        p.stereoWidth = 1.20f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode4;
        p.chorusMix = 0.65f;
        presets.push_back({
            "dimension_cathedral",
            "Dimension Cathedral",
            "Spatial",
            "Expansive 16-pole acoustic body immersed in tri-phase BBD chorus",
            p
        });
    }

    // 9. Lorenz Strange Ribbon
    {
        Mr16Parameters p;
        p.fundamentalHz = 220.0f;
        p.manifold = ManifoldType::ChladniPlate;
        p.material = MaterialType::Steel;
        p.decayScale = 2.0f;
        p.couplingDepth = 0.40f;
        p.chaosRateHz = 2.40f;
        p.chaosDepth = 0.85f;
        p.chaosDetuneCents = 95.0f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode2;
        presets.push_back({
            "lorenz_strange_ribbon",
            "Lorenz Strange Ribbon",
            "Chaotic",
            "Bifurcating twin-lobe Lorenz chaos modulating modal detuning and Q spread",
            p
        });
    }

    // 10. Buchla 292 Pluck
    {
        Mr16Parameters p;
        p.fundamentalHz = 130.81f; // C3
        p.manifold = ManifoldType::StiffBeam;
        p.material = MaterialType::Wood;
        p.decayScale = 0.70f;
        p.couplingDepth = 0.20f;
        p.vactrolSagEnable = true;
        p.vactrolSagAmount = 0.85f;
        p.vactrolDecaySec = 0.045f;
        p.chorusEnable = false;
        presets.push_back({
            "buchla_292_pluck",
            "Buchla 292 Pluck",
            "Optoelectronic",
            "Snappy optical lowpass gate dynamic sag with rubbery acoustic pluck",
            p
        });
    }

    // 11. Anvil & Hammer
    {
        Mr16Parameters p;
        p.fundamentalHz = 392.0f; // G4
        p.manifold = ManifoldType::ChladniPlate;
        p.material = MaterialType::Steel;
        p.decayScale = 1.80f;
        p.couplingDepth = 0.35f;
        p.exciterStrikeVelocity = 1.0f;
        p.exciterStrikeHardness = 0.95f;
        p.chorusEnable = false;
        presets.push_back({
            "anvil_hammer_strike",
            "Anvil & Hammer",
            "Percussive",
            "Hertzian hard-metal contact strike on a massive steel resonator",
            p
        });
    }

    // 12. Nylon Resonant Body
    {
        Mr16Parameters p;
        p.fundamentalHz = 196.0f; // G3
        p.manifold = ManifoldType::StiffBeam;
        p.material = MaterialType::Nylon;
        p.decayScale = 0.60f;
        p.couplingDepth = 0.12f;
        p.stereoWidth = 0.75f;
        p.chorusEnable = false;
        presets.push_back({
            "nylon_resonant_body",
            "Nylon Resonant Body",
            "Warm",
            "Muted viscoelastic body with steep high-frequency damping",
            p
        });
    }

    // 13. Poincare Shepard Horn
    {
        Mr16Parameters p;
        p.fundamentalHz = 174.61f; // F3
        p.manifold = ManifoldType::PoincareHorn;
        p.material = MaterialType::Brass;
        p.decayScale = 3.80f;
        p.couplingDepth = 0.40f;
        p.stereoWidth = 1.10f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode3;
        presets.push_back({
            "poincare_shepard_horn",
            "Poincaré Shepard Horn",
            "Experimental",
            "Hyperbolic horn dispersion coupled through dimension chorus",
            p
        });
    }

    // 14. Spectral Formant Morph
    {
        Mr16Parameters p;
        p.fundamentalHz = 110.0f; // A2
        p.manifold = ManifoldType::VocalFormant;
        p.manifoldMorph = 0.45f;
        p.material = MaterialType::Wood;
        p.decayScale = 1.90f;
        p.couplingDepth = 0.30f;
        p.chaosRateHz = 0.65f;
        p.chaosDepth = 0.50f;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode2;
        presets.push_back({
            "spectral_formant_morph",
            "Spectral Formant Morph",
            "Morphing",
            "Lorenz attractor continuously morphing through vocalic acoustic tubes",
            p
        });
    }

    // 15. Stochastic Wind Chime
    {
        Mr16Parameters p;
        p.fundamentalHz = 587.33f; // D5
        p.manifold = ManifoldType::ChladniPlate;
        p.material = MaterialType::Brass;
        p.decayScale = 2.60f;
        p.couplingDepth = 0.35f;
        p.poissonEnable = true;
        p.poissonEpm = 32.0f;
        p.poissonHumanize = 0.60f;
        p.microtonalScale = MicrotonalScale::Slendro;
        p.chorusEnable = true;
        p.chorusDimensionMode = DimensionMode::Mode1;
        presets.push_back({
            "stochastic_wind_chime",
            "Stochastic Wind Chime",
            "Ambient",
            "Gentle Poisson stochastic breezes chiming brass modal rods",
            p
        });
    }

    // 16. Dieter Rams Minimalist
    {
        Mr16Parameters p;
        p.fundamentalHz = 220.0f;
        p.manifold = ManifoldType::ChladniPlate;
        p.material = MaterialType::Steel;
        p.decayScale = 1.0f;
        p.couplingDepth = 0.15f;
        p.stereoWidth = 0.80f;
        p.chorusEnable = false;
        p.exciterStrikeVelocity = 0.70f;
        p.exciterStrikeHardness = 0.60f;
        presets.push_back({
            "dieter_rams_minimalist",
            "Dieter Rams Minimalist",
            "Pure",
            "Calibrated 220Hz biharmonic fundamental in matte anthracite clarity",
            p
        });
    }

    return presets;
}

void Mr16Engine::prepare(double sampleRate, int maxBlockSize) noexcept {
    mSampleRate = (sampleRate > 100.0) ? static_cast<float>(sampleRate) : 48000.0f;
    mMaxBlockSize = std::clamp(maxBlockSize, 16, static_cast<int>(kMaxBlockSize));

    mExciter.prepare(sampleRate);
    mModalMatrix.prepare(sampleRate);
    mLorenz.prepare(sampleRate);
    mChorus.prepare(sampleRate);
    mVactrolGate.prepare(sampleRate);

    mVolumeSmoother.setSampleRate(mSampleRate);
    mVolumeSmoother.setTimeConstant(0.020f); // 20 ms click-free volume slewing

    applyParametersToDsp();
    reset();
}

void Mr16Engine::reset() noexcept {
    mExciter.reset();
    mModalMatrix.reset();
    mLorenz.reset();
    mChorus.reset();
    mVactrolGate.reset();

    const float targetGain = mParams.outputMute ? 0.0f : dbToGain(mParams.masterVolumeDb);
    mVolumeSmoother.reset(targetGain);

    mQueueWriteHead.store(0, std::memory_order_relaxed);
    mQueueReadHead = 0;

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
    mModalMatrix.setFundamentalHz(mParams.fundamentalHz);
    mModalMatrix.setManifold(mParams.manifold, mParams.manifoldMorph);
    mModalMatrix.setMaterial(mParams.material);
    mModalMatrix.setDecayScale(mParams.decayScale);
    mModalMatrix.setCouplingDepth(mParams.couplingDepth);
    mModalMatrix.setStereoWidth(mParams.stereoWidth);
    mModalMatrix.setQScale(mParams.modalQScale);
    mModalMatrix.setOvertoneSpread(mParams.overtoneSpread);

    // Deck 03: 3D Lorenz Attractor
    mLorenz.setRateHz(mParams.chaosRateHz);
    mLorenz.setDepth(mParams.chaosDepth);
    mLorenz.setDetuneMaxCents(mParams.chaosDetuneCents);

    // Deck 04: Spatial Chorus
    mChorus.setEnabled(mParams.chorusEnable);
    if (mParams.chorusDimensionMode == DimensionMode::Manual) {
        mChorus.setParameters(mParams.chorusRateHz, mParams.chorusDepthMs, mParams.chorusMix);
    } else {
        mChorus.setMode(mParams.chorusDimensionMode);
    }
    mChorus.setDimensionSpread(mParams.chorusDimension);

    // Deck 05: Dynamics & LPG
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

bool Mr16Engine::enqueueTriggerChime(int keyIndex, float velocity) noexcept {
    const size_t writeIdx = mQueueWriteHead.load(std::memory_order_relaxed);
    const size_t nextIdx = (writeIdx + 1) & kEventQueueMask;
    if (nextIdx == mQueueReadHead) {
        return false;
    }
    mEventQueue[writeIdx] = { TriggerEvent::Type::ChimeKey, keyIndex, velocity, 0.65f, 0.0f };
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
                mExciter.triggerChimeKey(ev.index, ev.velocity);
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
        // Step 2: Kinetic Exciter Engine (Deck 01)
        // --------------------------------------------------------------------
        const float exciterOut = mExciter.processSample(monoExtIn);

        // --------------------------------------------------------------------
        // Step 3: 16-Pole Modal Resonator Matrix with Householder Scattering (Deck 02)
        // --------------------------------------------------------------------
        float resL = 0.0f;
        float resR = 0.0f;
        mModalMatrix.processSample(exciterOut, resL, resR);

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
        const float driveGain = 1.0f + mParams.driveSaturation * 2.5f;
        const float scaledL = lpgL * currentGain * driveGain;
        const float scaledR = lpgR * currentGain * driveGain;

        const float satL = mSaturator.processSample(scaledL);
        const float satR = mSaturator.processSample(scaledR);

        // --------------------------------------------------------------------
        // Step 6: Tri-Phase Spatial BBD Chorus & Master Bus Bounds (Deck 04/05/07)
        // --------------------------------------------------------------------
        float finalL = satL;
        float finalR = satR;
        if (mParams.chorusEnable) {
            mChorus.process(satL, satR, finalL, finalR);
        }

        // Master physical resonance saturation boundary check
        finalL = mSaturator.processSample(finalL);
        finalR = mSaturator.processSample(finalR);

        // Deck 05: Master Dry / Wet Mix
        if (mParams.externalAudioEnable && (inL != nullptr || inR != nullptr)) {
            const float dryMix = std::clamp(1.0f - mParams.dryWetMix, 0.0f, 1.0f);
            const float wetMix = std::clamp(mParams.dryWetMix, 0.0f, 1.0f);
            finalL = dryMix * sampleInL + wetMix * finalL;
            finalR = dryMix * sampleInR + wetMix * finalR;
        }

        outL[i] = finalL;
        outR[i] = finalR;

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
    mVisualizerTelemetry.manifoldMorph        = mParams.manifoldMorph;
    mVisualizerTelemetry.manifoldType         = static_cast<int>(mParams.manifold);
    mVisualizerTelemetry.inputRmsL            = mInputRmsL;
    mVisualizerTelemetry.inputRmsR            = mInputRmsR;
    mVisualizerTelemetry.outputRmsL           = mOutputRmsL;
    mVisualizerTelemetry.outputRmsR           = mOutputRmsR;
    mVisualizerTelemetry.exciterActivity      = mExciter.getExciterActivity();
    mVisualizerTelemetry.inContact            = mExciter.isMalletInContact();
}

void Mr16Engine::processBlock(float* bufferL, float* bufferR, int numSamples) noexcept {
    processBlock(bufferL, bufferR, bufferL, bufferR, numSamples);
}

VisualizerFrame Mr16Engine::getVisualizerFrame() const noexcept {
    return mVisualizerTelemetry;
}

} // namespace braun::mr16
