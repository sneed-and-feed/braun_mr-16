#pragma once

#include "DspMath.h"
#include "KineticExciter.h"
#include "ModalResonatorMatrix.h"
#include "LorenzAttractor.h"
#include "SpatialChorus.h"
#include "VactrolGate.h"
#include "BoundedSaturator.h"

#include <array>
#include <vector>
#include <atomic>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace braun::mr16 {

// ============================================================================
// Parameter Structures
// ============================================================================
struct Mr16Parameters {
    // Deck 01: Kinetic Exciter
    float exciterStrikeVelocity { 0.70f };
    float exciterStrikeHardness { 0.65f };
    float exciterBowVelocity    { 0.0f };
    float exciterBowPressure    { 0.0f };
    bool  poissonEnable         { false };
    float poissonEpm            { 18.0f };
    float poissonHumanize       { 0.50f };
    bool  euclideanEnable       { false };
    int   euclideanPulses       { 4 };
    int   euclideanSteps        { 16 };
    float euclideanBpm          { 120.0f };
    bool  externalAudioEnable   { false };
    float externalSensitivity   { 1.0f };
    float externalDirectMix     { 0.35f };
    MicrotonalScale microtonalScale { MicrotonalScale::TwelveTet };
    float chimeRootHz           { 220.0f };

    // Deck 02: 16-Pole Modal Resonator Matrix
    float fundamentalHz         { 220.0f };
    ManifoldType manifold       { ManifoldType::ChladniPlate };
    float manifoldMorph         { 0.0f };
    MaterialType material       { MaterialType::Wood };
    float decayScale            { 1.0f };
    float couplingDepth         { 0.25f };
    float stereoWidth           { 0.85f };

    // Deck 03: 3D Chaotic Lorenz Attractor
    float chaosRateHz           { 1.0f };
    float chaosDepth            { 0.50f };
    float chaosDetuneCents      { 50.0f };

    // Deck 04: Tri-Phase Spatial BBD Chorus
    bool  chorusEnable          { true };
    DimensionMode chorusDimensionMode { DimensionMode::Mode2 };
    float chorusRateHz          { 0.55f };
    float chorusDepthMs         { 2.20f };
    float chorusMix             { 0.45f };

    // Deck 05: Spatial Dispersion & Dynamics
    bool  vactrolSagEnable      { true };
    float vactrolSagAmount      { 0.50f };
    float vactrolDecaySec       { 0.080f };
    float saturatorKnee         { 0.72f };
    float saturatorCeiling      { 1.05f };

    // Deck 07: Master Utilities
    float masterVolumeDb        { 0.0f };
    bool  outputMute            { false };
    bool  freezeHold            { false };
};

// ============================================================================
// Factory Preset Definition
// ============================================================================
struct PresetDefinition {
    const char* id;
    const char* name;
    const char* category;
    const char* description;
    Mr16Parameters params;
};

// ============================================================================
// Visualizer Telemetry Frame for Deck 06 Phosphor CRT Scope
// ============================================================================
struct VisualizerFrame {
    std::array<float, ModalResonatorMatrix::kNumModes> modalEnergies { 0.0f };
    std::array<float, ModalResonatorMatrix::kNumModes> modalFrequencies { 0.0f };
    float lorenzX { 0.0f };
    float lorenzY { 0.0f };
    float lorenzZ { 0.0f };
    float lorenzNormX { 0.0f };
    float lorenzNormY { 0.0f };
    float lorenzNormZ { 0.0f };
    float chladniFundamentalHz { 220.0f };
    float manifoldMorph { 0.0f };
    int   manifoldType { 0 };
    float inputRmsL { 0.0f };
    float inputRmsR { 0.0f };
    float outputRmsL { 0.0f };
    float outputRmsR { 0.0f };
    float exciterActivity { 0.0f };
    bool  inContact { false };
    std::array<float, 256> scopeSamplesL { 0.0f };
    std::array<float, 256> scopeSamplesR { 0.0f };
};

// ============================================================================
// Lock-Free Performance Event Queue
// ============================================================================
struct TriggerEvent {
    enum class Type : int { Strike = 0, StrikeButton = 1, ChimeKey = 2, NoteOn = 3 };
    Type  type { Type::Strike };
    int   index { 0 };
    float velocity { 1.0f };
    float hardness { 0.65f };
    float freqHz { 220.0f };
};

// ============================================================================
// Master BRAUN MR-16 Engine
// Coordinates all 7 decks in hard real time (zero heap allocations in audio loop)
// ============================================================================
class Mr16Engine {
public:
    static constexpr size_t kMaxBlockSize = 2048;
    static constexpr size_t kEventQueueCapacity = 128;
    static constexpr size_t kEventQueueMask = kEventQueueCapacity - 1;

    Mr16Engine() noexcept;
    ~Mr16Engine() noexcept = default;

    // Factory presets
    [[nodiscard]] static std::vector<PresetDefinition> getFactoryPresets();

    // Lifecycle
    void prepare(double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;

    // Parameter configuration
    void setParameters(const Mr16Parameters& params) noexcept;
    [[nodiscard]] const Mr16Parameters& getParameters() const noexcept { return mParams; }

    // Performance triggers (lock-free thread-safe push)
    bool enqueueTriggerStrike(float velocity, float hardness = 0.65f) noexcept;
    bool enqueueTriggerButton(int buttonIndex, float velocity = 1.0f) noexcept;
    bool enqueueTriggerChime(int keyIndex, float velocity = 1.0f) noexcept;
    bool enqueueMidiNoteOn(int midiNote, float velocity) noexcept;

    // Hard real-time audio block processing
    // Strictly ZERO heap allocations, 1 ScopedNoDenormals per block, lock-free
    void processBlock(const float* inL, const float* inR,
                      float* outL, float* outR, int numSamples) noexcept;

    // In-place processing
    void processBlock(float* bufferL, float* bufferR, int numSamples) noexcept;

    // Telemetry query for Deck 06 Phosphor CRT Scope
    [[nodiscard]] VisualizerFrame getVisualizerFrame() const noexcept;

private:
    void applyParametersToDsp() noexcept;
    void drainEventQueue() noexcept;

    float mSampleRate { 48000.0f };
    int mMaxBlockSize { 512 };

    // Subsystem DSP instances (Decks 01 to 05)
    KineticExciter       mExciter;
    ModalResonatorMatrix mModalMatrix;
    LorenzAttractor      mLorenz;
    SpatialChorus        mChorus;
    VactrolGate          mVactrolGate;
    BoundedSaturator     mSaturator;

    // Parameters
    Mr16Parameters mParams;
    OnePoleSmoother mVolumeSmoother;

    // SPSC lock-free ring buffer for incoming UI/MIDI trigger events
    std::array<TriggerEvent, kEventQueueCapacity> mEventQueue {};
    std::atomic<size_t> mQueueWriteHead { 0 };
    size_t mQueueReadHead { 0 };

    // Telemetry tracking
    mutable VisualizerFrame mVisualizerTelemetry;
    float mInputRmsL { 0.0f };
    float mInputRmsR { 0.0f };
    float mOutputRmsL { 0.0f };
    float mOutputRmsR { 0.0f };
};

} // namespace braun::mr16
