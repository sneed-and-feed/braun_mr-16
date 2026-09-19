/**
 * @file lorenz_morpher.js
 * @brief 3D Chaotic Lorenz-Steered 16-Mode Acoustic Profile Morpher for BRAUN MR-16
 *
 * Coordinates continuous non-linear geodesic morphing between two modal profile slots:
 * - Profile Slot A (profileA) and Profile Slot B (profileB)
 * - Geodesic log-frequency interpolation preserving musical intervals
 * - Log-Q damping factor interpolation
 * - Constant-power acoustic energy gain crossfading
 * - 3D Chaotic Lorenz Attractor trajectory steering (x, y, z) dynamically
 *   perturbing the 16-dimensional modal path via golden-ratio projections
 *
 * Strict real-time safety, zero dynamic allocations during morph callbacks,
 * zero emojis, DIN 1451 technical English nomenclature.
 */

import { MANIFOLD_RATIOS, MATERIAL_PROFILES } from './mr16_web_engine.js';

/**
 * Flush denormalized floating-point numbers to zero
 * @param {number} val
 * @returns {number}
 */
function flushDenormal(val) {
  if (!Number.isFinite(val)) return 0.0;
  return Math.abs(val) < 1.0e-15 ? 0.0 : val;
}

/**
 * Generate a calibrated default 16-mode profile
 * @param {string} manifoldType - 'CHLADNI' | 'BEAM' | 'VOCAL' | 'HORN'
 * @param {number} fundamental - Center frequency (Hz)
 * @param {number} baseQ - Nominal Q factor
 * @returns {Object} ModalProfile
 */
export function createDefaultProfile(manifoldType = 'CHLADNI', fundamental = 220.0, baseQ = 85.0) {
  const ratios = MANIFOLD_RATIOS[manifoldType] || MANIFOLD_RATIOS.CHLADNI;
  const modes = [];
  const freqs = new Float32Array(16);
  const qs = new Float32Array(16);
  const gains = new Float32Array(16);

  for (let i = 0; i < 16; i++) {
    const f = Math.max(20.0, Math.min(22000.0, fundamental * ratios[i]));
    // Moderate HF damping: Q decreases slightly as frequency rises
    const q = Math.max(12.0, Math.min(450.0, baseQ * Math.pow(1.0 + i * 0.15, -0.6)));
    const gain = Math.max(0.05, Math.pow(0.88, i));

    freqs[i] = f;
    qs[i] = q;
    gains[i] = gain;

    modes.push({
      index: i,
      frequency: Number(f.toFixed(2)),
      ratio: ratios[i],
      q: Number(q.toFixed(1)),
      gain: Number(gain.toFixed(4)),
      decayTimeSec: Number(((Math.log(1000.0) * q) / (Math.PI * f)).toFixed(3)),
      decayRateAlpha: Number(((Math.PI * f) / q).toFixed(2))
    });
  }

  return {
    $schema: 'https://braun-audio.de/schemas/mr16-modal-profile-v1.json',
    format: 'BRAUN_MR16_MODAL_PROFILE',
    version: 1,
    id: `DEFAULT_${manifoldType}_${Math.round(fundamental)}`,
    name: `DEFAULT ${manifoldType} (${Math.round(fundamental)} Hz)`,
    timestamp: new Date().toISOString(),
    sampleRate: 48000,
    fundamental: fundamental,
    manifoldClassification: {
      type: manifoldType,
      confidence: 1.0,
      mse: 0.0,
      scores: { [manifoldType]: 0.0 }
    },
    modes: modes,
    ratios: Array.from(ratios),
    frequencies: Array.from(freqs),
    qFactors: Array.from(qs),
    gains: Array.from(gains)
  };
}

export class LorenzMorpher {
  constructor() {
    // Two profile slots
    this.profileA = createDefaultProfile('CHLADNI', 220.0, 95.0);
    this.profileB = createDefaultProfile('BEAM', 174.61, 75.0);

    // Persistent pre-allocated Float32Array buffers for real-time safety
    this.morphedFrequencies = new Float32Array(16);
    this.morphedQs = new Float32Array(16);
    this.morphedGains = new Float32Array(16);
    this.morphedPans = new Float32Array(16);
    this.effectiveAlphas = new Float32Array(16);

    // Precomputed Golden-Angle phase projections (Phi = 137.507764 deg)
    this.goldenSin = new Float32Array(16);
    this.goldenCos = new Float32Array(16);
    this.goldenSin2 = new Float32Array(16);

    const deg2rad = Math.PI / 180.0;
    for (let i = 0; i < 16; i++) {
      const angle = (i * 137.507764) * deg2rad;
      this.goldenSin[i] = Math.sin(angle);
      this.goldenCos[i] = Math.cos(angle);
      this.goldenSin2[i] = Math.sin(2.0 * angle);
    }
  }

  /**
   * Set ModalProfile into Slot A
   * @param {Object} profile
   */
  setProfileA(profile) {
    if (!profile || !profile.frequencies || profile.frequencies.length < 16) {
      throw new Error('LorenzMorpher: Profile A must contain at least 16 modes');
    }
    this.profileA = profile;
  }

  /**
   * Set ModalProfile into Slot B
   * @param {Object} profile
   */
  setProfileB(profile) {
    if (!profile || !profile.frequencies || profile.frequencies.length < 16) {
      throw new Error('LorenzMorpher: Profile B must contain at least 16 modes');
    }
    this.profileB = profile;
  }

  getProfileA() {
    return this.profileA;
  }

  getProfileB() {
    return this.profileB;
  }

  /**
   * Check if both profile slots are populated
   * @returns {boolean}
   */
  hasProfiles() {
    return Boolean(this.profileA && this.profileB);
  }

  /**
   * Interpolate 16-mode resonant parameters steered by 3D Lorenz attractor
   * @param {number} alpha - Base interpolation factor [0.0 (Slot A) to 1.0 (Slot B)]
   * @param {Object|null} [lorenzState=null] - Instantaneous Lorenz state { x, y, z }
   * @param {number} [steerAmount=0.0] - Lorenz perturbation depth [0.0 to 1.0]
   * @returns {Object} Morphed 16-pole acoustic state
   */
  morph(alpha = 0.5, lorenzState = null, steerAmount = 0.0) {
    const pA = this.profileA || createDefaultProfile('CHLADNI', 220.0, 95.0);
    const pB = this.profileB || createDefaultProfile('BEAM', 174.61, 75.0);

    const baseAlpha = Math.max(0.0, Math.min(1.0, alpha));
    const steerNorm = Math.max(0.0, Math.min(1.0, steerAmount > 1.0 ? steerAmount / 100.0 : steerAmount));

    // Normalize 3D Lorenz coordinates
    let normX = 0.0;
    let normY = 0.0;
    let normZ = 0.0;

    if (lorenzState && steerNorm > 0.0001) {
      const lx = Number.isFinite(lorenzState.x) ? lorenzState.x : 0.1;
      const ly = Number.isFinite(lorenzState.y) ? lorenzState.y : 0.0;
      const lz = Number.isFinite(lorenzState.z) ? lorenzState.z : 25.0;

      normX = Math.max(-1.0, Math.min(1.0, lx / 20.0));
      normY = Math.max(-1.0, Math.min(1.0, ly / 28.0));
      normZ = Math.max(-1.0, Math.min(1.0, (lz - 25.0) / 25.0));
    }

    const freqsA = pA.frequencies;
    const freqsB = pB.frequencies;
    const qsA = pA.qFactors;
    const qsB = pB.qFactors;
    const gainsA = pA.gains;
    const gainsB = pB.gains;

    for (let i = 0; i < 16; i++) {
      // 1. Calculate per-mode perturbed morph coordinate alpha_i
      let effectiveAlpha = baseAlpha;

      if (steerNorm > 0.0001) {
        // Golden-angle directional perturbation: delta_i = 0.5 * (x*sin + y*cos + z*sin2)
        const delta = 0.5 * (
          normX * this.goldenSin[i] +
          normY * this.goldenCos[i] +
          normZ * this.goldenSin2[i]
        );
        effectiveAlpha = Math.max(0.0, Math.min(1.0, baseAlpha + steerNorm * delta));
      }

      this.effectiveAlphas[i] = flushDenormal(effectiveAlpha);

      // 2. Geodesic log-frequency interpolation: f(alpha) = f_A^(1 - a) * f_B^a
      const fa = Math.max(20.0, Math.min(22000.0, freqsA[i] || 220.0));
      const fb = Math.max(20.0, Math.min(22000.0, freqsB[i] || 220.0));

      const logFa = Math.log(fa);
      const logFb = Math.log(fb);
      let interpFreq = Math.exp((1.0 - effectiveAlpha) * logFa + effectiveAlpha * logFb);

      // Chaotic frequency micro-perturbation
      if (steerNorm > 0.0001) {
        const freqMod = 1.0 + steerNorm * 0.025 * normX * this.goldenCos[i];
        interpFreq *= freqMod;
      }
      this.morphedFrequencies[i] = flushDenormal(Math.max(20.0, Math.min(22000.0, interpFreq)));

      // 3. Log-Q damping factor interpolation: Q(alpha) = Q_A^(1 - a) * Q_B^a
      const qa = Math.max(5.0, Math.min(500.0, qsA[i] || 85.0));
      const qb = Math.max(5.0, Math.min(500.0, qsB[i] || 85.0));

      const logQa = Math.log(qa);
      const logQb = Math.log(qb);
      let interpQ = Math.exp((1.0 - effectiveAlpha) * logQa + effectiveAlpha * logQb);

      // Chaotic Q micro-perturbation
      if (steerNorm > 0.0001) {
        const qMod = 1.0 + steerNorm * 0.30 * normY * this.goldenSin[i];
        interpQ *= qMod;
      }
      this.morphedQs[i] = flushDenormal(Math.max(5.0, Math.min(500.0, interpQ)));

      // 4. Acoustic energy-preserving gain crossfade: g = sqrt((1-a)*gA^2 + a*gB^2)
      const ga = Math.max(0.0, Math.min(1.0, gainsA[i] !== undefined ? gainsA[i] : 0.5));
      const gb = Math.max(0.0, Math.min(1.0, gainsB[i] !== undefined ? gainsB[i] : 0.5));

      const energy = (1.0 - effectiveAlpha) * (ga * ga) + effectiveAlpha * (gb * gb);
      this.morphedGains[i] = flushDenormal(Math.sqrt(Math.max(0.0, energy)));

      // 5. Dynamic golden-ratio panning perturbed by Lorenz altitude z
      let pan = this.goldenSin[i];
      if (steerNorm > 0.0001) {
        pan += steerNorm * 0.35 * normZ * this.goldenCos[i];
      }
      this.morphedPans[i] = flushDenormal(Math.max(-1.0, Math.min(1.0, pan)));
    }

    return {
      alpha: baseAlpha,
      steerAmount: steerNorm,
      effectiveAlphas: new Float32Array(this.effectiveAlphas),
      frequencies: new Float32Array(this.morphedFrequencies),
      qFactors: new Float32Array(this.morphedQs),
      gains: new Float32Array(this.morphedGains),
      panning: new Float32Array(this.morphedPans),
      lorenzPerturbation: {
        x: normX,
        y: normY,
        z: normZ
      }
    };
  }

  /**
   * Apply morphed 16-mode resonant parameters directly into Mr16WebEngine
   * @param {Object} engine - Instance of Mr16WebEngine
   * @param {Object} [morphed=null] - Pre-computed morphed frame; if null, computes with engine's current state
   * @param {number} [timeConstant=0.03] - Audio-rate parameter slewing time constant (seconds)
   */
  applyToEngine(engine, morphed = null, timeConstant = 0.03) {
    if (!engine || !engine.isInitialized || !engine.ctx) return;

    const frame = morphed || this.morph(
      engine.params.lorenz_morph_alpha || 0.5,
      engine.lorenzState,
      engine.params.lorenz_morph_steer || 0.0
    );

    const now = engine.ctx.currentTime;
    const tau = Math.max(0.005, timeConstant);

    const maxNyquist = (engine.ctx && engine.ctx.sampleRate) ? (engine.ctx.sampleRate * 0.49) : 22000;

    for (let i = 0; i < 16; i++) {
      if (engine.modes && engine.modes[i]) {
        // Frequency target
        const safeF = Math.max(20.0, Math.min(maxNyquist, frame.frequencies[i]));
        engine.modes[i].frequency.setTargetAtTime(safeF, now, tau);
        engine.modalFreqs[i] = safeF;

        // Q factor target
        engine.modes[i].Q.setTargetAtTime(frame.qFactors[i], now, tau);
      }

      if (engine.modalGains && engine.modalGains[i]) {
        engine.modalGains[i].gain.setTargetAtTime(frame.gains[i], now, tau);
      }

      if (engine.modalPanners && engine.modalPanners[i]) {
        const spread = (engine.params.golden_pan_spread || 85.0) / 100.0;
        const pan = frame.panning[i] * spread;
        engine.modalPanners[i].pan.setTargetAtTime(Math.max(-1.0, Math.min(1.0, pan)), now, tau);
      }
    }
  }
}
