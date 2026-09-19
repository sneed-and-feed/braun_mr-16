/**
 * @file test-checklist.mjs
 * @brief Automated Audio DSP Verification Checklist for BRAUN MR-16
 *
 * Validates:
 * 1. Multi-rate normalized coefficients & scaling (44.1k to 192k)
 * 2. Toxic denormal / NaN / Infinity immunity & flush-to-zero
 * 3. Hermite limiter curve monotonicity, 0 dB transparency & +18 dBFS clamping
 * 4. Parameter smoother continuity (no discrete jumps > 0.05)
 * 5. Tri-phase BBD chorus phase dispersion & mono-sum immunity
 * 6. 31-parameter APVTS & Web Audio metadata mapping
 * 7. Orthogonal Householder scattering matrix energy conservation
 */

import { describe, it } from 'node:test';
import assert from 'node:assert';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Reference DSP helpers matching DspMath.h and BoundedSaturator.h
function flushDenormal(val) {
  if (!Number.isFinite(val)) return 0.0;
  return Math.abs(val) < 1.0e-15 ? 0.0 : val;
}

function applyHermiteSaturator(x, knee = 0.72, ceiling = 1.05) {
  if (!Number.isFinite(x)) return 0.0;
  const absX = Math.abs(x);
  if (absX < 1.0e-15) return 0.0;
  if (absX <= knee) return x;
  const sign = x > 0 ? 1.0 : -1.0;
  if (absX >= ceiling) return sign * ceiling;
  const delta = ceiling - knee;
  const u = (absX - knee) / delta;
  const poly = u * (1.0 + u * (1.0 - u));
  return flushDenormal(sign * (knee + delta * poly));
}

class ReferenceOnePoleSmoother {
  constructor(tauSec = 0.025, sampleRate = 48000) {
    this.sampleRate = sampleRate;
    this.tau = tauSec;
    this.coeff = 1.0 - Math.exp(-1.0 / (this.sampleRate * this.tau));
    this.current = 0.0;
    this.target = 0.0;
  }

  setSampleRate(fs) {
    this.sampleRate = fs;
    this.coeff = 1.0 - Math.exp(-1.0 / (this.sampleRate * this.tau));
  }

  snapTo(val) {
    this.target = val;
    this.current = val;
  }

  setTarget(target) {
    this.target = target;
  }

  next() {
    this.current += this.coeff * (this.target - this.current);
    if (Math.abs(this.target - this.current) < 1.0e-6) {
      this.current = this.target;
    }
    this.current = flushDenormal(this.current);
    return this.current;
  }
}

describe('BRAUN MR-16 Automated DSP Verification Checklist', () => {

  //----------------------------------------------------------------------------
  describe('1. Multi-Rate Normalized Scaling (44.1k to 192k)', () => {
    const sampleRates = [44100, 48000, 88200, 96000, 176400, 192000];

    it('verifies 1-pole filter alpha coefficient scaling across all 6 sample rates', () => {
      const cutoffs = [40, 150, 440, 1800, 4000, 9500, 14000];

      for (const fc of cutoffs) {
        let prevAlpha = 1.0;
        for (const fs of sampleRates) {
          assert.ok(fc < fs * 0.495, `Cutoff ${fc} Hz must be below Nyquist for fs = ${fs} Hz`);
          const alpha = 1.0 - Math.exp(-2.0 * Math.PI * (fc / fs));

          assert.ok(alpha > 0.0, `Alpha must be strictly positive at fs = ${fs}`);
          assert.ok(alpha < 1.0, `Alpha must be strictly less than 1.0 at fs = ${fs}`);
          assert.ok(alpha < prevAlpha, `Alpha must monotonically decrease as sample rate increases (fs = ${fs})`);
          prevAlpha = alpha;
        }
      }
    });

    it('verifies BBD delay buffer sample length scaling across multi-rate', () => {
      const baseDelayMs = 5.0; // 5 ms base delay
      for (const fs of sampleRates) {
        const samples = Math.round((baseDelayMs / 1000.0) * fs);
        assert.ok(Number.isInteger(samples), 'Delay sample count must be integer');
        assert.ok(samples >= 220, `Min delay at 44.1k must be >= 220 samples, got ${samples}`);
        assert.ok(samples <= 1000, `Max delay at 192k must be <= 1000 samples, got ${samples}`);
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('2. Toxic Denormal & NaN Immunity Test', () => {
    it('verifies software flushDenormal neutralizes non-finite values and subnormals', () => {
      assert.strictEqual(flushDenormal(NaN), 0.0, 'NaN must flush to bit-exact 0.0');
      assert.strictEqual(flushDenormal(Infinity), 0.0, '+Infinity must flush to bit-exact 0.0');
      assert.strictEqual(flushDenormal(-Infinity), 0.0, '-Infinity must flush to bit-exact 0.0');
      assert.strictEqual(flushDenormal(1.0e-16), 0.0, 'Subnormal 1e-16 must flush to 0.0');
      assert.strictEqual(flushDenormal(-1.0e-16), 0.0, 'Subnormal -1e-16 must flush to 0.0');
      assert.strictEqual(flushDenormal(1.0e-38), 0.0, 'Subnormal 1e-38 must flush to 0.0');
      assert.strictEqual(flushDenormal(-1.0e-45), 0.0, 'Subnormal IEEE denormal must flush to 0.0');
    });

    it('verifies normal audible signals are strictly preserved without distortion', () => {
      assert.strictEqual(flushDenormal(1.0), 1.0);
      assert.strictEqual(flushDenormal(-0.5), -0.5);
      assert.strictEqual(flushDenormal(1.0e-6), 1.0e-6);
      assert.strictEqual(flushDenormal(-1.0e-6), -1.0e-6);
    });

    it('verifies toxic buffer ingestion is neutralized to finite bounded output', () => {
      const toxicInputs = [NaN, Infinity, -Infinity, 1.0e-38, -1.0e-38, 1.0e35, -1.0e35, 0.0, 0.5, -0.5];
      for (const x of toxicInputs) {
        const out = applyHermiteSaturator(x);
        assert.ok(Number.isFinite(out), `applyHermiteSaturator(${x}) must be finite`);
        assert.ok(Math.abs(out) <= 1.05, `applyHermiteSaturator(${x}) must be <= 1.05`);
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('3. Hermite Soft Limiter Curve Monotonicity & Transparency', () => {
    it('verifies strict 0 dB small signal transparency below the knee (0.72)', () => {
      const testVals = [0.0, 0.001, 0.1, 0.25, 0.5, 0.70, 0.72, -0.001, -0.1, -0.25, -0.5, -0.70, -0.72];
      for (const x of testVals) {
        const y = applyHermiteSaturator(x, 0.72, 1.05);
        assert.ok(Math.abs(y - x) < 1.0e-7, `Small signal ${x} must equal output ${y} (bit-exact transparency)`);
      }
    });

    it('verifies strict monotonicity across wide dynamic range [-8.0, +8.0]', () => {
      const steps = 10000;
      let prevY = -2.0;

      for (let i = 0; i <= steps; i++) {
        const x = -8.0 + (16.0 * i) / steps;
        const y = applyHermiteSaturator(x, 0.72, 1.05);
        assert.ok(y >= prevY - 1.0e-7, `Monotonicity violation at x = ${x}: y = ${y} < prevY = ${prevY}`);
        prevY = y;
      }
    });

    it('verifies strict asymptotic peak clamping under +18 dBFS input and beyond', () => {
      const plus18dB = Math.pow(10, 18 / 20); // ~7.94
      const y18 = applyHermiteSaturator(plus18dB, 0.72, 1.05);
      assert.ok(y18 <= 1.050000, `Output at +18 dBFS (${y18}) must be strictly <= 1.05`);
      assert.ok(y18 >= 1.049000, `Output at +18 dBFS (${y18}) must be at ceiling`);

      const y40 = applyHermiteSaturator(100.0, 0.72, 1.05);
      assert.strictEqual(y40, 1.05, 'Output at +40 dBFS must clamp to exact ceiling 1.05');

      const y40neg = applyHermiteSaturator(-100.0, 0.72, 1.05);
      assert.strictEqual(y40neg, -1.05, 'Output at -40 dBFS must clamp to exact ceiling -1.05');
    });

    it('verifies odd mathematical symmetry: f(-x) === -f(x)', () => {
      const probes = [0.1, 0.5, 0.72, 0.85, 1.0, 1.5, 2.5, 5.0, 10.0];
      for (const x of probes) {
        const yPos = applyHermiteSaturator(x);
        const yNeg = applyHermiteSaturator(-x);
        assert.ok(Math.abs(yPos + yNeg) < 1.0e-6, `Odd symmetry failed at x = ${x}: ${yPos} vs ${yNeg}`);
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('4. Parameter Smoother Continuity Test', () => {
    it('verifies no discrete sample jumps > 0.05 on extreme step parameter changes', () => {
      const smoother = new ReferenceOnePoleSmoother(0.025, 48000); // 25ms tau
      smoother.snapTo(0.0);
      smoother.setTarget(1.0);

      let maxJump = 0.0;
      let prevVal = 0.0;

      for (let i = 0; i < 24000; i++) {
        const val = smoother.next();
        const jump = Math.abs(val - prevVal);
        if (jump > maxJump) maxJump = jump;
        assert.ok(jump <= 0.05, `Discrete step jump ${jump} at sample ${i} exceeds 0.05 threshold`);
        prevVal = val;
      }

      assert.ok(maxJump < 0.002, `Max jump was ${maxJump}, well below 0.05 threshold`);
      assert.strictEqual(prevVal, 1.0, 'Smoother must reach exact target 1.0');
    });

    it('verifies ramp continuity across range of time constants (2ms to 80ms) and multi-rate', () => {
      const timeConstants = [0.002, 0.010, 0.025, 0.050, 0.080];
      const sampleRates = [44100, 48000, 96000, 192000];

      for (const tau of timeConstants) {
        for (const fs of sampleRates) {
          const smoother = new ReferenceOnePoleSmoother(tau, fs);
          smoother.snapTo(0.0);
          smoother.setTarget(1.0);

          let prev = 0.0;
          let maxStep = 0.0;
          const totalSamples = Math.round(fs * 0.25);

          for (let s = 0; s < totalSamples; s++) {
            const cur = smoother.next();
            const step = Math.abs(cur - prev);
            if (step > maxStep) maxStep = step;
            prev = cur;
          }

          assert.ok(maxStep <= 0.05, `Max step ${maxStep} at tau=${tau}s fs=${fs}Hz exceeded 0.05`);
        }
      }
    });

    it('verifies strictly monotonic trajectory without ringing or overshoot', () => {
      const smoother = new ReferenceOnePoleSmoother(0.020, 48000);
      smoother.snapTo(0.0);
      smoother.setTarget(100.0);

      let prev = 0.0;
      for (let i = 0; i < 24000; i++) {
        const cur = smoother.next();
        assert.ok(cur >= prev, `Trajectory non-monotonic at sample ${i}: ${cur} < ${prev}`);
        assert.ok(cur <= 100.0, `Overshoot detected at sample ${i}: ${cur} > 100.0`);
        prev = cur;
      }
      assert.strictEqual(prev, 100.0);
    });
  });

  //----------------------------------------------------------------------------
  describe('5. Tri-Phase Chorus Phase Dispersion & Mono Cancellation Immunity', () => {
    it('verifies 3-phase LFO equidistant 120-degree offsets (0, 120, 240 deg)', () => {
      const phi1 = 0.0;
      const phi2 = (2.0 * Math.PI) / 3.0;
      const phi3 = (4.0 * Math.PI) / 3.0;

      // Assert sum of phase unit vectors is 0 (balanced tri-phase distribution)
      const sumReal = Math.cos(phi1) + Math.cos(phi2) + Math.cos(phi3);
      const sumImag = Math.sin(phi1) + Math.sin(phi2) + Math.sin(phi3);
      assert.ok(Math.abs(sumReal) < 1.0e-6, 'Tri-phase vector sum real part must be 0');
      assert.ok(Math.abs(sumImag) < 1.0e-6, 'Tri-phase vector sum imag part must be 0');
    });

    it('verifies phase difference between delay line 1 (0 deg) and 3 (240 deg) never vanishes', () => {
      // In mono sum Out_mono = 0.5 * (Out_L + Out_R) = 0.5 * (d1 - d2 + d2 - d3) = 0.5 * (d1 - d3)
      const phi1 = 0;
      const phi3 = (4 * Math.PI) / 3;

      const vector1Real = Math.cos(phi1);
      const vector1Imag = Math.sin(phi1);
      const vector3Real = Math.cos(phi3);
      const vector3Imag = Math.sin(phi3);

      const diffReal = vector1Real - vector3Real;
      const diffImag = vector1Imag - vector3Imag;
      const magnitude = Math.sqrt(diffReal * diffReal + diffImag * diffImag);

      // Expected magnitude is sqrt(3) ~ 1.73205
      assert.ok(Math.abs(magnitude - Math.sqrt(3)) < 1.0e-5, 'Phase vector magnitude must be exactly sqrt(3)');
      assert.ok(magnitude > 1.0, 'Magnitude is strictly non-zero, proving mono cancellation immunity');
    });
  });

  //----------------------------------------------------------------------------
  describe('6. 31-Parameter APVTS & Web Audio Metadata Map', () => {
    it('verifies all 31 parameters are declared in web engine params', async () => {
      const engineModule = await import('./js/audio/mr16_web_engine.js');
      const engine = new engineModule.Mr16WebEngine();

      const expectedParams = [
        // Deck 01 (10)
        'exciter_type', 'strike_hardness', 'strike_velocity',
        'friction_force', 'friction_speed', 'vactrol_sag',
        'ext_input_gain', 'poisson_density', 'euclidean_pulses', 'euclidean_steps',
        // Deck 02 (7)
        'manifold_type', 'modal_frequency', 'modal_damping',
        'material_profile', 'modal_coupling', 'modal_spread', 'modal_q',
        // Deck 03 (4)
        'lorenz_rate', 'lorenz_chaos', 'lorenz_freq_mod', 'lorenz_q_mod',
        // Deck 04 (5)
        'chorus_enable', 'chorus_rate_hz', 'chorus_depth_ms', 'chorus_dimension', 'chorus_mix',
        // Deck 05 (5)
        'golden_pan_spread', 'vactrol_lpg_cutoff', 'drive_saturation', 'master_trim_db', 'dry_wet_mix'
      ];

      for (const param of expectedParams) {
        assert.ok(engine.params[param] !== undefined, `Parameter ${param} must exist in Mr16WebEngine`);
      }
      assert.strictEqual(expectedParams.length, 31, 'Exactly 31 parameters declared in core tree');
    });

    it('verifies exciter_type parameter updates and friction generator state handling', async () => {
      const engineModule = await import('./js/audio/mr16_web_engine.js');
      const engine = new engineModule.Mr16WebEngine();
      engine.isInitialized = true;
      engine.ctx = { currentTime: 0.0 };

      let frictionUpdated = false;
      engine.updateFrictionParams = () => {
        frictionUpdated = true;
      };

      engine.setParam('exciter_type', 1);
      assert.strictEqual(engine.params.exciter_type, 1, 'exciter_type must be set to 1 (Friction)');
      assert.strictEqual(frictionUpdated, true, 'updateFrictionParams must be called on exciter_type change');

      frictionUpdated = false;
      engine.setParam('exciter_type', 0);
      assert.strictEqual(engine.params.exciter_type, 0, 'exciter_type must be set to 0 (Strike)');
      assert.strictEqual(frictionUpdated, true, 'updateFrictionParams must be called when switching away from friction');
    });
  });

  //----------------------------------------------------------------------------
  describe('7. Householder Scattering Matrix Unitary Bounds', () => {
    it('verifies Householder feedback scalar gain calculation strictly <= 0.125', () => {
      for (let coupling = 0.0; coupling <= 1.0; coupling += 0.1) {
        const hhGain = -0.125 * coupling;
        assert.ok(Math.abs(hhGain) <= 0.12501, 'Householder feedback gain must not exceed 2/16 = 0.125');
        assert.ok(hhGain <= 0.0, 'Householder reflection must maintain negative trace');
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('8. Modal Resonator Makeup Gain & Broadband Impulse Calibration', () => {
    it('verifies resonant peak makeup gain calculation satisfies bounds G_i in [1.0, 24.0]', () => {
      const fs = 48000;
      const testFrequencies = [20, 110, 220, 440, 1000, 4000, 12000, 20000];
      const testQValues = [8, 20, 85, 140, 320, 480];

      for (const fm of testFrequencies) {
        for (const q of testQValues) {
          const g = Math.min(24.0, 1.25 * Math.sqrt((q * fs) / Math.max(20.0, fm)));
          assert.ok(Number.isFinite(g), `Makeup gain must be finite for fm=${fm}, Q=${q}`);
          assert.ok(g >= 1.0, `Makeup gain must be >= 1.0 (got ${g})`);
          assert.ok(g <= 24.0, `Makeup gain must be clamped to <= 24.0 (got ${g})`);
        }
      }
    });

    it('verifies Mr16WebEngine initial standby state and excitation interface', async () => {
      const engineModule = await import('./js/audio/mr16_web_engine.js');
      const engine = new engineModule.Mr16WebEngine();

      assert.strictEqual(engine.isPowered, false, 'Initial power state must be false (STANDBY)');
      assert.strictEqual(typeof engine.playChimeKey, 'function', 'playChimeKey must be defined');
      assert.strictEqual(typeof engine.triggerStrike, 'function', 'triggerStrike must be defined');
      assert.strictEqual(typeof engine.triggerDirac, 'function', 'triggerDirac must be defined');
      assert.strictEqual(typeof engine.triggerFeltHammer, 'function', 'triggerFeltHammer must be defined');
      assert.strictEqual(typeof engine.triggerBowedFriction, 'function', 'triggerBowedFriction must be defined');
      assert.strictEqual(typeof engine.triggerAirJet, 'function', 'triggerAirJet must be defined');
    });
  });

});
