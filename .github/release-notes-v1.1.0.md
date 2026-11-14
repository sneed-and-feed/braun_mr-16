# BRAUN MR-16 v1.1.0 — Multi-Rate Physical Modeling Calibration, Lock-Free Audio Lifecycle & WebView2 Sync Overhaul

[![Verification Status: 100% PASS](https://img.shields.io/badge/Verification-100%25%20PASS%20(216%2F216%20C%2B%2B%20%7C%20230%2F230%20Web)-24FF6A?style=for-the-badge&logo=checkmarx)](https://github.com/sneed-and-feed/braun_mr-16/blob/main/VERIFICATION_CHECKLIST.md)
[![Version: 1.1.0](https://img.shields.io/badge/Version-1.1.0-EE592B?style=for-the-badge)](https://github.com/sneed-and-feed/braun_mr-16/releases/tag/v1.1.0)
[![C++20 & Web Audio](https://img.shields.io/badge/DSP%20Parity-Verified-blue?style=for-the-badge)](https://github.com/sneed-and-feed/braun_mr-16)
[![License: MIT](https://img.shields.io/badge/License-MIT-black?style=for-the-badge)](https://github.com/sneed-and-feed/braun_mr-16/blob/main/LICENSE)

> *"Weniger, aber besser" — Less, but better.*  
> **Legal Notice**: Not affiliated with Braun GmbH. Dieter Rams inspired design homage.  
> **Manufacturer**: Sneed's Feed & Seed Ltd.  
> Direct technical companion to [BRAUN AS-42](https://sneed-and-feed.github.io/braun_as-42/) and [BRAUN RB-26](https://sneed-and-feed.github.io/braun_rb-26/).

---

## Executive Release Summary

**BRAUN MR-16 v1.1.0** is a major precision engineering release auditing and hardening every layer of the C++20 DSP engine, JUCE 8 VST3/CLAP/Standalone host lifecycle, WebView2 bidirectional parameter bridge, client-side WebAudio synthesizer, and GFM mathematical documentation. It eliminates multi-sample-rate physical modeling drift across 44.1–192 kHz, closes the physical Karnopp stick-slip bow resonator velocity loop, removes all real-time-unsafe host parameter locks from `processBlock()`, expands the synchronized APVTS parameter tree to **37 parameters** (`euclidean_enable`, `lorenz_morph_alpha`, `lorenz_morph_steer`, `soft_limiter`), aligns all 16 factory presets 1:1 across C++ and Web with calibrated per-material `modal_q` values, forwards 16-key Chime Strip microtonal frequencies up to `5000 Hz` with multi-block pitch persistence, hardens WebView2 resource containment and PE binary metadata, completes MIDI performance expressivity (CC64 sustain, Pitch Bend, CC1 Mod Wheel, CC120/121/123 panic), updates macOS CI runners to `macos-latest`, and expands continuous automated verification to **216 C++ headless DSP tests (+ 6 core verification tests)** and **230 Web Audio & UI tests across 49 suites**.

---

## 1. Root Causes & Engineering Fixes

### 1.1 Sample-Rate-Invariant Physical Modeling & Fast-Math Hardening (`source/dsp/*`)
- **Lorenz Attractor 192 kHz ULP Freeze & Speedup (`LorenzAttractor.h/.cpp`)**:
  - *Root Cause*: `mDt = 0.35f * mChaosRate / mSampleRate` fell to `1.82e-8` at 192 kHz and `chaosRate = 0.01 Hz`, hitting a `1.0e-5f` floor clamp (causing a $550\times$ orbital speedup) while `float32` state coordinates around `z = 25.0f` (`ULP ~ 1.9e-6`) suffered catastrophic RK4 increment underflow. In addition, `setDetuneMaxCents()` clamped at `200` cents instead of the `300`-cent UI specification.
  - *Fix*: Promoted attractor state and RK4 integration (`mX, mY, mZ, mDt, mSampleRate`) to `double` precision, widened `mDt` clamp to `[1.0e-9, 0.025]`, and aligned `setDetuneMaxCents` to `300.0f` cents.
- **Bitwise IEEE-754 Finite Guards (`DspMath.h`, `BoundedSaturator.h`, `SpatialChorus.cpp`, `VactrolGate.h`)**:
  - *Root Cause*: Compiler `/fp:fast` and `-ffast-math` flags permit eliding standard `std::isfinite` / `std::isnan` checks on audio buffers.
  - *Fix*: Implemented branch-free bitwise exponent inspection (`DspMath::isFinite(float)` and `DspMath::isFinite(double)`) across all input, feedback, delay-line, and output sanitizers.
- **Modal Matrix Multi-Rate Calibration & Smooth Ultrasonic Nyquist Rolloff (`ModalResonatorMatrix.h/.cpp`)**:
  - *Root Cause*: `reset()` overwrote `mQScale` (`50.0 -> 1.0`) and `mOvertoneSpread` (`-> 1.0`) prior to recalculation; `mRadiationCoeff` (`0.18f`) and `mEnergyDecayCoeff` (`0.9992f`) were hardcoded for 48 kHz; modes crossing `0.45 * fs` suffered an abrupt $5\times$ Q-normalization step drop (`qNorm -> 0.2f * qNorm`) causing audible timbre clicks during pitch bends or Lorenz sweeps; TPT SVF cutoff clamped at $0.495 f_s$ (`g ~ 63.6`).
  - *Fix*: Preserved `mQScale` and `mOvertoneSpread` across `reset()`, scaled radiation and energy-telemetry poles dynamically from 48 kHz across all sample rates, tightened SVF cutoff clamping to `0.48 * fs`, and introduced a smooth $C^1$ Hermite ultrasonic Q-norm rolloff across $[0.45 f_s, 0.49 f_s]$.
- **Closed-Loop Karnopp Bowing & Poisson Rain Proportionality (`KineticExciter.h/.cpp`, `Mr16Engine.h/.cpp`)**:
  - *Root Cause*: `Mr16Engine::processBlock` passed `resonatorFeedbackVelocity = 0.0f` into `mExciter.processSample()`, leaving the bowed string/plate model open-loop; `mPoissonEpm` clamped at `1200` EPM (`20 Hz`) despite APVTS `poisson_density` reaching `50 Hz` (`3000` EPM), and continuous rate sweeps reset `mPoissonCountdown`.
  - *Fix*: Coupled damped modal body velocity (`mBowBodyFilter` -> `mBowBodyVelocity`) back into `mExciter.processSample()` with 800 Hz lowpassed rosin micro-asperity during slip phase, expanded `mPoissonEpm` to `3000` EPM with proportional countdown rescaling during live rate automation, calibrated the 15 Hz DC blocker via `std::exp`, SR-scaled `mActivityDecayCoeff` and `VactrolGate` envelope ballistics, populated the 256-sample `scopeSamplesL/R` telemetry buffer, and removed the `2048`-sample cap on `mMaxBlockSize`.

### 1.2 Lock-Free JUCE Host Lifecycle, MIDI & Bus Topologies (`source/plugin/*`)
- **Zero-Lock Audio-Thread Standby Auto-Wake & Standalone Silent Cold-Boot Protection (`PluginProcessor.h/.cpp`)**:
  - *Root Cause*: `processBlock()` invoked `p->setValueNotifyingHost(1.0f)` directly on the real-time audio thread during Standby wake (risking priority inversion and host mutex contention) and auto-woke whenever input bus magnitude exceeded `-66 dBFS` (`0.0005f`), causing DAW audio-insert tracks to immediately pop out of Standby and causing Windows Standalone (`BRAUN_MR16.exe`) to wake on frame 1 from ambient microphone noise and blast saved autonomous Poisson/Euclidean/Friction states reloaded from `%APPDATA%\BRAUN_MR16.settings`.
  - *Fix*: Replaced audio-thread host calls with an atomic `mPendingPowerWakeNotify` flag serviced by a 30 Hz message-thread `juce::Timer` plus edge-triggered `mLastObservedParamPower` tracking; restricted Standby auto-wake strictly to interactive MIDI Note-On and UI exciter triggers; and disarmed `isPoweredOn`, `mEuclideanEnable`, and `poisson_density` on initial Standalone cold-boot `.settings` reload while preserving 100% VST3/CLAP DAW session recall.
- **Complete MIDI Expressivity & Zero-Allocation Mono Downmix (`PluginProcessor.h/.cpp`)**:
  - Added lock-free MIDI CC64 sustain ($3.5\times$ modal decay), 14-bit Pitch Bend ($\pm 2$ semitones), CC1 Mod Wheel (`couplingDepth` modulation), and CC120/121/123 panic/controller reset.
  - Added `0.5 * (L + R)` mono output bus downmixing using chunked 512-sample stack buffers (`0` heap allocations for block sizes `0`, `1`, `4096+` samples).
- **State Persistence, 37-Parameter APVTS Schema, 1:1 Factory Preset Parity, WebView2 Containment & PE Metadata (`Parameters.h`, `PluginProcessor.cpp`, `PluginEditor.h/.cpp`, `Mr16Engine.h/.cpp`, `CMakeLists.txt`, `scripts/package_release.py`)**:
  - Expanded APVTS from 33 to 37 automatable parameters (`euclidean_enable`, `lorenz_morph_alpha`, `lorenz_morph_steer`, `soft_limiter`), calibrated default `modal_q` to `85.0`, aligned `ext_input_gain` default `0.0 dB`, `poisson_density` max `50.0 Hz`, `chorus_rate_hz` max `8.0 Hz`, `vactrol_lpg_cutoff` min `20.0 Hz`, `SpatialChorus` max rate `8.0 Hz`, `ModalResonatorMatrix` max `f0 = 5000.0 Hz`, and `ManifoldType::DiffusePlate = 4`.
  - Forwarded per-key microtonal Chime Strip frequencies (`freq`) across WebView2 IPC (`handleExciterTriggerFromWeb` -> `triggerChimeKey` -> `enqueueTriggerChime`), and guarded `Mr16Engine::applyParametersToDsp()` with `mLastParamFundamentalHz` change detection so Chime, MIDI Note-On, and Euclidean pitches persist across audio blocks instead of resetting to `modal_frequency` on block $B+1$.
  - Synchronized all 16 factory presets (`CALIBRATED DEFAULT` through `INFINITE MODAL SINGULARITY`) 1:1 in ID, name, exciter mode, manifold, material, calibrated `modal_q`, `euclidean_enable` (armed only on `#8 EUCLIDEAN METALLOPHONE` and `#15 FROSTED GLASS ARPEGGIO`), Poisson clocking, Lorenz modulation, Dimension chorus, and CRT display mode across `Mr16Engine::getFactoryPresets()`, `PluginProcessor::getFactoryPresets()`, and `web/factory_presets.json`.
  - Added dedicated `EUCLIDEAN SEQ` audition controls (`#btn-audition-euclidean` in Web UI and `euclideanTriggerBtn` in Native Fallback UI) that wake Standby on enable and never wake power on disable, plus `currentProgram` host synchronization when selecting factory presets in Web UI.
  - Hardened `PluginEditor::getResource()` against `..` / `:` / URL-encoded path traversal with `file.isAChildOf(baseDir)` filesystem containment, and guarded `handleExciterTriggerFromWeb()` so only recognized interactive exciter events (`enable != false`) wake Standby.
  - Added CMake stale `.rc` invalidation for JUCE 8 Windows resources and `ctypes.windll.version` PE `VERSIONINFO` (`FileVersion` / `ProductVersion` = `1.1.0`, `CompanyName` = `Sneed's Feed & Seed Ltd.`) verification in `scripts/package_release.py`.
  - Fixed `setStateInformation` ordering so `apvts.replaceState(vt)` runs before custom XML attributes are restored, persisted `euclideanEnable` in XML state and `requestSync`, bumped `mStateSyncEpoch` so open WebView2 editors refresh immediately on DAW preset/session recall, suppressed WebView2 parameter-echo feedback loops via `mSuppressEchoParamIndex`, emitted `recordingState(false)` upon WAV save completion, and made fallback Chladni scope phase per-instance.

### 1.3 Web UI Contract & Client-Side WebAudio Engine Parity (`web/js/*`)
- **Percentage Knob Normalization, Power Standby & Preset Sync (`web/js/app.js`)**:
  - *Root Cause*: Normalizing `(0, 1]%` values inside `setParam()` mutated `this._state[id]`, causing subsequent drags or preset exports to misinterpret `0.5%` as `0.5` (`50%`) or jump to `100%`; clicking `#btn-power` on cold boot fired `window` `pointerdown` unlock before `click`, double-toggling back to Standby; and `setPower()` returned before firing `onPowerStateChanged` when `AudioContext` was null in JUCE mode.
  - *Fix*: Kept `this._state` strictly in UI/preset units (`[0, 100]%`) while converting to `[0, 1]` exclusively at the JUCE IPC boundary via `_toJuceParamValue(id, val)`. Prevented `#btn-power` double-toggle on cold boot, ensured `setPower()` always fires UI callbacks before checking `this.ctx`, guarded `_initJuceBridge()` polling against headless environments (`typeof window !== 'undefined'`) and cleared `_juceBridgePoll` in `destroy()`, synced modal-profile JSON imports and `preset.ui.vector_crt_mode` (`display_mode`) to JUCE APVTS, fixed `display_mode == 2` (`'MODAL_FFT'`), `#btn-audition-poisson`, `soft_limiter` sync, `OfflineAudioContext` sample decoding when `this.isJuce === true`, BFCache `pagehide`/`pageshow` CRT Standby guards, and `this.engine.destroy()` lifecycle teardown.
- **WebAudio DSP Parity (`mr16_web_engine.js`, `lorenz_morpher.js`)**:
  - Implemented `ext_input_gain` dB-to-linear gain scaling, `drive_saturation` pre-gain (`25% -> 1.0`), `soft_limiter` curve bypass toggle, `euclidean_enable` clock arm/disarm with `pulses <= 0` guard, continuous adjacent-manifold morphing (`CHLADNI -> BEAM -> VOCAL -> HORN -> DIFFUSE -> CHLADNI`) with live `lorenz_morph_steer` steering when no custom sample profile is loaded, strict `lorenz_morph_alpha ?? 0.0` zero-alpha preservation in `LorenzMorpher`, 4th-order Runge-Kutta (RK4) `_stepLorenz` integration with live `qModDepth` modulation on `modes[i].Q`, `0.48 * sampleRate` Nyquist clamping across `_buildAudioGraph`, `updateModalFrequencies`, and `updateMorph`, pre-allocated `_telemetryScratch` Float32Arrays, and `_lorenzRafId` cancellation in `destroy()`.

---

## 2. Verification Matrix (100% PASS)

| Verification Suite | Target | Test Cases | Scope & Physical Mechanics | Status |
|:---|:---:|:---:|:---|:---:|
| **Tier 1: Feature Coverage** | C++20 | 46 / 46 | Exciters, closed-loop Karnopp bow, 50 Hz Poisson, 192 kHz RK4 Lorenz, ultrasonic Q-rolloff, microtonal chime override, soft limiter, Lorenz steer | **PASS (100%)** |
| **Tier 2: Boundaries & Real-Time Safety** | C++20 | 10 / 10 | Multi-rate (44.1k–192k), block sizes (0, 1, 4096 + live scope), bitwise NaN/Inf/denormal immunity, 0 heap allocs | **PASS (100%)** |
| **Tier 3: Physical Acoustics** | C++20 | 160 / 160 | Karnopp stiction (40), Vactrol sag (40), Golden panning (40), Anti-runaway limiters (40) | **PASS (100%)** |
| **Core DSP Verification Runner** | C++20 | 6 / 6 | Standalone core verification binary (`mr16_dsp_verification_tests.exe`) | **PASS (100%)** |
| **Total Native C++ CTest Suites** | C++20 | **216 / 216 (+ 6/6)** | `ctest --test-dir build -C Release --output-on-failure` | **PASS (100%)** |
| **Complete Web Audio & UI Suite** | Web / Node | **230 / 230** | 49 automated test suites (`npm test`) including 37-param APVTS, Euclidean/Chime/Steer wiring, RK4, Nyquist & lifecycle checks | **PASS (100%)** |
| **GFM KaTeX Math Compliance** | Markdown | **15 / 15 files** | `python scripts/audit_gfm_math.py` across root, `.github/`, and `wiki/` | **PASS (0 issues)** |

---

## 3. Distributable Release Packages & SHA-256 Checksums

Pre-compiled Windows x64 binary packages and cryptographic verification manifests for **BRAUN MR-16 v1.1.0**:

| Archive Filename | Contents | Size (Bytes) | SHA-256 Digest |
|:---|:---|---:|:---|
| `BRAUN_MR16-v1.1.0-Windows-x64.zip` | `BRAUN_MR16.vst3` + `BRAUN_MR16.clap` + `BRAUN_MR16.exe` + docs | `10,739,277` | `1a4adcacb13a96289915057a0914e6da2b674def6e42ac27f505b40159f849b8` |
| `BRAUN_MR16-v1.1.0-VST3-Windows-x64.zip` | `BRAUN_MR16.vst3` + docs | `3,575,213` | `9f787eba233ddb09198ed57b30a14c5f0e32b7334ec75870004b88784febf0eb` |

Verify archive integrity:

```bash
sha256sum -c releases/SHA256SUMS.txt
```
