# BRAUN MR-16 · Modaler Resonator & Kinetischer Impulssynthesizer

[![Verification: 100% PASS](https://img.shields.io/badge/Verification-100%25%20PASS%20(207%2F207%20C%2B%2B%20%7C%20206%2F206%20Web)-24FF6A?style=for-the-badge&logo=checkmarx)](VERIFICATION_CHECKLIST.md)
[![Version: 1.0.11](https://img.shields.io/badge/Version-1.0.11-EE592B?style=for-the-badge)](https://github.com/sneed-and-feed/braun_mr-16/releases/tag/v1.0.11)
[![CI](https://github.com/sneed-and-feed/braun_mr-16/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/sneed-and-feed/braun_mr-16/actions)
[![Windows VST3 & CLAP](https://img.shields.io/badge/Windows-VST3%20%7C%20CLAP%20%7C%20Standalone-blue?style=for-the-badge&logo=windows)](#4-daw-setup--installation-paths)
[![macOS AU & VST3](https://img.shields.io/badge/macOS-AU%20%7C%20VST3%20%7C%20CLAP%20%7C%20Standalone-white?style=for-the-badge&logo=apple)](#4-daw-setup--installation-paths)
[![Linux VST3 & CLAP](https://img.shields.io/badge/Linux-VST3%20%7C%20CLAP%20%7C%20Standalone-FCC624?style=for-the-badge&logo=linux)](#4-daw-setup--installation-paths)
[![Web Audio API](https://img.shields.io/badge/Web%20Audio-100%25%20Client--Side-4A4A4A?style=for-the-badge)](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)
[![License: MIT](https://img.shields.io/badge/License-MIT-black?style=for-the-badge)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-4A4A4A?style=for-the-badge)](https://isocpp.org/)
[![JUCE 8](https://img.shields.io/badge/JUCE-8.0.6-EE592B?style=for-the-badge)](https://juce.com/)

> *"Weniger, aber besser" — Less, but better.*  
> **Legal Notice**: Not affiliated with Braun GmbH. Dieter Rams inspired design homage.  
> Manufactured by Sneed's Feed & Seed Ltd. Companion to [BRAUN AS-42](https://sneed-and-feed.github.io/braun_as-42/) and [BRAUN RB-26](https://sneed-and-feed.github.io/braun_rb-26/).

---

## 1. Acoustic Concept & Trilogy Overview

The **BRAUN MR-16** is a physical acoustic modal resonator and kinetic impulse synthesizer completing the Dieter Rams ambient trilogy:
1. **BRAUN AS-42**: Melodic & Harmonic Voice (felt piano acoustic modeling, Eno tape loops, microtonal drones).
2. **BRAUN MR-16**: Physical Acoustic Body & Kinetic Strike (tactile transient exciters, 16-pole modal resonator matrix, 4 geometric manifolds, 3D Lorenz attractor, and tri-phase spatial BBD chorus).
3. **BRAUN RB-26**: Spatial Diffusion Reverb (Poincaré hyperbolic cavity, 8-line FDN, Shepard-Risset spirals).

In acoustic physics, exciters and resonant bodies are coupled systems. The MR-16 models this interaction directly:
- **Kinetic Exciter Engine**: Hunt-Crossley mass-spring mallet strike, Karnopp stick-slip friction bow, Buchla 292 optical vactrol pluck, and external stereo line input.
- **Autonomous Clocks**: Stochastic Poisson rain generator and Euclidean polyrhythmic sequencer driving a 16-key microtonal chime strip.
- **16-Pole Modal Resonator Matrix**: 16 parallel 2nd-order topology-preserving state variable filters across 4 manifolds (Biharmonic Chladni plate, stiff struck beam, vocal tract, Poincaré horn) with 5 material damping profiles (wood, glass, steel, brass, nylon) and mutual Householder energy scattering.
- **Continuous Modulation & Space**: 3D chaotic Lorenz attractor modulating modal frequencies and Q spread; golden-ratio stereo panning ($\Phi = 137.5^\circ$); tri-phase BBD chorus with Roland Dimension D mono phase cancellation immunity.
- **DSP Performance**: Algorithmic lookahead latency is **0 samples** (zero latency). Hermite BoundedSaturator provides linear transparency below $-2.85\text{ dBFS}$ ($0.72$) and asymptotic saturation ceiling at $+0.42\text{ dBFS}$ ($1.05$), with multi-tier modal anti-runaway clamping.

---

## 2. Zero-Install Web Quickstart

Launch the zero-dependency local static server:

```bash
# Windows
start.bat

# macOS / Linux / Cross-Platform
npm start
# or: node server.js
```

Open `http://localhost:3816/` in Chrome, Edge, Firefox, or Safari for the full 19" 2U rackmount interface, 28 precision controls, 16-key chime strip, 3-mode vector CRT phosphor display, 16 factory presets, and 16-bit WAV master recorder.

> [!NOTE]
> The instrument initializes in **Standby** mode (`power = false`) by default to protect studio monitors. Click the orange **ACTIVE** button in Deck 07 to engage the audio graph.

---

## 3. Distributable Release Packages (v1.0.11)

Pre-compiled binary packages and cryptographic manifests published under [GitHub Releases v1.0.11](https://github.com/sneed-and-feed/braun_mr-16/releases/tag/v1.0.11):

| Platform | Format | Architecture | Distribution Package Archive | Verification Manifest |
|:---|:---|:---|:---|:---|
| **Windows** | Complete Bundle (Standalone, VST3, CLAP) | x86_64 | [`BRAUN_MR16-v1.0.11-Windows-x64.zip`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.11/BRAUN_MR16-v1.0.11-Windows-x64.zip) | `releases/SHA256SUMS.txt` |
| **Windows** | VST3 Plugin Only | x86_64 | [`BRAUN_MR16-v1.0.11-VST3-Windows-x64.zip`](https://github.com/sneed-and-feed/braun_mr-16/releases/download/v1.0.11/BRAUN_MR16-v1.0.11-VST3-Windows-x64.zip) | `releases/SHA256SUMS.txt` |

Verify cryptographic archive integrity:

```bash
sha256sum -c releases/SHA256SUMS.txt
```

---

## 4. DAW Setup & Installation Paths

- **Windows VST3**: `%CommonProgramFiles%\VST3\BRAUN_MR16.vst3`
- **Windows CLAP**: `%CommonProgramFiles%\CLAP\BRAUN_MR16.clap`
- **Windows Standalone**: Run `BRAUN_MR16.exe` (ASIO / WASAPI)
- **macOS Audio Unit (AU)**: `/Library/Audio/Plug-Ins/Components/BRAUN_MR16.component`
- **macOS VST3**: `/Library/Audio/Plug-Ins/VST3/BRAUN_MR16.vst3`
- **macOS CLAP**: `/Library/Audio/Plug-Ins/CLAP/BRAUN_MR16.clap`
- **Linux VST3 & CLAP**: `~/.vst3/BRAUN_MR16.vst3` and `~/.clap/BRAUN_MR16.clap`
- **Linux CI / GUI Option**: `MR16_USE_WEBVIEW` defaults to `OFF` on Linux for headless environments and standard DSP hosts. Build with `-DMR16_USE_WEBVIEW=ON` when WebKitGTK is available.

---

## 5. Curated Factory Presets

| # | Preset ID | Category | Audio Description |
|:---:|:---|:---|:---|
| 1 | `DEFAULT_CONCERT_CHLADNI` | Studio Benchmark | Calibrated 2D square steel plate struck by medium felt mallet with balanced Householder coupling. |
| 2 | `MARIMBA_ROSEWOOD_BAR` | Mallet & Wood | Stiff wooden beam resonator with Euler-Bernoulli dispersion and warm absorption. |
| 3 | `CRYSTAL_WINE_GLASS` | Friction & Glass | High-Q borosilicate glass rim excited by continuous stick-slip friction. |
| 4 | `TIBETAN_BRONZE_BELL` | Metallic Plates | Dense bronze plate excited by metal striker, rich Householder energy exchange, long ring-down. |
| 5 | `CATHEDRAL_TUBE_DRONE` | Acoustic Cavity | Poincaré horn resonator driven by air jet exciter with optical vactrol sag. |
| 6 | `VOCAL_FORMANT_CHOIR` | Vocal Tract | 16-pole vocal tract cavity morphing across vowels modulated by chaotic Lorenz drift. |
| 7 | `POISSON_ZINC_ROOF` | Kinetic Stochastic | Stochastic Poisson rain impulses impacting a thin sheet metal plate across golden panorama. |
| 8 | `EUCLIDEAN_METALLOPHONE` | Polyrhythmic | Clocked 5/16 Euclidean strike pattern driving interlocking brass chime modes. |
| 9 | `DIMENSION_CELESTE` | Spatial Chorus | Glockenspiel bells routed through Roland Dimension D tri-phase chorus with wide decorrelation. |
| 10 | `LORENZ_CHAOTIC_ORBIT` | Chaotic Morph | Metallic plate gong with center frequencies continuously warped along Lorenz butterfly orbit. |
| 11 | `BOWED_TITANIUM_SHEET` | Bowed Friction | Continuous Karnopp friction bow exciting an ultra-stiff titanium plate. |
| 12 | `HYPERBOLIC_SUB_RESONATOR`| Sub Bass | Sub-bass physical horn cavity tuned to 43.6 Hz with tight Buchla 292 LPG dynamics. |
| 13 | `AFRICAN_BALAFON` | World Acoustic | Traditional wooden slat marimba with membrane overtones and gourd damping. |
| 14 | `SOLAR_WIND_RESONANCE` | Ambient Drone | High-density Poisson micro-transients driving lightly damped nylon and brass resonators. |
| 15 | `FROSTED_GLASS_ARPEGGIO` | Generative | Interlocking 7/12 Euclidean rhythm exciting high-frequency crystal glass modes with BBD wash. |
| 16 | `INFINITE_MODAL_SINGULARITY`| Extreme Self-Oscillation | Maximum Householder feedback coupling (100%) held in bounded singing resonance by Hermite limiter. |

---

## 6. Documentation & Architecture Reference

- **[ARCHITECTURE.md](ARCHITECTURE.md)**: Full engineering specification, 73-line Mermaid signal-flow diagram, mathematical derivations for Decks 01-07, 32-parameter APVTS table, and Linux build instructions.
- **[VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md)**: Complete test registry with 207 C++ headless tests and 206 Web Audio automated tests passing at 100%.

---

## 7. Legal Notice & Homage Disclaimer

The **BRAUN MR-16** is an independent homage to the industrial design language of Dieter Rams and 1960s Braun audio equipment.

- **Manufacturer**: Sneed's Feed & Seed Ltd.
- **Homage Disclaimer**: Not affiliated with Braun GmbH. Dieter Rams inspired design homage.
- **Compatibility Identifiers**: Plugin manufacturer code `Brun`, plugin code `Mr16`, and CLAP identifier `com.sneedandfeed.mr16` are maintained for DAW session compatibility.

---

## License

MIT License. Copyright (c) 2026 Sneed's Feed & Seed Ltd. & Antigravity Audio Engineering.
