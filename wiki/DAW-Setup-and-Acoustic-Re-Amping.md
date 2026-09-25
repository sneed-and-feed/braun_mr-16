# DAW Setup & Acoustic Re-Amping

The **BRAUN MR-16** operates across two distinct operational paradigms in professional Digital Audio Workstations:
1. **Virtual Instrument (Instrument Mode):** Triggered via MIDI notes, chords, and CC messages from hardware keyboards, sequencer clips, or the onboard 16-key chime performance strip.
2. **Acoustic Resonator & Re-Amper (Audio Effect Mode):** Processing live audio inputs (drums, vocals, guitars, synthesizers) via the `EXT IN` line drive, transforming dry digital stems into resonant physical bodies.

---

## Plugin Formats & Installation Paths

The MR-16 is distributed as 64-bit native plugins with **0 samples algorithmic lookahead latency**:

| Format | Operating System | Default Installation Directory |
| :--- | :--- | :--- |
| **VST3** | Windows x64 | `C:\Program Files\Common Files\VST3\BRAUN_MR16.vst3` |
| **CLAP** | Windows x64 | `C:\Program Files\Common Files\CLAP\BRAUN_MR16.clap` |
| **Standalone (.exe)** | Windows x64 | Any directory (e.g. `C:\Program Files\Braun\MR16\BRAUN_MR16.exe`) |
| **VST3** | macOS (Universal) | `/Library/Audio/Plug-Ins/VST3/BRAUN_MR16.vst3` |
| **AUv2 (.component)**| macOS (Universal) | `/Library/Audio/Plug-Ins/Components/BRAUN_MR16.component` |
| **CLAP** | macOS (Universal) | `/Library/Audio/Plug-Ins/CLAP/BRAUN_MR16.clap` |
| **VST3** | Linux x64 | `~/.vst3/BRAUN_MR16.vst3` or `/usr/lib/vst3/BRAUN_MR16.vst3` |
| **CLAP** | Linux x64 | `~/.clap/BRAUN_MR16.clap` or `/usr/lib/clap/BRAUN_MR16.clap` |

---

## DAW Host Configuration Guide

### 1. Ableton Live (v11 / v12)

#### Instrument Mode:
1. Open the Ableton browser, navigate to **Plug-Ins** > **VST3** > **Sneed's Feed & Seed Ltd.** > **BRAUN MR-16**.
2. Drag the plugin onto an empty **MIDI Track**.
3. Create a MIDI clip or arm the track for hardware keyboard input. Playing MIDI notes will dynamically track the fundamental modal frequency ($f_0$).

#### Audio Effect Insert / Acoustic Re-Amping Mode:
1. Drag **BRAUN MR-16** directly onto an **Audio Track** (or return bus).
2. Click the `EXT IN` segment button in **Deck 01: Kinetic Exciter Engine**.
3. Adjust `EXT IN GAIN` to calibrate incoming audio level ($0.0\text{ dB}$ nominal).
4. Set `DRY / WET` mix in Deck 05 to blend the dry signal with the physical acoustic resonance.

```text
[ Audio Track ] ──> [ MR-16 Insert ] ──(EXT IN)──> [ 16-Pole Resonator ] ──> [ Master Bus ]
```

---

### 2. FL Studio (v20 / v21 / v24)

1. Open **Options** > **Manage Plugins** and click **Find installed plugins**. Ensure `BRAUN MR-16` appears under both Generator and Effect categories.
2. **As an Instrument:** Drag `BRAUN MR-16` from the Browser into the **Channel Rack**. Open the Piano Roll to compose microtonal modal melodies.
3. **As a Mixer Effect:** Select a Mixer Insert slot, click the FX slot dropdown, and choose `BRAUN MR-16`. In Deck 01, switch to `EXT IN`.

---

### 3. Cockos REAPER (v6 / v7)

1. Press `Ctrl + T` to create a new track. Click the **FX** button and type `MR-16`.
2. Select either `VST3i: BRAUN MR-16` or `CLAPi: BRAUN MR-16`.
3. In REAPER's 2 In / 2 Out pin connector routing matrix, the MR-16 accepts stereo audio inputs while responding simultaneously to track MIDI events.

---

### 4. Bitwig Studio (v4 / v5)

Bitwig offers native hosting for the **CLAP** format with advanced non-destructive modulation:
1. Drag the CLAP version onto an instrument or audio track.
2. The MR-16 exposes all 33 APVTS parameters to Bitwig's internal macro modulators, LFOs, and voice stacks.
3. Bitwig Note Expression (micro-pitch, timbre, and pressure) maps seamlessly to `modal_frequency` and `strike_velocity`.

---

### 5. Steinberg Cubase & Nuendo (v12 / v13)

1. Add an **Instrument Track** for MIDI composition, or an **Audio Track Insert** for physical re-amping.
2. Right-click any parameter knob in the MR-16 to access Cubase's native context menu:
   - **Show "Parameter" Automation Track**
   - **Learn MIDI CC**
   - **Reset to Default**

---

## Acoustic Re-Amping & "Re-Chassising" Workflow

### What is Acoustic Re-Chassising?
Electronic drum machines, digital synthesizers, and ITB (in-the-box) audio stems often suffer from sterile, two-dimensional transients and phase rigidity. **Acoustic Re-Chassising** injects these digital signals into the MR-16's physical modal matrix, exciting a simulated physical object (steel plate, rosewood beam, bronze bell, or resonant horn cavity) in real time.

```text
+-------------------+      Line Audio       +---------------------------------------------+
| 808 / 909 Drum    | ───────────────────>  | BRAUN MR-16 (Acoustic Resonator)           |
| Synthetic Click   |                       | • Deck 01: EXT IN DRIVE (+6 dB)             |
+-------------------+                       | • Deck 02: MANIFOLD: BEAM (Rosewood)        |
                                            | • Deck 02: HOUSEHOLDER COUPLING (50%)       |
                                            | • Deck 05: GOLDEN STEREO PANNING (85%)      |
                                            +---------------------------------------------+
                                                                   │
                                                                   ▼
                                            +---------------------------------------------+
                                            | Organic Acoustic Wood & Metal Percussion    |
                                            +---------------------------------------------+
```

### Studio Re-Chassising Recipes

#### 1. Transforming Electronic Snares into Acoustic Rosewood Bars
- **Source:** Sterile 808 snare or rimshot sample.
- **Deck 01 (Exciter):** Set `EXCITER MODE` to `EXT IN`. Adjust `EXT IN GAIN` to `+3.0 dB`.
- **Deck 02 (Resonator):**
  - `MANIFOLD TYPE`: `BEAM` (Euler-Bernoulli Stiff Beam).
  - `MATERIAL PROFILE`: `WOOD` (Rosewood/Spruce).
  - `MODAL FREQUENCY`: `196.0 Hz` (G3).
  - `MODAL DAMPING`: `0.85s`.
  - `MODAL COUPLING`: `0.35` (35% Householder energy exchange).
- **Deck 05 (Dynamics):** Set `DRY / WET` to `70% Wet`.
- **Result:** The electronic click triggers the physical mass and dispersion of a real wooden marimba bar.

#### 2. Transforming Dry Vocal Stems into a Resonant Formant Chamber
- **Source:** Dry lead vocal track.
- **Deck 01 (Exciter):** Select `EXT IN` with `EXT IN GAIN = -2.0 dB`.
- **Deck 02 (Resonator):**
  - `MANIFOLD TYPE`: `VOCAL` (Vocal Formant Tract).
  - `MODAL FREQUENCY`: `146.8 Hz` (D3).
  - `MODAL Q`: `85.0`.
  - `MODAL DAMPING`: `2.2s`.
- **Deck 03 (Lorenz):** Set `LORENZ RATE` to `0.25 Hz` and `LORENZ FREQ MOD` to `20%`.
- **Deck 04 (Chorus):** Engage `CHORUS ACTIVE` with `DIMENSION = 0.85`.
- **Result:** The vocal track rings through an acoustic formant cavity that morphs vowels continuously with chaotic Lorenz drift.

#### 3. Transforming Synth Arpeggios into Crystalline Glass Gongs
- **Source:** Bright synthesizer saw/square arpeggio.
- **Deck 01 (Exciter):** Select `EXT IN`.
- **Deck 02 (Resonator):**
  - `MANIFOLD TYPE`: `CHLADNI` (Biharmonic Square Plate).
  - `MATERIAL PROFILE`: `GLASS` (Borosilicate).
  - `MODAL FREQUENCY`: `440.0 Hz` (A4).
  - `MODAL DAMPING`: `3.5s`.
  - `MODAL COUPLING`: `0.65` (High sympathetic ringing).
- **Result:** Every synth note excites high-$Q$ glass plate overtones, showering the arrangement in crystalline acoustic harmonics.

### Input vs. Output Vector Scope Monitoring
When re-amping external audio, Deck 06 allows direct visual comparison between the incoming dry signal and the outgoing resonated physical acoustic:
- Click `MONITOR: IN` to inspect the raw incoming audio waveform on the CRT display.
- Click `MONITOR: OUT` to inspect the 2D Chladni nodal plate vibration or stereo Lissajous phase correlation after modal resonance.

---

## DAW Automation & Host Context Menu Parity

### Right-Click Parameter Automation
In JUCE 8, all rotary knobs and discrete buttons maintain total host context menu parity:
- **Right-Click / Ctrl-Click:** Instantly opens the DAW's native automation menu.
- **Host Parameter Encoders:** Parameter IDs (`modal_frequency`, `modal_damping`, etc.) map directly to motorized console faders and DAW automation lanes.

### Total Session Recall & State Persistence
The MR-16 serializes all 33 APVTS parameters, discrete switch states (`power_state`, `scope_source`, `exciter_type`, `manifold_type`, `material_profile`, `chorus_enable`, `chorus_dimension`, `display_mode`, `poisson_density`), and the active program name directly into the DAW project XML state tree.

### Zero-Latency Editor Lifecycle
Thanks to JUCE 8 `withUserScript(window.__JUCE_INITIAL_PARAMS__)` technology introduced in `v1.0.12`, closing and reopening the MR-16 plugin GUI in any host (even during high-CPU audio playback) **never resets knob positions or causes audio dropouts**. All parameters are injected synchronously before the interface evaluates, guaranteeing 100% session stability.
