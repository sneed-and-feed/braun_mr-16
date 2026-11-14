# Changelog

All notable changes to the **BRAUN MR-16** physical acoustic modal resonator and synthesizer are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.1.0] - 2026-11-14

### Added
- **Closed-Loop Karnopp Stick-Slip Bow Resonator Feedback (`Mr16Engine`, `KineticExciter`)**: Coupled damped resonator body velocity (`mBowBodyFilter` -> `mBowBodyVelocity`) back into `KineticExciter::processSample()`, plus an 800 Hz lowpassed rosin micro-asperity texture during slip phase for self-locking Helmholtz bowing harmonics.
- **Full MIDI Expressivity & Panic Handling (`PluginProcessor`)**: Lock-free audio-thread handling for CC64 Sustain Pedal ($3.5\times$ decay scaling), 14-bit Pitch Bend ($\pm 2$ semitones), CC1 Mod Wheel (`couplingDepth` offset), and CC120 / CC121 / CC123 All-Sound-Off / Reset-All-Controllers / All-Notes-Off panic clearing.
- **Zero-Allocation Mono Bus Downmixing (`PluginProcessor`)**: Added `0.5 * (L + R)` stereo-to-mono output downmixing using chunked 512-sample stack scratch buffers, supporting arbitrary host block sizes ($0$, $1$, $4096+$ samples) with zero heap allocations.
- **Live Master Scope Waveform Telemetry (`Mr16Engine`, `VisualizerFrame`)**: Populated the 256-sample `scopeSamplesL` / `scopeSamplesR` ring buffer in `Mr16Engine::processBlock()` for real-time Lissajous and waveform rendering.
- **Embedded WebView2 Sample-to-Modal Extraction (`web/js/app.js`)**: Added `OfflineAudioContext(1, 1, 48000)` decoding fallback in `_handleAudioFileExtraction()` when running inside the native JUCE WebView2 host (`this.isJuce === true`), enabling drag-and-drop sample modal extraction without instantiating a live browser audio graph.
- **Expanded 37-Parameter APVTS Schema & Functional Controls (`Parameters.h`, `PluginProcessor`, `PluginEditor`, `Mr16Engine`, `web/`)**: Added automatable `euclidean_enable`, `lorenz_morph_alpha`, `lorenz_morph_steer`, and `soft_limiter` APVTS parameters across C++, Native Fallback UI, WebView2 IPC, and WebAudio, plus a dedicated `EUCLIDEAN SEQ` audition button (`#btn-audition-euclidean` / `euclideanTriggerBtn`), calibrated per-preset `modal_q` defaults (`85.0` base reference), and 16-key Chime Strip microtonal pitch forwarding up to `5000 Hz` with multi-block pitch persistence across `setParameters()`.
- **Expanded Automated Regression Suites (`Tier1_FeatureTests.h`, `Tier2_BoundaryTests.h`, `mr16_dsp_verification_tests.cpp`, `web/test-checklist.mjs`)**: Added 8 new C++ headless DSP tests (`T1_LOR_04`, `T1_MOD_13`, `T1_EXC_10`, `T1_R2_01`, `T1_R2_02`, `T1_R2_03`, `T2_TOX_03`, `T2_RATE_03`), wired `mr16_dsp_verification_tests` into CTest with canonical 16-preset assertions, and added 9 new WebAudio/UI/security regression tests across Suites 6, 11 & 12 (**216/216** C++ headless + **6/6** core verification, **230/230** Web across 49 suites).

### Fixed
- **3D Lorenz Attractor Multi-Rate Stagnation & Speedup (`LorenzAttractor`)**: Upgraded state variables (`mX, mY, mZ, mDt, mSampleRate`) to `double` precision and widened the `mDt` clamp from `[1e-5, 0.025]` to `[1e-9, 0.025]`, eliminating the $550\times$ orbital speedup and `float32` ULP underflow freeze at 192 kHz (`chaosRate = 0.01 Hz`) and raising max detune clamp from `200` to `300` cents to match UI/docs.
- **Bitwise IEEE-754 NaN/Inf Sanitization Under `/fp:fast` (`DspMath.h`, `BoundedSaturator.h`, `SpatialChorus.cpp`, `VactrolGate.h`)**: Replaced `std::isfinite` / `std::isnan` checks with bitwise exponent inspection (`DspMath::isFinite(float/double)`), preventing compiler fast-math elision of toxic input guards.
- **Sample-Rate-Invariant Modal Matrix & Ultrasonic Nyquist Protection (`ModalResonatorMatrix`)**: Preserved `mQScale` and `mOvertoneSpread` across `reset()`, scaled `mRadiationCoeff` and `mEnergyDecayCoeff` dynamically from 48 kHz across 44.1–192 kHz, tightened TPT SVF cutoff clamping to `0.48 * fs`, and replaced the discontinuous ultrasonic Q-norm step drop with a smooth $C^1$ Hermite rolloff across $[0.45 f_s, 0.49 f_s]$.
- **Vactrol Gate Multi-Rate Ballistics (`VactrolGate`)**: Scaled `mEnvAttackCoeff` and `mEnvReleaseCoeff` dynamically with host sample rate and clamped SVF cutoff to $0.48 f_s$.
- **Poisson Rain Density Range & Live Sweep Proportionality (`KineticExciter`, `Parameters.h`)**: Expanded `mPoissonEpm` upper bound from `1200` to `3000` EPM (`50 Hz`) matching APVTS `poisson_density`, rescaled `mPoissonCountdown` proportionally during continuous rate automation, calibrated the 15 Hz DC-blocker pole via exact `std::exp`, and SR-scaled `mActivityDecayCoeff`.
- **Lock-Free Audio-Thread Standby Auto-Wake & Standalone Silent Cold-Boot Guard (`PluginProcessor`)**: Removed real-time-unsafe `setValueNotifyingHost` calls from `processBlock()`, deferring host parameter notification to a 30 Hz message-thread `juce::Timer` with edge-triggered `mLastObservedParamPower` detection; removed ambient audio-input (`> -66 dB`) Standby auto-wake so DAW audio tracks stay muted in Standby and Windows Standalone never auto-wakes from default microphone room noise on cold launch; disarmed `isPoweredOn`, `euclideanEnable`, and `poisson_density` during initial Standalone `.settings` reload while preserving full VST3/CLAP DAW session recall.
- **Host State Recall, 1:1 Factory Preset Parity, WebView2 Resource Containment & PE Metadata (`PluginProcessor`, `PluginEditor`, `Mr16Engine`, `Parameters.h`, `CMakeLists.txt`, `scripts/package_release.py`)**: Aligned all 16 factory presets 1:1 in ID, name, exciter mode, manifold, material, Poisson/Euclidean clocking, Lorenz modulation, Dimension chorus, and CRT display mode across `Mr16Engine::getFactoryPresets()`, `PluginProcessor::getFactoryPresets()`, and `web/factory_presets.json`; hardened `PluginEditor::getResource()` against `..` / `:` / URL-encoded path traversal with `file.isAChildOf(baseDir)` containment and guarded `handleExciterTriggerFromWeb()` against waking Standby on unrecognized or `enable: false` events; fixed `apvts.replaceState(vt)` ordering in `setStateInformation`, persisted `euclideanEnable` in XML state, added `mStateSyncEpoch` UI synchronization and `mSuppressEchoParamIndex` echo loop suppression, emitted `recordingState(false)` upon WAV save completion, and added CMake stale `.rc` invalidation plus Windows PE `VERSIONINFO` (`FileVersion` / `ProductVersion` = `1.1.0`) verification in `scripts/package_release.py`.
- **Web UI Percentage Knob Contract & WebAudio DSP Parity (`web/js/app.js`, `mr16_web_engine.js`, `lorenz_morpher.js`, `headless-audio-context.mjs`)**: Separated `_toJuceParamValue(id, val)` from internal state storage so `[0, 100]%` JSON presets stay intact while `(0, 1]%` knob positions never jump to `100%`; prevented cold-boot `#btn-power` double-toggling (`pointerdown` unlock vs `click`) and ensured `setPower()` fires UI callbacks even when `AudioContext` is uninitialized in JUCE WebView2 mode; guarded `_initJuceBridge()` polling against headless environments (`typeof window !== 'undefined'`) and cleared `_juceBridgePoll` in `destroy()`; synced modal-profile JSON imports and `vector_crt_mode` (`display_mode`) to JUCE APVTS; aligned `#btn-audition-poisson`, `soft_limiter`, `ext_input_gain` dB-to-linear gain, `drive_saturation` `[0, 100]%` scaling, RK4 Lorenz integration with live `qModDepth`, $0.48 f_s$ Nyquist clamping, zero-allocation telemetry & headless mock buffers, BFCache CRT Standby guards, and `destroy()` RAF cleanup.
- **Documentation, CI Runner & GFM KaTeX Math Compliance (`README.md`, `ARCHITECTURE.md`, `VERIFICATION_CHECKLIST.md`, `wiki/*.md`, `.github/workflows/build-and-release.yml`)**: Synchronized all 37 APVTS parameter ranges/defaults, 5 geometric manifolds, and test counts, updated the macOS GitHub Actions runner from retired `macos-14` to `macos-latest`, and achieved **0 GFM math compliance issues** across every Markdown document via `scripts/audit_gfm_math.py`.

---

## [1.0.14] - 2026-10-18

### Fixed
- **Poisson Rain Persistence Across Audio Blocks (`KineticExciter::setPoissonRain`)**: Prevented per-block `mPoissonCountdown` resets when inter-arrival times exceeded one audio buffer.
- **Modal Matrix Real-Time CPU Optimization (`ModalResonatorMatrix`)**: Precomputed static mode dispersion ratios and Q-dependent coupling damping, adding parameter change guards.
- **Lorenz Q-Modulation Wiring & UI Sync (`Parameters.h`, `PluginEditor.cpp`, `web/js/app.js`, `crt-display.js`)**: Wired `lorenz_q_mod` to `chaosQMod`, added `modal_q` layout slot in native fallback UI Deck 02, normalized percentage knobs on JUCE emission, and added synchronous High-DPI resize calibration on the CRT canvas.

---

## [1.0.12] - 2026-10-02

### Fixed
- **Synchronous WebView2 Initial State Injection (`PluginEditor.cpp`, `web/js/app.js`)**: Injected `window.__JUCE_INITIAL_PARAMS__` via JUCE 8 `withUserScript` before DOM evaluation, eliminating knob jump on editor reopen.
- **Discrete Selector & DOM Ready Lifecycle Guards**: Added `_syncInitialDiscreteState()` and `document.readyState` bootstrap guards.

---

## [1.0.11] - 2026-09-28

### Fixed
- **APVTS Parameter Buffer Alignment**: Expanded `PluginEditor.h` parameter tracking bounds to `kNumParams = 33` and persisted `scope_source` and `currentProgram` in XML state.

---

## [1.0.10] - 2026-09-25

### Added
- Initial cross-platform release of the 7-deck **BRAUN MR-16** physical acoustic modal resonator (VST3, CLAP, Standalone, and client-side Web Audio showcase).
