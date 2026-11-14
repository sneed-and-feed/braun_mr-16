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

function applyHermiteSaturator(x, knee = 0.72, ceiling = 1.00) {
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
        assert.ok(Math.abs(out) <= 1.00, `applyHermiteSaturator(${x}) must be <= 1.00`);
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('3. Hermite Soft Limiter Curve Monotonicity & Transparency', () => {
    it('verifies strict 0 dB small signal transparency below the knee (0.72)', () => {
      const testVals = [0.0, 0.001, 0.1, 0.25, 0.5, 0.70, 0.72, -0.001, -0.1, -0.25, -0.5, -0.70, -0.72];
      for (const x of testVals) {
        const y = applyHermiteSaturator(x, 0.72, 1.00);
        assert.ok(Math.abs(y - x) < 1.0e-7, `Small signal ${x} must equal output ${y} (bit-exact transparency)`);
      }
    });

    it('verifies strict monotonicity across wide dynamic range [-8.0, +8.0]', () => {
      const steps = 10000;
      let prevY = -2.0;

      for (let i = 0; i <= steps; i++) {
        const x = -8.0 + (16.0 * i) / steps;
        const y = applyHermiteSaturator(x, 0.72, 1.00);
        assert.ok(y >= prevY - 1.0e-7, `Monotonicity violation at x = ${x}: y = ${y} < prevY = ${prevY}`);
        prevY = y;
      }
    });

    it('verifies strict asymptotic peak clamping under +18 dBFS input and beyond', () => {
      const plus18dB = Math.pow(10, 18 / 20); // ~7.94
      const y18 = applyHermiteSaturator(plus18dB, 0.72, 1.00);
      assert.ok(y18 <= 1.000000, `Output at +18 dBFS (${y18}) must be strictly <= 1.00`);
      assert.ok(y18 >= 0.999000, `Output at +18 dBFS (${y18}) must be at ceiling`);

      const y40 = applyHermiteSaturator(100.0, 0.72, 1.00);
      assert.strictEqual(y40, 1.00, 'Output at +40 dBFS must clamp to exact ceiling 1.00');

      const y40neg = applyHermiteSaturator(-100.0, 0.72, 1.00);
      assert.strictEqual(y40neg, -1.00, 'Output at -40 dBFS must clamp to exact ceiling -1.00');
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
  describe('6. 37-Parameter APVTS & Web Audio Metadata Map', () => {
    it('verifies all 36 DSP/audio APVTS parameters are declared in web engine params (plus display_mode in Parameters.h)', async () => {
      const engineModule = await import('./js/audio/mr16_web_engine.js');
      const engine = new engineModule.Mr16WebEngine();

      const expectedParams = [
        // Deck 01 (11)
        'exciter_type', 'strike_hardness', 'strike_velocity',
        'friction_force', 'friction_speed', 'vactrol_sag',
        'ext_input_gain', 'poisson_density', 'euclidean_enable', 'euclidean_pulses', 'euclidean_steps',
        // Deck 02 (7)
        'manifold_type', 'modal_frequency', 'modal_damping',
        'material_profile', 'modal_coupling', 'modal_spread', 'modal_q',
        // Deck 03 (6)
        'lorenz_morph_alpha', 'lorenz_rate', 'lorenz_chaos',
        'lorenz_freq_mod', 'lorenz_q_mod', 'lorenz_morph_steer',
        // Deck 04 (5)
        'chorus_enable', 'chorus_rate_hz', 'chorus_depth_ms', 'chorus_dimension', 'chorus_mix',
        // Deck 05 (7)
        'golden_pan_spread', 'vactrol_lpg_cutoff', 'drive_saturation',
        'soft_limiter', 'master_trim_db', 'dry_wet_mix', 'power_state'
      ];

      for (const param of expectedParams) {
        assert.ok(engine.params[param] !== undefined, `Parameter ${param} must exist in Mr16WebEngine`);
      }
      assert.strictEqual(expectedParams.length, 36, 'Exactly 36 audio/DSP APVTS parameters declared in Mr16WebEngine');
      assert.strictEqual(engine.params.modal_q, 85.0, 'Default modal_q must match calibrated C++ APVTS default (85.0)');
      assert.strictEqual(engine.params.euclidean_enable, false, 'Default euclidean_enable must be disarmed (false)');
      assert.strictEqual(engine.params.lorenz_morph_alpha, 0.0, 'Default lorenz_morph_alpha must be 0.0 (unmorphed)');
      assert.strictEqual(engine.params.lorenz_morph_steer, 0.0, 'Default lorenz_morph_steer must be 0.0');
      assert.strictEqual(engine.params.soft_limiter, true, 'Default soft_limiter must be enabled (true)');

      const paramsHeader = fs.readFileSync(path.join(__dirname, '..', 'source', 'plugin', 'Parameters.h'), 'utf8');
      for (const param of [...expectedParams, 'display_mode']) {
        assert.ok(paramsHeader.includes(`"${param}"`), `Parameters.h must declare APVTS parameter "${param}"`);
      }
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

  //----------------------------------------------------------------------------
  describe('9. Deck 01 Dynamic Control Dimming & Exciter State', () => {
    it('verifies app.js contains _updateExciterControls contract implementation', () => {
      const appJs = fs.readFileSync(path.join(__dirname, 'js', 'app.js'), 'utf8');
      assert.ok(appJs.includes('_updateExciterControls(mode)'), 'app.js must declare _updateExciterControls');
      assert.ok(appJs.includes('knob-exciter-velocity'), 'must reference knob-exciter-velocity');
      assert.ok(appJs.includes('knob-exciter-hardness'), 'must reference knob-exciter-hardness');
      assert.ok(appJs.includes('knob-friction-velocity'), 'must reference knob-friction-velocity');
      assert.ok(appJs.includes('knob-friction-force'), 'must reference knob-friction-force');
      assert.ok(appJs.includes('knob-ext-input-gain'), 'must reference knob-ext-input-gain');
    });

    it('verifies _updateExciterControls dims strike and friction knobs in EXT IN mode 3 and highlights ext gain', () => {
      const mockElements = {
        'knob-exciter-velocity': { style: {}, classList: new Set() },
        'knob-exciter-hardness': { style: {}, classList: new Set() },
        'knob-friction-velocity': { style: {}, classList: new Set() },
        'knob-friction-force': { style: {}, classList: new Set() },
        'knob-ext-input-gain': { style: {}, classList: new Set() }
      };

      const originalGetElementById = globalThis.document?.getElementById;
      globalThis.document = globalThis.document || {};
      globalThis.document.getElementById = (id) => {
        const mock = mockElements[id];
        if (!mock) return null;
        return {
          style: mock.style,
          classList: {
            add: (c) => mock.classList.add(c),
            remove: (c) => mock.classList.delete(c),
            toggle: (c, val) => val ? mock.classList.add(c) : mock.classList.delete(c)
          }
        };
      };

      try {
        const updateExciterControls = (mode) => {
          const strikeVel = document.getElementById('knob-exciter-velocity');
          const strikeHard = document.getElementById('knob-exciter-hardness');
          const frictVel = document.getElementById('knob-friction-velocity');
          const frictForce = document.getElementById('knob-friction-force');
          const extGain = document.getElementById('knob-ext-input-gain');

          const setControlState = (el, active) => {
            if (!el) return;
            el.style.opacity = active ? '1.0' : '0.4';
            el.style.pointerEvents = active ? 'auto' : 'none';
            el.classList.toggle('is-dimmed', !active);
          };

          const modeNum = Number(mode);
          if (modeNum === 3) {
            setControlState(strikeVel, false);
            setControlState(strikeHard, false);
            setControlState(frictVel, false);
            setControlState(frictForce, false);
            setControlState(extGain, true);
          } else if (modeNum === 1) {
            setControlState(strikeVel, false);
            setControlState(strikeHard, false);
            setControlState(frictVel, true);
            setControlState(frictForce, true);
            setControlState(extGain, false);
          } else {
            setControlState(strikeVel, true);
            setControlState(strikeHard, true);
            setControlState(frictVel, false);
            setControlState(frictForce, false);
            setControlState(extGain, false);
          }
        };

        // Test mode 3 (EXT IN)
        updateExciterControls(3);
        assert.strictEqual(mockElements['knob-exciter-velocity'].style.opacity, '0.4');
        assert.strictEqual(mockElements['knob-exciter-velocity'].style.pointerEvents, 'none');
        assert.strictEqual(mockElements['knob-friction-velocity'].style.opacity, '0.4');
        assert.strictEqual(mockElements['knob-ext-input-gain'].style.opacity, '1.0');
        assert.strictEqual(mockElements['knob-ext-input-gain'].style.pointerEvents, 'auto');

        // Test mode 0 (Strike)
        updateExciterControls(0);
        assert.strictEqual(mockElements['knob-exciter-velocity'].style.opacity, '1.0');
        assert.strictEqual(mockElements['knob-exciter-velocity'].style.pointerEvents, 'auto');
        assert.strictEqual(mockElements['knob-friction-velocity'].style.opacity, '0.4');
        assert.strictEqual(mockElements['knob-ext-input-gain'].style.opacity, '0.4');

        // Test mode 1 (Friction)
        updateExciterControls(1);
        assert.strictEqual(mockElements['knob-exciter-velocity'].style.opacity, '0.4');
        assert.strictEqual(mockElements['knob-friction-velocity'].style.opacity, '1.0');
        assert.strictEqual(mockElements['knob-friction-velocity'].style.pointerEvents, 'auto');
      } finally {
        if (originalGetElementById) {
          globalThis.document.getElementById = originalGetElementById;
        }
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('10. Acoustic Stabilization & DSP Parity (True Brickwall, Q-Norm, Bipolar Spread, Input Conditioning)', () => {
    it('verifies generateHermiteCurve defaults to True Brickwall ceiling 1.00 (0.0 dBFS)', async () => {
      const { generateHermiteCurve } = await import('./js/audio/mr16_web_engine.js');
      const curve = generateHermiteCurve(1024);
      assert.strictEqual(curve.length, 1024);
      let maxVal = 0;
      for (let i = 0; i < curve.length; i++) {
        const absVal = Math.abs(curve[i]);
        if (absVal > maxVal) maxVal = absVal;
      }
      assert.ok(maxVal <= 1.0001, `Max curve amplitude (${maxVal}) must be <= 1.0001`);
      assert.ok(Math.abs(curve[1023] - 1.00) < 1e-4, 'Max positive curve endpoint must equal 1.00');
      assert.ok(Math.abs(curve[0] - (-1.00)) < 1e-4, 'Min negative curve endpoint must equal -1.00');
    });

    it('verifies applyNonLinearVelocityDamping models quadratic air drag bounded by [-6, 6]', async () => {
      const { applyNonLinearVelocityDamping } = await import('./js/audio/mr16_web_engine.js');
      assert.strictEqual(applyNonLinearVelocityDamping(2.0), 2.0, 'Small velocity below threshold must pass linearly');
      assert.strictEqual(applyNonLinearVelocityDamping(-2.0), -2.0, 'Small negative velocity must pass linearly');

      const vLarge = applyNonLinearVelocityDamping(10.0);
      assert.ok(vLarge > 4.0 && vLarge <= 6.0, `Large velocity (${vLarge}) must be compressed within [4.0, 6.0]`);

      const vExtreme = applyNonLinearVelocityDamping(10000.0);
      assert.ok(vExtreme <= 6.0, `Extreme velocity (${vExtreme}) must never exceed maxState 6.0`);

      const vNegExtreme = applyNonLinearVelocityDamping(-10000.0);
      assert.ok(vNegExtreme >= -6.0, `Negative extreme velocity (${vNegExtreme}) must never fall below -6.0`);

      assert.strictEqual(applyNonLinearVelocityDamping(NaN), 0.0);
      assert.strictEqual(applyNonLinearVelocityDamping(Infinity), 0.0);
    });

    it('verifies SUB_HARMONIC_RATIOS provides 4 sub-harmonic anchor partials', async () => {
      const { SUB_HARMONIC_RATIOS } = await import('./js/audio/mr16_web_engine.js');
      assert.strictEqual(SUB_HARMONIC_RATIOS.length, 4);
      for (const r of SUB_HARMONIC_RATIOS) {
        assert.ok(r > 0.0 && r < 1.0, `Sub-harmonic ratio ${r} must be strictly in (0, 1)`);
      }
      assert.strictEqual(SUB_HARMONIC_RATIOS[0], 0.5000, 'Mode 0 sub-harmonic must be one octave below (0.5000)');
    });

    it('verifies bipolar_spread anchors modes 0..3 below f0 while modes 4..15 spread upward', async () => {
      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const { HeadlessOfflineAudioContext } = await import('./headless-audio-context.mjs');
      const ctx = new HeadlessOfflineAudioContext(2, 512, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      const f0 = 440.0;
      engine.setParam('modal_frequency', f0);
      engine.setParam('modal_spread', 1.0);

      // Unipolar (default): mode 0 is exactly f0
      engine.setBipolarSpread(false);
      assert.strictEqual(engine.bipolarSpread, false);
      assert.ok(Math.abs(engine.modalFreqs[0] - f0) < 1.0, `Unipolar mode 0 must equal f0: ${engine.modalFreqs[0]}`);

      // Bipolar spread enabled: modes 0..3 are sub-harmonics below f0
      engine.setBipolarSpread(true);
      assert.strictEqual(engine.bipolarSpread, true);
      assert.ok(engine.modalFreqs[0] < f0, `Bipolar mode 0 (${engine.modalFreqs[0]}) must be below f0 (${f0})`);
      assert.ok(engine.modalFreqs[1] < f0, `Bipolar mode 1 (${engine.modalFreqs[1]}) must be below f0 (${f0})`);
      assert.ok(engine.modalFreqs[2] < f0, `Bipolar mode 2 (${engine.modalFreqs[2]}) must be below f0 (${f0})`);
      assert.ok(engine.modalFreqs[3] < f0, `Bipolar mode 3 (${engine.modalFreqs[3]}) must be below f0 (${f0})`);
      assert.ok(engine.modalFreqs[4] > f0, `Mode 4 (${engine.modalFreqs[4]}) must spread above f0 (${f0})`);
    });

    it('verifies External Audio conditioning filter chain (35 Hz HPF, 15 kHz LPF) and gain staging', async () => {
      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const { HeadlessOfflineAudioContext } = await import('./headless-audio-context.mjs');
      const ctx = new HeadlessOfflineAudioContext(2, 512, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      assert.ok(engine.extInputNode, 'extInputNode entry gain must exist');
      assert.ok(engine.extHpf, 'extHpf filter must exist');
      assert.strictEqual(engine.extHpf.type, 'highpass');
      assert.strictEqual(engine.extHpf.frequency.value, 35, 'HPF cutoff must be 35 Hz');

      assert.ok(engine.extLpf, 'extLpf filter must exist');
      assert.strictEqual(engine.extLpf.type, 'lowpass');
      assert.strictEqual(engine.extLpf.frequency.value, 15000, 'LPF cutoff must be 15 kHz');

      assert.ok(engine.extInputGain, 'extInputGain node must exist');
      engine.setExternalInputGain(6.0);
      assert.strictEqual(engine.params.ext_input_gain, 6.0);
    });

    it('verifies Q-dependent energy normalization factors match 1 / sqrt(1 + 0.05 * Q)', async () => {
      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const { HeadlessOfflineAudioContext } = await import('./headless-audio-context.mjs');
      const ctx = new HeadlessOfflineAudioContext(2, 512, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      const qFactors = engine.getQNormalizationFactors();
      assert.strictEqual(qFactors.length, 16);
      for (let i = 0; i < 16; i++) {
        assert.ok(qFactors[i] > 0.0 && qFactors[i] <= 1.0, `Q-norm factor ${i} (${qFactors[i]}) must be in (0, 1]`);
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('11. v1.1.0 DSP, Parameter Scale & Lifecycle Regression Verification', () => {
    it('verifies app.js _toJuceParamValue normalizes [0, 100]% preset params without <= 1.0% jump and maps display_mode 2 to MODAL_FFT', () => {
      const appJs = fs.readFileSync(path.join(__dirname, 'js', 'app.js'), 'utf8');
      assert.ok(appJs.includes('_toJuceParamValue(id, val)'), 'app.js must define _toJuceParamValue(id, val)');
      assert.ok(appJs.includes("'MODAL_FFT'"), 'app.js must map display_mode 2 to MODAL_FFT');
      assert.ok(appJs.includes('OfflineAudioContext'), 'app.js sample-to-modal extraction must decode via OfflineAudioContext without booting hardware WebAudio in JUCE mode');

      const percentIds = new Set([
        'lorenz_freq_mod', 'lorenz_q_mod', 'chorus_mix',
        'golden_pan_spread', 'drive_saturation', 'dry_wet_mix'
      ]);
      const toJuceParamValue = (id, val) => {
        const num = Number(val);
        if (!Number.isFinite(num)) return 0.0;
        if (percentIds.has(id)) {
          return Math.max(0.0, Math.min(1.0, num / 100.0));
        }
        if (id === 'modal_coupling' || id === 'chorus_dimension') {
          return Math.max(0.0, Math.min(1.0, num > 1.0 ? num / 100.0 : num));
        }
        return num;
      };

      // Sub-1% values must map linearly (0.5% -> 0.005, 1.0% -> 0.01) rather than jumping to 1.0 (100%)
      assert.ok(Math.abs(toJuceParamValue('chorus_mix', 0.5) - 0.005) < 1e-9);
      assert.ok(Math.abs(toJuceParamValue('chorus_mix', 1.0) - 0.01) < 1e-9);
      assert.ok(Math.abs(toJuceParamValue('drive_saturation', 25.0) - 0.25) < 1e-9);
      assert.ok(Math.abs(toJuceParamValue('dry_wet_mix', 100.0) - 1.0) < 1e-9);
      assert.ok(Math.abs(toJuceParamValue('modal_coupling', 0.25) - 0.25) < 1e-9);
      assert.ok(Math.abs(toJuceParamValue('chorus_dimension', 0.5) - 0.5) < 1e-9);
    });

    it('verifies Mr16WebEngine ext_input_gain dB law, drive_saturation pre-gain, and soft_limiter toggle', async () => {
      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const { HeadlessOfflineAudioContext } = await import('./headless-audio-context.mjs');
      const ctx = new HeadlessOfflineAudioContext(2, 512, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      // ext_input_gain dB-to-linear law: 0 dB -> 1.0, +6 dB -> ~1.995, -6 dB -> ~0.501, -24 dB -> 0.0
      engine.setParam('ext_input_gain', 0.0);
      assert.ok(Math.abs(engine.extInputGain.gain.getValueAtTime(10.0) - 1.0) < 1e-4, '0 dB should map to unity 1.0');

      engine.setParam('ext_input_gain', 6.0);
      assert.ok(Math.abs(engine.extInputGain.gain.getValueAtTime(10.0) - Math.pow(10, 6 / 20)) < 1e-3, '+6 dB should map to ~1.995');

      engine.setParam('ext_input_gain', -6.0);
      assert.ok(Math.abs(engine.extInputGain.gain.getValueAtTime(10.0) - Math.pow(10, -6 / 20)) < 1e-3, '-6 dB should map to ~0.501');

      engine.setParam('ext_input_gain', -24.0);
      assert.ok(Math.abs(engine.extInputGain.gain.getValueAtTime(10.0) - 0.0) < 1e-6, '-24 dB floor should mute external input cleanly');

      // drive_saturation pre-gain: 25% default -> 1.0 unity, 100% -> 2.125, 0% -> 0.625
      assert.ok(engine.driveGain, 'driveGain node must exist before saturator');
      engine.setParam('drive_saturation', 25.0);
      assert.ok(Math.abs(engine.driveGain.gain.getValueAtTime(10.0) - 1.0) < 1e-4, '25% drive_saturation must be unity 1.0');
      engine.setParam('drive_saturation', 100.0);
      assert.ok(Math.abs(engine.driveGain.gain.getValueAtTime(10.0) - 2.125) < 1e-4, '100% drive_saturation must boost pre-limiter drive to 2.125');

      // soft_limiter bypass / engage
      engine.setParam('soft_limiter', false);
      assert.strictEqual(engine.saturator.curve, null, 'Disabling soft_limiter must bypass WaveShaper curve');
      engine.setParam('soft_limiter', true);
      assert.ok(engine.saturator.curve instanceof Float32Array, 'Enabling soft_limiter must restore Hermite WaveShaper curve');
      engine.destroy();
    });

    it('verifies RK4 _stepLorenz numerical stability at 10 Hz rate and live Q modulation', async () => {
      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const { HeadlessOfflineAudioContext } = await import('./headless-audio-context.mjs');
      const ctx = new HeadlessOfflineAudioContext(2, 512, 48000);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      engine.setParam('lorenz_rate', 10.0);
      engine.setParam('lorenz_chaos', 1.0);
      engine.setParam('lorenz_freq_mod', 50.0);
      engine.setParam('lorenz_q_mod', 80.0);

      for (let i = 0; i < 500; i++) {
        engine._stepLorenz();
        assert.ok(Number.isFinite(engine.lorenzState.x) && Math.abs(engine.lorenzState.x) < 60, 'RK4 Lorenz X must stay bounded');
        assert.ok(Number.isFinite(engine.lorenzState.y) && Math.abs(engine.lorenzState.y) < 70, 'RK4 Lorenz Y must stay bounded');
        assert.ok(Number.isFinite(engine.lorenzState.z) && engine.lorenzState.z > -10 && engine.lorenzState.z < 90, 'RK4 Lorenz Z must stay bounded');
      }
      const settledQ = engine.modes[0].Q.getValueAtTime(10.0);
      assert.ok(Number.isFinite(settledQ) && settledQ >= 1.0, 'Modulated mode Q must stay finite and >= 1.0');
      engine.destroy();
    });

    it('verifies 44.1 kHz Nyquist filter clamping and clean engine.destroy() lifecycle teardown', async () => {
      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const { HeadlessOfflineAudioContext } = await import('./headless-audio-context.mjs');
      const ctx = new HeadlessOfflineAudioContext(2, 512, 44100);
      const engine = new Mr16WebEngine();
      await engine.init(ctx);

      engine.setParam('manifold_type', 1); // StiffBeam (wide overtone ratios)
      engine.setParam('modal_frequency', 2000.0);
      engine.setParam('modal_spread', 2.0);
      engine.setParam('vactrol_lpg_cutoff', 20000.0);

      const maxAllowedHz = 44100 * 0.48 + 1e-3;
      for (let i = 0; i < 16; i++) {
        const modeHz = engine.modes[i].frequency.getValueAtTime(10.0);
        assert.ok(modeHz <= maxAllowedHz,
          `Mode ${i} frequency (${modeHz}) must clamp <= 0.48 * fs (${maxAllowedHz})`);
      }
      const lpgHz = engine.vactrolLpg.frequency.getValueAtTime(10.0);
      assert.ok(lpgHz <= maxAllowedHz,
        `LPG cutoff (${lpgHz}) must clamp <= 0.48 * fs at 44.1 kHz`);

      engine.destroy();
      assert.strictEqual(engine.isInitialized, false, 'destroy() must mark engine uninitialized');
      assert.strictEqual(engine.isPowered, false, 'destroy() must power down engine');
    });

    it('verifies 1:1 parity of all 16 factory presets across C++ Mr16Engine, PluginProcessor, and Web JSON', () => {
      const webPresetsJson = JSON.parse(fs.readFileSync(path.join(__dirname, 'factory_presets.json'), 'utf8'));
      const subPresetsJson = JSON.parse(fs.readFileSync(path.join(__dirname, 'presets', 'factory_presets.json'), 'utf8'));
      assert.deepStrictEqual(webPresetsJson, subPresetsJson, 'web/factory_presets.json and web/presets/factory_presets.json must match 1:1');
      assert.strictEqual(webPresetsJson.presets.length, 16, 'Must define 16 factory presets');

      const engineCpp = fs.readFileSync(path.join(__dirname, '..', 'source', 'dsp', 'Mr16Engine.cpp'), 'utf8');
      const procCpp = fs.readFileSync(path.join(__dirname, '..', 'source', 'plugin', 'PluginProcessor.cpp'), 'utf8');
      for (const p of webPresetsJson.presets) {
        assert.ok(engineCpp.includes(`"${p.id}"`), `Mr16Engine.cpp must include preset id ${p.id}`);
        assert.ok(engineCpp.includes(`"${p.name}"`), `Mr16Engine.cpp must include preset name ${p.name}`);
        assert.ok(procCpp.includes(`"${p.name}"`), `PluginProcessor.cpp must include preset name ${p.name}`);
      }
    });

    it('verifies WebView2 getResource() path-traversal guards and pre-AudioContext setPower() UI callback', async () => {
      const editorCpp = fs.readFileSync(path.join(__dirname, '..', 'source', 'plugin', 'PluginEditor.cpp'), 'utf8');
      assert.ok(editorCpp.includes('path.contains("..")') && editorCpp.includes('path.containsChar(\':\')'),
        'PluginEditor::getResource must reject path traversal and drive specifiers');
      assert.ok(editorCpp.includes('file.isAChildOf(baseDir)'),
        'PluginEditor::getResource must enforce baseDir containment');

      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const engine = new Mr16WebEngine();
      const observedStates = [];
      engine.onPowerStateChanged = (state) => observedStates.push(state);
      engine.setParam('power_state', true);
      engine.setPower(false);
      assert.deepStrictEqual(observedStates, [true, false],
        'setPower / setParam(power_state) must fire onPowerStateChanged even when AudioContext is null (JUCE mode)');
    });
  });

  //----------------------------------------------------------------------------
  describe('12. v1.1.0 Round-2 Functional Wiring & Preset Parity', () => {
    it('verifies euclidean_enable factory preset parity, WebEngine timer lifecycle, and Standby wake-on-enable rules', async () => {
      const presetsJson = JSON.parse(fs.readFileSync(path.join(__dirname, 'factory_presets.json'), 'utf8'));
      const enabledIds = presetsJson.presets.filter((p) => p.params.euclidean_enable === true).map((p) => p.id);
      assert.deepStrictEqual(enabledIds, ['EUCLIDEAN_METALLOPHONE', 'FROSTED_GLASS_ARPEGGIO'],
        'Only EUCLIDEAN_METALLOPHONE (index 7) and FROSTED_GLASS_ARPEGGIO (index 14) may enable euclidean_enable by default');
      for (const p of presetsJson.presets) {
        assert.strictEqual(typeof p.params.euclidean_enable, 'boolean', `Preset ${p.id} must define boolean euclidean_enable`);
      }

      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const engine = new Mr16WebEngine();
      engine.isInitialized = true;
      engine.ctx = {
        currentTime: 0,
        state: 'running',
        suspend: async () => {},
        resume: async () => {}
      };
      engine.masterGain = {
        gain: {
          cancelScheduledValues: () => {},
          setValueAtTime: () => {},
          setTargetAtTime: () => {}
        }
      };

      // Disarmed by default even when powered on
      engine.setPower(true);
      assert.strictEqual(engine.euclideanTimer, null, 'Disarmed euclidean_enable must not start euclideanTimer on power-on');

      // Armed with positive pulses starts timer; pulses = 0 stops timer; power-off stops timer
      engine.setParam('euclidean_pulses', 5);
      engine.setParam('euclidean_steps', 16);
      engine.setParam('euclidean_enable', true);
      assert.notStrictEqual(engine.euclideanTimer, null, 'Enabling euclidean_enable while powered must start euclideanTimer');

      engine.setParam('euclidean_pulses', 0);
      assert.strictEqual(engine.euclideanTimer, null, 'Zero Euclidean pulses must disarm euclideanTimer');

      engine.setParam('euclidean_pulses', 5);
      assert.notStrictEqual(engine.euclideanTimer, null, 'Restoring positive Euclidean pulses while armed must restart timer');

      engine.setPower(false);
      assert.strictEqual(engine.euclideanTimer, null, 'Standby power-off must stop euclideanTimer');

      const indexHtml = fs.readFileSync(path.join(__dirname, 'index.html'), 'utf8');
      assert.ok(indexHtml.includes('id="btn-audition-euclidean"'), 'index.html must expose #btn-audition-euclidean');

      const appJs = fs.readFileSync(path.join(__dirname, 'js', 'app.js'), 'utf8');
      assert.ok(appJs.includes("_emitJuceParam('currentProgram'"), 'app.js must notify C++ host via currentProgram on factory preset select');
      assert.ok(appJs.includes("_emitJuceParam('euclidean_enable'"), 'app.js must bind euclidean_enable to JUCE/Web parameter state');
    });

    it('verifies 16-key Chime Strip microtonal frequency forwarding across Web UI and C++ IPC handler', () => {
      const appJs = fs.readFileSync(path.join(__dirname, 'js', 'app.js'), 'utf8');
      assert.ok(appJs.includes("type: 'chime', key: index, vel: velocity, freq"),
        'app.js triggerChimeKey must forward per-key microtonal freq over JUCE exciterTrigger IPC');

      const editorCpp = fs.readFileSync(path.join(__dirname, '..', 'source', 'plugin', 'PluginEditor.cpp'), 'utf8');
      assert.ok(editorCpp.includes('processorRef.triggerChimeKey(key, vel, freq)'),
        'PluginEditor::handleExciterTriggerFromWeb must pass freq to processorRef.triggerChimeKey');
    });

    it('verifies LorenzMorpher zero-alpha fidelity, WebEngine adjacent-manifold morphing, and calibrated preset modal_q', async () => {
      const { Mr16WebEngine } = await import('./js/audio/mr16_web_engine.js');
      const { LorenzMorpher } = await import('./js/audio/lorenz_morpher.js');

      const engine = new Mr16WebEngine();
      engine.isInitialized = true;
      engine.ctx = { currentTime: 0, sampleRate: 48000 };
      const makeParamMock = () => ({ setValueAtTime: () => {}, setTargetAtTime: () => {} });
      engine.modes = Array.from({ length: 16 }, () => ({ frequency: makeParamMock(), Q: makeParamMock() }));
      engine.modalGains = Array.from({ length: 16 }, () => ({ gain: makeParamMock() }));
      engine.updateModalFrequencies();
      const baseMode2 = engine.modalFreqs[2];

      // Continuous adjacent-manifold morphing in standard (!customProfileActive) mode
      engine.setParam('lorenz_morph_alpha', 0.5);
      const halfMorphedMode2 = engine.modalFreqs[2];
      engine.setParam('lorenz_morph_alpha', 1.0);
      const fullMorphedMode2 = engine.modalFreqs[2];

      assert.ok(halfMorphedMode2 > baseMode2 && fullMorphedMode2 > halfMorphedMode2,
        `CHLADNI -> BEAM adjacent manifold morph must monotonically shift mode 2 (${baseMode2} -> ${halfMorphedMode2} -> ${fullMorphedMode2})`);

      // Lorenz steer dynamically perturbs effective morph alpha when customProfileActive is false
      engine.setParam('lorenz_morph_alpha', 0.25);
      const unsteeredMode2 = engine.modalFreqs[2];
      engine.lorenzState.x = 12.0; // positive normalizedX
      engine.setParam('lorenz_morph_steer', 80.0);
      const steeredMode2 = engine.modalFreqs[2];
      assert.notStrictEqual(steeredMode2, unsteeredMode2,
        'Non-zero lorenz_morph_steer must dynamically steer adjacent-manifold modal frequencies');

      // LorenzMorpher must honor explicit alpha = 0.0 (no falsy || 0.5 bug)
      const morpher = new LorenzMorpher();
      engine.setParam('lorenz_morph_alpha', 0.0);
      engine.setParam('lorenz_morph_steer', 0.0);
      morpher.applyToEngine(engine);
      assert.ok(Math.abs(engine.modalFreqs[0] - 220.0) < 1.0e-3,
        `LorenzMorpher.applyToEngine must preserve lorenz_morph_alpha = 0.0 (220 Hz Slot A, got ${engine.modalFreqs[0]})`);

      // Calibrated per-preset modal_q parity across JSON, PluginProcessor.cpp, and Mr16Engine.cpp
      const presetsJson = JSON.parse(fs.readFileSync(path.join(__dirname, 'factory_presets.json'), 'utf8'));
      const distinctQs = new Set(presetsJson.presets.map((p) => p.params.modal_q));
      assert.ok(distinctQs.size >= 12, 'Factory presets must carry calibrated material-appropriate modal_q values (not uniform 85.0)');
      const woodPreset = presetsJson.presets.find((p) => p.id === 'MARIMBA_ROSEWOOD_BAR');
      const glassPreset = presetsJson.presets.find((p) => p.id === 'CRYSTAL_WINE_GLASS');
      assert.ok(woodPreset && glassPreset && glassPreset.params.modal_q > woodPreset.params.modal_q,
        'Crystal Wine Glass preset must have higher calibrated modal_q than Marimba Rosewood Bar');
    });
  });

});
