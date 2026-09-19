#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>
#include <atomic>
#include <array>
#include <string>
#include <cmath>
#include <algorithm>

#if __has_include("../dsp/Mr16Engine.h")
#include "../dsp/Mr16Engine.h"
#define MR16_HAS_DSP_ENGINE 1
#else
#define MR16_HAS_DSP_ENGINE 0
#endif

namespace braun::mr16 {

// ============================================================================
// Parameter IDs (APVTS Identifiers across all 7 Decks)
// ============================================================================
namespace ParamIDs {
    // Deck 01: Kinetic Exciter Engine
    inline const juce::ParameterID exciterType      { "exciter_type", 1 };
    inline const juce::ParameterID strikeHardness   { "strike_hardness", 1 };
    inline const juce::ParameterID strikeVelocity   { "strike_velocity", 1 };
    inline const juce::ParameterID frictionForce    { "friction_force", 1 };
    inline const juce::ParameterID frictionSpeed    { "friction_speed", 1 };
    inline const juce::ParameterID vactrolSag       { "vactrol_sag", 1 };
    inline const juce::ParameterID extInputGain     { "ext_input_gain", 1 };
    inline const juce::ParameterID poissonDensity   { "poisson_density", 1 };
    inline const juce::ParameterID euclideanPulses  { "euclidean_pulses", 1 };
    inline const juce::ParameterID euclideanSteps   { "euclidean_steps", 1 };

    // Deck 02: 16-Pole Modal Resonator Matrix
    inline const juce::ParameterID manifoldType     { "manifold_type", 1 };
    inline const juce::ParameterID modalFrequency   { "modal_frequency", 1 };
    inline const juce::ParameterID modalDamping     { "modal_damping", 1 };
    inline const juce::ParameterID materialProfile  { "material_profile", 1 };
    inline const juce::ParameterID modalCoupling    { "modal_coupling", 1 };
    inline const juce::ParameterID modalSpread      { "modal_spread", 1 };
    inline const juce::ParameterID modalQ           { "modal_q", 1 };

    // Deck 03: Kinetic Morph & 3D Chaotic Attractor
    inline const juce::ParameterID lorenzRate       { "lorenz_rate", 1 };
    inline const juce::ParameterID lorenzChaos      { "lorenz_chaos", 1 };
    inline const juce::ParameterID lorenzFreqMod    { "lorenz_freq_mod", 1 };
    inline const juce::ParameterID lorenzQMod       { "lorenz_q_mod", 1 };

    // Deck 04: Tri-Phase Spatial BBD Chorus
    inline const juce::ParameterID chorusEnable     { "chorus_enable", 1 };
    inline const juce::ParameterID chorusRateHz     { "chorus_rate_hz", 1 };
    inline const juce::ParameterID chorusDepthMs    { "chorus_depth_ms", 1 };
    inline const juce::ParameterID chorusDimension  { "chorus_dimension", 1 };
    inline const juce::ParameterID chorusMix        { "chorus_mix", 1 };

    // Deck 05: Spatial Dispersion, Vactrol LPG & Dynamics
    inline const juce::ParameterID goldenPanSpread  { "golden_pan_spread", 1 };
    inline const juce::ParameterID vactrolLpgCutoff { "vactrol_lpg_cutoff", 1 };
    inline const juce::ParameterID driveSaturation  { "drive_saturation", 1 };
    inline const juce::ParameterID masterTrimDb     { "master_trim_db", 1 };
    inline const juce::ParameterID dryWetMix        { "dry_wet_mix", 1 };
    inline const juce::ParameterID powerState       { "power_state", 1 };

    // Deck 06: Vector Phosphor CRT Display
    inline const juce::ParameterID displayMode      { "display_mode", 1 };
}

// ============================================================================
// Strongly-Typed Enums
// ============================================================================
enum class ExciterType : int {
    Strike   = 0,
    Friction = 1,
    Vactrol  = 2,
    ExtIn    = 3
};

#if !MR16_HAS_DSP_ENGINE
enum class ManifoldType : int {
    ChladniPlate = 0,
    StiffBeam    = 1,
    VocalFormant = 2,
    PoincareHorn = 3
};

enum class MaterialType : int {
    Wood  = 0,
    Glass = 1,
    Steel = 2,
    Brass = 3,
    Nylon = 4
};
#endif

using MaterialProfile = MaterialType;
using ExciterModel = ExciterType;

enum class DisplayMode : int {
    Chladni   = 0,
    Attractor = 1,
    ModalFft  = 2
};

// ============================================================================
// Choice Helpers & Option String Arrays
// ============================================================================
inline const juce::StringArray& getExciterTypeChoices() {
    static const juce::StringArray choices {
        "Mass-Spring Strike",
        "Stick-Slip Friction",
        "Vactrol Air Jet",
        "External Line In"
    };
    return choices;
}

inline const juce::StringArray& getManifoldTypeChoices() {
    static const juce::StringArray choices {
        "Biharmonic Chladni Plate",
        "Stiff Struck Beam",
        "Vocal Formant Tract",
        "Poincaré Hyperbolic Horn"
    };
    return choices;
}

inline const juce::StringArray& getMaterialProfileChoices() {
    static const juce::StringArray choices {
        "Acoustic Wood",
        "Resonant Glass",
        "Hardened Steel",
        "Harmonic Brass",
        "Nylon / Polymer"
    };
    return choices;
}

inline const juce::StringArray& getDisplayModeChoices() {
    static const juce::StringArray choices {
        "Chladni 2D Geometry",
        "3D Lorenz Attractor",
        "16-Pole Modal FFT"
    };
    return choices;
}

// Choice-to-Index & Index-to-Choice Mappings
inline ExciterType exciterTypeFromIndex(int index) noexcept {
    switch (index) {
        case 0: return ExciterType::Strike;
        case 1: return ExciterType::Friction;
        case 2: return ExciterType::Vactrol;
        case 3: return ExciterType::ExtIn;
        default: return ExciterType::Strike;
    }
}

inline int indexFromExciterType(ExciterType type) noexcept {
    return static_cast<int>(type);
}

inline ManifoldType manifoldTypeFromIndex(int index) noexcept {
    switch (index) {
        case 0: return ManifoldType::ChladniPlate;
        case 1: return ManifoldType::StiffBeam;
        case 2: return ManifoldType::VocalFormant;
        case 3: return ManifoldType::PoincareHorn;
        default: return ManifoldType::ChladniPlate;
    }
}

inline int indexFromManifoldType(ManifoldType type) noexcept {
    return static_cast<int>(type);
}

inline MaterialProfile materialProfileFromIndex(int index) noexcept {
    switch (index) {
        case 0: return MaterialProfile::Wood;
        case 1: return MaterialProfile::Glass;
        case 2: return MaterialProfile::Steel;
        case 3: return MaterialProfile::Brass;
        case 4: return MaterialProfile::Nylon;
        default: return MaterialProfile::Wood;
    }
}

inline int indexFromMaterialProfile(MaterialProfile profile) noexcept {
    return static_cast<int>(profile);
}

inline DisplayMode displayModeFromIndex(int index) noexcept {
    switch (index) {
        case 0: return DisplayMode::Chladni;
        case 1: return DisplayMode::Attractor;
        case 2: return DisplayMode::ModalFft;
        default: return DisplayMode::Chladni;
    }
}

inline int indexFromDisplayMode(DisplayMode mode) noexcept {
    return static_cast<int>(mode);
}

// ============================================================================
// Parameter Metadata Table for 2-Way APVTS / Web / GUI Synchronization
// ============================================================================
struct ParameterMetadata {
    int deck;
    const char* apvtsId;
    const char* webId;
    const char* name;
    const char* unit;
    float minVal;
    float maxVal;
    float defaultVal;
    bool isBool;
    bool isChoice;
};

inline const std::array<ParameterMetadata, 33>& getParameterMetadataTable() {
    static const std::array<ParameterMetadata, 33> table {{
        // Deck 01: Kinetic Exciter Engine
        { 1, "exciter_type",      "exciterType",     "Exciter Model",       "",     0.0f,    3.0f,     0.0f,    false, true  },
        { 1, "strike_hardness",   "strikeHardness",  "Strike Hardness",     "%",    0.0f,    1.0f,     0.50f,   false, false },
        { 1, "strike_velocity",   "strikeVelocity",  "Strike Velocity",     "%",    0.0f,    1.0f,     0.80f,   false, false },
        { 1, "friction_force",    "frictionForce",   "Friction Force",      "%",    0.0f,    1.0f,     0.35f,   false, false },
        { 1, "friction_speed",    "frictionSpeed",   "Friction Speed",      "%",    0.0f,    1.0f,     0.40f,   false, false },
        { 1, "vactrol_sag",       "vactrolSag",      "Vactrol Sag",         "%",    0.0f,    1.0f,     0.60f,   false, false },
        { 1, "ext_input_gain",    "extInputGain",    "Ext Input Gain",      "dB",   -24.0f,  12.0f,    0.0f,    false, false },
        { 1, "poisson_density",   "poissonDensity",  "Poisson Density",     "Hz",   0.0f,    25.0f,    0.0f,    false, false },
        { 1, "euclidean_pulses",  "euclideanPulses", "Euclidean Pulses",    "",     0.0f,    32.0f,    4.0f,    false, false },
        { 1, "euclidean_steps",   "euclideanSteps",  "Euclidean Steps",     "",     1.0f,    32.0f,    16.0f,   false, false },

        // Deck 02: 16-Pole Modal Resonator Matrix
        { 2, "manifold_type",     "manifoldType",    "Resonant Manifold",   "",     0.0f,    3.0f,     0.0f,    false, true  },
        { 2, "modal_frequency",   "modalFrequency",  "Fundamental Freq",    "Hz",   20.0f,   5000.0f,  440.0f,  false, false },
        { 2, "modal_damping",     "modalDamping",    "Modal Damping RT60",  "s",    0.05f,   10.0f,    1.80f,   false, false },
        { 2, "material_profile",  "materialProfile", "Material Profile",    "",     0.0f,    4.0f,     0.0f,    false, true  },
        { 2, "modal_coupling",    "modalCoupling",   "Modal Coupling",      "%",    0.0f,    1.0f,     0.25f,   false, false },
        { 2, "modal_spread",      "modalSpread",     "Harmonic Dispersion", "x",    0.2f,    3.0f,     1.00f,   false, false },
        { 2, "modal_q",           "modalQ",          "Resonance Q",         "Q",    5.0f,    500.0f,   50.0f,   false, false },

        // Deck 03: Kinetic Morph & 3D Chaotic Attractor
        { 3, "lorenz_rate",       "lorenzRate",      "Chaos Rate",          "Hz",   0.01f,   10.0f,    0.50f,   false, false },
        { 3, "lorenz_chaos",      "lorenzChaos",     "Chaos Parameter",     "%",    0.0f,    1.0f,     0.50f,   false, false },
        { 3, "lorenz_freq_mod",   "lorenzFreqMod",   "Freq Chaos Mod",      "%",    0.0f,    1.0f,     0.20f,   false, false },
        { 3, "lorenz_q_mod",      "lorenzQMod",      "Damping Chaos Mod",   "%",    0.0f,    1.0f,     0.15f,   false, false },

        // Deck 04: Tri-Phase Spatial BBD Chorus
        { 4, "chorus_enable",     "chorusEnable",    "Chorus Enable",       "",     0.0f,    1.0f,     1.0f,    true,  false },
        { 4, "chorus_rate_hz",    "chorusRateHz",    "Chorus Rate",         "Hz",   0.05f,   5.0f,     0.80f,   false, false },
        { 4, "chorus_depth_ms",   "chorusDepthMs",   "Chorus Depth",        "ms",   0.1f,    10.0f,    2.50f,   false, false },
        { 4, "chorus_dimension",  "chorusDimension", "Dimension Spread",    "%",    0.0f,    1.0f,     0.75f,   false, false },
        { 4, "chorus_mix",        "chorusMix",       "Chorus Mix",          "%",    0.0f,    1.0f,     0.50f,   false, false },

        // Deck 05: Spatial Dispersion, Vactrol LPG & Dynamics
        { 5, "golden_pan_spread", "goldenPanSpread", "Spatial Dispersion",  "%",    0.0f,    1.0f,     0.80f,   false, false },
        { 5, "vactrol_lpg_cutoff","vactrolLpgCutoff","Vactrol LPG Cutoff",  "Hz",   100.0f,  20000.0f, 12000.0f,false, false },
        { 5, "drive_saturation",  "driveSaturation", "Resonator Drive",     "%",    0.0f,    1.0f,     0.25f,   false, false },
        { 5, "master_trim_db",    "masterTrimDb",    "Output Trim",         "dB",   -24.0f,  12.0f,    0.0f,    false, false },
        { 5, "dry_wet_mix",       "dryWetMix",       "Dry / Wet Mix",       "%",    0.0f,    1.0f,     0.65f,   false, false },
        { 5, "power_state",       "powerState",      "Power Standby",       "",     0.0f,    1.0f,     1.0f,    true,  false },

        // Deck 06: Vector Phosphor CRT Display
        { 6, "display_mode",      "displayMode",     "Vector Display Mode", "",     0.0f,    2.0f,     0.0f,    false, true  }
    }};
    return table;
}

// ============================================================================
// Real-Time Telemetry Frame for 60 FPS Phosphor CRT Scope & Meters
// ============================================================================
#if !MR16_HAS_DSP_ENGINE
struct VisualizerFrame {
    std::array<float, 16> modalEnergies {};
    float lorenzX { 0.0f };
    float lorenzY { 0.0f };
    float lorenzZ { 0.0f };
    float exciterActivity { 0.0f };
    std::array<float, 256> scopeSamplesL {};
    std::array<float, 256> scopeSamplesR {};
};
#endif

// ============================================================================
// Lock-Free Plain-Old-Data (POD) Parameter Snapshot Structure
// Strictly cache-line aligned (64 bytes) to avoid false sharing across cores
// ============================================================================
struct alignas(64) Mr16ParameterSnapshot {
    // Deck 01: Kinetic Exciter Engine
    ExciterType exciterType      { ExciterType::Strike };
    float       strikeHardness   { 0.50f };
    float       strikeVelocity   { 0.80f };
    float       frictionForce    { 0.35f };
    float       frictionSpeed    { 0.40f };
    float       vactrolSag       { 0.60f };
    float       extInputGainDb   { 0.0f };
    float       poissonDensity   { 0.0f };
    bool        euclideanEnable  { false };
    int         euclideanPulses  { 4 };
    int         euclideanSteps   { 16 };

    // Deck 02: 16-Pole Modal Resonator Matrix
    ManifoldType    manifoldType    { ManifoldType::ChladniPlate };
    float           modalFrequency  { 440.0f };
    float           modalDamping    { 1.80f };
    MaterialProfile materialProfile { MaterialProfile::Wood };
    float           modalCoupling   { 0.25f };
    float           modalSpread     { 1.00f };
    float           modalQ          { 50.0f };

    // Deck 03: Kinetic Morph & 3D Chaotic Attractor
    float lorenzRate       { 0.50f };
    float lorenzChaos      { 0.50f };
    float lorenzFreqMod    { 0.20f };
    float lorenzQMod       { 0.15f };

    // Deck 04: Tri-Phase Spatial BBD Chorus
    bool  chorusEnable     { true };
    float chorusRateHz     { 0.80f };
    float chorusDepthMs    { 2.50f };
    float chorusDimension  { 0.75f };
    float chorusMix        { 0.50f };

    // Deck 05: Spatial Dispersion, Vactrol LPG & Dynamics
    float goldenPanSpread   { 0.80f };
    float vactrolLpgCutoff  { 12000.0f };
    float driveSaturationDb { 0.25f };
    float masterTrimDb      { 0.0f };
    float dryWetMix         { 0.65f };
    bool  powerState        { true };

    // Deck 06: Vector Phosphor CRT Display
    DisplayMode displayMode { DisplayMode::Chladni };

#if MR16_HAS_DSP_ENGINE
    [[nodiscard]] braun::mr16::Mr16Parameters toDspParams() const noexcept {
        braun::mr16::Mr16Parameters p;
        p.exciterStrikeVelocity = strikeVelocity;
        p.exciterStrikeHardness = strikeHardness;
        const bool isFriction   = (exciterType == ExciterType::Friction);
        p.exciterBowPressure    = isFriction ? frictionForce : 0.0f;
        p.exciterBowVelocity    = isFriction ? frictionSpeed : 0.0f;
        p.poissonEnable         = (poissonDensity > 0.01f);
        p.poissonEpm            = poissonDensity * 60.0f;
        p.euclideanEnable       = euclideanEnable;
        p.euclideanPulses       = euclideanPulses;
        p.euclideanSteps        = euclideanSteps;
        p.externalAudioEnable   = (exciterType == ExciterType::ExtIn && extInputGainDb > -23.9f);
        p.externalSensitivity   = std::pow(10.0f, extInputGainDb * 0.05f);

        p.fundamentalHz         = modalFrequency;
        p.manifold              = static_cast<braun::mr16::ManifoldType>(static_cast<int>(manifoldType));
        p.material              = static_cast<braun::mr16::MaterialType>(static_cast<int>(materialProfile));
        p.decayScale            = modalDamping;
        p.couplingDepth         = modalCoupling;
        p.stereoWidth           = goldenPanSpread;
        p.modalQScale           = modalQ / 50.0f;
        p.overtoneSpread        = modalSpread;

        p.chaosRateHz           = lorenzRate;
        p.chaosDepth            = lorenzChaos;
        p.chaosDetuneCents      = lorenzFreqMod * 300.0f;

        p.chorusEnable          = chorusEnable;
        p.chorusRateHz          = chorusRateHz;
        p.chorusDepthMs         = chorusDepthMs;
        p.chorusMix             = chorusMix;
        p.chorusDimension       = chorusDimension;
        p.chorusDimensionMode   = braun::mr16::DimensionMode::Manual;

        p.vactrolSagEnable      = (vactrolSag > 0.02f);
        p.vactrolSagAmount      = vactrolSag;
        p.vactrolLpgCutoff      = vactrolLpgCutoff;
        p.driveSaturation       = driveSaturationDb;
        p.dryWetMix             = dryWetMix;
        p.masterVolumeDb        = masterTrimDb;
        p.outputMute            = !powerState;
        return p;
    }
#endif
};

// ============================================================================
// Cached Atomic Parameter Pointers for Wait-Free Real-Time Snapshotting
// ============================================================================
struct Mr16AtomicPointers {
    std::atomic<float>* exciterType      { nullptr };
    std::atomic<float>* strikeHardness   { nullptr };
    std::atomic<float>* strikeVelocity   { nullptr };
    std::atomic<float>* frictionForce    { nullptr };
    std::atomic<float>* frictionSpeed    { nullptr };
    std::atomic<float>* vactrolSag       { nullptr };
    std::atomic<float>* extInputGain     { nullptr };
    std::atomic<float>* poissonDensity   { nullptr };
    std::atomic<bool>*  euclideanEnable  { nullptr };
    std::atomic<float>* euclideanPulses  { nullptr };
    std::atomic<float>* euclideanSteps   { nullptr };

    std::atomic<float>* manifoldType     { nullptr };
    std::atomic<float>* modalFrequency   { nullptr };
    std::atomic<float>* modalDamping     { nullptr };
    std::atomic<float>* materialProfile  { nullptr };
    std::atomic<float>* modalCoupling    { nullptr };
    std::atomic<float>* modalSpread      { nullptr };
    std::atomic<float>* modalQ           { nullptr };

    std::atomic<float>* lorenzRate       { nullptr };
    std::atomic<float>* lorenzChaos      { nullptr };
    std::atomic<float>* lorenzFreqMod    { nullptr };
    std::atomic<float>* lorenzQMod       { nullptr };

    std::atomic<float>* chorusEnable     { nullptr };
    std::atomic<float>* chorusRateHz     { nullptr };
    std::atomic<float>* chorusDepthMs    { nullptr };
    std::atomic<float>* chorusDimension  { nullptr };
    std::atomic<float>* chorusMix        { nullptr };

    std::atomic<float>* goldenPanSpread  { nullptr };
    std::atomic<float>* vactrolLpgCutoff { nullptr };
    std::atomic<float>* driveSaturation  { nullptr };
    std::atomic<float>* masterTrimDb     { nullptr };
    std::atomic<float>* dryWetMix        { nullptr };
    std::atomic<float>* powerState       { nullptr };

    std::atomic<float>* displayMode      { nullptr };

    void initialize(juce::AudioProcessorValueTreeState& apvts) noexcept {
        exciterType      = apvts.getRawParameterValue(ParamIDs::exciterType.getParamID());
        strikeHardness   = apvts.getRawParameterValue(ParamIDs::strikeHardness.getParamID());
        strikeVelocity   = apvts.getRawParameterValue(ParamIDs::strikeVelocity.getParamID());
        frictionForce    = apvts.getRawParameterValue(ParamIDs::frictionForce.getParamID());
        frictionSpeed    = apvts.getRawParameterValue(ParamIDs::frictionSpeed.getParamID());
        vactrolSag       = apvts.getRawParameterValue(ParamIDs::vactrolSag.getParamID());
        extInputGain     = apvts.getRawParameterValue(ParamIDs::extInputGain.getParamID());
        poissonDensity   = apvts.getRawParameterValue(ParamIDs::poissonDensity.getParamID());
        euclideanPulses  = apvts.getRawParameterValue(ParamIDs::euclideanPulses.getParamID());
        euclideanSteps   = apvts.getRawParameterValue(ParamIDs::euclideanSteps.getParamID());

        manifoldType     = apvts.getRawParameterValue(ParamIDs::manifoldType.getParamID());
        modalFrequency   = apvts.getRawParameterValue(ParamIDs::modalFrequency.getParamID());
        modalDamping     = apvts.getRawParameterValue(ParamIDs::modalDamping.getParamID());
        materialProfile  = apvts.getRawParameterValue(ParamIDs::materialProfile.getParamID());
        modalCoupling    = apvts.getRawParameterValue(ParamIDs::modalCoupling.getParamID());
        modalSpread      = apvts.getRawParameterValue(ParamIDs::modalSpread.getParamID());
        modalQ           = apvts.getRawParameterValue(ParamIDs::modalQ.getParamID());

        lorenzRate       = apvts.getRawParameterValue(ParamIDs::lorenzRate.getParamID());
        lorenzChaos      = apvts.getRawParameterValue(ParamIDs::lorenzChaos.getParamID());
        lorenzFreqMod    = apvts.getRawParameterValue(ParamIDs::lorenzFreqMod.getParamID());
        lorenzQMod       = apvts.getRawParameterValue(ParamIDs::lorenzQMod.getParamID());

        chorusEnable     = apvts.getRawParameterValue(ParamIDs::chorusEnable.getParamID());
        chorusRateHz     = apvts.getRawParameterValue(ParamIDs::chorusRateHz.getParamID());
        chorusDepthMs    = apvts.getRawParameterValue(ParamIDs::chorusDepthMs.getParamID());
        chorusDimension  = apvts.getRawParameterValue(ParamIDs::chorusDimension.getParamID());
        chorusMix        = apvts.getRawParameterValue(ParamIDs::chorusMix.getParamID());

        goldenPanSpread  = apvts.getRawParameterValue(ParamIDs::goldenPanSpread.getParamID());
        vactrolLpgCutoff = apvts.getRawParameterValue(ParamIDs::vactrolLpgCutoff.getParamID());
        driveSaturation  = apvts.getRawParameterValue(ParamIDs::driveSaturation.getParamID());
        masterTrimDb     = apvts.getRawParameterValue(ParamIDs::masterTrimDb.getParamID());
        dryWetMix        = apvts.getRawParameterValue(ParamIDs::dryWetMix.getParamID());
        powerState       = apvts.getRawParameterValue(ParamIDs::powerState.getParamID());

        displayMode      = apvts.getRawParameterValue(ParamIDs::displayMode.getParamID());
    }

    [[nodiscard]] Mr16ParameterSnapshot loadSnapshot() const noexcept {
        Mr16ParameterSnapshot s;

        // Deck 01
        if (exciterType) {
            const int idx = static_cast<int>(std::round(exciterType->load(std::memory_order_relaxed)));
            s.exciterType = exciterTypeFromIndex(idx);
        }
        if (strikeHardness)  s.strikeHardness  = strikeHardness->load(std::memory_order_relaxed);
        if (strikeVelocity)  s.strikeVelocity  = strikeVelocity->load(std::memory_order_relaxed);
        if (frictionForce)   s.frictionForce   = frictionForce->load(std::memory_order_relaxed);
        if (frictionSpeed)   s.frictionSpeed   = frictionSpeed->load(std::memory_order_relaxed);
        if (vactrolSag)      s.vactrolSag      = vactrolSag->load(std::memory_order_relaxed);
        if (extInputGain)    s.extInputGainDb  = extInputGain->load(std::memory_order_relaxed);
        if (poissonDensity)  s.poissonDensity  = poissonDensity->load(std::memory_order_relaxed);
        if (euclideanEnable) s.euclideanEnable = euclideanEnable->load(std::memory_order_relaxed);
        if (euclideanPulses) s.euclideanPulses = static_cast<int>(std::round(euclideanPulses->load(std::memory_order_relaxed)));
        if (euclideanSteps)  s.euclideanSteps  = static_cast<int>(std::round(euclideanSteps->load(std::memory_order_relaxed)));

        // Deck 02
        if (manifoldType) {
            const int idx = static_cast<int>(std::round(manifoldType->load(std::memory_order_relaxed)));
            s.manifoldType = manifoldTypeFromIndex(idx);
        }
        if (modalFrequency)  s.modalFrequency  = modalFrequency->load(std::memory_order_relaxed);
        if (modalDamping)    s.modalDamping    = modalDamping->load(std::memory_order_relaxed);
        if (materialProfile) {
            const int idx = static_cast<int>(std::round(materialProfile->load(std::memory_order_relaxed)));
            s.materialProfile = materialProfileFromIndex(idx);
        }
        if (modalCoupling)   s.modalCoupling   = modalCoupling->load(std::memory_order_relaxed);
        if (modalSpread)     s.modalSpread     = modalSpread->load(std::memory_order_relaxed);
        if (modalQ)          s.modalQ          = modalQ->load(std::memory_order_relaxed);

        // Deck 03
        if (lorenzRate)      s.lorenzRate      = lorenzRate->load(std::memory_order_relaxed);
        if (lorenzChaos)     s.lorenzChaos     = lorenzChaos->load(std::memory_order_relaxed);
        if (lorenzFreqMod)   s.lorenzFreqMod   = lorenzFreqMod->load(std::memory_order_relaxed);
        if (lorenzQMod)      s.lorenzQMod      = lorenzQMod->load(std::memory_order_relaxed);

        // Deck 04
        if (chorusEnable)    s.chorusEnable    = (chorusEnable->load(std::memory_order_relaxed) > 0.5f);
        if (chorusRateHz)    s.chorusRateHz    = chorusRateHz->load(std::memory_order_relaxed);
        if (chorusDepthMs)   s.chorusDepthMs   = chorusDepthMs->load(std::memory_order_relaxed);
        if (chorusDimension) s.chorusDimension = chorusDimension->load(std::memory_order_relaxed);
        if (chorusMix)       s.chorusMix       = chorusMix->load(std::memory_order_relaxed);

        // Deck 05
        if (goldenPanSpread)   s.goldenPanSpread   = goldenPanSpread->load(std::memory_order_relaxed);
        if (vactrolLpgCutoff)  s.vactrolLpgCutoff  = vactrolLpgCutoff->load(std::memory_order_relaxed);
        if (driveSaturation)   s.driveSaturationDb = driveSaturation->load(std::memory_order_relaxed);
        if (masterTrimDb)      s.masterTrimDb      = masterTrimDb->load(std::memory_order_relaxed);
        if (dryWetMix)         s.dryWetMix         = dryWetMix->load(std::memory_order_relaxed);
        if (powerState)        s.powerState        = (powerState->load(std::memory_order_relaxed) > 0.5f);

        // Deck 06
        if (displayMode) {
            const int idx = static_cast<int>(std::round(displayMode->load(std::memory_order_relaxed)));
            s.displayMode = displayModeFromIndex(idx);
        }

        return s;
    }
};

// ============================================================================
// APVTS ParameterLayout Factory
// ============================================================================
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ------------------------------------------------------------------------
    // Deck 01: Kinetic Exciter Engine
    // ------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::exciterType,
        "Exciter Model",
        getExciterTypeChoices(),
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::strikeHardness,
        "Strike Hardness",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.50f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::strikeVelocity,
        "Strike Velocity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.80f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::frictionForce,
        "Friction Force",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.35f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::frictionSpeed,
        "Friction Speed",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.40f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::vactrolSag,
        "Vactrol Sag",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.60f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::extInputGain,
        "Ext Input Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f, 1.0f),
        -24.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 1) + " dB";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::poissonDensity,
        "Poisson Density",
        juce::NormalisableRange<float>(0.0f, 25.0f, 0.1f, 0.5f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 1) + " Hz";
            })));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::euclideanPulses,
        "Euclidean Pulses",
        0, 32,
        4));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::euclideanSteps,
        "Euclidean Steps",
        1, 32,
        16));

    // ------------------------------------------------------------------------
    // Deck 02: 16-Pole Modal Resonator Matrix
    // ------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::manifoldType,
        "Resonant Manifold",
        getManifoldTypeChoices(),
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::modalFrequency,
        "Fundamental Freq",
        juce::NormalisableRange<float>(20.0f, 5000.0f, 0.1f, 0.35f),
        440.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 1) + " Hz";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::modalDamping,
        "Modal Damping RT60",
        juce::NormalisableRange<float>(0.05f, 10.0f, 0.01f, 0.35f),
        1.80f,
        juce::AudioParameterFloatAttributes().withLabel("s")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 2) + " s";
            })));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::materialProfile,
        "Material Profile",
        getMaterialProfileChoices(),
        0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::modalCoupling,
        "Modal Coupling",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.25f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::modalSpread,
        "Harmonic Dispersion",
        juce::NormalisableRange<float>(0.2f, 3.0f, 0.01f, 0.8f),
        1.00f,
        juce::AudioParameterFloatAttributes().withLabel("x")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 2) + " x";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::modalQ,
        "Resonance Q",
        juce::NormalisableRange<float>(5.0f, 500.0f, 0.1f, 0.4f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("Q")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 1) + " Q";
            })));

    // ------------------------------------------------------------------------
    // Deck 03: Kinetic Morph & 3D Chaotic Attractor
    // ------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lorenzRate,
        "Chaos Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.4f),
        0.50f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 2) + " Hz";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lorenzChaos,
        "Chaos Parameter",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.50f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lorenzFreqMod,
        "Freq Chaos Mod",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.20f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lorenzQMod,
        "Damping Chaos Mod",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.15f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    // ------------------------------------------------------------------------
    // Deck 04: Tri-Phase Spatial BBD Chorus
    // ------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::chorusEnable,
        "Chorus Enable",
        true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::chorusRateHz,
        "Chorus Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.5f),
        0.80f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 2) + " Hz";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::chorusDepthMs,
        "Chorus Depth",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.6f),
        2.50f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 2) + " ms";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::chorusDimension,
        "Dimension Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.75f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::chorusMix,
        "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.50f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    // ------------------------------------------------------------------------
    // Deck 05: Spatial Dispersion, Vactrol LPG & Dynamics
    // ------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::goldenPanSpread,
        "Spatial Dispersion",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.80f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::vactrolLpgCutoff,
        "Vactrol LPG Cutoff",
        juce::NormalisableRange<float>(100.0f, 20000.0f, 1.0f, 0.35f),
        12000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val))) + " Hz";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::driveSaturation,
        "Resonator Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.25f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::masterTrimDb,
        "Output Trim",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(val, 1) + " dB";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::dryWetMix,
        "Dry / Wet Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f),
        0.65f,
        juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float val, int) {
                return juce::String(static_cast<int>(std::round(val * 100.0f))) + " %";
            })));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::powerState,
        "Power Standby",
        true));

    // ------------------------------------------------------------------------
    // Deck 06: Vector Phosphor CRT Display
    // ------------------------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::displayMode,
        "Vector Display Mode",
        getDisplayModeChoices(),
        0));

    return { params.begin(), params.end() };
}

} // namespace braun::mr16

namespace mr16 = braun::mr16;
