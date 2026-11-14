#include "PluginProcessor.h"
#include "PluginEditor.h"

BRAUN_MR16AudioProcessor::BRAUN_MR16AudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", mr16::createParameterLayout())
{
    atomicPointers.initialize(apvts);
    recorderThread.startThread();
#if MR16_HAS_DSP_ENGINE
    mr16Engine.reset();
#endif
    if (wrapperType == wrapperType_Standalone)
    {
        setPower(false);
    }
    startTimerHz(30);
}

BRAUN_MR16AudioProcessor::~BRAUN_MR16AudioProcessor()
{
    stopTimer();
    stopRecording();
    recorderThread.stopThread(2000);
}

void BRAUN_MR16AudioProcessor::timerCallback()
{
    if (mPendingPowerWakeNotify.exchange(false, std::memory_order_acq_rel))
    {
        setPower(true);
    }
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
    setParamBool(mr16::ParamIDs::euclideanEnable, s.euclideanEnable);
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
    setParamFloat(mr16::ParamIDs::lorenzMorphAlpha, s.lorenzMorphAlpha);
    setParamFloat(mr16::ParamIDs::lorenzRate, s.lorenzRate);
    setParamFloat(mr16::ParamIDs::lorenzChaos, s.lorenzChaos);
    setParamFloat(mr16::ParamIDs::lorenzFreqMod, s.lorenzFreqMod);
    setParamFloat(mr16::ParamIDs::lorenzQMod, s.lorenzQMod);
    setParamFloat(mr16::ParamIDs::lorenzMorphSteer, s.lorenzMorphSteer);

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
    setParamBool(mr16::ParamIDs::softLimiter, s.softLimiter);
    setParamFloat(mr16::ParamIDs::masterTrimDb, s.masterTrimDb);
    setParamFloat(mr16::ParamIDs::dryWetMix, s.dryWetMix);
    setParamBool(mr16::ParamIDs::powerState, s.powerState);

    // Deck 06
    setParamChoice(mr16::ParamIDs::displayMode, static_cast<float>(mr16::indexFromDisplayMode(s.displayMode)));

    isPoweredOn.store(s.powerState, std::memory_order_relaxed);
    mLastObservedParamPower.store(s.powerState, std::memory_order_relaxed);
    mPendingPowerWakeNotify.store(false, std::memory_order_release);
    mEuclideanEnable.store(s.euclideanEnable, std::memory_order_relaxed);
    mStateSyncEpoch.fetch_add(1, std::memory_order_release);
}

#if MR16_HAS_DSP_ENGINE
void BRAUN_MR16AudioProcessor::setPresetParameters(const braun::mr16::Mr16Parameters& p)
{
    mr16::Mr16ParameterSnapshot s;

    // Map exciter
    s.exciterType    = mr16::exciterTypeFromIndex(p.exciterMode);
    s.strikeVelocity = p.exciterStrikeVelocity;
    s.strikeHardness = p.exciterStrikeHardness;
    s.frictionForce  = (p.exciterBowPressure > 0.0f) ? p.exciterBowPressure : p.uiFrictionForce;
    s.frictionSpeed  = (p.exciterBowVelocity > 0.0f) ? p.exciterBowVelocity : p.uiFrictionSpeed;
    s.vactrolSag     = p.vactrolSagEnable ? p.vactrolSagAmount : 0.0f;
    s.extInputGainDb = (p.externalSensitivity > 0.0f) ? (20.0f * std::log10(p.externalSensitivity)) : 0.0f;
    s.poissonDensity = p.poissonEnable ? (p.poissonEpm / 60.0f) : 0.0f;
    s.euclideanEnable = p.euclideanEnable;
    s.euclideanPulses = p.euclideanPulses;
    s.euclideanSteps  = p.euclideanSteps;
    mEuclideanEnable.store(p.euclideanEnable, std::memory_order_relaxed);

    // Map modal matrix
    s.manifoldType    = static_cast<mr16::ManifoldType>(static_cast<int>(p.manifold));
    s.modalFrequency  = p.fundamentalHz;
    s.modalDamping    = p.decayScale;
    s.materialProfile = static_cast<mr16::MaterialProfile>(static_cast<int>(p.material));
    s.modalCoupling   = p.couplingDepth;
    s.modalSpread     = std::clamp(p.overtoneSpread, 0.2f, 3.0f);
    s.modalQ          = std::clamp(p.modalQScale * 85.0f, 5.0f, 500.0f);

    // Map attractor
    s.lorenzMorphAlpha = std::clamp(p.manifoldMorph, 0.0f, 1.0f);
    s.lorenzRate       = p.chaosRateHz;
    s.lorenzChaos      = p.chaosDepth;
    s.lorenzFreqMod    = std::clamp(p.chaosDetuneCents / 300.0f, 0.0f, 1.0f);
    s.lorenzQMod       = std::clamp(p.chaosQMod, 0.0f, 1.0f);
    s.lorenzMorphSteer = std::clamp(p.lorenzMorphSteer, 0.0f, 1.0f);

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
    s.vactrolLpgCutoff  = std::clamp(p.vactrolLpgCutoff, 20.0f, 20000.0f);
    s.driveSaturationDb = p.driveSaturation;
    s.softLimiter       = p.softLimiterEnable;
    s.masterTrimDb      = p.masterVolumeDb;
    s.dryWetMix         = p.dryWetMix;
    s.powerState        = !p.outputMute;
    s.displayMode       = mr16::displayModeFromIndex(p.displayMode);

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

        auto makePreset = [](const char* name, int excType, float strikeHard, float strikeVel,
                             float fricForce, float fricSpeed, float sag, float poissonHz,
                             bool euclOn, int euclPulses, int euclSteps,
                             mr16::ManifoldType mf, float f0, float damping, mr16::MaterialProfile mat,
                             float coupling, float spread, float modalQVal,
                             float chaosRate, float chaosDepth, float freqMod, float qMod,
                             bool chorusOn, float chorusRate, float chorusDepth, float chorusDim, float chorusMix,
                             float panSpread, float lpgCutoff, float drive, float trimDb, float dryWet,
                             mr16::DisplayMode disp) {
            Preset p;
            p.name = name;
            p.params.exciterType = mr16::exciterTypeFromIndex(excType);
            p.params.strikeHardness = strikeHard;
            p.params.strikeVelocity = strikeVel;
            p.params.frictionForce = fricForce;
            p.params.frictionSpeed = fricSpeed;
            p.params.vactrolSag = sag;
            p.params.extInputGainDb = 0.0f;
            p.params.poissonDensity = poissonHz;
            p.params.euclideanEnable = euclOn;
            p.params.euclideanPulses = euclPulses;
            p.params.euclideanSteps = euclSteps;
            p.params.manifoldType = mf;
            p.params.modalFrequency = f0;
            p.params.modalDamping = damping;
            p.params.materialProfile = mat;
            p.params.modalCoupling = coupling;
            p.params.modalSpread = spread;
            p.params.modalQ = modalQVal;
            p.params.lorenzMorphAlpha = 0.0f;
            p.params.lorenzRate = chaosRate;
            p.params.lorenzChaos = chaosDepth;
            p.params.lorenzFreqMod = freqMod;
            p.params.lorenzQMod = qMod;
            p.params.lorenzMorphSteer = 0.0f;
            p.params.chorusEnable = chorusOn;
            p.params.chorusRateHz = chorusRate;
            p.params.chorusDepthMs = chorusDepth;
            p.params.chorusDimension = chorusDim;
            p.params.chorusMix = chorusMix;
            p.params.goldenPanSpread = panSpread;
            p.params.vactrolLpgCutoff = lpgCutoff;
            p.params.driveSaturationDb = drive;
            p.params.softLimiter = true;
            p.params.masterTrimDb = trimDb;
            p.params.dryWetMix = dryWet;
            p.params.powerState = true;
            p.params.displayMode = disp;
            return p;
        };

        list.push_back(makePreset("CALIBRATED DEFAULT",         0, 0.65f, 0.80f, 0.50f, 0.40f, 0.35f,  0.0f, false, 4, 16, mr16::ManifoldType::ChladniPlate, 220.00f,  1.80f, mr16::MaterialProfile::Steel, 0.40f, 1.00f,  85.0f, 0.85f, 0.50f, 0.15f, 0.20f, true,  0.65f, 1.40f, 0.75f, 0.45f, 0.85f, 14000.0f, 0.25f,  0.0f, 0.65f, mr16::DisplayMode::Chladni));
        list.push_back(makePreset("MARIMBA ROSEWOOD BAR",       0, 0.35f, 0.85f, 0.30f, 0.20f, 0.25f,  0.0f, false, 4, 16, mr16::ManifoldType::StiffBeam,    174.61f,  0.90f, mr16::MaterialProfile::Wood,  0.20f, 1.00f,  45.0f, 0.40f, 0.30f, 0.05f, 0.10f, true,  0.40f, 0.80f, 0.50f, 0.25f, 0.70f,  8500.0f, 0.15f,  0.0f, 0.70f, mr16::DisplayMode::ModalFft));
        list.push_back(makePreset("CRYSTAL WINE GLASS",         1, 0.70f, 0.60f, 0.62f, 0.55f, 0.40f,  0.0f, false, 2, 16, mr16::ManifoldType::ChladniPlate, 587.33f,  4.50f, mr16::MaterialProfile::Glass, 0.35f, 0.85f, 220.0f, 0.30f, 0.40f, 0.08f, 0.15f, true,  0.50f, 1.20f, 0.80f, 0.40f, 0.90f, 16000.0f, 0.10f, -1.0f, 0.85f, mr16::DisplayMode::Chladni));
        list.push_back(makePreset("TIBETAN BRONZE BELL",        0, 0.85f, 0.95f, 0.50f, 0.40f, 0.50f,  0.0f, false, 3, 16, mr16::ManifoldType::ChladniPlate, 130.81f,  5.20f, mr16::MaterialProfile::Brass, 0.78f, 1.25f, 160.0f, 0.60f, 0.65f, 0.18f, 0.35f, true,  0.35f, 2.20f, 0.90f, 0.50f, 0.95f, 12000.0f, 0.35f, -2.0f, 0.80f, mr16::DisplayMode::Chladni));
        list.push_back(makePreset("CATHEDRAL TUBE DRONE",       2, 0.40f, 0.75f, 0.40f, 0.30f, 0.70f,  0.0f, false, 2, 16, mr16::ManifoldType::PoincareHorn,  65.41f,  6.00f, mr16::MaterialProfile::Brass, 0.50f, 1.10f, 110.0f, 0.25f, 0.45f, 0.10f, 0.25f, true,  0.28f, 1.80f, 0.85f, 0.45f, 0.85f,  4500.0f, 0.20f,  0.0f, 0.90f, mr16::DisplayMode::ModalFft));
        list.push_back(makePreset("VOCAL FORMANT CHOIR",        2, 0.50f, 0.80f, 0.45f, 0.40f, 0.55f,  0.0f, false, 4, 16, mr16::ManifoldType::VocalFormant, 110.00f,  3.20f, mr16::MaterialProfile::Nylon, 0.30f, 1.00f,  70.0f, 0.70f, 0.55f, 0.25f, 0.30f, true,  0.55f, 2.00f, 0.85f, 0.55f, 0.80f,  7500.0f, 0.15f,  0.0f, 0.85f, mr16::DisplayMode::Attractor));
        list.push_back(makePreset("POISSON ZINC ROOF",          0, 0.75f, 0.70f, 0.20f, 0.20f, 0.20f, 14.5f, false, 4, 16, mr16::ManifoldType::ChladniPlate, 330.00f,  1.20f, mr16::MaterialProfile::Steel, 0.45f, 1.15f,  95.0f, 0.90f, 0.40f, 0.12f, 0.15f, true,  0.45f, 1.00f, 0.60f, 0.30f, 0.95f, 15000.0f, 0.20f,  0.0f, 0.75f, mr16::DisplayMode::Chladni));
        list.push_back(makePreset("EUCLIDEAN METALLOPHONE",     0, 0.80f, 0.85f, 0.30f, 0.30f, 0.30f,  0.0f, true,  5, 16, mr16::ManifoldType::StiffBeam,    329.63f,  2.40f, mr16::MaterialProfile::Brass, 0.55f, 1.05f, 125.0f, 0.50f, 0.35f, 0.10f, 0.15f, true,  0.60f, 1.50f, 0.70f, 0.35f, 0.85f, 13500.0f, 0.22f,  0.0f, 0.70f, mr16::DisplayMode::ModalFft));
        list.push_back(makePreset("DIMENSION CELESTE",          0, 0.90f, 0.85f, 0.20f, 0.20f, 0.25f,  0.0f, false, 4, 16, mr16::ManifoldType::StiffBeam,    659.25f,  3.80f, mr16::MaterialProfile::Steel, 0.35f, 1.00f, 180.0f, 0.45f, 0.30f, 0.08f, 0.12f, true,  0.45f, 2.40f, 1.00f, 0.65f, 0.90f, 18000.0f, 0.12f, -1.0f, 0.75f, mr16::DisplayMode::Chladni));
        list.push_back(makePreset("LORENZ CHAOTIC ORBIT",       0, 0.70f, 0.90f, 0.40f, 0.40f, 0.40f,  0.0f, false, 3, 16, mr16::ManifoldType::ChladniPlate, 196.00f,  4.00f, mr16::MaterialProfile::Steel, 0.60f, 1.20f, 140.0f, 2.40f, 0.85f, 0.45f, 0.60f, true,  0.70f, 1.60f, 0.80f, 0.40f, 0.90f, 12500.0f, 0.30f,  0.0f, 0.85f, mr16::DisplayMode::Attractor));
        list.push_back(makePreset("BOWED TITANIUM SHEET",       1, 0.60f, 0.70f, 0.75f, 0.65f, 0.50f,  0.0f, false, 2, 16, mr16::ManifoldType::ChladniPlate, 146.83f,  3.80f, mr16::MaterialProfile::Steel, 0.50f, 1.10f, 150.0f, 0.60f, 0.50f, 0.15f, 0.25f, true,  0.35f, 1.80f, 0.75f, 0.45f, 0.85f,  9500.0f, 0.25f, -1.0f, 0.90f, mr16::DisplayMode::Chladni));
        list.push_back(makePreset("HYPERBOLIC SUB RESONATOR",   0, 0.30f, 0.90f, 0.20f, 0.20f, 0.45f,  0.0f, false, 4, 16, mr16::ManifoldType::PoincareHorn,  43.65f,  2.50f, mr16::MaterialProfile::Wood,  0.30f, 0.90f,  55.0f, 0.30f, 0.30f, 0.05f, 0.10f, false, 0.30f, 1.00f, 0.40f, 0.00f, 0.40f,   450.0f, 0.15f,  1.0f, 0.70f, mr16::DisplayMode::ModalFft));
        list.push_back(makePreset("AFRICAN BALAFON",            0, 0.75f, 0.85f, 0.30f, 0.30f, 0.30f,  0.0f, false, 5, 16, mr16::ManifoldType::StiffBeam,    196.00f,  1.40f, mr16::MaterialProfile::Wood,  0.45f, 1.15f,  50.0f, 0.50f, 0.40f, 0.12f, 0.20f, true,  0.50f, 1.20f, 0.65f, 0.30f, 0.75f, 11000.0f, 0.28f,  0.0f, 0.75f, mr16::DisplayMode::ModalFft));
        list.push_back(makePreset("SOLAR WIND RESONANCE",       2, 0.45f, 0.65f, 0.40f, 0.40f, 0.65f, 32.0f, false, 3, 16, mr16::ManifoldType::PoincareHorn, 220.00f,  4.20f, mr16::MaterialProfile::Nylon, 0.40f, 1.10f,  90.0f, 0.35f, 0.55f, 0.18f, 0.30f, true,  0.40f, 2.00f, 0.85f, 0.50f, 0.90f,  8500.0f, 0.18f,  0.0f, 0.80f, mr16::DisplayMode::Attractor));
        list.push_back(makePreset("FROSTED GLASS ARPEGGIO",     0, 0.85f, 0.80f, 0.30f, 0.30f, 0.30f,  0.0f, true,  7, 12, mr16::ManifoldType::ChladniPlate, 880.00f,  3.60f, mr16::MaterialProfile::Glass, 0.40f, 0.90f, 210.0f, 0.65f, 0.45f, 0.14f, 0.20f, true,  0.75f, 1.50f, 0.80f, 0.50f, 0.90f, 17000.0f, 0.14f, -1.0f, 0.75f, mr16::DisplayMode::Chladni));
        list.push_back(makePreset("INFINITE MODAL SINGULARITY", 0, 0.70f, 0.80f, 0.50f, 0.50f, 0.40f,  0.0f, false, 1, 16, mr16::ManifoldType::ChladniPlate, 220.00f, 10.00f, mr16::MaterialProfile::Steel, 0.98f, 1.00f, 250.0f, 1.10f, 0.75f, 0.20f, 0.40f, true,  0.40f, 2.20f, 0.95f, 0.50f, 0.95f, 14000.0f, 0.65f, -3.0f, 0.95f, mr16::DisplayMode::Attractor));

        return list;
    }();

    return presets;
}

void BRAUN_MR16AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSustainPedalActive.store(false, std::memory_order_relaxed);
    mPitchBendSemitones.store(0.0f, std::memory_order_relaxed);
    mModWheelNorm.store(0.0f, std::memory_order_relaxed);

#if MR16_HAS_DSP_ENGINE
    mr16Engine.prepare(sampleRate, samplesPerBlock);

    const auto snapshot = atomicPointers.loadSnapshot();
    mEuclideanEnable.store(snapshot.euclideanEnable, std::memory_order_relaxed);
    auto dspParams = snapshot.toDspParams();
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
    mSustainPedalActive.store(false, std::memory_order_relaxed);
    mPitchBendSemitones.store(0.0f, std::memory_order_relaxed);
    mModWheelNorm.store(0.0f, std::memory_order_relaxed);
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

    // Synchronize APVTS powerState parameter changes from host automation or GUI (edge-triggered)
    if (atomicPointers.powerState != nullptr && !mPendingPowerWakeNotify.load(std::memory_order_acquire))
    {
        const bool paramPower = (atomicPointers.powerState->load(std::memory_order_relaxed) > 0.5f);
        const bool lastObserved = mLastObservedParamPower.load(std::memory_order_relaxed);
        if (paramPower != lastObserved)
        {
            mLastObservedParamPower.store(paramPower, std::memory_order_relaxed);
            isPoweredOn.store(paramPower, std::memory_order_relaxed);
            if (!paramPower)
                mPendingEngineReset.store(true, std::memory_order_release);
        }
    }

    // Process any explicitly queued engine resets (e.g. from powering down)
    if (mPendingEngineReset.exchange(false, std::memory_order_acq_rel))
    {
#if MR16_HAS_DSP_ENGINE
        mr16Engine.reset();
#endif
    }

    // Standby Power Gating: If powered down, output clean silence unless explicitly awakened
    // by interactive MIDI Note-On or queued UI exciter triggers. Never wake on ambient audio
    // input (which would defeat Standby on active DAW audio tracks and wake Standalone on cold
    // boot from laptop/interface microphone noise).
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

        // Check if there are queued interactive exciter events in mr16Engine
        bool hasQueuedTriggers = false;
#if MR16_HAS_DSP_ENGINE
        if (mr16Engine.hasPendingEvents())
            hasQueuedTriggers = true;
#endif

        if (hasNoteOn || hasQueuedTriggers)
        {
            // Real-time lock-free wake: never call setValueNotifyingHost on the audio thread!
            // Message-thread timer flushes the APVTS power_state parameter asynchronously.
            isPoweredOn.store(true, std::memory_order_relaxed);
            mPendingPowerWakeNotify.store(true, std::memory_order_release);
        }
        else
        {
            // STANDBY state: ensure engine is idle/reset, buffer is completely cleared with silence,
            // and no sound escapes.
#if MR16_HAS_DSP_ENGINE
            mr16Engine.reset();
#endif
            buffer.clear();
            const float* zeroL = buffer.getReadPointer(0);
            const float* zeroR = (numChannels > 1) ? buffer.getReadPointer(1) : zeroL;
            pushScopeSamples(zeroL, zeroR, numSamples);
            midiMessages.clear();
            return;
        }
    }

    // Process incoming MIDI Note-On, Pitch Bend, and CC events strictly lock-free (0 heap allocations)
    for (const auto metadata : midiMessages)
    {
        if (metadata.numBytes >= 3)
        {
            const auto* rawData = metadata.data;
            const uint8_t status = rawData[0] & 0xF0;
            const uint8_t data1 = rawData[1] & 0x7F;
            const uint8_t data2 = rawData[2] & 0x7F;
            if (status == 0x90 && data2 > 0)
            {
#if MR16_HAS_DSP_ENGINE
                mr16Engine.enqueueMidiNoteOn(static_cast<int>(data1), static_cast<float>(data2) / 127.0f);
#endif
            }
            else if (status == 0xB0)
            {
                if (data1 == 64)
                {
                    // Sustain pedal CC 64: non-destructively extends modal decay while held, releasing cleanly on < 64
                    mSustainPedalActive.store(data2 >= 64, std::memory_order_relaxed);
                }
                else if (data1 == 1)
                {
                    // Mod Wheel CC 1: additive Householder inter-modal scattering coupling
                    mModWheelNorm.store(static_cast<float>(data2) / 127.0f, std::memory_order_relaxed);
                }
                else if (data1 == 120 || data1 == 121 || data1 == 123)
                {
                    // All Sound Off / Reset All Controllers / All Notes Off
                    mSustainPedalActive.store(false, std::memory_order_relaxed);
                    mModWheelNorm.store(0.0f, std::memory_order_relaxed);
                    mPitchBendSemitones.store(0.0f, std::memory_order_relaxed);
                    if (data1 == 120)
                    {
#if MR16_HAS_DSP_ENGINE
                        mr16Engine.reset();
#endif
                    }
                }
            }
            else if (status == 0xE0)
            {
                // 14-bit Pitch Bend (+/- 2 semitones)
                const int bend14 = static_cast<int>(data1) | (static_cast<int>(data2) << 7);
                const float bendNorm = std::clamp(static_cast<float>(bend14 - 8192) / 8192.0f, -1.0f, 1.0f);
                mPitchBendSemitones.store(bendNorm * 2.0f, std::memory_order_relaxed);
            }
        }
    }

    // Wait-free POD snapshot load with std::memory_order_relaxed (0 locks, 0 dynamic allocations)
    const auto snapshot = atomicPointers.loadSnapshot();
    mEuclideanEnable.store(snapshot.euclideanEnable, std::memory_order_relaxed);

#if MR16_HAS_DSP_ENGINE
    auto dspParams = snapshot.toDspParams();
    if (mSustainPedalActive.load(std::memory_order_relaxed))
    {
        dspParams.decayScale = std::min(20.0f, dspParams.decayScale * 3.5f);
    }
    const float pbSemi = mPitchBendSemitones.load(std::memory_order_relaxed);
    if (std::abs(pbSemi) > 1.0e-4f)
    {
        dspParams.fundamentalHz = std::clamp(dspParams.fundamentalHz * braun::mr16::semitonesToRatio(pbSemi), 20.0f, 5000.0f);
    }
    const float modWheel = mModWheelNorm.load(std::memory_order_relaxed);
    if (modWheel > 1.0e-4f)
    {
        dspParams.couplingDepth = std::clamp(dspParams.couplingDepth + modWheel, 0.0f, 1.0f);
    }
    // Guarantee that DSP engine mute state strictly tracks isPoweredOn
    dspParams.outputMute = !isPoweredOn.load(std::memory_order_relaxed);
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

    if (numChannels >= 2)
    {
        mr16Engine.processBlock(inChannels[0], inChannels[1], outChannels[0], outChannels[1], numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            outChannels[0][i] = braun::mr16::flushDenormal(outChannels[0][i]);
            outChannels[1][i] = braun::mr16::flushDenormal(outChannels[1][i]);
        }

        // Zero any auxiliary output channels > 2
        for (int ch = 2; ch < numChannels; ++ch)
        {
            buffer.clear(ch, 0, numSamples);
        }
    }
    else
    {
        // Mono output bus: render stereo in fixed stack chunks and downmix 0.5*(L+R)
        // (0 heap allocations for any block size, preserving balanced stereo/Dimension-D energy)
        constexpr int kMonoChunk = 512;
        float tmpL[kMonoChunk];
        float tmpR[kMonoChunk];
        int offset = 0;
        while (offset < numSamples)
        {
            const int chunk = std::min(kMonoChunk, numSamples - offset);
            const float* chunkInL = (inChannels[0] != nullptr) ? (inChannels[0] + offset) : nullptr;
            const float* chunkInR = (inChannels[1] != nullptr) ? (inChannels[1] + offset) : nullptr;
            mr16Engine.processBlock(chunkInL, chunkInR, tmpL, tmpR, chunk);
            for (int s = 0; s < chunk; ++s)
            {
                outChannels[0][offset + s] = braun::mr16::flushDenormal(0.5f * (tmpL[s] + tmpR[s]));
            }
            offset += chunk;
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

void BRAUN_MR16AudioProcessor::triggerChimeKey(int keyIndex, float velocity, float freqHz) noexcept
{
    if (!isPoweredOn.load(std::memory_order_relaxed))
        setPower(true);

#if MR16_HAS_DSP_ENGINE
    mr16Engine.enqueueTriggerChime(keyIndex, velocity, freqHz);
#else
    juce::ignoreUnused(keyIndex, velocity, freqHz);
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
    mStateSyncEpoch.fetch_add(1, std::memory_order_release);
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
    state.setProperty("scopeSource", mScopeSource.load(std::memory_order_relaxed), nullptr);
    state.setProperty("isPoweredOn", isPoweredOn.load(std::memory_order_relaxed), nullptr);
    state.setProperty("euclideanEnable", isEuclideanEnabled(), nullptr);
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
        if (vt.hasProperty("scopeSource"))
            mScopeSource.store(static_cast<int>(vt.getProperty("scopeSource")), std::memory_order_relaxed);

        const bool hasEuclideanParamChild = vt.getChildWithProperty("id", mr16::ParamIDs::euclideanEnable.getParamID()).isValid();
        const bool hasLegacyEuclideanProp = vt.hasProperty("euclideanEnable");
        const bool legacyEuclideanVal = hasLegacyEuclideanProp ? static_cast<bool>(vt.getProperty("euclideanEnable")) : false;

        const bool hasExplicitPowerProp = vt.hasProperty("isPoweredOn");
        const bool explicitPowerVal = hasExplicitPowerProp ? static_cast<bool>(vt.getProperty("isPoweredOn")) : true;

        apvts.replaceState(vt);

        if (!hasEuclideanParamChild && hasLegacyEuclideanProp)
            setEuclideanEnable(legacyEuclideanVal);
        else
            mEuclideanEnable.store(isEuclideanEnabled(), std::memory_order_relaxed);

        bool restoredPower = explicitPowerVal;
        if (!hasExplicitPowerProp)
        {
            if (auto* p = apvts.getRawParameterValue(mr16::ParamIDs::powerState.getParamID()))
                restoredPower = (p->load(std::memory_order_relaxed) > 0.5f);
        }

        // Standalone cold-boot monitor protection: never auto-blast saved self-oscillating,
        // Poisson rain, Euclidean, or bowed friction state when StandaloneFilterWindow reloads .settings
        if (wrapperType == wrapperType_Standalone && !mInitialStandaloneStateLoaded)
        {
            mInitialStandaloneStateLoaded = true;
            restoredPower = false;
            setEuclideanEnable(false);
            if (auto* pPoisson = apvts.getParameter(mr16::ParamIDs::poissonDensity.getParamID()))
                pPoisson->setValueNotifyingHost(0.0f);
#if MR16_HAS_DSP_ENGINE
            mr16Engine.reset();
#endif
        }

        // Explicitly sync power state & notify Editor
        setPower(restoredPower);
        mStateSyncEpoch.fetch_add(1, std::memory_order_release);
    }
}

// JUCE plugin entry point export
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BRAUN_MR16AudioProcessor();
}
