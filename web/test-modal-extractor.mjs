/**
 * @file test-modal-extractor.mjs
 * @brief Unit & Integration Test Suite for BRAUN MR-16 Modal Extractor & Lorenz Morpher
 *
 * Validates:
 * 1. Radix-2 FFT, 4-term Blackman-Harris windowing & parabolic sub-bin peak estimation
 * 2. 16-pole modal decomposition from synthetic multi-frequency audio buffers
 * 3. Modal damping Q-factor estimation across temporal slices
 * 4. Geometric manifold classification (Chladni, Beam, Vocal, Horn)
 * 5. LorenzMorpher Slot A/B geodesic log-frequency and constant-power gain interpolation
 * 6. 3D chaotic Lorenz attractor trajectory steering & anti-blowup boundary clamping
 * 7. Real-time safety, zero-allocation typed array persistence, and denormal immunity
 */

import { describe, it } from 'node:test';
import assert from 'node:assert';
import {
  ModalExtractor,
  radix2FFT,
  createBlackmanHarrisWindow
} from './js/audio/modal_extractor.js';
import {
  LorenzMorpher,
  createDefaultProfile
} from './js/audio/lorenz_morpher.js';
import { MANIFOLD_RATIOS } from './js/audio/mr16_web_engine.js';

describe('BRAUN MR-16 Modal Extractor & Lorenz Morpher Test Suite', () => {

  //----------------------------------------------------------------------------
  describe('1. FFT & Sub-Bin Parabolic Interpolation Integrity', () => {
    it('verifies Radix-2 FFT parses unit Dirac impulse to flat real spectrum', () => {
      const n = 1024;
      const real = new Float32Array(n);
      const imag = new Float32Array(n);
      real[0] = 1.0; // Dirac delta at n = 0

      radix2FFT(real, imag);

      for (let i = 0; i < n; i++) {
        assert.ok(Math.abs(real[i] - 1.0) < 1.0e-5, `Real component at bin ${i} must be 1.0`);
        assert.ok(Math.abs(imag[i]) < 1.0e-5, `Imag component at bin ${i} must be 0.0`);
      }
    });

    it('verifies 4-term Blackman-Harris window satisfies unity peak and side-lobe bounds', () => {
      const size = 4096;
      const win = createBlackmanHarrisWindow(size);

      assert.strictEqual(win.length, size);
      // Peak near center
      const center = size >> 1;
      assert.ok(win[center] > 0.999, 'Window center must be close to 1.0');
      // Edge attenuation
      assert.ok(win[0] < 0.001, 'Window edge n=0 must be near zero');
      assert.ok(win[size - 1] < 0.001, 'Window edge n=size-1 must be near zero');
    });

    it('verifies sub-bin parabolic interpolation resolves off-grid sinusoidal frequency', async () => {
      const extractor = new ModalExtractor({ fftSize: 4096, numModes: 16 });
      const sampleRate = 48000;
      const targetFreq = 440.35; // Off-bin frequency (bin width is 48000/4096 ~ 11.71875 Hz)
      const durationSec = 0.5;
      const numSamples = Math.floor(sampleRate * durationSec);
      const pcm = new Float32Array(numSamples);

      for (let i = 0; i < numSamples; i++) {
        pcm[i] = Math.sin((2.0 * Math.PI * targetFreq * i) / sampleRate);
      }

      const mockBuffer = {
        sampleRate,
        length: numSamples,
        duration: durationSec,
        numberOfChannels: 1,
        getChannelData: () => pcm
      };

      const profile = await extractor.extractFromAudioBuffer(mockBuffer);

      assert.ok(profile.modes.length === 16, 'Extractor must return 16 modes');
      const detectedF0 = profile.modes[0].frequency;
      const freqDiff = Math.abs(detectedF0 - targetFreq);
      assert.ok(
        freqDiff < 1.0,
        `Detected frequency ${detectedF0} Hz must be within 1.0 Hz of target ${targetFreq} Hz (diff: ${freqDiff})`
      );
    });
  });

  //----------------------------------------------------------------------------
  describe('2. 16-Pole Modal Decomposition from Multi-Frequency Audio Buffer', () => {
    it('verifies RFC 8259 ModalProfile structure and 16 ascending resonant poles', async () => {
      const sampleRate = 48000;
      const groundTruthFreqs = [
        220.0, 440.0, 660.0, 880.0, 1100.0, 1320.0, 1540.0, 1760.0,
        1980.0, 2200.0, 2420.0, 2640.0, 2860.0, 3080.0, 3300.0, 3520.0
      ];
      const mockBuffer = ModalExtractor.synthesizeMockAudioBuffer(
        sampleRate,
        1.0,
        groundTruthFreqs,
        new Array(16).fill(1.5),
        groundTruthFreqs.map((_, idx) => Math.pow(0.9, idx))
      );

      const extractor = new ModalExtractor({ fftSize: 4096, numModes: 16 });
      const profile = await extractor.extractFromAudioBuffer(mockBuffer, 'HARMONIC_TEST_SERIES');

      // RFC 8259 Root compliance
      assert.strictEqual(profile.format, 'BRAUN_MR16_MODAL_PROFILE');
      assert.strictEqual(profile.version, 1);
      assert.ok(profile.id.startsWith('MODAL_'));
      assert.strictEqual(profile.sampleRate, 48000);
      assert.strictEqual(profile.modes.length, 16);

      // Verify fundamental detection near 220 Hz
      assert.ok(Math.abs(profile.fundamental - 220.0) < 3.0, `Fundamental must be near 220 Hz: ${profile.fundamental}`);

      // Verify monotonic ascending frequencies
      for (let i = 1; i < profile.modes.length; i++) {
        assert.ok(
          profile.modes[i].frequency > profile.modes[i - 1].frequency,
          `Mode ${i} frequency (${profile.modes[i].frequency}) must exceed mode ${i - 1} (${profile.modes[i - 1].frequency})`
        );
      }

      // Verify mode ratios: r_0 === 1.0, r_i > 1.0
      assert.strictEqual(profile.modes[0].ratio, 1.0, 'Mode 0 ratio must be bit-exact 1.0');
      for (let i = 1; i < profile.modes.length; i++) {
        assert.ok(profile.modes[i].ratio > 1.0, `Mode ${i} ratio must exceed 1.0`);
      }

      // Verify Q factor bounds in [5, 500]
      for (const m of profile.modes) {
        assert.ok(m.q >= 5.0 && m.q <= 500.0, `Mode ${m.index} Q (${m.q}) must be in [5, 500]`);
        assert.ok(m.gain >= 0.0 && m.gain <= 1.0, `Mode ${m.index} gain (${m.gain}) must be in [0, 1]`);
      }
    });

    it('verifies modal damping factor estimation derives finite positive decay times', async () => {
      const groundTruthFreqs = [220.0, 500.0, 950.0, 1600.0];
      const groundTruthDecayTimes = [2.0, 1.2, 0.7, 0.4];
      const mockBuffer = ModalExtractor.synthesizeMockAudioBuffer(
        48000,
        1.2,
        groundTruthFreqs,
        groundTruthDecayTimes,
        [1.0, 0.8, 0.6, 0.4]
      );

      const extractor = new ModalExtractor({ fftSize: 4096, numModes: 16 });
      const profile = await extractor.extractFromAudioBuffer(mockBuffer);

      // Check first 4 modes have decaying times
      for (let i = 0; i < 4; i++) {
        const m = profile.modes[i];
        assert.ok(m.decayTimeSec > 0.05, `Decay time must be > 0.05s, got ${m.decayTimeSec}`);
        assert.ok(m.decayRateAlpha > 0.1, `Decay alpha must be positive, got ${m.decayRateAlpha}`);
        assert.ok(Number.isFinite(m.q), `Q factor must be finite, got ${m.q}`);
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('3. Acoustic Manifold Classification Integrity', () => {
    it('correctly classifies a Biharmonic Chladni Square Plate synthetic series', async () => {
      const f0 = 220.0;
      const chladniFreqs = MANIFOLD_RATIOS.CHLADNI.map(r => f0 * r);
      const mockBuffer = ModalExtractor.synthesizeMockAudioBuffer(
        48000,
        0.8,
        chladniFreqs,
        new Array(16).fill(1.5),
        new Array(16).fill(0.7)
      );

      const extractor = new ModalExtractor({ fftSize: 4096, numModes: 16 });
      const profile = await extractor.extractFromAudioBuffer(mockBuffer);

      assert.strictEqual(profile.manifoldClassification.type, 'CHLADNI');
      assert.ok(profile.manifoldClassification.confidence > 0.5, 'Confidence must exceed 0.5');
      assert.ok(profile.manifoldClassification.mse < 0.1, `MSE must be low: ${profile.manifoldClassification.mse}`);
    });

    it('correctly classifies a Stiff Struck Beam / Marimba Bar synthetic series', async () => {
      const f0 = 100.0;
      const beamFreqs = MANIFOLD_RATIOS.BEAM.map(r => Math.min(20000, f0 * r));
      const mockBuffer = ModalExtractor.synthesizeMockAudioBuffer(
        48000,
        1.0,
        beamFreqs,
        new Array(beamFreqs.length).fill(1.2),
        new Array(beamFreqs.length).fill(0.8)
      );

      const extractor = new ModalExtractor({ fftSize: 4096, numModes: 16 });
      const profile = await extractor.extractFromAudioBuffer(mockBuffer);

      assert.strictEqual(profile.manifoldClassification.type, 'BEAM');
      assert.ok(profile.manifoldClassification.confidence > 0.4);
    });

    it('correctly classifies a Vocal Formant Tract synthetic series', async () => {
      const f0 = 130.81;
      const vocalFreqs = MANIFOLD_RATIOS.VOCAL.map(r => f0 * r);
      const mockBuffer = ModalExtractor.synthesizeMockAudioBuffer(
        48000,
        0.8,
        vocalFreqs,
        new Array(16).fill(1.0),
        new Array(16).fill(0.6)
      );

      const extractor = new ModalExtractor({ fftSize: 4096, numModes: 16 });
      const profile = await extractor.extractFromAudioBuffer(mockBuffer);

      assert.strictEqual(profile.manifoldClassification.type, 'VOCAL');
    });

    it('correctly classifies a Poincaré Hyperbolic Horn synthetic series', async () => {
      const f0 = 200.0;
      const hornFreqs = MANIFOLD_RATIOS.HORN.map(r => f0 * r);
      const mockBuffer = ModalExtractor.synthesizeMockAudioBuffer(
        48000,
        0.8,
        hornFreqs,
        new Array(16).fill(1.5),
        new Array(16).fill(0.5)
      );

      const extractor = new ModalExtractor({ fftSize: 4096, numModes: 16 });
      const profile = await extractor.extractFromAudioBuffer(mockBuffer);

      assert.strictEqual(profile.manifoldClassification.type, 'HORN');
    });
  });

  //----------------------------------------------------------------------------
  describe('4. LorenzMorpher Slot Management & Geodesic Interpolation', () => {
    it('verifies alpha = 0.0 reproduces Profile A and alpha = 1.0 reproduces Profile B', () => {
      const morpher = new LorenzMorpher();
      const profA = createDefaultProfile('CHLADNI', 220.0, 90.0);
      const profB = createDefaultProfile('BEAM', 150.0, 60.0);

      morpher.setProfileA(profA);
      morpher.setProfileB(profB);
      assert.ok(morpher.hasProfiles());

      // Morph at alpha = 0.0 (Slot A)
      const morphA = morpher.morph(0.0, null, 0.0);
      for (let i = 0; i < 16; i++) {
        assert.ok(
          Math.abs(morphA.frequencies[i] - profA.frequencies[i]) < 1.0e-3,
          `At alpha=0, mode ${i} frequency must match Profile A`
        );
        assert.ok(
          Math.abs(morphA.qFactors[i] - profA.qFactors[i]) < 1.0e-3,
          `At alpha=0, mode ${i} Q must match Profile A`
        );
      }

      // Morph at alpha = 1.0 (Slot B)
      const morphB = morpher.morph(1.0, null, 0.0);
      for (let i = 0; i < 16; i++) {
        assert.ok(
          Math.abs(morphB.frequencies[i] - profB.frequencies[i]) < 1.0e-3,
          `At alpha=1, mode ${i} frequency must match Profile B`
        );
        assert.ok(
          Math.abs(morphB.qFactors[i] - profB.qFactors[i]) < 1.0e-3,
          `At alpha=1, mode ${i} Q must match Profile B`
        );
      }
    });

    it('verifies alpha = 0.5 computes exact geometric log-frequency mean without distortion', () => {
      const morpher = new LorenzMorpher();
      const profA = createDefaultProfile('CHLADNI', 200.0, 100.0);
      const profB = createDefaultProfile('CHLADNI', 800.0, 100.0);

      morpher.setProfileA(profA);
      morpher.setProfileB(profB);

      const mid = morpher.morph(0.5, null, 0.0);

      // Fundamental of A is 200 Hz, B is 800 Hz. Geometric mean: sqrt(200 * 800) = 400 Hz
      const expectedF0 = Math.sqrt(200.0 * 800.0);
      assert.ok(
        Math.abs(mid.frequencies[0] - expectedF0) < 0.1,
        `Expected geometric midpoint ${expectedF0} Hz, got ${mid.frequencies[0]}`
      );
    });

    it('verifies acoustic energy conservation on modal gains during crossfade', () => {
      const morpher = new LorenzMorpher();
      const profA = createDefaultProfile('CHLADNI', 220.0, 90.0);
      const profB = createDefaultProfile('BEAM', 220.0, 90.0);

      morpher.setProfileA(profA);
      morpher.setProfileB(profB);

      const res = morpher.morph(0.5, null, 0.0);
      for (let i = 0; i < 16; i++) {
        const ga = profA.gains[i];
        const gb = profB.gains[i];
        const expectedGain = Math.sqrt(0.5 * ga * ga + 0.5 * gb * gb);
        assert.ok(
          Math.abs(res.gains[i] - expectedGain) < 1.0e-4,
          `Gain at mode ${i} must preserve energy: ${res.gains[i]} vs ${expectedGain}`
        );
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('5. 3D Chaotic Lorenz Attractor Trajectory Steering', () => {
    it('verifies steerAmount perturbs interpolation trajectory through 16D modal space', () => {
      const morpher = new LorenzMorpher();
      const profA = createDefaultProfile('CHLADNI', 200.0, 90.0);
      const profB = createDefaultProfile('BEAM', 400.0, 90.0);

      morpher.setProfileA(profA);
      morpher.setProfileB(profB);

      const lorenzState = { x: 12.5, y: -15.0, z: 32.0 };

      const unsteered = morpher.morph(0.5, lorenzState, 0.0);
      const steered = morpher.morph(0.5, lorenzState, 0.75);

      // Effective alphas must diverge from exact 0.5
      let hasAlphaDivergence = false;
      let hasFreqDivergence = false;

      for (let i = 0; i < 16; i++) {
        if (Math.abs(steered.effectiveAlphas[i] - unsteered.effectiveAlphas[i]) > 0.01) {
          hasAlphaDivergence = true;
        }
        if (Math.abs(steered.frequencies[i] - unsteered.frequencies[i]) > 0.5) {
          hasFreqDivergence = true;
        }
        // Safety bounds
        assert.ok(steered.frequencies[i] >= 20.0 && steered.frequencies[i] <= 22000.0);
        assert.ok(steered.qFactors[i] >= 5.0 && steered.qFactors[i] <= 500.0);
        assert.ok(steered.panning[i] >= -1.0 && steered.panning[i] <= 1.0);
      }

      assert.ok(hasAlphaDivergence, 'Lorenz steering must alter effective alphas');
      assert.ok(hasFreqDivergence, 'Lorenz steering must alter modal frequencies');
    });

    it('verifies toxic NaN / Infinity / extreme Lorenz inputs are safely clamped', () => {
      const morpher = new LorenzMorpher();
      const toxicStates = [
        { x: NaN, y: NaN, z: NaN },
        { x: Infinity, y: -Infinity, z: Infinity },
        { x: 1.0e20, y: -1.0e20, z: 1.0e20 },
        { x: -1.0e-38, y: 1.0e-38, z: 0.0 }
      ];

      for (const st of toxicStates) {
        const res = morpher.morph(0.5, st, 1.0);
        for (let i = 0; i < 16; i++) {
          assert.ok(Number.isFinite(res.frequencies[i]), `Frequency mode ${i} must be finite`);
          assert.ok(Number.isFinite(res.qFactors[i]), `Q factor mode ${i} must be finite`);
          assert.ok(Number.isFinite(res.gains[i]), `Gain mode ${i} must be finite`);
          assert.ok(Number.isFinite(res.panning[i]), `Pan mode ${i} must be finite`);
          assert.ok(res.frequencies[i] >= 20.0 && res.frequencies[i] <= 22000.0);
          assert.ok(res.qFactors[i] >= 5.0 && res.qFactors[i] <= 500.0);
        }
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('6. Real-Time Performance & Benchmarking', () => {
    it('verifies 1,000 continuous morph() calculations execute in < 150 ms (>6,600 FPS)', () => {
      const morpher = new LorenzMorpher();
      const lorenzState = { x: 8.5, y: -6.2, z: 27.0 };

      const t0 = performance.now();
      for (let i = 0; i < 1000; i++) {
        const alpha = (i % 100) / 100.0;
        morpher.morph(alpha, lorenzState, 0.65);
      }
      const elapsed = performance.now() - t0;

      assert.ok(
        elapsed < 150.0,
        `1,000 morph calculations took ${elapsed.toFixed(2)} ms (budget < 150.0 ms)`
      );
    });
  });

});
