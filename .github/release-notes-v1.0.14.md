# BRAUN MR-16 v1.0.14 — Release Notes

> *"Weniger, aber besser" — Less, but better.*  
> **Legal Notice**: Not affiliated with Braun GmbH. Dieter Rams inspired design homage.  
> Manufactured by Sneed's Feed & Seed Ltd. Companion to [BRAUN AS-42](https://sneed-and-feed.github.io/braun_as-42/) and [BRAUN RB-26](https://sneed-and-feed.github.io/braun_rb-26/).

---

## Key Improvements & Bug Fixes in v1.0.14

### 1. Kinetic Exciter Poisson Rain Persistence Across Audio Blocks
- **Fixed Per-Block Countdown Reset Bug (`KineticExciter::setPoissonRain`)**: Previously, calling `Mr16Engine::setParameters()` on every audio block (`PluginProcessor::processBlock`) unconditionally invoked `updatePoissonCountdown()`, resetting the exponential inter-arrival countdown `mPoissonCountdown` before it could reach zero whenever the inter-arrival time exceeded one audio buffer (~5–10 ms). `setPoissonRain` now only initializes or recalculates the countdown when transitioning from disabled to enabled, when `mPoissonCountdown <= 0`, or when the event rate (`epm`) changes.
- **Cached External Audio Sensitivity Curve**: Precomputed `mCalibratedExtSensitivity = std::pow(mExtSensitivity, 1.25f)` on parameter change rather than evaluating `std::pow` on every audio sample.
- **Guarded Euclidean & Microtonal Scale Setters**: Avoided redundant `updateEuclideanRhythm()` and `computeScaleRatios()` recalculations when parameters are unchanged across blocks.

### 2. 16-Pole Modal Resonator Matrix Real-Time CPU Optimization
- **Precomputed Mode Dispersion, Beating & Coupling Damping (`ModalResonatorMatrix`)**: Cached static mode dispersion/beating ratios (`mEffectiveUnspreadRatios`) and Q-dependent coupling damping (`mCouplingQDamping[i] = 1.0f / std::sqrt(1.0f + mBaseQ[i] * 0.05f)`) inside `calculateDampingAndQ()`, eliminating 32× `std::sqrt` and 16× `std::sin` evaluations per audio sample.
- **Setter Change Guards & Unmodulated Fast-Path**: Added epsilon change guards to all modal setters (`setFundamentalHz`, `setManifold`, `setMaterial`, `setDecayScale`, `setQScale`, `setOvertoneSpread`, `setStereoWidth`, `setBipolarSpread`) and `applyModulation()`, eliminating redundant 16-mode `std::tan` and `std::cos`/`std::sin` recalculations when parameters and modulation depth are static.

### 3. 3D Lorenz Attractor Q-Modulation Wiring & UI Parameter Sync
- **Wired `lorenz_q_mod` APVTS Parameter (`Parameters.h` & `LorenzAttractor`)**: Connected `Mr16ParameterSnapshot::lorenzQMod` in `toDspParams()` to `Mr16Parameters::chaosQMod` and `LorenzAttractor::setQModDepth()`, enabling continuous host automation and UI control of chaotic modal Q-spread depth.
- **Native Fallback UI Deck 02 Layout (`PluginEditor.cpp`)**: Added layout slot for the `modal_q` (`Resonance Q`) rotary knob in Deck 02 of the native JUCE fallback UI.
- **WebView Percentage Knob Normalization (`web/js/app.js`)**: Normalized `0..100%` Web UI knobs (`modal_coupling`, `lorenz_freq_mod`, `lorenz_q_mod`, `chorus_dimension`, `chorus_mix`, `golden_pan_spread`, `drive_saturation`, `dry_wet_mix`) by `0.01` prior to emitting `paramChange` events to the JUCE backend, preventing values in `(0.0, 1.0]%` from being misinterpreted as normalized `[0.0, 1.0]` values.
- **CRT Vector Scope Immediate High-DPI Resize Sync (`web/js/ui/crt-display.js`)**: Ensured `_requestResize()` executes an immediate synchronous `clientWidth`/`clientHeight` DPR calibration in addition to `requestAnimationFrame` coalescing, eliminating transient canvas dimension lag on rapid viewport changes.

---

## Verification Summary
- **Native Headless C++ DSP Suite (`mr16_headless_dsp_tests`)**: **208 / 208 PASS** (Tier 1 Features: 40/40, Tier 2 Boundaries & Real-Time Zero-Allocation Safety: 8/8, Tier 3 Physical Acoustics & Anti-Runaway Limiters: 160/160).
- **Web Audio & UI Verification Suite (`npm test`)**: **221 / 221 PASS** across all 7 automated verification suites.
- **Zero Memory Leaks, Zero Denormals, Zero NaNs/Infs**.
