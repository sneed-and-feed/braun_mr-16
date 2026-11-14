#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include "Parameters.h"
#include "../dsp/DspMath.h"
#if __has_include("../dsp/Mr16Engine.h")
#include "../dsp/Mr16Engine.h"
#endif

class BRAUN_MR16AudioProcessor : public juce::AudioProcessor,
                                 private juce::Timer
{
public:
    BRAUN_MR16AudioProcessor();
    ~BRAUN_MR16AudioProcessor() override;

    // Hard real-time lifecycle
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Audio callback: Guaranteed 0 heap allocations, 0 locks, block-level ScopedNoDenormals
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    void setCurrentProgramIndex(int index) noexcept { mCurrentProgram = std::clamp(index, 0, getNumPrograms() - 1); }
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    // Preset & State Serialization
    void setPresetParameters(const mr16::Mr16ParameterSnapshot& p);
#if MR16_HAS_DSP_ENGINE
    void setPresetParameters(const braun::mr16::Mr16Parameters& p);
#endif
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // APVTS & Engine Accessors
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const mr16::Mr16AtomicPointers& getAtomicPointers() const noexcept { return atomicPointers; }

#if MR16_HAS_DSP_ENGINE
    braun::mr16::Mr16Engine& getDspEngine() noexcept { return mr16Engine; }
#endif

    // Power lifecycle control (message-thread safe; audio thread uses lock-free wake flag)
    void setPower(bool powered) noexcept {
        const bool prev = isPoweredOn.exchange(powered, std::memory_order_relaxed);
        mLastObservedParamPower.store(powered, std::memory_order_relaxed);
        mPendingPowerWakeNotify.store(false, std::memory_order_release);
        if (!powered && prev)
            mPendingEngineReset.store(true, std::memory_order_release);
        if (auto* p = apvts.getParameter(mr16::ParamIDs::powerState.getParamID()))
        {
            if (auto* pBool = dynamic_cast<juce::AudioParameterBool*>(p))
                *pBool = powered;
            else
                p->setValueNotifyingHost(powered ? 1.0f : 0.0f);
        }
        mStateSyncEpoch.fetch_add(1, std::memory_order_release);
    }
    bool isPower() const noexcept { return isPoweredOn.load(std::memory_order_relaxed); }

    void setEuclideanEnable(bool enabled) noexcept {
        mEuclideanEnable.store(enabled, std::memory_order_relaxed);
        if (auto* p = apvts.getParameter(mr16::ParamIDs::euclideanEnable.getParamID()))
        {
            if (auto* pBool = dynamic_cast<juce::AudioParameterBool*>(p))
                *pBool = enabled;
            else
                p->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
        }
        mStateSyncEpoch.fetch_add(1, std::memory_order_release);
    }
    bool isEuclideanEnabled() const noexcept {
        if (atomicPointers.euclideanEnable != nullptr)
            return atomicPointers.euclideanEnable->load(std::memory_order_relaxed) > 0.5f;
        return mEuclideanEnable.load(std::memory_order_relaxed);
    }

    uint32_t getStateSyncEpoch() const noexcept { return mStateSyncEpoch.load(std::memory_order_acquire); }

    // Direct performance triggers
    void triggerStrike(float velocity, float hardness = 0.65f) noexcept;
    void triggerStrikeButton(int buttonIndex, float velocity = 1.0f) noexcept;
    void triggerChimeKey(int keyIndex, float velocity = 1.0f, float freqHz = 0.0f) noexcept;
    void triggerMidiNote(int midiNote, float velocity) noexcept;

    // Lock-Free SPSC Telemetry Access for 60 FPS CRT Scope
    bool popVisualizerFrame(mr16::VisualizerFrame& frame) noexcept;

    // Lock-Free Oscilloscope / Lissajous Visualizer Buffer
    static constexpr int kScopeBufferSize = 2048;
    void pushScopeSamples(const float* left, const float* right, int numSamples) noexcept;
    void getScopeSamples(float* destL, float* destR, int numSamplesToRead) const noexcept;
    void setScopeSource(int source) noexcept;
    int getScopeSource() const noexcept;
    bool isMonitoringInput() const noexcept;

    // Lossless WAV Background Recorder
    void startRecording();
    void stopRecording();
    bool isRecording() const noexcept;
    juce::File getLastRecordedFile() const;
    bool consumeRecordingSavedDirty() noexcept;

    // Preset definition structure
    struct Preset {
        const char* name;
        mr16::Mr16ParameterSnapshot params;
    };
    static const std::vector<Preset>& getFactoryPresets();

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState apvts;
    mr16::Mr16AtomicPointers atomicPointers;
    std::atomic<bool> isPoweredOn { true };
    std::atomic<bool> mLastObservedParamPower { true };
    std::atomic<bool> mPendingPowerWakeNotify { false };
    std::atomic<bool> mPendingEngineReset { false };
    std::atomic<bool> mEuclideanEnable { false };
    std::atomic<bool> mSustainPedalActive { false };
    std::atomic<float> mPitchBendSemitones { 0.0f };
    std::atomic<float> mModWheelNorm { 0.0f };
    std::atomic<int> mScopeSource { 0 };
    std::atomic<uint32_t> mStateSyncEpoch { 1 };
    int mCurrentProgram { 0 };
    bool mInitialStandaloneStateLoaded { false };

#if MR16_HAS_DSP_ENGINE
    braun::mr16::Mr16Engine mr16Engine;
#endif

    // Visualizer waveform circular buffer
    std::atomic<int> scopeWritePos { 0 };
    float scopeBufferL[kScopeBufferSize] {};
    float scopeBufferR[kScopeBufferSize] {};

    // SPSC visualizer frame staging
    std::atomic<bool> newVisualizerData { false };
    mr16::VisualizerFrame stagedVisualizerFrame {};

    // Lock-free background WAV recorder
    juce::TimeSliceThread recorderThread { "Braun MR-16 WAV Recorder Thread" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    std::atomic<int> activeWriterWorkers { 0 };
    juce::File lastRecordedFile;
    std::atomic<bool> recordingSavedDirty { false };
    juce::CriticalSection recorderLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BRAUN_MR16AudioProcessor)
};
