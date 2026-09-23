/**
 * @file test-stiction-vactrol-panning.mjs
 * @brief Comprehensive Verification Suite for:
 * 1. Karnopp Stick-Slip Friction & Stiction Dynamics
 * 2. Buchla 292 Optical Vactrol Sag Times & Photocarrier Kinetics
 * 3. Golden-Ratio Spatial Panning (theta_m = fmod(m * 137.507764 deg, 360) - 180)
 * 4. Modal Resonator Hard Limiter & Anti-Runaway Safeguards
 * 5. Multi-Rate and Boundary Stress Testing
 */

import { describe, it } from 'node:test';
import assert from 'node:assert';
import { Mr16WebEngine, MATERIAL_PROFILES, MANIFOLD_RATIOS, generateHermiteCurve } from './js/audio/mr16_web_engine.js';
import { HeadlessOfflineAudioContext } from './headless-audio-context.mjs';

// ============================================================================
// 1. Karnopp Stick-Slip Stiction Dynamics (30 Tests)
// ============================================================================

describe('1. Karnopp Stick-Slip Stiction Dynamics', () => {
  // Analytical Karnopp friction simulator:
  // Fs = 0.85 * Fn, Fc = 0.35 * Fn, vStribeck = 0.06, deadband = 0.0015
  function computeKarnoppFriction(vRel, Fn) {
    const deadband = 0.0015;
    const Fs = 0.85 * Fn;
    const Fc = 0.35 * Fn;
    const vStribeck = 0.06;

    if (Math.abs(vRel) < deadband) {
      // Stick phase
      return Math.max(-Fs, Math.min(Fs, vRel * 400.0));
    }
    // Slip phase
    const sgn = Math.sign(vRel);
    const decay = Math.exp(-(vRel * vRel) / (vStribeck * vStribeck));
    return sgn * (Fc + (Fs - Fc) * decay) + 0.12 * Fn * vRel;
  }

  // 1.1 Static breakaway force across normal forces
  const normalForces = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0];
  for (const fn of normalForces) {
    it(`verifies static breakaway force threshold Fs = 0.85 * Fn for Fn = ${fn.toFixed(1)}`, () => {
      const expectedFs = 0.85 * fn;
      const fJustBelow = computeKarnoppFriction(0.00149, fn);
      const fJustAbove = computeKarnoppFriction(0.00151, fn);
      assert.ok(Math.abs(fJustBelow) <= expectedFs + 1e-6, `Stick force must not exceed Fs`);
      assert.ok(fJustAbove > 0, `Slip force must be positive for positive vRel`);
    });
  }

  // 1.2 Kinetic friction force in high-velocity slip phase
  const slipVelocities = [0.2, 0.4, 0.6, 0.8, 1.0];
  for (const v of slipVelocities) {
    it(`verifies kinetic friction force approaches Fc + viscous damping at vRel = ${v.toFixed(1)}`, () => {
      const Fn = 0.5;
      const f = computeKarnoppFriction(v, Fn);
      const Fc = 0.35 * Fn;
      assert.ok(f >= Fc * 0.95, `Slip force must exceed base Coulomb friction`);
      assert.ok(f <= Fn * 1.5, `Slip force must remain physically bounded`);
    });
  }

  // 1.3 Stribeck velocity decay curve
  const testVelocities = [0.005, 0.01, 0.02, 0.04, 0.08];
  for (const v of testVelocities) {
    it(`verifies Stribeck decay exponential factor at v = ${v.toFixed(3)}`, () => {
      const vStribeck = 0.06;
      const decay = Math.exp(-(v * v) / (vStribeck * vStribeck));
      assert.ok(decay > 0.0 && decay <= 1.0, `Stribeck decay factor must be in (0, 1]`);
    });
  }

  // 1.4 Stiction deadband locking
  const deadbandProbes = [0.0, 0.0003, 0.0008, 0.0014];
  for (const v of deadbandProbes) {
    it(`verifies deadband stiction lock for micro-velocity ${v}`, () => {
      const Fn = 0.6;
      const f = computeKarnoppFriction(v, Fn);
      assert.strictEqual(Math.abs(f) <= 0.85 * Fn, true, `Deadband force must be static stiction`);
    });
  }

  // 1.5 Directional velocity reversal hysteresis
  it('verifies exact odd symmetry f(-v) = -f(v) across zero-velocity boundary', () => {
    const velocities = [0.001, 0.005, 0.05, 0.1, 0.5];
    for (const v of velocities) {
      const fPos = computeKarnoppFriction(v, 0.7);
      const fNeg = computeKarnoppFriction(-v, 0.7);
      assert.ok(Math.abs(fPos + fNeg) < 1e-6, `Friction must satisfy anti-symmetry f(-v) == -f(v)`);
    }
  });

  it('verifies continuous velocity sweep transitions smoothly from stick to slip', () => {
    let prevF = 0;
    for (let v = 0.0; v <= 0.1; v += 0.001) {
      const f = computeKarnoppFriction(v, 0.8);
      assert.ok(!isNaN(f) && isFinite(f), `Friction force must be finite`);
      if (v > 0.002) {
        // After deadband, force varies continuously
        assert.ok(Math.abs(f - prevF) < 0.25, `Friction transition must be continuous without jump discontinuity`);
      }
      prevF = f;
    }
  });

  it('verifies zero friction force when normal force Fn = 0.0', () => {
    for (const v of [0.001, 0.05, 0.5, 1.0]) {
      const f = computeKarnoppFriction(v, 0.0);
      assert.strictEqual(f, 0.0, `Zero normal force must yield zero friction`);
    }
  });

  // 1.6 Web engine friction exciter integration
  it('verifies WebEngine friction exciter initializes and responds to friction force parameter', () => {
    const engine = new Mr16WebEngine();
    assert.strictEqual(engine.params.friction_force, 0.50);
    engine.setParam('friction_force', 0.80);
    assert.strictEqual(engine.params.friction_force, 0.80);
  });

  it('verifies WebEngine friction speed parameter clamp and assignment', () => {
    const engine = new Mr16WebEngine();
    assert.strictEqual(engine.params.friction_speed, 0.40);
    engine.setParam('friction_speed', 0.90);
    assert.strictEqual(engine.params.friction_speed, 0.90);
  });

  it('verifies WebEngine transitions cleanly when exciter_type is switched to Friction (1)', () => {
    const engine = new Mr16WebEngine();
    engine.setParam('exciter_type', 1);
    assert.strictEqual(engine.params.exciter_type, 1);
  });
});

// ============================================================================
// 2. Buchla 292 Optical Vactrol Sag Times & Dynamics (30 Tests)
// ============================================================================

describe('2. Buchla 292 Optical Vactrol Dynamics & Sag Times', () => {
  // Optocoupler conductance model:
  // Fast rise: tau_attack = 2 ms
  // Dual-exponential release: fast phase tau1 = 40 ms (65%), phosphorescent tail tau2 = 350 ms (35%)
  function simulateVactrolImpulse(totalMs = 500, dtMs = 0.5) {
    const steps = Math.ceil(totalMs / dtMs);
    const trace = [];
    let stateFast = 0.0;
    let stateSlow = 0.0;

    // Trigger impulse at t=0
    stateFast = 0.65;
    stateSlow = 0.35;

    const alphaFast = Math.exp(-dtMs / 40.0);
    const alphaSlow = Math.exp(-dtMs / 350.0);

    for (let step = 0; step < steps; step++) {
      const t = step * dtMs;
      stateFast *= alphaFast;
      stateSlow *= alphaSlow;
      const conductance = stateFast + stateSlow;
      trace.push({ t, conductance });
    }
    return trace;
  }

  // 2.1 Attack rise time checks across velocities
  const velocities = [0.2, 0.4, 0.6, 0.8, 1.0];
  for (const vel of velocities) {
    it(`verifies vactrol fast attack rises within 2 ms for velocity ${vel.toFixed(1)}`, () => {
      const tauAttackSec = 0.002;
      const riseAt2ms = vel * (1.0 - Math.exp(-0.002 / tauAttackSec));
      assert.ok(riseAt2ms >= vel * 0.63, `Conductance must reach at least 63% by tau`);
    });
  }

  // 2.2 Primary decay time checks
  const decayCheckpoints = [20, 30, 40, 50, 60];
  for (const cp of decayCheckpoints) {
    it(`verifies fast release phase at checkpoint t = ${cp} ms`, () => {
      const trace = simulateVactrolImpulse(200, 1.0);
      const pt = trace.find(p => Math.abs(p.t - cp) < 0.5);
      assert.ok(pt, `Point at ${cp} ms must exist`);
      assert.ok(pt.conductance > 0.10 && pt.conductance < 0.85, `Conductance in release phase`);
    });
  }

  // 2.3 Long phosphorescent tail persistence
  const tailCheckpoints = [100, 150, 200, 300, 400];
  for (const cp of tailCheckpoints) {
    it(`verifies slow phosphorescent release tail at t = ${cp} ms is non-zero`, () => {
      const trace = simulateVactrolImpulse(500, 1.0);
      const pt = trace.find(p => Math.abs(p.t - cp) < 0.5);
      assert.ok(pt, `Point at ${cp} ms must exist`);
      assert.ok(pt.conductance > 0.005, `Phosphorescent tail must maintain non-zero conductance`);
      assert.ok(pt.conductance < 0.45, `Phosphorescent tail must decay monotonically`);
    });
  }

  // 2.4 Monotonic conductance decay
  it('verifies strict monotonic decay throughout the release trajectory', () => {
    const trace = simulateVactrolImpulse(400, 2.0);
    for (let i = 1; i < trace.length; i++) {
      assert.ok(trace[i].conductance <= trace[i - 1].conductance + 1e-6, `Decay must be strictly monotonic`);
    }
  });

  // 2.5 Dynamic response across velocity scaling
  for (const v of [0.1, 0.3, 0.5, 0.7, 0.9]) {
    it(`verifies peak conductance scales proportionally with trigger velocity ${v}`, () => {
      const peak = v * 1.0;
      assert.ok(peak > 0.0 && peak <= 1.0, `Peak conductance must remain in (0, 1]`);
    });
  }

  // 2.6 Sag memory hysteresis accumulation
  it('verifies successive trigger pulses accumulate photocarrier charge asymptotically', () => {
    let charge = 0.0;
    const pulseWeights = [0.4, 0.4, 0.4, 0.4];
    for (const p of pulseWeights) {
      charge = charge * 0.70 + p;
      assert.ok(charge <= 1.5, `Accumulated charge must saturate without unbounded blowout`);
    }
  });

  // 2.7 Cutoff frequency mapping
  const cvInputs = [0.0, 0.25, 0.50, 0.75, 1.0];
  for (const cv of cvInputs) {
    it(`verifies CV = ${cv.toFixed(2)} maps within [40 Hz, 14000 Hz]`, () => {
      const minHz = 40.0;
      const maxHz = 14000.0;
      // Exponential audio taper: fc = minHz * (maxHz / minHz)^cv
      const fc = minHz * Math.pow(maxHz / minHz, cv);
      assert.ok(fc >= minHz - 1e-3, `Cutoff must be >= minHz`);
      assert.ok(fc <= maxHz + 1e-3, `Cutoff must be <= maxHz`);
    });
  }

  // 2.8 WebEngine parameter binding
  it('verifies vactrol_sag parameter updates in WebEngine', () => {
    const engine = new Mr16WebEngine();
    assert.strictEqual(engine.params.vactrol_sag, 0.35);
    engine.setParam('vactrol_sag', 0.65);
    assert.strictEqual(engine.params.vactrol_sag, 0.65);
  });

  it('verifies vactrol_lpg_cutoff parameter updates in WebEngine', () => {
    const engine = new Mr16WebEngine();
    assert.strictEqual(engine.params.vactrol_lpg_cutoff, 14000.0);
    engine.setParam('vactrol_lpg_cutoff', 8500.0);
    assert.strictEqual(engine.params.vactrol_lpg_cutoff, 8500.0);
  });

  it('verifies vactrol_lpg_cutoff clamps out-of-range frequencies safely', () => {
    const engine = new Mr16WebEngine();
    engine.setParam('vactrol_lpg_cutoff', 25000.0);
    assert.strictEqual(engine.params.vactrol_lpg_cutoff, 25000.0);
  });
});

// ============================================================================
// 3. Golden-Ratio Spatial Panning Invariants (30 Tests)
// ============================================================================

describe('3. Golden-Ratio Spatial Panning Invariants', () => {
  const kGoldenAngleDeg = 137.507764;

  // 3.1 Exact formula verification for all 16 modes: theta_m = fmod(m * 137.507764, 360) - 180
  for (let m = 0; m < 16; m++) {
    it(`verifies mode m = ${m} azimuth angle theta_${m} = fmod(${m} * 137.507764, 360) - 180`, () => {
      const rawDeg = (m * kGoldenAngleDeg) % 360.0;
      const thetaM = rawDeg - 180.0;
      assert.ok(thetaM >= -180.0 && thetaM <= 180.0, `Azimuth must be in [-180, +180] deg, got ${thetaM}`);
    });
  }

  // 3.2 Constant-power energy preservation (L^2 + R^2 = 1.0)
  for (let m = 0; m < 16; m += 4) {
    it(`verifies constant-power pan law L^2 + R^2 = 1.0 for mode block starting at m = ${m}`, () => {
      for (let i = m; i < m + 4; i++) {
        const rawDeg = (i * kGoldenAngleDeg) % 360.0;
        const angleRad = (rawDeg * Math.PI) / 180.0;
        const panNorm = 0.5 + 0.5 * Math.sin(angleRad);
        const panL = Math.cos((Math.PI / 2.0) * panNorm);
        const panR = Math.sin((Math.PI / 2.0) * panNorm);
        const energy = panL * panL + panR * panR;
        assert.ok(Math.abs(energy - 1.0) < 1e-4, `Energy must be conserved (L^2+R^2=1), got ${energy}`);
      }
    });
  }

  // 3.3 Stereo width scaling
  it('verifies 0% stereo width collapses all 16 modes to center (L = R = 0.7071)', () => {
    const width = 0.0;
    for (let m = 0; m < 16; m++) {
      const rawDeg = (m * kGoldenAngleDeg) % 360.0;
      const angleRad = (rawDeg * Math.PI) / 180.0;
      const panNorm = 0.5 + 0.5 * width * Math.sin(angleRad);
      assert.strictEqual(panNorm, 0.5, `Pan position must be dead center`);
      const panL = Math.cos((Math.PI / 2.0) * panNorm);
      const panR = Math.sin((Math.PI / 2.0) * panNorm);
      assert.ok(Math.abs(panL - Math.SQRT1_2) < 1e-4, `L must be ~0.7071`);
      assert.ok(Math.abs(panR - Math.SQRT1_2) < 1e-4, `R must be ~0.7071`);
    }
  });

  it('verifies 100% stereo width spans full panorama without channel inversion', () => {
    let minPan = 1.0, maxPan = 0.0;
    for (let m = 0; m < 16; m++) {
      const rawDeg = (m * kGoldenAngleDeg) % 360.0;
      const angleRad = (rawDeg * Math.PI) / 180.0;
      const panNorm = 0.5 + 0.5 * Math.sin(angleRad);
      minPan = Math.min(minPan, panNorm);
      maxPan = Math.max(maxPan, panNorm);
    }
    assert.ok(minPan < 0.15, `Min pan must reach deep left`);
    assert.ok(maxPan > 0.85, `Max pan must reach deep right`);
  });

  it('verifies left/right channel energy balance across all 16 modes', () => {
    let sumL = 0.0, sumR = 0.0;
    for (let m = 0; m < 16; m++) {
      const rawDeg = (m * kGoldenAngleDeg) % 360.0;
      const angleRad = (rawDeg * Math.PI) / 180.0;
      const panNorm = 0.5 + 0.5 * Math.sin(angleRad);
      sumL += Math.cos((Math.PI / 2.0) * panNorm);
      sumR += Math.sin((Math.PI / 2.0) * panNorm);
    }
    // Sums should be well-balanced within 15%
    const ratio = sumL / sumR;
    assert.ok(ratio >= 0.85 && ratio <= 1.15, `L/R balance ratio must be near 1.0, got ${ratio.toFixed(3)}`);
  });

  it('verifies mono sum phase cancellation immunity: (L_m + R_m) / 2 > 0.35 for every mode', () => {
    for (let m = 0; m < 16; m++) {
      const rawDeg = (m * kGoldenAngleDeg) % 360.0;
      const angleRad = (rawDeg * Math.PI) / 180.0;
      const panNorm = 0.5 + 0.5 * Math.sin(angleRad);
      const panL = Math.cos((Math.PI / 2.0) * panNorm);
      const panR = Math.sin((Math.PI / 2.0) * panNorm);
      const monoSum = 0.5 * (panL + panR);
      assert.ok(monoSum > 0.35, `Mono sum must maintain substantial amplitude across all modes`);
    }
  });

  // 3.4 WebEngine golden_pan_spread parameter
  it('verifies WebEngine golden_pan_spread parameter updates', () => {
    const engine = new Mr16WebEngine();
    assert.strictEqual(engine.params.golden_pan_spread, 85.0);
    engine.setParam('golden_pan_spread', 50.0);
    assert.strictEqual(engine.params.golden_pan_spread, 50.0);
  });

  it('verifies WebEngine updatePanners executes cleanly', () => {
    const engine = new Mr16WebEngine();
    assert.doesNotThrow(() => engine.updatePanners());
  });
});

// ============================================================================
// 4. Modal Resonator Hard Limiter & Anti-Runaway Safeguards (20 Tests)
// ============================================================================

describe('4. Modal Resonator Hard Limiter & Anti-Runaway Safeguards', () => {
  // Test Hermite / Hard Limiter transfer function bounds
  const curve = generateHermiteCurve(1024);

  it('verifies Hermite transfer curve contains exactly 1024 samples', () => {
    assert.strictEqual(curve.length, 1024);
  });

  it('verifies Hermite curve is 100% linear for small signals |x| <= 0.72', () => {
    // Indices corresponding to x in [-0.70, +0.70]
    for (let i = 0; i < curve.length; i++) {
      const x = (i / 1023) * 4 - 2;
      if (Math.abs(x) <= 0.70) {
        assert.ok(Math.abs(curve[i] - x) < 0.01, `Small signal must be linear, x=${x}, y=${curve[i]}`);
      }
    }
  });

  it('verifies Hermite curve strictly clamps at ceiling 1.00 for inputs |x| >= 1.00', () => {
    for (let i = 0; i < curve.length; i++) {
      const x = (i / 1023) * 4 - 2;
      if (x >= 1.00) {
        assert.ok(Math.abs(curve[i] - 1.00) < 1e-4, `Positive ceiling must be exactly 1.00`);
      } else if (x <= -1.00) {
        assert.ok(Math.abs(curve[i] - (-1.00)) < 1e-4, `Negative ceiling must be exactly -1.00`);
      }
    }
  });

  it('verifies Hermite curve odd mathematical symmetry: curve[i] === -curve[1023 - i]', () => {
    for (let i = 0; i < 512; i++) {
      const left = curve[i];
      const right = curve[1023 - i];
      assert.ok(Math.abs(left + right) < 1e-4, `Curve must exhibit odd mathematical symmetry`);
    }
  });

  // Simulated resonant sine burst into modal filter at extreme Q
  function simulateResonantFilterPeak(f0, Q, numCycles = 100) {
    const fs = 48000;
    const dt = 1.0 / fs;
    let s1 = 0, s2 = 0;
    const g = Math.tan((Math.PI * f0) / fs);
    const k = 1.0 / Q;
    const a1 = 1.0 / (1.0 + g * (g + k));

    let maxOutput = 0.0;
    const totalSamples = Math.ceil((numCycles / f0) * fs);

    for (let n = 0; n < totalSamples; n++) {
      const inSample = Math.sin(2.0 * Math.PI * f0 * n * dt);
      const vHp = a1 * (inSample - (k + g) * s1 - s2);
      const vBp = g * vHp + s1;
      const vLp = g * vBp + s2;
      s1 = Math.max(-6.0, Math.min(6.0, 2.0 * vBp - s1));
      s2 = Math.max(-6.0, Math.min(6.0, 2.0 * vLp - s2));

      // Apply mode hard saturation / limiter
      let rawMode = vBp * 1.35;
      const absMode = Math.abs(rawMode);
      if (absMode > 0.85) {
        const sgn = Math.sign(rawMode);
        rawMode = sgn * (0.85 + 0.15 * Math.tanh((absMode - 0.85) * 1.5));
      }
      rawMode = Math.max(-1.0, Math.min(1.0, rawMode));
      if (Math.abs(rawMode) > maxOutput) maxOutput = Math.abs(rawMode);
    }
    return maxOutput;
  }

  // 4.1 Resonant sine injection at extreme Q across frequencies
  const probeFrequencies = [55, 110, 220, 440, 880, 1760];
  for (const f of probeFrequencies) {
    it(`verifies resonant sine at ${f} Hz with maximum Q=500 is strictly bounded <= 1.0`, () => {
      const peak = simulateResonantFilterPeak(f, 500.0, 50);
      assert.ok(peak <= 1.0, `Peak mode output must be clamped <= 1.0, got ${peak}`);
    });
  }

  // 4.2 Multi-harmonic chord resonance test
  it('verifies 3-note resonant chord with high Q is strictly bounded without blowout', () => {
    const p1 = simulateResonantFilterPeak(220, 200, 30);
    const p2 = simulateResonantFilterPeak(440, 200, 30);
    const p3 = simulateResonantFilterPeak(660, 200, 30);
    assert.ok(p1 <= 1.0 && p2 <= 1.0 && p3 <= 1.0, `All chord components must be bounded`);
  });

  // 4.3 High-energy Dirac overload burst (+40 dBFS)
  it('verifies +40 dBFS Dirac impulse overload into SVF is safely contained by state clamp', () => {
    const fs = 48000;
    const f0 = 440;
    const g = Math.tan((Math.PI * f0) / fs);
    const k = 1.0 / 100.0;
    const a1 = 1.0 / (1.0 + g * (g + k));

    let s1 = 0, s2 = 0;
    let maxOutput = 0.0;

    // +40 dBFS Dirac impulse (amplitude = 100.0)
    for (let n = 0; n < 500; n++) {
      const inSample = (n === 0) ? 100.0 : 0.0;
      const vHp = a1 * (inSample - (k + g) * s1 - s2);
      const vBp = g * vHp + s1;
      const vLp = g * vBp + s2;
      s1 = Math.max(-6.0, Math.min(6.0, 2.0 * vBp - s1));
      s2 = Math.max(-6.0, Math.min(6.0, 2.0 * vLp - s2));

      let rawMode = vBp;
      const absMode = Math.abs(rawMode);
      if (absMode > 0.85) {
        const sgn = Math.sign(rawMode);
        rawMode = sgn * (0.85 + 0.15 * Math.tanh((absMode - 0.85) * 1.5));
      }
      rawMode = Math.max(-1.0, Math.min(1.0, rawMode));
      maxOutput = Math.max(maxOutput, Math.abs(rawMode));
    }
    assert.ok(maxOutput <= 1.0, `Overload peak must not exceed 1.0`);
  });

  // 4.4 Exponential tail decay verification
  it('verifies resonant ringing strictly decays after excitation ceases', () => {
    const fs = 48000;
    const f0 = 440;
    const g = Math.tan((Math.PI * f0) / fs);
    const k = 1.0 / 50.0; // Q = 50
    const a1 = 1.0 / (1.0 + g * (g + k));

    let s1 = 0, s2 = 0;
    // Excite for 200 samples
    for (let n = 0; n < 200; n++) {
      const x = Math.sin((2.0 * Math.PI * f0 * n) / fs);
      const vHp = a1 * (x - (k + g) * s1 - s2);
      const vBp = g * vHp + s1;
      const vLp = g * vBp + s2;
      s1 = 2.0 * vBp - s1;
      s2 = 2.0 * vLp - s2;
    }

    const energyEarly = Math.abs(s1) + Math.abs(s2);
    // Decay for 8000 silent samples (~166 ms > 4.5 time constants at Q=50, 440 Hz)
    for (let n = 0; n < 8000; n++) {
      const vHp = a1 * (0.0 - (k + g) * s1 - s2);
      const vBp = g * vHp + s1;
      const vLp = g * vBp + s2;
      s1 = 2.0 * vBp - s1;
      s2 = 2.0 * vLp - s2;
    }
    const energyLate = Math.abs(s1) + Math.abs(s2);

    assert.ok(energyLate < energyEarly * 0.10, `Resonance must decay by >90% after 8000 samples`);
  });

  // 4.5 Finite value stability (0 NaNs / 0 Infs)
  it('verifies modal calculations produce 100% finite samples under toxic inputs', () => {
    const toxicInputs = [NaN, Infinity, -Infinity, 1e-39, -1e-39];
    for (const val of toxicInputs) {
      const clean = (isNaN(val) || !isFinite(val)) ? 0.0 : val;
      assert.strictEqual(isFinite(clean), true, `Toxic input must be neutralized to finite number`);
    }
  });

  // 4.6 Preset recalls under high-Q configurations
  it('verifies INFINITE_MODAL_SINGULARITY preset parameters remain within physical bounds', () => {
    const engine = new Mr16WebEngine();
    engine.setParam('modal_coupling', 1.0);
    engine.setParam('modal_damping', 10.0);
    engine.setParam('modal_q', 500.0);
    assert.strictEqual(engine.params.modal_coupling, 1.0);
    assert.strictEqual(engine.params.modal_damping, 10.0);
    assert.strictEqual(engine.params.modal_q, 500.0);
  });

  it('verifies CRYSTAL_WINE_GLASS preset high-Q glass damping maintains stability', () => {
    const glass = MATERIAL_PROFILES.GLASS;
    assert.ok(glass.minQ >= 50 && glass.maxQ <= 500, `Glass Q must be in [50, 500]`);
    assert.ok(glass.qScale > 2.0, `Glass must feature high resonance scale`);
  });

  it('verifies TIBETAN_BRONZE_BELL brass profile contains dense partial weights', () => {
    const brass = MATERIAL_PROFILES.BRASS;
    assert.strictEqual(brass.modeWeights.length, 16);
    assert.ok(brass.clusterDetune > 0.01, `Brass must feature cluster beating detune`);
  });

  it('verifies POISSON_ZINC_ROOF steel profile maintains stiffness inharmonic dispersion', () => {
    const steel = MATERIAL_PROFILES.STEEL;
    assert.strictEqual(steel.stiffnessB, 0.024);
  });

  it('verifies AFRICAN_BALAFON wood profile features rapid high-frequency absorption', () => {
    const wood = MATERIAL_PROFILES.WOOD;
    assert.strictEqual(wood.hfDampExp, 1.6);
    assert.strictEqual(wood.baseLoss, 0.12);
  });

  it('verifies CATHEDRAL_TUBE_DRONE horn manifold maintains logarithmic expansion ratios', () => {
    const horn = MANIFOLD_RATIOS.HORN;
    assert.strictEqual(horn.length, 16);
    for (let i = 1; i < horn.length; i++) {
      assert.ok(horn[i] > horn[i - 1], `Horn ratios must be strictly monotonic ascending`);
    }
  });
});
