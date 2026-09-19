#include "PluginProcessor.h"
#include "PluginEditor.h"

BRAUN_MR16AudioProcessor::BRAUN_MR16AudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", mr16::createParameterLayout())
{
    atomicPointers.initialize(apvts);
    atomicPointers.euclideanEnable = &mEuclideanEnable;
    recorderThread.startThread();
#if MR16_HAS_DSP_ENGINE
    mr16Engine.reset();
#endif
}

BRAUN_MR16AudioProcessor::~BRAUN_MR16AudioProcessor()
{
    stopRecording();
    recorderThread.stopThread(2000);
}

const juce::String BRAUN_MR16AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BRAUN_MR16AudioProcessor::acceptsMidi() const
{
    return true;
}

bool BRAUN_MR16AudioProcessor::producesMidi() const
{
    return false;
}

bool BRAUN_MR16AudioProcessor::isMidiEffect() const
{
    return false;
}

double BRAUN_MR16AudioProcessor::getTailLengthSeconds() const
{
    return 15.0;
}

int BRAUN_MR16AudioProcessor::getNumPrograms()
{
    return 16;
}

int BRAUN_MR16AudioProcessor::getCurrentProgram()
{
    return mCurrentProgram;
}

void BRAUN_MR16AudioProcessor::setPresetParameters(const mr16::Mr16ParameterSnapshot& s)
{
    auto setParamFloat = [this](const juce::ParameterID& pid, float val) {
        if (auto* param = apvts.getParameter(pid.getParamID()))
            param->setValueNotifyingHost(std::clamp(param->convertTo0to1(val), 0.0f, 1.0f));
    };
    auto setParamChoice = [this](const juce::ParameterID& pid, float choiceIndex) {
        if (auto* param = apvts.getParameter(pid.getParamID()))
            param->setValueNotifyingHost(std::clamp(param->convertTo0to1(choiceIndex), 0.0f, 1.0f));
    };
    auto setParamBool = [this](const juce::ParameterID& pid, bool val) {
        if (auto* param = apvts.getParameter(pid.getParamID()))
            param->setValueNotifyingHost(val ? 1.0f : 0.0f);
    };

    // Deck 01
    setParamChoice(mr16::ParamIDs::exciterType, static_cast<float>(mr16::indexFromExciterType(s.exciterType)));
    setParamFloat(mr16::ParamIDs::strikeHardness, s.strikeHardness);
    setParamFloat(mr16::ParamIDs::strikeVelocity, s.strikeVelocity);
    setParamFloat(mr16::ParamIDs::frictionForce, s.frictionForce);
    setParamFloat(mr16::ParamIDs::frictionSpeed, s.frictionSpeed);
    setParamFloat(mr16::ParamIDs::vactrolSag, s.vactrolSag);
    setParamFloat(mr16::ParamIDs::extInputGain, s.extInputGainDb);
    setParamFloat(mr16::ParamIDs::poissonDensity, s.poissonDensity);
    setParamFloat(mr16::ParamIDs::euclideanPulses, static_cast<float>(s.euclideanPulses));
    setParamFloat(mr16::ParamIDs::euclideanSteps, static_cast<float>(s.euclideanSteps));

    // Deck 02
    setParamChoice(mr16::ParamIDs::manifoldType, static_cast<float>(mr16::indexFromManifoldType(s.manifoldType)));
    setParamFloat(mr16::ParamIDs::modalFrequency, s.modalFrequency);
    setParamFloat(mr16::ParamIDs::modalDamping, s.modalDamping);
    setParamChoice(mr16::ParamIDs::materialProfile, static_cast<float>(mr16::indexFromMaterialProfile(s.materialProfile)));
    setParamFloat(mr16::ParamIDs::modalCoupling, s.modalCoupling);
    setParamFloat(mr16::ParamIDs::modalSpread, s.modalSpread);
    setParamFloat(mr16::ParamIDs::modalQ, s.modalQ);

    // Deck 03
    setParamFloat(mr16::ParamIDs::lorenzRate, s.lorenzRate);
    setParamFloat(mr16::ParamIDs::lorenzChaos, s.lorenzChaos);
    setParamFloat(mr16::ParamIDs::lorenzFreqMod, s.lorenzFreqMod);
    setParamFloat(mr16::ParamIDs::lorenzQMod, s.lorenzQMod);

    // Deck 04
    setParamBool(mr16::ParamIDs::chorusEnable, s.chorusEnable);
    setParamFloat(mr16::ParamIDs::chorusRateHz, s.chorusRateHz);
    setParamFloat(mr16::ParamIDs::chorusDepthMs, s.chorusDepthMs);
    setParamFloat(mr16::ParamIDs::chorusDimension, s.chorusDimension);
    setParamFloat(mr16::ParamIDs::chorusMix, s.chorusMix);

    // Deck 05
    setParamFloat(mr16::ParamIDs::goldenPanSpread, s.goldenPanSpread);
    setParamFloat(mr16::ParamIDs::vactrolLpgCutoff, s.vactrolLpgCutoff);
    setParamFloat(mr16::ParamIDs::driveSaturation, s.driveSaturationDb);
    setParamFloat(mr16::ParamIDs::masterTrimDb, s.masterTrimDb);
    setParamFloat(mr16::ParamIDs::dryWetMix, s.dryWetMix);
    setParamBool(mr16::ParamIDs::powerState, s.powerState);

    // Deck 06
    setParamChoice(mr16::ParamIDs::displayMode, static_cast<float>(mr16::indexFromDisplayMode(s.displayMode)));

    mEuclideanEnable.store(s.euclideanEnable, std::memory_order_relaxed);
}

#if MR16_HAS_DSP_ENGINE
void BRAUN_MR16AudioProcessor::setPresetParameters(const braun::mr16::Mr16Parameters& p)
{
    mr16::Mr16ParameterSnapshot s;

    // Map exciter
    if (p.poissonEnable) s.exciterType = mr16::ExciterType::Strike;
    else if (p.exciterBowVelocity > 0.05f) s.exciterType = mr16::ExciterType::Friction;
    else if (p.externalAudioEnable) s.exciterType = mr16::ExciterType::ExtIn;
    else s.exciterType = mr16::ExciterType::Strike;

    s.strikeVelocity = p.exciterStrikeVelocity;
    s.strikeHardness = p.exciterStrikeHardness;
    s.frictionForce  = p.exciterBowPressure;
    s.frictionSpeed  = p.exciterBowVelocity;
    s.vactrolSag     = p.vactrolSagAmount;
    s.extInputGainDb = (p.externalSensitivity > 0.0f) ? (20.0f * std::log10(p.externalSensitivity)) : 0.0f;
    s.poissonDensity = p.poissonEnable ? (p.poissonEpm / 60.0f) : 0.0f;
    s.euclideanEnable = p.euclideanEnable;
    s.euclideanPulses = p.euclideanPulses;
    s.euclideanSteps  = p.euclideanSteps;
    mEuclideanEnable.store(p.euclideanEnable, std::memory_order_relaxed);

    // Map modal matrix
    s.manifoldType   = static_cast<mr16::ManifoldType>(static_cast<int>(p.manifold));
    s.modalFrequency = p.fundamentalHz;
    s.modalDamping   = p.decayScale;
    s.materialProfile = static_cast<mr16::MaterialProfile>(static_cast<int>(p.material));
    s.modalCoupling  = p.couplingDepth;
    s.modalSpread    = 1.0f;
    s.modalQ         = 50.0f;

    // Map attractor
    s.lorenzRate    = p.chaosRateHz;
    s.lorenzChaos   = p.chaosDepth;
    s.lorenzFreqMod = std::clamp(p.chaosDetuneCents / 100.0f, 0.0f, 1.0f);
    s.lorenzQMod    = p.chaosDepth * 0.3f;

    // Map chorus
    s.chorusEnable    = p.chorusEnable;
    s.chorusRateHz    = p.chorusRateHz;
    s.chorusDepthMs   = p.chorusDepthMs;
    s.chorusDimension = p.chorusDimension;
    s.chorusMix       = p.chorusMix;
    if (p.chorusDimensionMode != braun::mr16::DimensionMode::Manual) {
        switch (p.chorusDimensionMode) {
            case braun::mr16::DimensionMode::Mode1:
                s.chorusRateHz = 0.40f; s.chorusDepthMs = 1.50f; s.chorusMix = 0.35f; s.chorusDimension = 0.35f; break;
            case braun::mr16::DimensionMode::Mode2:
                s.chorusRateHz = 0.55f; s.chorusDepthMs = 2.20f; s.chorusMix = 0.45f; s.chorusDimension = 0.65f; break;
            case braun::mr16::DimensionMode::Mode3:
                s.chorusRateHz = 0.75f; s.chorusDepthMs = 3.20f; s.chorusMix = 0.55f; s.chorusDimension = 0.85f; break;
            case braun::mr16::DimensionMode::Mode4:
                s.chorusRateHz = 1.10f; s.chorusDepthMs = 4.50f; s.chorusMix = 0.65f; s.chorusDimension = 1.00f; break;
            default: break;
        }
    }

    // Map dynamics
    s.goldenPanSpread   = p.stereoWidth;
    s.vactrolLpgCutoff  = 12000.0f;
    s.driveSaturationDb = p.driveSaturation;
    s.masterTrimDb      = p.masterVolumeDb;
    s.dryWetMix         = p.dryWetMix;
    s.powerState        = !p.outputMute;
    s.displayMode       = mr16::DisplayMode::Chladni;

    setPresetParameters(s);
}
#endif

void BRAUN_MR16AudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    mCurrentProgram = index;

#if MR16_HAS_DSP_ENGINE
    const auto presets = braun::mr16::Mr16Engine::getFactoryPresets();
    if (static_cast<size_t>(index) < presets.size())
    {
        mEuclideanEnable.store(presets[static_cast<size_t>(index)].params.euclideanEnable, std::memory_order_relaxed);
        setPresetParameters(presets[static_cast<size_t>(index)].params);
        return;
    }
#endif

    const auto& defaultPresets = getFactoryPresets();
    if (static_cast<size_t>(index) < defaultPresets.size())
    {
        mEuclideanEnable.store(defaultPresets[static_cast<size_t>(index)].params.euclideanEnable, std::memory_order_relaxed);
        setPresetParameters(defaultPresets[static_cast<size_t>(index)].params);
    }
}

const juce::String BRAUN_MR16AudioProcessor::getProgramName(int index)
{
#if MR16_HAS_DSP_ENGINE
    const auto presets = braun::mr16::Mr16Engine::getFactoryPresets();
    if (index >= 0 && static_cast<size_t>(index) < presets.size())
    {
        return juce::String(presets[static_cast<size_t>(index)].name);
    }
#endif

    const auto& defaultPresets = getFactoryPresets();
    if (index >= 0 && static_cast<size_t>(index) < defaultPresets.size())
    {
        return juce::String(defaultPresets[static_cast<size_t>(index)].name);
    }

    return "Default";
}

void BRAUN_MR16AudioProcessor::changeProgramName(int, const juce::String&)
{
}

const std::vector<BRAUN_MR16AudioProcessor::Preset>& BRAUN_MR16AudioProcessor::getFactoryPresets()
{
    static const std::vector<Preset> presets = []{
        std::vector<Preset> list;
        list.reserve(16);

        auto makePreset = [](const char* name, float f0, mr16::ManifoldType mf, mr16::MaterialProfile mat,
                             float damping, float coupling, float strikeVel, float strikeHard,
                             float chaosRate, float chaosDepth, bool chorusOn, float chorusMix) {
            Preset p;
            p.name = name;
            p.params.modalFrequency = f0;
            p.params.manifoldType = mf;
            p.params.materialProfile = mat;
            p.params.modalDamping = damping;
            p.params.modalCoupling = coupling;
            p.params.strikeVelocity = strikeVel;
            p.params.strikeHardness = strikeHard;
            p.params.lorenzRate = chaosRate;
            p.params.lorenzChaos = chaosDepth;
            p.params.chorusEnable = chorusOn;
            p.params.chorusMix = chorusMix;
            p.params.dryWetMix = 0.65f;
            p.params.powerState = true;
            return p;
        };

        list.push_back(makePreset("Chladni Zinc Plate", 220.0f, mr16::ManifoldType::ChladniPlate, mr16::MaterialProfile::Steel, 1.80f, 0.28f, 0.80f, 0.70f, 0.50f, 0.20f, true, 0.45f));
        list.push_back(makePreset("Deep Marimba Bar", 110.0f, mr16::ManifoldType::StiffBeam, mr16::MaterialProfile::Wood, 0.85f, 0.15f, 0.85f, 0.40f, 0.30f, 0.10f, false, 0.20f));
        list.push_back(makePreset("Hyperbolic Bell Flare", 330.0f, mr16::ManifoldType::PoincareHorn, mr16::MaterialProfile::Brass, 2.40f, 0.45f, 0.75f, 0.65f, 0.35f, 0.40f, true, 0.50f));
        list.push_back(makePreset("Vocal Formant Choir", 146.83f, mr16::ManifoldType::VocalFormant, mr16::MaterialProfile::Nylon, 1.50f, 0.35f, 0.60f, 0.50f, 0.80f, 0.60f, true, 0.55f));
        list.push_back(makePreset("Monsoon Zinc Roof", 185.0f, mr16::ManifoldType::ChladniPlate, mr16::MaterialProfile::Steel, 0.90f, 0.20f, 0.50f, 0.80f, 0.20f, 0.15f, true, 0.40f));
        auto euclideanPreset = makePreset("Euclidean Glass Chime", 523.25f, mr16::ManifoldType::ChladniPlate, mr16::MaterialProfile::Glass, 2.00f, 0.30f, 0.70f, 0.75f, 0.40f, 0.25f, true, 0.50f);
        euclideanPreset.params.euclideanEnable = true;
        list.push_back(euclideanPreset);
        list.push_back(makePreset("Bowed Crystal Rod", 440.0f, mr16::ManifoldType::StiffBeam, mr16::MaterialProfile::Glass, 3.20f, 0.38f, 0.40f, 0.60f, 0.60f, 0.30f, true, 0.50f));
        list.push_back(makePreset("Dimension Brass Matrix", 261.63f, mr16::ManifoldType::PoincareHorn, mr16::MaterialProfile::Brass, 4.50f, 0.32f, 0.80f, 0.70f, 0.45f, 0.35f, true, 0.75f));
        list.push_back(makePreset("Lorenz Butterfly Orbit", 196.0f, mr16::ManifoldType::ChladniPlate, mr16::MaterialProfile::Wood, 2.00f, 0.40f, 0.65f, 0.55f, 1.80f, 0.85f, true, 0.40f));
        list.push_back(makePreset("Buchla Optical Pluck", 220.0f, mr16::ManifoldType::StiffBeam, mr16::MaterialProfile::Wood, 0.70f, 0.18f, 0.90f, 0.85f, 0.20f, 0.10f, false, 0.30f));
        list.push_back(makePreset("Stiff Anvil Strike", 175.0f, mr16::ManifoldType::StiffBeam, mr16::MaterialProfile::Steel, 1.80f, 0.50f, 0.95f, 0.90f, 0.25f, 0.15f, true, 0.35f));
        list.push_back(makePreset("Nylon Resonant Body", 130.81f, mr16::ManifoldType::ChladniPlate, mr16::MaterialProfile::Nylon, 0.80f, 0.12f, 0.70f, 0.45f, 0.15f, 0.05f, false, 0.25f));
        list.push_back(makePreset("Hyperbolic Air Column", 293.66f, mr16::ManifoldType::PoincareHorn, mr16::MaterialProfile::Glass, 3.50f, 0.42f, 0.60f, 0.60f, 0.55f, 0.45f, true, 0.60f));
        list.push_back(makePreset("Sub-Harmonic Drone", 65.41f, mr16::ManifoldType::ChladniPlate, mr16::MaterialProfile::Wood, 2.80f, 0.30f, 0.80f, 0.50f, 0.10f, 0.20f, true, 0.40f));
        list.push_back(makePreset("Ethereal Space Chime", 659.25f, mr16::ManifoldType::PoincareHorn, mr16::MaterialProfile::Steel, 2.60f, 0.50f, 0.75f, 0.80f, 0.30f, 0.35f, true, 0.65f));
        list.push_back(makePreset("Kinetischer Impuls Master", 220.0f, mr16::ManifoldType::ChladniPlate, mr16::MaterialProfile::Steel, 1.80f, 0.30f, 0.80f, 0.65f, 0.50f, 0.30f, true, 0.50f));

        return list;
    }();

    return presets;
}

void BRAUN_MR16AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
#if MR16_HAS_DSP_ENGINE
    mr16Engine.prepare(sampleRate, samplesPerBlock);

    const auto snapshot = atomicPointers.loadSnapshot();
    auto dspParams = snapshot.toDspParams();
    dspParams.euclideanEnable = mEuclideanEnable.load(std::memory_order_relaxed);
    mr16Engine.setParameters(dspParams);
    mr16Engine.reset();
#endif

    mPendingEngineReset.store(false, std::memory_order_release);

    scopeWritePos.store(0, std::memory_order_relaxed);
    for (int i = 0; i < kScopeBufferSize; ++i)
    {
        scopeBufferL[i] = 0.0f;
        scopeBufferR[i] = 0.0f;
    }
}

void BRAUN_MR16AudioProcessor::releaseResources()
{
#if MR16_HAS_DSP_ENGINE
    mr16Engine.reset();
#endif
}

void BRAUN_MR16AudioProcessor::reset()
{
    mPendingEngineReset.store(true, std::memory_order_release);
#if MR16_HAS_DSP_ENGINE
    mr16Engine.reset();
#endif
}

bool BRAUN_MR16AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();

    if (mainOutput != juce::AudioChannelSet::stereo() && mainOutput != juce::AudioChannelSet::mono())
        return false;

    if (mainInput != juce::AudioChannelSet::stereo() && mainInput != juce::AudioChannelSet::mono() && !mainInput.isDisabled())
        return false;

    return true;
}

void BRAUN_MR16AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Mandatory Real-Time Invariant: Hardware FTZ/DAZ activated strictly once at block level
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    float* outChannels[2];
    outChannels[0] = buffer.getWritePointer(0);
    outChannels[1] = (numChannels > 1) ? buffer.getWritePointer(1) : outChannels[0];

    // Synchronize APVTS powerState parameter changes from host automation or GUI
    if (atomicPointers.powerState != nullptr)
    {
        const bool paramPower = (atomicPointers.powerState->load(std::memory_order_relaxed) > 0.5f);
        const bool currentPower = isPoweredOn.load(std::memory_order_relaxed);
        if (paramPower != currentPower)
        {
            isPoweredOn.store(paramPower, std::memory_order_relaxed);
            mPendingEngineReset.store(true, std::memory_order_release);
        }
    }

    // Process any explicitly queued engine resets (e.g. from setPower or power parameter changes)
    if (mPendingEngineReset.exchange(false, std::memory_order_acq_rel))
    {
#if MR16_HAS_DSP_ENGINE
        mr16Engine.reset();
#endif
    }

    // Standby Power Gating: If powered down, output clean silence unless smoothly awakened by MIDI Note-On
    if (!isPoweredOn.load(std::memory_order_relaxed))
    {
        bool hasNoteOn = false;
        for (const auto metadata : midiMessages)
        {
            if (metadata.numBytes >= 3)
            {
                const auto* rawData = metadata.data;
                if ((rawData[0] & 0xF0) == 0x90 && rawData[2] > 0)
                {
                    hasNoteOn = true;
                    break;
                }
            }
        }

        if (hasNoteOn)
        {
            // Smoothly wake the synthesizer from standby
            isPoweredOn.store(true, std::memory_order_relaxed);
#if MR16_HAS_DSP_ENGINE
            mr16Engine.reset();
#endif
            if (auto* p = apvts.getParameter(mr16::ParamIDs::powerState.getParamID()))
                p->setValueNotifyingHost(1.0f);
        }
        else
        {
            // STANDBY state: ensure engine is idle/reset, buffer is completely cleared with silence,
            // and no sound escapes.
#if MR16_HAS_DSP_ENGINE
            mr16Engine.reset();
#endif
            buffer.clear();
            pushScopeSamples(outChannels[0], outChannels[1], numSamples);
            midiMessages.clear();
            return;
        }
    }

    // Process incoming MIDI Note-On, Note-Off, and CC events for active engine
    for (const auto metadata : midiMessages)
    {
        if (metadata.numBytes >= 3)
        {
            const auto* rawData = metadata.data;
            const uint8_t status = rawData[0] & 0xF0;
            const uint8_t note = rawData[1] & 0x7F;
            const uint8_t vel = rawData[2] & 0x7F;
            if (status == 0x90 && vel > 0)
            {
#if MR16_HAS_DSP_ENGINE
                mr16Engine.enqueueMidiNoteOn(static_cast<int>(note), static_cast<float>(vel) / 127.0f);
#endif
            }
            else if (status == 0xB0 && note == 64)
            {
                // Sustain pedal CC 64
                const bool sustainOn = (vel >= 64);
                if (auto* freezeParam = apvts.getParameter(mr16::ParamIDs::vactrolSag.getParamID()))
                {
                    if (sustainOn)
                        freezeParam->setValueNotifyingHost(1.0f);
                }
            }
        }
    }

    // Wait-free POD snapshot load with std::memory_order_relaxed (0 locks, 0 dynamic allocations)
    const auto snapshot = atomicPointers.loadSnapshot();

#if MR16_HAS_DSP_ENGINE
    auto dspParams = snapshot.toDspParams();
    dspParams.euclideanEnable = mEuclideanEnable.load(std::memory_order_relaxed);
    mr16Engine.setParameters(dspParams);

    // Clean audio input handling:
    // When mainInput bus is active, input streams are fed to mr16Engine where KineticExciter
    // handles 15 Hz DC blocking and click-free parameter-slewed crossfading based on externalAudioEnable.
    const auto* mainInputBus = getBus(true, 0);
    const bool isInputBusActive = (mainInputBus != nullptr && mainInputBus->isEnabled() && getTotalNumInputChannels() > 0);

    const float* inChannels[2];
    inChannels[0] = isInputBusActive ? buffer.getReadPointer(0) : nullptr;
    inChannels[1] = (isInputBusActive && getTotalNumInputChannels() > 1) ? buffer.getReadPointer(1) : inChannels[0];

    const bool monitorIn = isMonitoringInput();

    // If monitoring input audio, push input buffer samples to visualizer scope BEFORE engine modifies buffer
    if (monitorIn && inChannels[0] != nullptr)
    {
        pushScopeSamples(inChannels[0], inChannels[1], numSamples);
    }

    mr16Engine.processBlock(inChannels[0], inChannels[1], outChannels[0], outChannels[1], numSamples);

    // Apply branchless subnormal flush on output samples
    for (int i = 0; i < numSamples; ++i)
    {
        outChannels[0][i] = braun::mr16::flushDenormal(outChannels[0][i]);
        if (numChannels > 1)
        {
            outChannels[1][i] = braun::mr16::flushDenormal(outChannels[1][i]);
        }
    }

    // Update telemetry frame for 60 FPS Phosphor CRT display
    stagedVisualizerFrame = mr16Engine.getVisualizerFrame();
    newVisualizerData.store(true, std::memory_order_release);
#endif

    // Push processed audio frames to wait-free visualizer oscilloscope buffer if not monitoring input
    if (!monitorIn || inChannels[0] == nullptr)
    {
        pushScopeSamples(outChannels[0], outChannels[1], numSamples);
    }

    // Push processed audio to lossless WAV recorder if active (lock-free)
    if (activeWriter.load(std::memory_order_acquire) != nullptr)
    {
        activeWriterWorkers.fetch_add(1, std::memory_order_acquire);
        if (auto* writer = activeWriter.load(std::memory_order_acquire))
        {
            const float* channels[] = { outChannels[0], outChannels[1] };
            writer->write(channels, numSamples);
        }
        activeWriterWorkers.fetch_sub(1, std::memory_order_release);
    }

    midiMessages.clear();
}

void BRAUN_MR16AudioProcessor::triggerStrike(float velocity, float hardness) noexcept
{
    if (!isPoweredOn.load(std::memory_order_relaxed))
        setPower(true);

#if MR16_HAS_DSP_ENGINE
    mr16Engine.enqueueTriggerStrike(velocity, hardness);
#else
    juce::ignoreUnused(velocity, hardness);
#endif
}

void BRAUN_MR16AudioProcessor::triggerStrikeButton(int buttonIndex, float velocity) noexcept
{
    if (!isPoweredOn.load(std::memory_order_relaxed))
        setPower(true);

#if MR16_HAS_DSP_ENGINE
    mr16Engine.enqueueTriggerButton(buttonIndex, velocity);
#else
    juce::ignoreUnused(buttonIndex, velocity);
#endif
}

void BRAUN_MR16AudioProcessor::triggerChimeKey(int keyIndex, float velocity) noexcept
{
    if (!isPoweredOn.load(std::memory_order_relaxed))
        setPower(true);

#if MR16_HAS_DSP_ENGINE
    mr16Engine.enqueueTriggerChime(keyIndex, velocity);
#else
    juce::ignoreUnused(keyIndex, velocity);
#endif
}

void BRAUN_MR16AudioProcessor::triggerMidiNote(int midiNote, float velocity) noexcept
{
    if (!isPoweredOn.load(std::memory_order_relaxed))
        setPower(true);

#if MR16_HAS_DSP_ENGINE
    mr16Engine.enqueueMidiNoteOn(midiNote, velocity);
#else
    juce::ignoreUnused(midiNote, velocity);
#endif
}


bool BRAUN_MR16AudioProcessor::popVisualizerFrame(mr16::VisualizerFrame& frame) noexcept
{
    if (newVisualizerData.exchange(false, std::memory_order_acq_rel))
    {
        frame = stagedVisualizerFrame;
        getScopeSamples(frame.scopeSamplesL.data(), frame.scopeSamplesR.data(), 256);
        return true;
    }
    return false;
}

void BRAUN_MR16AudioProcessor::pushScopeSamples(const float* left, const float* right, int numSamples) noexcept
{
    if (left == nullptr || numSamples <= 0)
        return;

    int pos = scopeWritePos.load(std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        scopeBufferL[pos] = left[i];
        scopeBufferR[pos] = (right != nullptr) ? right[i] : left[i];
        pos = (pos + 1);
        if (pos >= kScopeBufferSize)
            pos = 0;
    }
    scopeWritePos.store(pos, std::memory_order_release);
}

void BRAUN_MR16AudioProcessor::getScopeSamples(float* destL, float* destR, int numSamplesToRead) const noexcept
{
    if (destL == nullptr || numSamplesToRead <= 0)
        return;

    numSamplesToRead = std::min(numSamplesToRead, kScopeBufferSize);

    const int writePos = scopeWritePos.load(std::memory_order_acquire);
    int readPos = ((writePos - numSamplesToRead) % kScopeBufferSize + kScopeBufferSize) % kScopeBufferSize;
    for (int i = 0; i < numSamplesToRead; ++i)
    {
        destL[i] = scopeBufferL[readPos];
        if (destR != nullptr)
            destR[i] = scopeBufferR[readPos];
        readPos = (readPos + 1);
        if (readPos >= kScopeBufferSize)
            readPos = 0;
    }
}

void BRAUN_MR16AudioProcessor::setScopeSource(int source) noexcept
{
    mScopeSource.store(source, std::memory_order_relaxed);
}

int BRAUN_MR16AudioProcessor::getScopeSource() const noexcept
{
    return mScopeSource.load(std::memory_order_relaxed);
}

bool BRAUN_MR16AudioProcessor::isMonitoringInput() const noexcept
{
    const bool isInputBusActive = (getBus(true, 0) != nullptr && getBus(true, 0)->isEnabled() && getTotalNumInputChannels() > 0);
    const auto snapshot = atomicPointers.loadSnapshot();
    return (mScopeSource.load(std::memory_order_relaxed) == 1)
        || (snapshot.exciterType == mr16::ExciterType::ExtIn && isInputBusActive);
}

void BRAUN_MR16AudioProcessor::startRecording()
{
    const juce::ScopedLock sl(recorderLock);
    if (activeWriter.load(std::memory_order_relaxed) != nullptr || threadedWriter != nullptr)
        return;

    const double sampleRateToUse = getSampleRate() > 0.0 ? getSampleRate() : 48000.0;

    auto musicDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userMusicDirectory);
    if (!musicDir.isDirectory() && !musicDir.createDirectory().wasOk())
    {
        musicDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDocumentsDirectory);
        if (!musicDir.isDirectory() && !musicDir.createDirectory().wasOk())
            musicDir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory);
    }

    const auto recordingsDir = musicDir.getChildFile("Braun MR-16 Recordings");
    if (!recordingsDir.exists())
    {
        const auto result = recordingsDir.createDirectory();
        if (result.failed())
            return;
    }

    const juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d-%H-%M-%S");
    const juce::File wavFile = recordingsDir.getNonexistentChildFile("braun-mr16-" + timestamp, ".wav");

    if (auto stream = wavFile.createOutputStream())
    {
        juce::WavAudioFormat wavFormat;
        if (auto* rawWriter = wavFormat.createWriterFor(stream.get(), sampleRateToUse, 2, 16, {}, 0))
        {
            stream.release();
            lastRecordedFile = wavFile;
            threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(rawWriter, recorderThread, 131072);
            activeWriter.store(threadedWriter.get(), std::memory_order_release);
        }
    }
}

void BRAUN_MR16AudioProcessor::stopRecording()
{
    const juce::ScopedLock sl(recorderLock);
    if (activeWriter.load(std::memory_order_relaxed) == nullptr && threadedWriter == nullptr)
        return;

    {
        const juce::ScopedLock slCb(getCallbackLock());
        activeWriter.store(nullptr, std::memory_order_release);
    }

    while (activeWriterWorkers.load(std::memory_order_acquire) > 0)
    {
        juce::Thread::yield();
    }

    threadedWriter.reset();

    recordingSavedDirty.store(true, std::memory_order_relaxed);

    if (lastRecordedFile.existsAsFile() && getActiveEditor() != nullptr)
    {
        juce::Thread::launch([f = lastRecordedFile] { f.revealToUser(); });
    }
}

bool BRAUN_MR16AudioProcessor::isRecording() const noexcept
{
    return activeWriter.load(std::memory_order_relaxed) != nullptr;
}

juce::File BRAUN_MR16AudioProcessor::getLastRecordedFile() const
{
    const juce::ScopedLock sl(recorderLock);
    return lastRecordedFile;
}

bool BRAUN_MR16AudioProcessor::consumeRecordingSavedDirty() noexcept
{
    return recordingSavedDirty.exchange(false, std::memory_order_relaxed);
}

bool BRAUN_MR16AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BRAUN_MR16AudioProcessor::createEditor()
{
    return new BRAUN_MR16AudioProcessorEditor(*this);
}

void BRAUN_MR16AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("currentProgram", mCurrentProgram, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BRAUN_MR16AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto vt = juce::ValueTree::fromXml(*xmlState);
        if (vt.hasProperty("currentProgram"))
            mCurrentProgram = static_cast<int>(vt.getProperty("currentProgram"));
        apvts.replaceState(vt);
    }
}

// JUCE plugin entry point export
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BRAUN_MR16AudioProcessor();
}
