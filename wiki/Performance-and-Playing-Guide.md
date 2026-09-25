# Performance & Playing Guide

The **BRAUN MR-16** is designed as both a research-grade acoustic resonator and an expressive physical instrument. Following Dieter Rams' functionalist design ergonomics, all performance controls are immediately accessible without nested menus, secondary pages, or modal dialogs.

---

## 16-Key Tactile Microtonal Chime Strip

The bottom chassis features a 16-key microtonal chime performance strip dynamically quantized to the selected harmonic scale:

```text
+---------------------------------------------------------------------------------------------------------------+
| [ A ]   [ S ]   [ D ]   [ F ]   [ G ]   [ H ]   [ J ]   [ K ]   [ L ]   [ ; ]   [ ' ]   [ Z ]   [ X ]   [ C ] |
| 220Hz   247Hz   277Hz   330Hz   370Hz   440Hz   494Hz   554Hz   659Hz   740Hz   880Hz   988Hz   1.1k    1.3k  |
+---------------------------------------------------------------------------------------------------------------+
```

### Touch, Mouse, and Glissando Mechanics
- **Direct Strike:** Click or tap any key on the chime strip to strike that modal frequency with the active exciter.
- **Velocity Sensitivity:** Vertical click position modulates strike velocity:
  - Clicking near the **top edge** produces a soft, intimate strike ($v \approx 0.35$).
  - Clicking near the **bottom edge** produces a forceful kinetic impact ($v \approx 1.00$).
- **Glissando Swipes:** Click/touch and drag horizontally across the strip to trigger shimmering runs. Because every key is quantized to the active scale, **there are zero discordant notes**.

### Computer Keyboard Hotkey Map
For laptop or desktop performance without an external MIDI controller, the computer keyboard directly triggers all 16 chime degrees and kinetic impulses:

| Key Binding | Chime Degree | Pitch (Root = A 220 Hz, Budd Pentatonic) | Action |
|:---:|:---:|:---:|:---|
| **`A`** | Chime 01 | 220.0 Hz | Strike Scale Degree 1 |
| **`S`** | Chime 02 | 246.9 Hz | Strike Scale Degree 2 |
| **`D`** | Chime 03 | 277.2 Hz | Strike Scale Degree 3 |
| **`F`** | Chime 04 | 329.6 Hz | Strike Scale Degree 4 |
| **`G`** | Chime 05 | 369.9 Hz | Strike Scale Degree 5 |
| **`H`** | Chime 06 | 440.0 Hz (Octave 1) | Strike Scale Degree 6 |
| **`J`** | Chime 07 | 493.9 Hz | Strike Scale Degree 7 |
| **`K`** | Chime 08 | 554.4 Hz | Strike Scale Degree 8 |
| **`L`** | Chime 09 | 659.3 Hz | Strike Scale Degree 9 |
| **`;`** | Chime 10 | 739.9 Hz | Strike Scale Degree 10 |
| **`'`** | Chime 11 | 880.0 Hz (Octave 2) | Strike Scale Degree 11 |
| **`Z`** | Chime 12 | 987.8 Hz | Strike Scale Degree 12 |
| **`X`** | Chime 13 | 1108.7 Hz | Strike Scale Degree 13 |
| **`C`** | Chime 14 | 1318.5 Hz | Strike Scale Degree 14 |
| **`V`** | Chime 15 | 1479.9 Hz | Strike Scale Degree 15 |
| **`B`** | Chime 16 | 1760.0 Hz (Octave 3) | Strike Scale Degree 16 |
| **`Spacebar`** | Master Impulse | Single-Sample Dirac Pulse ($\delta[n]$) | Trigger Pure Impulse Response |
| **`Escape`** | UI Clear | - | Unfocus knobs, cancel drag overlays |

---

## Tactile Strike Audition Buttons

The performance bar above the chime strip houses four dedicated tactile strike buttons:

```text
[ TACTILE STRIKE: ]   [ DIRAC IMPULSE ]   [ FELT HAMMER ]   [ BOWED FRICTION ]   [ AIR JET ]
```

1. **`DIRAC IMPULSE`**: Injects a theoretical unit impulse $\delta[n] = 1.0$ at sample $n = 0$. This bypasses exciter colorations, exciting all 16 modes simultaneously with equal energy to reveal the pure acoustic impulse response and Householder ring-down of the physical body.
2. **`FELT HAMMER`**: Triggers a kinetic mallet strike using the Hunt-Crossley contact model. The impact character is governed by `strike_hardness` and `strike_velocity`.
3. **`BOWED FRICTION`**: Triggers a sustained stick-slip friction burst using the Karnopp friction model. The harmonic squeal is governed by `friction_force` and `friction_speed`.
4. **`AIR JET`**: Triggers an optical vactrol air puff with exponential photocarrier sag governed by `vactrol_sag`.

---

## Six Curated Microtonal Performance Scales

The MR-16 features six harmonic scales designed specifically for resonant physical acoustics. Select the scale from the `SCALE` dropdown in the performance bar:

```text
[ ROOT: A (220 Hz) v ]   [ SCALE: BUDD PENTATONIC v ]
```

### 1. `BUDD PENTATONIC` (Harold Budd Homage)
- **Intervals:** 0, 2, 4, 7, 9 semitones (repeating across octaves).
- **Acoustic Mood:** Warm, consonant, pastoral ambient spaces. Inspired by Harold Budd's minimalist piano compositions (*The Pearl*, *The Pavilion of Dreams*).
- **Harmonic Alignment:** Perfect 5ths and major 3rds align naturally with Chladni plate and marimba beam overtones, eliminating inharmonic beating.

### 2. `LYDIAN DREAM`
- **Intervals:** 0, 2, 4, 6, 7, 9, 11 semitones.
- **Acoustic Mood:** Celestial, suspended, ethereal wonder. The raised 4th degree ($+6\text{ st}$, tritone) creates an ascending physical resonance that blooms through the tri-phase BBD chorus.

### 3. `DORIAN MYSTIC`
- **Intervals:** 0, 2, 3, 5, 7, 9, 10 semitones.
- **Acoustic Mood:** Somber, medieval, meditative reflection. The natural 6th degree paired with a minor 3rd provides a rich harmonic bed for bowed friction and metal plate percussion.

### 4. `YOSHIMURA AMBIENT`
- **Intervals:** 0, 2, 4, 7, 11 semitones.
- **Acoustic Mood:** Architectural Japanese environmental music. Inspired by Hiroshi Yoshimura's *Music for Nine Post Cards* and *Green*. Characterized by wide spatial intervals that emphasize the decay of physical materials (glass, rosewood).

### 5. `AEOLIAN MIDNIGHT`
- **Intervals:** 0, 2, 3, 5, 7, 8, 10 semitones.
- **Acoustic Mood:** Nocturnal, deep, brooding melancholy. Accentuates the deep fundamental resonances of the Poincaré hyperbolic horn and dense bell bronze.

### 6. `PYTHAGOREAN JUST`
- **Intervals:** Derived from pure mathematical integer frequency ratios:
  
  ```math
  f_k = f_{\mathrm{root}} \cdot \left(\frac{3}{2}\right)^p \cdot 2^{-q}
  ```

- **Acoustic Mood:** Pure harmonic consonant transparency. Because 3:2 fifths are geometrically aligned without the 2-cent compromise of 12-Tone Equal Temperament (12-TET), sustained modal chords produce zero phase cancellation or acoustic warbling.

### Root Frequency Tuning
The base pitch of the entire performance strip can be snapped to six standard concert acoustic roots:

| Root Key | Base Frequency | Recommended Acoustic Use |
|:---:|:---:|:---|
| **`A`** | 220.00 Hz | Concert A3 pitch standard; versatile for bells and chimes |
| **`C`** | 130.81 Hz | Warm foundational C3; ideal for marimba bars and vocal formants |
| **`D`** | 146.83 Hz | Deep Celtic and ambient drone root |
| **`E`** | 164.81 Hz | Bright, expressive acoustic guitar and cello range |
| **`F`** | 174.61 Hz | Solfeggio resonant frequency; deep spiritual drone bed |
| **`G`** | 196.00 Hz | Rich baritone anchor; ideal for bowed metal sheets |

The frequency for chime degree $i$ is calculated dynamically:

```math
f_i = f_{\mathrm{root}} \cdot 2^{\frac{s_i}{12}}
```

where $s_i$ is the semitone offset of the $i$-th degree in the active scale.

---

## Generative Ambient Performance

The MR-16 can perform autonomously as a generative sound installation without human input:

### 1. Stochastic Poisson Raindroplets
1. Set `EXCITER MODE` to `STRIKE`.
2. Set `strike_hardness` to `0.45` and `strike_velocity` to `0.70`.
3. Increase `poisson_density` to `15.0 Hz` (or click `REC/POISSON` in the audition bar).
4. Select `MATERIAL: GLASS` and `MANIFOLD: CHLADNI`.
5. Set `modal_damping` to `2.4s` and `golden_pan_spread` to `90%`.
6. **Result:** An endlessly evolving shower of physical glass chime droplets scattered across the stereo horizon.

### 2. Polyrhythmic Euclidean Metallophone
1. Set `poisson_density` to `0 Hz`.
2. Configure `euclidean_pulses` to `5` and `euclidean_steps` to `16`.
3. Select `MATERIAL: BRASS` and set `modal_frequency` to `330 Hz`.
4. Turn `modal_coupling` to `0.55` to induce sympathetic mode sharing.
5. Engage `CHORUS ACTIVE` with `chorus_dimension = 0.85`.
6. **Result:** An interlocking, mathematically balanced West African bell pattern echoing through three-dimensional space.

### 3. Lorenz-Driven Chaotic Drift
1. Turn `lorenz_rate` to `0.40 Hz` and `lorenz_chaos` to `0.65`.
2. Increase `lorenz_freq_mod` to `25%` and `lorenz_q_mod` to `35%`.
3. Hold or sequence notes on the chime strip.
4. **Result:** The modal body smoothly breathes and shifts between harmonic and inharmonic timbres as the 3D Lorenz attractor traces its twin-lobed strange orbit.

---

## Hardware MIDI Controller Integration

The MR-16 responds fully to standard USB/Bluetooth MIDI keyboards, pad controllers, and MPE transmitters:

- **MIDI Note On:** Instantly updates `modal_frequency` to match the incoming note ($f = 440 \cdot 2^{(m - 69)/12}$), re-exciting the 16 modes with incoming key velocity.
- **Pitch Bend Wheel:** Smoothly glides the fundamental frequency by $\pm 2$ semitones with continuous real-time filter interpolation.
- **Modulation Wheel (CC 1):** Mapped by default to `modal_coupling`, smoothly increasing Householder sympathetic ring-down and inharmonic feedback as the wheel is advanced.
- **Sustain Pedal (CC 64):** Multiplies `modal_damping` by $3.5\times$ while depressed, allowing struck chords and chime clusters to sustain naturally.
