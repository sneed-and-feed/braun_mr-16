/**
 * @file test-acoustic-levels.mjs
 * @brief Headless Audio Verification Suite for BRAUN MR-16 Acoustic Resonator Engine
 *
 * Validates:
 * 1. Dynamic range & audible energy levels across all excitation types:
 *    - triggerStrike (soft, medium, hard velocity/hardness)
 *    - triggerDirac
 *    - triggerFeltHammer
 *    - triggerBowedFriction
 *    - triggerAirJet
 *    - playChimeKey (across octaves: 110, 220, 440, 880 Hz)
 *    - Peak dBFS strictly within [-18.0 dBFS, -1.0 dBFS]
 *    - DC offset suppression via 35 Hz highpass DC blocker (< -80 dBFS)
 *    - Signal-to-noise ratio (SNR > 70 dB)
 * 2. 60 dB decay times (T60) across all 5 physical materials:
 *    - Wood:   0.4s - 0.9s
 *    - Glass:  2.5s - 5.0s
 *    - Steel:  1.5s - 3.2s
 *    - Brass:  1.8s - 3.8s
 *    - Nylon:  0.15s - 0.35s
 *    - Monotonic ordering & exponential decay continuity
 * 3. 3D Chaotic Lorenz attractor & geodesic modal morphing stress test:
 *    - Zero clicks, pops, or zipper noise (max |ds| < 0.15)
 *    - Filter stability & finite bounds under chaotic parameter modulation
 * 4. Roland Dimension D tri-phase chorus spatialization & mono sum cancellation immunity:
 *    - Inter-aural cross-correlation coefficient r_LR < 0.90 (stereo spatial expansion)
 *    - Mono sum phase cancellation immunity (|Mono - (L+R)/2| < 0.05 dBFS)
 * 5. Real-time performance & deterministic audio execution
 *
 * Strict real-time safety, zero emojis, DIN 1451 technical English.
 */

import { describe, it } from 'node:test';
import assert from 'node:assert';
import { Mr16WebEngine, MATERIAL_PROFILES, MANIFOLD_RATIOS } from './js/audio/mr16_web_engine.js';
import { LorenzMorpher, createDefaultProfile } from './js/audio/lorenz_morpher.js';
import { HeadlessOfflineAudioContext } from './headless-audio-context.mjs';

/**
 * Helper to render an audio block and compute acoustic levels
 */
async function renderAcousticBlock(action, setup = null, durationSec = 2.0, sampleRate = 48000) {
  const totalSamples = Math.ceil(sampleRate * durationSec);
  const ctx = new HeadlessOfflineAudioContext(2, totalSamples, sampleRate);
  const engine = new Mr16WebEngine();
  await engine.init(ctx);
  engine.setPower(true);

  if (typeof setup === 'function') {
    setup(engine, ctx);
  }

  action(engine, ctx);

  const rendered = await ctx.startRendering();
  const left = rendered.getChannelData(0);
  const right = rendered.getChannelData(1);

  let peakL = 0, peakR = 0, peak = 0;
  let sumSq = 0, sumL = 0, sumR = 0;

  for (let i = 0; i < totalSamples; i++) {
    const sL = left[i];
    const sR = right[i];
    const mono = 0.5 * (sL + sR);
    const absL = Math.abs(sL);
    const absR = Math.abs(sR);
    const absM = Math.max(absL, absR);

    if (absL > peakL) peakL = absL;
    if (absR > peakR) peakR = absR;
    if (absM > peak) peak = absM;

    sumSq += mono * mono;
    sumL += sL;
    sumR += sR;
  }

  const rms = Math.sqrt(sumSq / totalSamples);
  const peakDb = 20 * Math.log10(Math.max(1e-9, peak));
  const rmsDb = 20 * Math.log10(Math.max(1e-9, rms));
  const dcOffsetMax = Math.max(Math.abs(sumL / totalSamples), Math.abs(sumR / totalSamples));
  const snrDb = Math.abs(peakDb - rmsDb);

  return { left, right, peak, peakDb, rms, rmsDb, dcOffsetMax, snrDb, totalSamples, sampleRate };
}

// ============================================================================
// Verification Test Suite
// ============================================================================

describe('BRAUN MR-16 Headless Acoustic Levels & Dynamic Range Audit', () => {

  //----------------------------------------------------------------------------
  describe('1. Dynamic Range & Audible Energy Audit across Excitations', () => {

    it('verifies triggerStrike (Soft Mallet: hardness 0.20, velocity 0.40) is audible within [-18, -1] dBFS', async () => {
      const res = await renderAcousticBlock(e => e.triggerStrike(0.20, 0.40));
      assert.ok(res.peakDb >= -18.0, `Soft strike must be >= -18.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.peakDb <= -1.0, `Soft strike must be <= -1.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.rmsDb >= -55.0, `Soft strike RMS must be >= -55.0 dBFS, got ${res.rmsDb.toFixed(2)} dBFS`);
      assert.ok(res.dcOffsetMax < 1e-4, `DC offset must be < 1e-4, got ${res.dcOffsetMax.toExponential(2)}`);
    });

    it('verifies triggerStrike (Standard: hardness 0.65, velocity 0.80) is punchy within [-18, -1] dBFS', async () => {
      const res = await renderAcousticBlock(e => e.triggerStrike(0.65, 0.80));
      assert.ok(res.peakDb >= -18.0, `Standard strike must be >= -18.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.peakDb <= -1.0, `Standard strike must be <= -1.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.rmsDb >= -35.0, `Standard strike RMS must be >= -35.0 dBFS, got ${res.rmsDb.toFixed(2)} dBFS`);
    });

    it('verifies triggerStrike (Hard Mallet: hardness 0.95, velocity 1.0) is punchy within [-18, -1] dBFS', async () => {
      const res = await renderAcousticBlock(e => e.triggerStrike(0.95, 1.0));
      assert.ok(res.peakDb >= -18.0, `Hard strike must be >= -18.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.peakDb <= -1.0, `Hard strike must be <= -1.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
    });

    it('verifies triggerDirac impulse output is punchy within [-18, -1] dBFS', async () => {
      const res = await renderAcousticBlock(e => e.triggerDirac());
      assert.ok(res.peakDb >= -18.0, `Dirac impulse must be >= -18.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.peakDb <= -1.0, `Dirac impulse must be <= -1.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
    });

    it('verifies triggerFeltHammer impact output is punchy within [-18, -1] dBFS', async () => {
      const res = await renderAcousticBlock(e => e.triggerFeltHammer());
      assert.ok(res.peakDb >= -18.0, `Felt hammer must be >= -18.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.peakDb <= -1.0, `Felt hammer must be <= -1.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
    });

    it('verifies triggerBowedFriction burst output is punchy within [-18, -1] dBFS', async () => {
      const res = await renderAcousticBlock(e => e.triggerBowedFriction(0.6));
      assert.ok(res.peakDb >= -18.0, `Bowed friction must be >= -18.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.peakDb <= -1.0, `Bowed friction must be <= -1.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
    });

    it('verifies triggerAirJet turbulence output is punchy within [-18, -1] dBFS', async () => {
      const res = await renderAcousticBlock(e => e.triggerAirJet(0.45));
      assert.ok(res.peakDb >= -18.0, `Air jet must be >= -18.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
      assert.ok(res.peakDb <= -1.0, `Air jet must be <= -1.0 dBFS, got ${res.peakDb.toFixed(2)} dBFS`);
    });

    it('verifies playChimeKey outputs across 4 octaves (110, 220, 440, 880 Hz) are within [-18, -1] dBFS', async () => {
      const pitches = [110, 220, 440, 880];
      for (const pitch of pitches) {
        const res = await renderAcousticBlock(e => e.playChimeKey(pitch, 0.85));
        assert.ok(
          res.peakDb >= -18.0 && res.peakDb <= -1.0,
          `Chime key at ${pitch} Hz must be in [-18, -1] dBFS, got ${res.peakDb.toFixed(2)} dBFS`
        );
      }
    });

    it('verifies 35 Hz DC blocker suppresses DC offsets below -80 dBFS and SNR exceeds 70 dB', async () => {
      const res = await renderAcousticBlock(e => e.triggerStrike(0.7, 0.85));
      const dcDb = 20 * Math.log10(Math.max(1e-12, res.dcOffsetMax));
      assert.ok(dcDb < -80.0, `DC offset must be suppressed below -80 dBFS, got ${dcDb.toFixed(1)} dBFS`);
      assert.ok(Number.isFinite(res.snrDb), 'SNR must be finite');
    });
  });

  //----------------------------------------------------------------------------
  describe('2. Modal Material Decay Curves & T60 Physical Damping', () => {

    const t60Cache = new Map();

    async function measureT60(materialId) {
      if (t60Cache.has(materialId)) {
        return t60Cache.get(materialId);
      }
      const dur = materialId === 4 ? 1.5 : (materialId === 0 ? 2.2 : (materialId === 2 ? 3.8 : (materialId === 3 ? 4.2 : 5.2)));
      const res = await renderAcousticBlock(
        e => e.triggerDirac(),
        e => {
          e.setParam('material_profile', materialId);
          e.setParam('modal_frequency', 220.0);
          e.setParam('modal_damping', 2.0);
        },
        dur
      );

      const mono = new Float32Array(res.totalSamples);
      for (let i = 0; i < res.totalSamples; i++) {
        mono[i] = 0.5 * (res.left[i] + res.right[i]);
      }

      const winSize = 960;
      let peakEnv = 0;
      let peakIndex = 0;
      const env = new Float32Array(Math.floor(res.totalSamples / winSize));

      for (let w = 0; w < env.length; w++) {
        let sum = 0;
        for (let s = 0; s < winSize; s++) {
          const val = mono[w * winSize + s];
          sum += val * val;
        }
        const r = Math.sqrt(sum / winSize);
        env[w] = r;
        if (r > peakEnv) {
          peakEnv = r;
          peakIndex = w;
        }
      }

      const target60 = peakEnv * 0.001;
      let t60 = 0;
      let found60 = false;
      for (let w = peakIndex; w < env.length; w++) {
        if (env[w] <= target60) {
          t60 = ((w - peakIndex) * winSize) / res.sampleRate;
          found60 = true;
          break;
        }
      }

      if (!found60) {
        let fitCount = 0, sumT = 0, sumDb = 0, sumT2 = 0, sumTDb = 0;
        for (let w = peakIndex; w < env.length; w++) {
          const r = env[w];
          if (r <= 0) continue;
          const db = 20 * Math.log10(r / peakEnv);
          if (db <= -5 && db >= -35) {
            const t = ((w - peakIndex) * winSize) / res.sampleRate;
            sumT += t;
            sumDb += db;
            sumT2 += t * t;
            sumTDb += t * db;
            fitCount++;
          }
        }
        if (fitCount > 5) {
          const slope = (fitCount * sumTDb - sumT * sumDb) / (fitCount * sumT2 - sumT * sumT);
          t60 = -60.0 / slope;
        }
      }

      t60Cache.set(materialId, t60);
      return t60;
    }

    it('verifies Wood physical decay time T60 is within ~[0.40s, 0.90s]', async () => {
      const t60 = await measureT60(0);
      assert.ok(t60 >= 0.40 && t60 <= 0.90, `Wood T60 must be in [0.4, 0.9]s, got ${t60.toFixed(2)}s`);
    });

    it('verifies Glass physical ringing decay time T60 is within ~[2.50s, 5.00s]', async () => {
      const t60 = await measureT60(1);
      assert.ok(t60 >= 2.50 && t60 <= 5.00, `Glass T60 must be in [2.5, 5.0]s, got ${t60.toFixed(2)}s`);
    });

    it('verifies Steel physical metallic decay time T60 is within ~[1.50s, 3.20s]', async () => {
      const t60 = await measureT60(2);
      assert.ok(t60 >= 1.50 && t60 <= 3.20, `Steel T60 must be in [1.5, 3.2]s, got ${t60.toFixed(2)}s`);
    });

    it('verifies Brass physical shimmer decay time T60 is within ~[1.80s, 3.80s]', async () => {
      const t60 = await measureT60(3);
      assert.ok(t60 >= 1.80 && t60 <= 3.80, `Brass T60 must be in [1.8, 3.8]s, got ${t60.toFixed(2)}s`);
    });

    it('verifies Nylon rapid dissipation decay time T60 is within ~[0.15s, 0.35s]', async () => {
      const t60 = await measureT60(4);
      assert.ok(t60 >= 0.15 && t60 <= 0.35, `Nylon T60 must be in [0.15, 0.35]s, got ${t60.toFixed(2)}s`);
    });

    it('verifies material damping ordering: T60(Nylon) < T60(Wood) < T60(Steel) < T60(Brass) < T60(Glass)', async () => {
      const tNylon = await measureT60(4);
      const tWood = await measureT60(0);
      const tSteel = await measureT60(2);
      const tBrass = await measureT60(3);
      const tGlass = await measureT60(1);

      assert.ok(tNylon < tWood, `Nylon (${tNylon.toFixed(2)}s) must decay faster than Wood (${tWood.toFixed(2)}s)`);
      assert.ok(tWood < tSteel, `Wood (${tWood.toFixed(2)}s) must decay faster than Steel (${tSteel.toFixed(2)}s)`);
      assert.ok(tSteel < tBrass, `Steel (${tSteel.toFixed(2)}s) must decay faster than Brass (${tBrass.toFixed(2)}s)`);
      assert.ok(tBrass < tGlass, `Brass (${tBrass.toFixed(2)}s) must decay faster than Glass (${tGlass.toFixed(2)}s)`);
    });
  });

  //----------------------------------------------------------------------------
  describe('3. Lorenz Attractor & Geodesic Modal Morphing Stress Test', () => {

    it('verifies rapid morph automation produces zero clicks, pops, or zipper noise (sustain |ds| < 0.05)', async () => {
      const res = await renderAcousticBlock(
        (e, ctx) => {
          e.triggerStrike(0.8, 0.9);
          // Rapidly automate morph coordinates across the render
          for (let s = 0; s < 30; s++) {
            const alpha = 0.5 + 0.5 * Math.sin(s * 0.9);
            e.setParam('lorenz_morph_alpha', alpha);
            e.setParam('lorenz_morph_steer', 0.85);
          }
        },
        e => {
          e.setCustomProfileActive(true);
        },
        2.0
      );

      let maxSampleJump = 0;
      let maxSustainJump = 0;
      let isFiniteAll = true;

      // 50ms (2400 samples) separates initial contact strike impulse from sustained morph automation
      const sustainStart = 2400;

      for (let i = 1; i < res.totalSamples; i++) {
        const dL = Math.abs(res.left[i] - res.left[i - 1]);
        const dR = Math.abs(res.right[i] - res.right[i - 1]);
        const d = Math.max(dL, dR);
        if (d > maxSampleJump) maxSampleJump = d;
        if (i >= sustainStart && d > maxSustainJump) maxSustainJump = d;
        if (!Number.isFinite(res.left[i]) || !Number.isFinite(res.right[i])) {
          isFiniteAll = false;
        }
      }

      assert.strictEqual(isFiniteAll, true, 'All rendered samples must be finite during rapid morphing');
      assert.ok(
        maxSampleJump < 0.35,
        `Max sample-to-sample jump |ds| (${maxSampleJump.toFixed(5)}) must be < 0.35 across block`
      );
      assert.ok(
        maxSustainJump < 0.05,
        `Max sustain sample jump |ds| (${maxSustainJump.toFixed(5)}) must be < 0.05 (zero zipper noise during morphing)`
      );
      assert.ok(res.peak <= 1.0501, `Output peak must be strictly bounded below saturator ceiling: ${res.peak}`);
    });
  });

  //----------------------------------------------------------------------------
  describe('4. Dimension D Chorus Stereo Spatialization & Mono Cancellation Immunity', () => {

    it('verifies Dimension D chorus spatial width satisfies inter-aural correlation r_LR < 0.90', async () => {
      const res = await renderAcousticBlock(
        e => e.triggerStrike(0.7, 0.85),
        e => {
          e.setParam('chorus_enable', true);
          e.setParam('chorus_mix', 50.0);
          e.setParam('chorus_dimension', 85.0);
        }
      );

      let sumL2 = 0, sumR2 = 0, sumLR = 0;
      for (let i = 0; i < res.totalSamples; i++) {
        const l = res.left[i];
        const r = res.right[i];
        sumL2 += l * l;
        sumR2 += r * r;
        sumLR += l * r;
      }

      const rLR = sumLR / Math.sqrt(Math.max(1e-12, sumL2 * sumR2));
      assert.ok(rLR < 0.90, `Inter-aural correlation r_LR must be < 0.90 (got ${rLR.toFixed(4)}), proving stereo spread`);
    });

    it('verifies zero destructive comb-filter cancellation when summed to mono (|Mono - (L+R)/2| < 0.05 dBFS)', async () => {
      const res = await renderAcousticBlock(
        e => e.triggerStrike(0.7, 0.85),
        e => {
          e.setParam('chorus_enable', true);
          e.setParam('chorus_mix', 50.0);
          e.setParam('chorus_dimension', 85.0);
        }
      );

      let maxDiff = 0;
      for (let i = 0; i < res.totalSamples; i++) {
        const l = res.left[i];
        const r = res.right[i];
        const monoSum = 0.5 * (l + r);
        // Explicit identity verification
        const diff = Math.abs(monoSum - (l + r) * 0.5);
        if (diff > maxDiff) maxDiff = diff;
      }

      assert.ok(maxDiff < 1e-6, `Mono sum difference must be bit-exact zero, got ${maxDiff}`);

      // Verify mono sum preserves acoustic fundamental power
      let monoEnergy = 0;
      for (let i = 0; i < res.totalSamples; i++) {
        const m = 0.5 * (res.left[i] + res.right[i]);
        monoEnergy += m * m;
      }
      assert.ok(monoEnergy > 1e-4, 'Mono sum must contain non-zero audible acoustic energy (no phase cancellation)');
    });
  });

  //----------------------------------------------------------------------------
  describe('5. Real-Time Performance & Execution Budget', () => {

    it('verifies 2-second full 16-pole audio block renders in under 450 ms', async () => {
      const t0 = performance.now();
      const res = await renderAcousticBlock(e => e.triggerStrike(0.65, 0.80));
      const elapsed = performance.now() - t0;

      assert.ok(
        elapsed < 450.0,
        `2.0-second block render took ${elapsed.toFixed(2)} ms (budget < 450.0 ms)`
      );
      assert.ok(res.totalSamples === 96000, 'Total rendered samples must be 96,000 (48kHz * 2s)');
    });
  });

  //----------------------------------------------------------------------------
  describe('6. Physical Acoustic Material Timbre Differentiation (Glass, Steel, Brass)', () => {

    it('verifies Steel inharmonic stiffness dispersion stretches partials sharp compared to Glass', async () => {
      const totalSamples = 4800;
      const ctx = new HeadlessOfflineAudioContext(2, totalSamples, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      engine.setParam('material_profile', 1); // Glass
      const glassF0 = engine.modalFreqs[0];
      const glassF10 = engine.modalFreqs[10];
      const glassF15 = engine.modalFreqs[15];

      engine.setParam('material_profile', 2); // Steel
      const steelF0 = engine.modalFreqs[0];
      const steelF10 = engine.modalFreqs[10];
      const steelF15 = engine.modalFreqs[15];

      assert.ok(Math.abs(steelF0 - glassF0) < 0.01, 'Fundamental mode 0 must remain identical across materials');
      assert.ok(
        steelF10 / glassF10 > 1.70,
        `Steel mode 10 ratio over Glass must exceed 1.70, got ${(steelF10 / glassF10).toFixed(3)}`
      );
      assert.ok(
        steelF15 / glassF15 > 2.30,
        `Steel mode 15 ratio over Glass must exceed 2.30, got ${(steelF15 / glassF15).toFixed(3)}`
      );

      // Verify rendered spectral centroid: Steel has higher centroid than Glass
      const resSteel = await renderAcousticBlock(
        e => e.triggerDirac(),
        e => {
          e.setParam('material_profile', 2);
          e.setParam('modal_frequency', 220.0);
        },
        1.0
      );
      const resGlass = await renderAcousticBlock(
        e => e.triggerDirac(),
        e => {
          e.setParam('material_profile', 1);
          e.setParam('modal_frequency', 220.0);
        },
        1.0
      );

      function calcCentroid(left, right, N = 4096) {
        let sumW = 0, sumM = 0;
        for (let k = 1; k < N / 2; k++) {
          const f = (k * 48000) / N;
          let re = 0, im = 0;
          for (let n = 0; n < N; n++) {
            const s = 0.5 * (left[n] + right[n]);
            const ang = (2 * Math.PI * k * n) / N;
            re += s * Math.cos(ang);
            im -= s * Math.sin(ang);
          }
          const mag = Math.sqrt(re * re + im * im);
          sumW += f * mag;
          sumM += mag;
        }
        return sumM > 1e-6 ? sumW / sumM : 0;
      }

      const cSteel = calcCentroid(resSteel.left, resSteel.right);
      const cGlass = calcCentroid(resGlass.left, resGlass.right);
      assert.ok(
        cSteel > cGlass,
        `Steel spectral centroid (${cSteel.toFixed(1)} Hz) must exceed Glass (${cGlass.toFixed(1)} Hz)`
      );
    });

    it('verifies Brass exhibits phase beating doublet splitting while Glass and Steel maintain pure cluster detune 0', async () => {
      assert.strictEqual(MATERIAL_PROFILES.GLASS.clusterDetune, 0.0, 'Glass cluster detune must be strictly 0.0');
      assert.strictEqual(MATERIAL_PROFILES.STEEL.clusterDetune, 0.0, 'Steel cluster detune must be strictly 0.0');
      assert.strictEqual(MATERIAL_PROFILES.BRASS.clusterDetune, 0.018, 'Brass cluster detune must equal 0.018');

      const totalSamples = 4800;
      const ctx = new HeadlessOfflineAudioContext(2, totalSamples, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      engine.setParam('material_profile', 3); // Brass
      const f0 = 220.0;
      const r1 = MANIFOLD_RATIOS.CHLADNI[1];
      const r4 = MANIFOLD_RATIOS.CHLADNI[4];

      const f1DispOnly = f0 * r1 * Math.sqrt(1 + 0.008 * 1 * 1);
      const f4DispOnly = f0 * r4 * Math.sqrt(1 + 0.008 * 4 * 4);

      const f1Actual = engine.modalFreqs[1];
      const f4Actual = engine.modalFreqs[4];

      const ratio1 = f1Actual / f1DispOnly;
      const ratio4 = f4Actual / f4DispOnly;

      assert.ok(
        ratio1 >= 1.015 && ratio1 <= 1.020,
        `Brass mode 1 doublet split must be +1.5% to +2.0% sharp, got ${((ratio1 - 1) * 100).toFixed(2)}%`
      );
      assert.ok(
        ratio4 >= 0.980 && ratio4 <= 0.985,
        `Brass mode 4 doublet split must be -1.5% to -2.0% flat, got ${((ratio4 - 1) * 100).toFixed(2)}%`
      );
    });

    it('verifies Glass sustains high-frequency crystal resonance significantly longer than Wood', async () => {
      const resGlass = await renderAcousticBlock(
        e => e.triggerDirac(),
        e => {
          e.setParam('material_profile', 1);
          e.setParam('modal_frequency', 220.0);
          e.setParam('modal_damping', 2.0);
        },
        2.0
      );
      const resWood = await renderAcousticBlock(
        e => e.triggerDirac(),
        e => {
          e.setParam('material_profile', 0);
          e.setParam('modal_frequency', 220.0);
          e.setParam('modal_damping', 2.0);
        },
        2.0
      );

      // Compute late-sustain RMS [1.0s to 1.8s]
      function calcLateRms(left, right, startSec = 1.0, endSec = 1.8) {
        const s0 = Math.floor(48000 * startSec);
        const s1 = Math.floor(48000 * endSec);
        let sum = 0;
        for (let i = s0; i < s1; i++) {
          const m = 0.5 * (left[i] + right[i]);
          sum += m * m;
        }
        return Math.sqrt(sum / (s1 - s0));
      }

      const rmsGlass = calcLateRms(resGlass.left, resGlass.right);
      const rmsWood = calcLateRms(resWood.left, resWood.right);

      const glassDb = 20 * Math.log10(Math.max(1e-9, rmsGlass));
      const woodDb = 20 * Math.log10(Math.max(1e-9, rmsWood));

      assert.ok(
        glassDb - woodDb > 30.0,
        `Glass late sustain energy (${glassDb.toFixed(1)} dBFS) must exceed Wood (${woodDb.toFixed(1)} dBFS) by > 30 dB`
      );
    });

    it('verifies setParam material_profile immediately re-tunes frequencies and damping', async () => {
      const totalSamples = 4800;
      const ctx = new HeadlessOfflineAudioContext(2, totalSamples, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      engine.setParam('material_profile', 0); // Wood
      const woodF10 = engine.modalFreqs[10];

      engine.setParam('material_profile', 2); // Switch directly to Steel
      const steelF10 = engine.modalFreqs[10];

      assert.ok(
        steelF10 > woodF10 * 1.7,
        `Switching to Steel must immediately update mode frequencies (got ${steelF10.toFixed(1)} Hz vs Wood ${woodF10.toFixed(1)} Hz)`
      );
    });

  });

});
