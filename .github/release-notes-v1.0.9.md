# BRAUN MR-16 v1.0.9 — Anti-Runaway Resonator Limiting, 200+ Test Suite Expansion & Architectural Restructure

[![Verification Status: 100% PASS](https://img.shields.io/badge/Verification-100%25%20PASS%20(207%2F207%20C%2B%2B%20%7C%20206%2F206%20Web)-24FF6A?style=for-the-badge&logo=checkmarx)](https://github.com/sneed-and-feed/braun_mr-16/blob/main/VERIFICATION_CHECKLIST.md)
[![Version: 1.0.9](https://img.shields.io/badge/Version-1.0.9-EE592B?style=for-the-badge)](https://github.com/sneed-and-feed/braun_mr-16/releases/tag/v1.0.9)
[![C++20 & Web Audio](https://img.shields.io/badge/DSP%20Parity-Verified-blue?style=for-the-badge)](https://github.com/sneed-and-feed/braun_mr-16)
[![License: MIT](https://img.shields.io/badge/License-MIT-black?style=for-the-badge)](https://github.com/sneed-and-feed/braun_mr-16/blob/main/LICENSE)

> *"Weniger, aber besser" — Less, but better.*  
> **Legal Notice**: Not affiliated with Braun GmbH. Dieter Rams inspired design homage.  
> **Manufacturer**: Sneed's Feed & Seed Ltd.  
> Direct technical companion to the [BRAUN AS-42](https://sneed-and-feed.github.io/braun_as-42/) and [BRAUN RB-26](https://sneed-and-feed.github.io/braun_rb-26/).

---

## Executive Release Summary

The **BRAUN MR-16 v1.0.9** patch release introduces a comprehensive overhaul of the modal matrix stability architecture, resolves community-reported high-Q resonance outbursts, expands the automated DSP verification harness to **over 200 tests per target platform** (207 C++ tests and 206 Web Audio tests), restructures project documentation into a concise 1-page README accompanied by an exhaustive `ARCHITECTURE.md` specification, and standardizes manufacturer metadata under **Sneed's Feed & Seed Ltd.**

---

## 1. Key Highlights & Architectural Changes

### 1.1 Modal Resonator Anti-Runaway Multi-Tier Hard Limiting
- **Community Outburst Mitigation**: Resolves reports of extreme volume spikes when fundamental excitation frequencies align with high-Q modal resonances ($Q = 500$) or dense inharmonic chord clusters.
- **Per-Mode Non-Linear Soft Limiter**: Introduced smooth hyperbolic saturation on individual modal bandpass outputs above $0.85$ peak via transfer curve $y = \mathrm{sgn}(x) \cdot (0.85 + 0.15 \cdot \tanh(1.5 \cdot (|x| - 0.85)))$ with strict hard clamp at $[-1.0, +1.0]$.
- **Lyapunov State Bound Safeguard**: SVF integrator states ($s_1, s_2$) are bounded to $[-6.0, +6.0]$, preventing numerical divergence under infinite feedback coupling or continuous Dirac excitation bursts.
- **Central Bus Brickwall Safety Ceiling**: Summed modal matrix output is constrained to $[-0.95, +0.95]$ peak prior to spatial panning and wet/dry summing.
- **Master Output Hard Ceiling**: Final master output bus is protected with a hard brickwall ceiling at $1.05$ ($+0.42\text{ dBFS}$), guaranteeing zero ear-fatiguing output blowouts across all factory presets and external audio inputs.

### 1.2 Documentation Restructure: 1-Page README & ARCHITECTURE Split
- **Concise 1-Page README**: Streamlined `README.md` to focus on musician and producer essentials: acoustic concepts, zero-install web quickstart, DAW installation paths, factory presets, and verified download links.
- **Dedicated `ARCHITECTURE.md`**: Extracted mathematical derivations, the 73-line Mermaid signal flow topology diagram, 32-parameter APVTS table, and multi-platform compilation guides into a standalone engineering specification.

### 1.3 Legal Homage & Sneed's Feed & Seed Ltd. Manufacturer Standardization
- **Legal Non-Affiliation Homage**: Updated all documentation, source files, and dialogs to explicitly state non-affiliation with Braun GmbH and credit the industrial design language to Dieter Rams.
- **Official Manufacturer Attribution**: Standardized company name as `Sneed's Feed & Seed Ltd.` with bundle identifier `com.sneedandfeed.mr16` and plugin manufacturer code `Brun` across CMake, JUCE APVTS, CLAP extensions, and distribution guides.

### 1.4 Linux Headless & Native Platform Ergonomics
- **Linux WebView Default to OFF**: Configured `MR16_USE_WEBVIEW` to default to `OFF` on Linux systems (`CMAKE_SYSTEM_NAME STREQUAL "Linux"`), facilitating streamlined headless continuous integration builds and pure native JUCE DSP plugin hosting without WebKitGTK runtime dependencies.
- **Opt-In Web GUI**: Linux users with WebKitGTK installed can enable the WebView interface via `-DMR16_USE_WEBVIEW=ON`.

---

## 2. Test Suite Expansion & DSP Parity (207 C++ / 206 Web)

The verification harness has been expanded with Tier 3 Physical Acoustic verification suites covering real-world mechanics and extreme operational corner cases:

| Verification Suite | Target | Test Cases | Scope & Physical Mechanics | Status |
|:---|:---:|:---:|:---|:---:|
| **Tier 1: Feature Coverage** | C++20 | 39 / 39 | Exciters, Manifolds, Materials, Lorenz RK4, BBD Chorus | **PASS (100%)** |
| **Tier 2: Boundaries & Corners** | C++20 | 8 / 8 | Multi-rate (44.1k–192k), buffer scaling (1–2048), toxic denormals | **PASS (100%)** |
| **Tier 3: Physical Acoustics** | C++20 | 160 / 160 | Karnopp stiction (40), Vactrol sag (40), Golden panning (40), Limiters (40) | **PASS (100%)** |
| **Total Headless C++ DSP** | C++20 | **207 / 207** | Native headless verification binary (`mr16_headless_dsp_tests.exe`) | **PASS (100%)** |
| **Web UI & Architecture** | Web Audio | 15 / 15 | Token parity, Rams rules, 16-bit 48kHz WAV RIFF export | **PASS (100%)** |
| **Checklist & Coefficients** | Web Audio | 16 / 16 | Multi-rate coefficients, Hermite saturation, Householder scattering | **PASS (100%)** |
| **Physical Dynamics Suite** | Web Audio | 106 / 106 | Stiction dynamics, Buchla sag, golden panning law, anti-runaway clamping | **PASS (100%)** |
| **Complete Web Audio API Suite** | Web Audio | **206 / 206** | `npm test` automated test suites | **PASS (100%)** |

### Verified Physical Dynamics:
- **Karnopp Stick-Slip Friction**: Verified breakaway force thresholds across normal forces $F_n \in [0.05, 1.0]$, Stribeck curve decay across velocities, deadband stick damping, and strict velocity anti-symmetry: $f(-v) = -f(v)$.
- **Buchla 292 Optical Vactrol Sag**: Verified optical rise time ($<2\text{ ms}$), primary release decay ($40\text{ ms}$), phosphorescent tail persistence ($150\text{ ms} - 400\text{ ms}$), and multi-pulse charge accumulation hysteresis.
- **Golden-Ratio Spatial Panning**: Verified exact azimuth formula $\theta(m) = \mathrm{fmod}(m \cdot 137.507764^\circ, 360^\circ) - 180^\circ$, constant-power pan law $L^2 + R^2 = 1.0$ across all 16 modes, stereo width scaling, and quadrant panorama energy balance.
- **Anti-Runaway Hard Limiting**: Verified single-mode resonance containment ($\le 1.0$) at $Q = 500$, fundamental frequency sweeps ($55\text{ Hz} - 1760\text{ Hz}$), dense harmonic chord cluster overloads, and extreme $+40\text{ dBFS}$ Dirac impulse clamping.

---

## 3. Distributable Release Packages & SHA-256 Checksums

Pre-compiled binary packages and cryptographic verification manifests for BRAUN MR-16 v1.0.9:

| Platform | Format | Architecture | Filename | SHA-256 Checksum |
|:---|:---|:---:|:---|:---|
| **Windows** | Standalone (.exe) | x86_64 | [`BRAUN_MR16_v1.0.9_Standalone_Win64.zip`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.9/BRAUN_MR16_v1.0.9_Standalone_Win64.zip) | `5051d3f5dac62c46ec3cdb6dbc1278a0195af2a6db8b58cbf27bb4d9a3b199ea` |
| **Windows** | VST3 Plugin | x86_64 | [`BRAUN_MR16_v1.0.9_VST3_Win64.zip`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.9/BRAUN_MR16_v1.0.9_VST3_Win64.zip) | `960dcceef209f7adf4c5444258ba6922ad0ee61659c52a81b30ecc8eebd7285b` |
| **Windows** | CLAP Plugin | x86_64 | [`BRAUN_MR16_v1.0.9_CLAP_Win64.zip`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.9/BRAUN_MR16_v1.0.9_CLAP_Win64.zip) | `395eb9d1e67f4a9ea4f9acfc17ef324b59616ea4b858d8a467a99db6ae9d5bff` |
| **macOS** | Universal (AU/VST3/CLAP/App) | arm64 + x86_64 | [`BRAUN_MR16_v1.0.9_macOS_Universal.zip`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.9/BRAUN_MR16_v1.0.9_macOS_Universal.zip) | *(CI Generated Artifact)* |
| **Linux** | Standalone, VST3, CLAP | x86_64 | [`BRAUN_MR16_v1.0.9_Linux_x64.tar.gz`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.9/BRAUN_MR16_v1.0.9_Linux_x64.tar.gz) | *(CI Generated Artifact)* |
| **Web** | Zero-Install Showcase | Cross-Platform | [`BRAUN_MR16_v1.0.9_Web_Showcase.zip`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.9/BRAUN_MR16_v1.0.9_Web_Showcase.zip) | `0da76c5e53c27fb93b42c75cf5ec9f906adeaa8096879dd5519fd06fe72a637e` |
| **Windows** | Full Bundle (Standalone+VST3) | x86_64 | `BRAUN_MR16-v1.0.9-Windows-x64.zip` | `91bdcba38a8b4c6a65afa2f2d993454fea79c6d567b00a1e463689ac0bdf72a1` |
| **Windows** | VST3 Distributable | x86_64 | `BRAUN_MR16-v1.0.9-VST3-Windows-x64.zip` | `6eebe3292b60e8fceef102bdce8a61149189e540676a8fe5a2aada2fa6b558e1` |

### Checksum Verification
```bash
# Verify archive integrity against published manifest
sha256sum -c releases/SHA256SUMS.txt
```

---

## 4. Installation & Quickstart

### Digital Audio Workstations (DAWs)
- **Windows VST3**: Extract to `%CommonProgramFiles%\VST3\BRAUN_MR16.vst3`
- **Windows CLAP**: Extract to `%CommonProgramFiles%\CLAP\BRAUN_MR16.clap`
- **Windows Standalone**: Run `BRAUN_MR16.exe` (supports ASIO and WASAPI low-latency drivers)
- **macOS AU**: Copy to `/Library/Audio/Plug-Ins/Components/BRAUN_MR16.component`
- **macOS VST3**: Copy to `/Library/Audio/Plug-Ins/VST3/BRAUN_MR16.vst3`
- **macOS CLAP**: Copy to `/Library/Audio/Plug-Ins/CLAP/BRAUN_MR16.clap`
- **Linux VST3 / CLAP**: Copy to `~/.vst3/BRAUN_MR16.vst3` and `~/.clap/BRAUN_MR16.clap`

### Zero-Install Web Showcase
1. Extract `BRAUN_MR16_v1.0.9_Web_Showcase.zip`.
2. Double-click `start.bat` on Windows (or execute `node server.js` / `npm start` on macOS/Linux).
3. Open `http://localhost:3816/` in any modern browser (Chrome, Edge, Firefox, Safari).
4. Click the orange **ACTIVE** button in Deck 07 to initialize the Web Audio context.
