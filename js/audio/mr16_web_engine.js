/**
 * @file mr16_web_engine.js
 * @brief Complete Web Audio API DSP Modal Resonator Engine for BRAUN MR-16
 * Features:
 * - Deck 01: Mass-spring contact strike, Karnopp friction, Buchla 292 vactrol air jet, Poisson rain, Euclidean polyrhythm clock
 * - Deck 02: 16-pole parallel modal bandpass filter matrix with 4 acoustic manifolds, 5 material profiles, and orthogonal Householder feedback
 * - Deck 03: 3D continuous chaotic Lorenz attractor modulation
 * - Deck 04: Tri-phase analog BBD chorus (0°, 120°, 240°) with Roland Dimension D stereo matrix and mono cancellation immunity
 * - Deck 05: Golden-angle stereo spatialization (137.5°), dynamic vactrol LPG, Hermite soft-knee saturator & limiter, and 35 Hz DC blocker
 * - Telemetry output for 60 FPS CRT display (modal energies, frequencies, Lorenz state, waveform)
 * - Strict real-time safety, zero garbage collection in audio callbacks
 */

import { LorenzMorpher } from './lorenz_morpher.js';

// 4 Acoustic Manifold Modal Frequency Ratios (16 modes)
export const MANIFOLD_RATIOS = {
  // 0: Biharmonic Chladni 2D Square Plate
  CHLADNI: [
    1.000, 2.083, 3.414, 3.892, 4.981, 5.964, 6.812, 8.245,
    9.172, 10.421, 11.834, 13.112, 14.653, 16.022, 17.581, 19.204
  ],
  // 1: Stiff Struck Beam / Marimba Bar (Euler-Bernoulli fn ~ (n + 0.5)^2)
  BEAM: [
    1.000, 2.756, 5.404, 8.933, 13.344, 18.637, 24.811, 31.867,
    39.805, 48.624, 58.326, 68.909, 80.374, 92.721, 105.950, 120.061
  ],
  // 2: Vocal Formant Tract ([a, e, i, o, u] resonant cavities)
  VOCAL: [
    1.000, 1.340, 1.820, 2.450, 2.910, 3.520, 4.100, 4.750,
    5.380, 6.050, 6.780, 7.550, 8.360, 9.220, 10.120, 11.080
  ],
  // 3: Poincaré Hyperbolic Negative-Curvature Horn
  HORN: [
    1.000, 1.414, 1.732, 2.000, 2.236, 2.449, 2.646, 2.828,
    3.000, 3.162, 3.317, 3.464, 3.606, 3.742, 3.873, 4.000
  ]
};

// Material Damping Factors and Physical Properties
export const MATERIAL_PROFILES = {
  // 0: Wood (Rosewood / Spruce) - rapid HF absorption, warm fundamental (~0.4s - 0.85s T60)
  WOOD: {
    stiffnessB: 0.0005,
    clusterDetune: 0.0,
    qScale: 0.30,
    hfDampExp: 1.6,
    baseLoss: 0.12,
    minQ: 12,
    maxQ: 70,
    modeWeights: [1.50, 1.50, 1.50, 1.50, 1.00, 0.75, 0.55, 0.40, 0.30, 0.22, 0.16, 0.12, 0.09, 0.07, 0.05, 0.04]
  },
  // 1: Glass (Quartz / Borosilicate) - pure crystal chime, sustained upper partials (~2.8s - 4.8s T60)
  GLASS: {
    stiffnessB: 0.0002,
    clusterDetune: 0.0,
    qScale: 2.35,
    hfDampExp: 0.45,
    baseLoss: 0.015,
    minQ: 80,
    maxQ: 480,
    modeWeights: [0.85, 0.90, 0.95, 1.00, 1.00, 1.35, 1.38, 1.41, 1.44, 1.47, 1.50, 1.53, 1.56, 1.59, 1.62, 1.65]
  },
  // 2: Steel (High-Carbon Spring Steel) - sharp inharmonic dispersion, metallic clang (~1.6s - 2.8s T60)
  STEEL: {
    stiffnessB: 0.024,
    clusterDetune: 0.0,
    qScale: 1.10,
    hfDampExp: 0.75,
    baseLoss: 0.038,
    minQ: 40,
    maxQ: 320,
    modeWeights: [1.00, 1.35, 1.35, 1.35, 1.35, 1.35, 1.35, 1.15, 1.00, 0.90, 0.85, 0.80, 0.80, 0.80, 0.80, 0.80]
  },
  // 3: Brass (Acoustic Bronze / Cymbal Brass) - warm rich bloom, phase beating shimmer (~2.0s - 3.5s T60)
  BRASS: {
    stiffnessB: 0.008,
    clusterDetune: 0.018,
    qScale: 1.35,
    hfDampExp: 0.85,
    baseLoss: 0.045,
    minQ: 45,
    maxQ: 350,
    modeWeights: [1.35, 1.35, 1.35, 1.35, 1.35, 1.20, 1.20, 1.20, 1.20, 1.20, 1.20, 1.05, 0.95, 0.85, 0.75, 0.70]
  },
  // 4: Nylon (Polymer) - rapid viscous dissipation, muted viscous thud (~0.15s - 0.35s T60)
  NYLON: {
    stiffnessB: 0.001,
    clusterDetune: 0.0,
    qScale: 0.12,
    hfDampExp: 2.0,
    baseLoss: 0.25,
    minQ: 6,
    maxQ: 28,
    modeWeights: [1.40, 1.00, 0.70, 0.45, 0.30, 0.20, 0.14, 0.10, 0.07, 0.05, 0.04, 0.03, 0.02, 0.02, 0.01, 0.01]
  }
};

/**
 * Generate 1024-point Float32Array Hermite / Tanh Soft Limiter transfer curve
 */
export function generateHermiteCurve(samples = 1024) {
  const curve = new Float32Array(samples);
  for (let i = 0; i < samples; i++) {
    const x = (i / (samples - 1)) * 4 - 2; // [-2, +2]
    const absX = Math.abs(x);
    if (absX <= 0.72) {
      curve[i] = x;
    } else if (absX < 1.05) {
      const u = (absX - 0.72) / 0.33;
      const y = 0.72 + 0.33 * (u * (1 + u * (1 - u)));
      curve[i] = Math.sign(x) * y;
    } else {
      curve[i] = Math.sign(x) * 1.05;
    }
  }
  return curve;
}

export class Mr16WebEngine {
  constructor() {
    this.ctx = null;
    this.isInitialized = false;
    this.isPowered = false;

    // Default Parameters mapped 1:1 with APVTS IDs
    this.params = {
      // Deck 01
      exciter_type: 0, // 0: Strike, 1: Friction, 2: Vactrol, 3: Ext In
      strike_hardness: 0.65,
      strike_velocity: 0.80,
      friction_force: 0.50,
      friction_speed: 0.40,
      vactrol_sag: 0.35,
      ext_input_gain: 0.0,
      poisson_density: 0.0,
      euclidean_pulses: 4,
      euclidean_steps: 16,

      // Deck 02
      manifold_type: 0, // 0: Chladni, 1: Beam, 2: Vocal, 3: Horn
      modal_frequency: 220.0,
      modal_damping: 1.80,
      material_profile: 2, // 0: Wood, 1: Glass, 2: Steel, 3: Brass, 4: Nylon
      modal_coupling: 0.40,
      modal_spread: 1.00,
      modal_q: 85.0,

      // Deck 03
      lorenz_rate: 0.85,
      lorenz_chaos: 0.50, // maps to rho in [12, 45]
      lorenz_freq_mod: 15.0,
      lorenz_q_mod: 20.0,
      lorenz_morph_alpha: 0.50,
      lorenz_morph_steer: 0.0,

      // Deck 04
      chorus_enable: true,
      chorus_rate_hz: 0.65,
      chorus_depth_ms: 1.40,
      chorus_dimension: 0.75,
      chorus_mix: 45.0,

      // Deck 05
      golden_pan_spread: 85.0,
      vactrol_lpg_cutoff: 14000.0,
      drive_saturation: 25.0,
      master_trim_db: 0.0,
      dry_wet_mix: 65.0,
      soft_limiter: true,
      power_state: false
    };

    // Telemetry storage
    this.modalEnergies = new Float32Array(16);
    this.modalFreqs = new Float32Array(16);
    this.timeData = new Float32Array(512);
    this.transientKick = 0;

    // Lorenz Attractor Differential State
    this.lorenzState = {
      x: 0.1,
      y: 0.0,
      z: 0.0,
      rate: 0.85,
      chaos: 28.0
    };

    // Autonomous timers
    this.poissonTimer = null;
    this.euclideanTimer = null;
    this.euclideanStepIndex = 0;
    this.frictionGainNode = null;
    this.frictionOsc = null;
    this.onPowerStateChanged = null;

    // Worklet vs Native fallback flag
    this.useWorklet = false;

    // 16-Pole Modal Profile Morpher
    this.morpher = new LorenzMorpher();
    this.customProfileActive = false;
    this.activeProfileSlot = 'A';
  }

  _ensureAudioReady() {
    if (!this.ctx) {
      this.init();
    }
    if (this.ctx && this.ctx.state === 'suspended' && typeof this.ctx.resume === 'function') {
      this.ctx.resume();
    }
    if (!this.isPowered) {
      this.setPower(true);
    }
    return Boolean(this.ctx);
  }

  async init(customCtx = null) {
    if (this.isInitialized && this.ctx) {
      if (this.ctx.state === 'suspended' && typeof this.ctx.resume === 'function') {
        await this.ctx.resume();
      }
      return;
    }

    if (customCtx) {
      this.ctx = customCtx;
    } else {
      const AudioContextClass = typeof window !== 'undefined'
        ? (window.AudioContext || window.webkitAudioContext)
        : globalThis.AudioContext;
      if (!AudioContextClass) {
        console.warn('Web Audio API not supported in this environment.');
        return;
      }

      this.ctx = new AudioContextClass({
        latencyHint: 'interactive',
        sampleRate: 48000
      });
    }

    this._buildAudioGraph();
    this._startLorenzModulationLoop();
    this.isInitialized = true;
  }

  _buildAudioGraph() {
    const ctx = this.ctx;

    // Master Bus & Output
    this.masterGain = ctx.createGain();
    this.masterGain.gain.setValueAtTime(this.isPowered ? 1.0 : 0.0, ctx.currentTime);

    this.analyser = ctx.createAnalyser();
    this.analyser.fftSize = 512;
    this.analyser.smoothingTimeConstant = 0.8;

    this.dcBlocker = ctx.createBiquadFilter();
    this.dcBlocker.type = 'highpass';
    this.dcBlocker.frequency.setValueAtTime(35, ctx.currentTime);
    this.dcBlocker.Q.setValueAtTime(0.707, ctx.currentTime);

    // Hermite Saturator
    this.saturator = ctx.createWaveShaper();
    this.saturator.curve = generateHermiteCurve(1024);
    this.saturator.oversample = '2x';

    // Buchla 292 Dynamic LPG Filter
    this.vactrolLpg = ctx.createBiquadFilter();
    this.vactrolLpg.type = 'lowpass';
    this.vactrolLpg.frequency.setValueAtTime(this.params.vactrol_lpg_cutoff, ctx.currentTime);
    this.vactrolLpg.Q.setValueAtTime(0.707, ctx.currentTime);

    // Summing & Routing
    this.wetGain = ctx.createGain();
    this.dryGain = ctx.createGain();
    this.exciterBus = ctx.createGain();
    this.resonatorBusL = ctx.createGain();
    this.resonatorBusR = ctx.createGain();
    this.resonatorBusL.gain.setValueAtTime(0.5, ctx.currentTime);
    this.resonatorBusR.gain.setValueAtTime(0.5, ctx.currentTime);

    // Connect Dynamics chain
    this.vactrolLpg.connect(this.saturator);
    this.saturator.connect(this.dcBlocker);
    this.dcBlocker.connect(this.masterGain);
    this.masterGain.connect(this.analyser);
    this.analyser.connect(ctx.destination);

    // --- Deck 02: 16-Pole Modal Resonator Matrix ---
    this.modes = [];
    this.modalGains = [];
    this.modalPanners = [];
    this.modeAnalysers = [];

    // Householder Reflection Bus & Acyclic Resonant Tail Regenerator
    // H = I - (2/16) * 1 * 1^T => feedback gain = -2/16 * coupling = -0.125 * coupling
    this.householderSummer = ctx.createGain();
    this.householderFeedback = ctx.createGain();
    const couplingVal = this.params.modal_coupling ?? 0.40;
    const normCoupling = couplingVal > 1.0 ? couplingVal / 100 : couplingVal;
    const hhGain = -0.125 * normCoupling;
    this.householderFeedback.gain.setValueAtTime(hhGain, ctx.currentTime);
    this.householderSummer.connect(this.householderFeedback);

    // Feedforward Modal Energy Scattering Bus from Exciter
    this.feedforwardScatter = ctx.createGain();
    this.feedforwardScatter.gain.setValueAtTime(normCoupling * 0.4, ctx.currentTime);
    this.exciterBus.connect(this.feedforwardScatter);

    // Acyclic Resonant Tail Regenerator (Decoupled from bp to eliminate Web Audio 128-sample cyclic phase cancellation)
    this.tailDiffuser1 = ctx.createDelay(0.05);
    this.tailDiffuser2 = ctx.createDelay(0.05);
    this.tailDiffuser1.delayTime.setValueAtTime(0.0137, ctx.currentTime);
    this.tailDiffuser2.delayTime.setValueAtTime(0.0193, ctx.currentTime);

    this.tailFilter = ctx.createBiquadFilter();
    this.tailFilter.type = 'lowpass';
    this.tailFilter.frequency.setValueAtTime(3600, ctx.currentTime);

    this.householderFeedback.connect(this.tailFilter);
    this.tailFilter.connect(this.tailDiffuser1);
    this.tailFilter.connect(this.tailDiffuser2);
    this.tailDiffuser1.connect(this.resonatorBusL);
    this.tailDiffuser2.connect(this.resonatorBusR);

    for (let i = 0; i < 16; i++) {
      const bp = ctx.createBiquadFilter();
      bp.type = 'bandpass';

      const gainNode = ctx.createGain();
      gainNode.gain.setValueAtTime(1.0, ctx.currentTime);

      // Golden ratio panning: theta_i = fmod(i * 137.507764 deg, 360) - 180
      let pan = 0;
      if (ctx.createStereoPanner) {
        const panner = ctx.createStereoPanner();
        const goldenDeg = (i * 137.507764) % 360;
        pan = Math.sin((goldenDeg * Math.PI) / 180) * (this.params.golden_pan_spread / 100);
        panner.pan.setValueAtTime(Math.max(-1, Math.min(1, pan)), ctx.currentTime);
        this.modalPanners.push(panner);

        bp.connect(gainNode);
        gainNode.connect(panner);

        const splitter = ctx.createChannelSplitter(2);
        panner.connect(splitter);
        splitter.connect(this.resonatorBusL, 0);
        splitter.connect(this.resonatorBusR, 1);
      } else {
        bp.connect(gainNode);
        gainNode.connect(this.resonatorBusL);
        gainNode.connect(this.resonatorBusR);
        this.modalPanners.push(null);
      }

      // Mode telemetry tap (analyser per mode for peak energy monitoring)
      const modeAnalyser = ctx.createAnalyser();
      modeAnalyser.fftSize = 64;
      gainNode.connect(modeAnalyser);
      this.modeAnalysers.push(modeAnalyser);

      // Householder summer collects all mode outputs for cross-modal scattering
      gainNode.connect(this.householderSummer);

      // Exciter bus feeds into all modal filters directly
      this.exciterBus.connect(bp);
      // Feedforward scattering connects to modes
      this.feedforwardScatter.connect(bp);

      this.modes.push(bp);
      this.modalGains.push(gainNode);
    }

    // --- Deck 04: Tri-Phase Spatial BBD Chorus ---
    this._buildChorusGraph();

    // Dry / Wet Routing
    this.exciterBus.connect(this.dryGain);
    this.dryGain.connect(this.vactrolLpg);
    this.wetGain.connect(this.vactrolLpg);

    // Initial Filter Tuning
    this.updateModalFrequencies(true);
    this.updateDynamics();

    // Continuous Stick-Slip Friction Noise Generator (Deck 01)
    this._buildFrictionExciter();
  }

  _buildChorusGraph() {
    const ctx = this.ctx;

    this.chorusInputL = ctx.createGain();
    this.chorusInputR = ctx.createGain();
    this.resonatorBusL.connect(this.chorusInputL);
    this.resonatorBusR.connect(this.chorusInputR);

    // 3 Modulated BBD Delay Lines
    this.delay1 = ctx.createDelay(0.05);
    this.delay2 = ctx.createDelay(0.05);
    this.delay3 = ctx.createDelay(0.05);

    this.baseDelaySec = 0.005; // 5.0 ms
    this.delay1.delayTime.setValueAtTime(this.baseDelaySec, ctx.currentTime);
    this.delay2.delayTime.setValueAtTime(this.baseDelaySec, ctx.currentTime);
    this.delay3.delayTime.setValueAtTime(this.baseDelaySec, ctx.currentTime);

    // Feed mono-sum into chorus delay taps
    const monoSum = ctx.createGain();
    monoSum.gain.setValueAtTime(0.5, ctx.currentTime);
    this.chorusInputL.connect(monoSum);
    this.chorusInputR.connect(monoSum);

    monoSum.connect(this.delay1);
    monoSum.connect(this.delay2);
    monoSum.connect(this.delay3);

    // Dimension Matrix Nodes:
    // Out_L = (d1 - d2) * scale
    // Out_R = (d2 - d3) * scale
    this.dimL_d1 = ctx.createGain();
    this.dimL_d2 = ctx.createGain();
    this.dimR_d2 = ctx.createGain();
    this.dimR_d3 = ctx.createGain();

    const dimVal = this.params.chorus_dimension ?? 75.0;
    const dimScale = (dimVal > 1.0 ? dimVal / 100 : dimVal) * 0.707;
    this.dimL_d1.gain.setValueAtTime(dimScale, ctx.currentTime);
    this.dimL_d2.gain.setValueAtTime(-dimScale, ctx.currentTime);
    this.dimR_d2.gain.setValueAtTime(dimScale, ctx.currentTime);
    this.dimR_d3.gain.setValueAtTime(-dimScale, ctx.currentTime);

    this.delay1.connect(this.dimL_d1);
    this.delay2.connect(this.dimL_d2);
    this.delay2.connect(this.dimR_d2);
    this.delay3.connect(this.dimR_d3);

    this.chorusOutL = ctx.createGain();
    this.chorusOutR = ctx.createGain();

    this.dimL_d1.connect(this.chorusOutL);
    this.dimL_d2.connect(this.chorusOutL);
    this.dimR_d2.connect(this.chorusOutR);
    this.dimR_d3.connect(this.chorusOutR);

    // Chorus dry/wet balance with proper stereo channel preservation
    this.chorusDryGain = ctx.createGain();
    this.chorusWetGain = ctx.createGain();

    const chorusMixNorm = this.params.chorus_enable ? (this.params.chorus_mix / 100) : 0;
    this.chorusDryGain.gain.setValueAtTime(Math.cos(chorusMixNorm * Math.PI * 0.5), ctx.currentTime);
    this.chorusWetGain.gain.setValueAtTime(Math.sin(chorusMixNorm * Math.PI * 0.5), ctx.currentTime);

    const dryMerger = ctx.createChannelMerger(2);
    this.chorusInputL.connect(dryMerger, 0, 0);
    this.chorusInputR.connect(dryMerger, 0, 1);
    dryMerger.connect(this.chorusDryGain);
    this.chorusDryGain.connect(this.wetGain);

    const wetMerger = ctx.createChannelMerger(2);
    this.chorusOutL.connect(wetMerger, 0, 0);
    this.chorusOutR.connect(wetMerger, 0, 1);
    wetMerger.connect(this.chorusWetGain);
    this.chorusWetGain.connect(this.wetGain);

    // Tri-phase LFO Modulation via requestAnimationFrame in JS
    this.chorusPhase = 0;
  }

  _buildFrictionExciter() {
    const ctx = this.ctx;
    // Pre-render 2-second looped pink noise buffer for stick-slip Karnopp friction
    const bufferSize = ctx.sampleRate * 2;
    const noiseBuffer = ctx.createBuffer(1, bufferSize, ctx.sampleRate);
    const output = noiseBuffer.getChannelData(0);

    let b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    for (let i = 0; i < bufferSize; i++) {
      const white = Math.random() * 2 - 1;
      b0 = 0.99886 * b0 + white * 0.0555179;
      b1 = 0.99332 * b1 + white * 0.0750759;
      b2 = 0.96900 * b2 + white * 0.1538520;
      b3 = 0.86650 * b3 + white * 0.3104856;
      b4 = 0.55000 * b4 + white * 0.5329522;
      b5 = -0.7616 * b5 - white * 0.0168980;
      output[i] = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362) * 0.11;
      b6 = white * 0.115926;
    }

    this.frictionNoiseSource = ctx.createBufferSource();
    this.frictionNoiseSource.buffer = noiseBuffer;
    this.frictionNoiseSource.loop = true;

    this.frictionFilter = ctx.createBiquadFilter();
    this.frictionFilter.type = 'bandpass';
    this.frictionFilter.frequency.setValueAtTime(1200, ctx.currentTime);
    this.frictionFilter.Q.setValueAtTime(1.5, ctx.currentTime);

    this.frictionGain = ctx.createGain();
    this.frictionGain.gain.setValueAtTime(0.0, ctx.currentTime); // default silent

    this.frictionNoiseSource.connect(this.frictionFilter);
    this.frictionFilter.connect(this.frictionGain);
    this.frictionGain.connect(this.exciterBus);

    try {
      this.frictionNoiseSource.start();
    } catch (_) {}
  }

  // --- Parameter Update & Synchronization ---
  setParam(id, val) {
    if (this.params[id] === undefined) return;
    this.params[id] = val;

    if (!this.isInitialized || !this.ctx) return;
    const ctx = this.ctx;
    const now = ctx.currentTime;

    switch (id) {
      case 'manifold_type':
      case 'modal_frequency':
      case 'modal_spread':
        this.updateModalFrequencies();
        break;

      case 'material_profile':
        this.updateModalFrequencies();
        this.updateModalDamping();
        break;

      case 'modal_damping':
      case 'modal_q':
        this.updateModalDamping();
        break;

      case 'lorenz_morph_alpha':
      case 'lorenz_morph_steer':
        this.updateMorph();
        break;

      case 'modal_coupling': {
        const norm = val > 1.0 ? val / 100 : val;
        const hh = -0.125 * norm;
        this.householderFeedback.gain.setTargetAtTime(hh, now, 0.03);
        if (this.feedforwardScatter) {
          this.feedforwardScatter.gain.setTargetAtTime(norm * 0.4, now, 0.03);
        }
        break;
      }

      case 'golden_pan_spread':
        this.updatePanners();
        break;

      case 'vactrol_lpg_cutoff':
        this.vactrolLpg.frequency.setTargetAtTime(Math.max(20, Math.min(20000, val)), now, 0.02);
        break;

      case 'master_trim_db': {
        const gainLinear = Math.pow(10, val / 20);
        this.masterGain.gain.setTargetAtTime(gainLinear, now, 0.02);
        break;
      }

      case 'dry_wet_mix': {
        const norm = val / 100;
        this.dryGain.gain.setTargetAtTime(Math.cos(norm * Math.PI * 0.5) * 0.7, now, 0.02);
        this.wetGain.gain.setTargetAtTime(Math.sin(norm * Math.PI * 0.5), now, 0.02);
        break;
      }

      case 'chorus_enable':
      case 'chorus_mix':
      case 'chorus_dimension':
        this.updateChorusParams();
        break;

      case 'exciter_type':
      case 'friction_force':
      case 'friction_speed':
        this.updateFrictionParams();
        break;

      case 'poisson_density':
        this.setPoissonDensity(val);
        break;

      case 'power_state':
        this.setPower(Boolean(val));
        break;
    }
  }

  updateModalFrequencies(immediate = false) {
    if (!this.ctx || !this.modes.length) return;
    const now = this.ctx.currentTime;
    const f0 = this.params.modal_frequency;
    const spread = this.params.modal_spread;

    // Manifold ratio table
    let ratios;
    switch (this.params.manifold_type) {
      case 1: ratios = MANIFOLD_RATIOS.BEAM; break;
      case 2: ratios = MANIFOLD_RATIOS.VOCAL; break;
      case 3: ratios = MANIFOLD_RATIOS.HORN; break;
      default: ratios = MANIFOLD_RATIOS.CHLADNI; break;
    }

    let profile;
    switch (this.params.material_profile) {
      case 0: profile = MATERIAL_PROFILES.WOOD; break;
      case 1: profile = MATERIAL_PROFILES.GLASS; break;
      case 3: profile = MATERIAL_PROFILES.BRASS; break;
      case 4: profile = MATERIAL_PROFILES.NYLON; break;
      default: profile = MATERIAL_PROFILES.STEEL; break;
    }

    const stiffnessB = profile ? (profile.stiffnessB || 0.0) : 0.0;
    const clusterDetune = profile ? (profile.clusterDetune || 0.0) : 0.0;

    for (let i = 0; i < 16; i++) {
      const rm = ratios[i] || (1 + i);
      // Inharmonic stiffness dispersion and cluster beating doublet splitting:
      // r_m' = r_m * sqrt(1 + B * m^2) * [1 + D * sin(m * pi / 2.5)]
      const dispersion = Math.sqrt(1.0 + stiffnessB * i * i);
      const beating = 1.0 + clusterDetune * Math.sin((i * Math.PI) / 2.5);
      const rmPrime = rm * dispersion * beating;

      const fm = f0 * (1 + (rmPrime - 1) * spread);
      const maxNyquist = (this.ctx && this.ctx.sampleRate) ? (this.ctx.sampleRate * 0.49) : 22000;
      const clampedFm = Math.max(20, Math.min(maxNyquist, fm));
      this.modalFreqs[i] = clampedFm;
      if (immediate) {
        this.modes[i].frequency.setValueAtTime(clampedFm, now);
      } else {
        this.modes[i].frequency.setTargetAtTime(clampedFm, now, 0.03);
      }
    }
    this.updateModalDamping(immediate);
  }

  updateModalDamping(immediate = false) {
    if (!this.ctx || !this.modes.length) return;
    const now = this.ctx.currentTime;
    const baseDamping = this.params.modal_damping; // seconds
    const userQ = this.params.modal_q || 85.0;

    let profile;
    switch (this.params.material_profile) {
      case 0: profile = MATERIAL_PROFILES.WOOD; break;
      case 1: profile = MATERIAL_PROFILES.GLASS; break;
      case 3: profile = MATERIAL_PROFILES.BRASS; break;
      case 4: profile = MATERIAL_PROFILES.NYLON; break;
      default: profile = MATERIAL_PROFILES.STEEL; break;
    }

    const modeWeights = profile?.modeWeights || [];
    const fs = (this.ctx && this.ctx.sampleRate) ? this.ctx.sampleRate : 48000;

    for (let i = 0; i < 16; i++) {
      const fm = this.modalFreqs[i] || 220;
      // Loss tangent curve: kappa_i = (1 + baseLoss * i^hfDampExp)^(-1)
      const kappa = 1 / (1 + profile.baseLoss * Math.pow(i, profile.hfDampExp));
      const tau = Math.max(0.02, baseDamping * kappa);
      // Q = pi * fm * tau / ln(1000) ~ 0.455 * fm * tau
      const qCalc = 0.455 * fm * tau * profile.qScale * (userQ / 85.0);
      const clampedQ = Math.max(profile.minQ, Math.min(profile.maxQ, qCalc));

      const weight = modeWeights[i] !== undefined ? modeWeights[i] : 1.0;

      // Equalized octave loudness curve:
      // Bandpass peak response is proportional to fm/fs; compensating with (fm/220)^0.58 balances octaves
      const makeupGain = Math.min(
        32.0,
        3.2 * Math.sqrt((clampedQ * fs) / Math.max(20.0, fm)) *
          Math.pow(Math.max(100.0, fm) / 220.0, 0.58)
      ) * weight;

      if (immediate) {
        this.modes[i].Q.setValueAtTime(clampedQ, now);
        if (this.modalGains[i]) {
          this.modalGains[i].gain.setValueAtTime(makeupGain, now);
        }
      } else {
        this.modes[i].Q.setTargetAtTime(clampedQ, now, 0.04);
        if (this.modalGains[i]) {
          this.modalGains[i].gain.setTargetAtTime(makeupGain, now, 0.04);
        }
      }
    }
  }

  // --- Custom Modal Profile & Lorenz Morpher Integration ---
  loadModalProfile(profile, slot = 'A') {
    if (!this.morpher) {
      this.morpher = new LorenzMorpher();
    }
    if (slot === 'B') {
      this.morpher.setProfileB(profile);
    } else {
      this.morpher.setProfileA(profile);
    }
    this.customProfileActive = true;
    if (this.ctx && this.modes.length) {
      this.updateMorph();
    }
  }

  setCustomProfileActive(active) {
    this.customProfileActive = Boolean(active);
    if (!this.customProfileActive) {
      this.updateModalFrequencies();
      this.updateModalDamping();
    } else {
      this.updateMorph();
    }
  }

  updateMorph() {
    if (!this.ctx || !this.modes.length || !this.morpher) return;
    const morphed = this.morpher.morph(
      this.params.lorenz_morph_alpha !== undefined ? this.params.lorenz_morph_alpha : 0.5,
      this.lorenzState,
      this.params.lorenz_morph_steer !== undefined ? this.params.lorenz_morph_steer : 0.0
    );
    const now = this.ctx.currentTime;
    for (let i = 0; i < 16; i++) {
      if (this.modes[i]) {
        this.modalFreqs[i] = morphed.frequencies[i];
        this.modes[i].frequency.setTargetAtTime(morphed.frequencies[i], now, 0.03);
        this.modes[i].Q.setTargetAtTime(morphed.qFactors[i], now, 0.03);
      }
      if (this.modalGains[i]) {
        this.modalGains[i].gain.setTargetAtTime(morphed.gains[i], now, 0.03);
      }
      if (this.modalPanners[i]) {
        const spread = (this.params.golden_pan_spread || 85.0) / 100.0;
        this.modalPanners[i].pan.setTargetAtTime(
          Math.max(-1.0, Math.min(1.0, morphed.panning[i] * spread)),
          now,
          0.02
        );
      }
    }
  }

  updatePanners() {
    if (!this.ctx || !this.modalPanners.length) return;
    const now = this.ctx.currentTime;
    const spread = this.params.golden_pan_spread / 100;

    for (let i = 0; i < 16; i++) {
      if (this.modalPanners[i]) {
        const goldenDeg = (i * 137.507764) % 360;
        const pan = Math.sin((goldenDeg * Math.PI) / 180) * spread;
        this.modalPanners[i].pan.setTargetAtTime(Math.max(-1, Math.min(1, pan)), now, 0.02);
      }
    }
  }

  updateChorusParams() {
    if (!this.ctx || !this.chorusDryGain) return;
    const now = this.ctx.currentTime;
    const enabled = Boolean(this.params.chorus_enable);
    const mixNorm = enabled ? (this.params.chorus_mix / 100) : 0;
    const dimVal = this.params.chorus_dimension ?? 75.0;
    const dimNorm = (dimVal > 1.0 ? dimVal / 100 : dimVal) * 0.707;

    this.chorusDryGain.gain.setTargetAtTime(Math.cos(mixNorm * Math.PI * 0.5), now, 0.02);
    this.chorusWetGain.gain.setTargetAtTime(Math.sin(mixNorm * Math.PI * 0.5), now, 0.02);

    if (this.dimL_d1) {
      this.dimL_d1.gain.setTargetAtTime(dimNorm, now, 0.02);
      this.dimL_d2.gain.setTargetAtTime(-dimNorm, now, 0.02);
      this.dimR_d2.gain.setTargetAtTime(dimNorm, now, 0.02);
      this.dimR_d3.gain.setTargetAtTime(-dimNorm, now, 0.02);
    }
  }

  updateFrictionParams() {
    if (!this.ctx || !this.frictionGain) return;
    const now = this.ctx.currentTime;
    const force = this.params.friction_force;
    const speed = this.params.friction_speed;

    // Continuous friction output if exciter is in Friction mode (1) and force/speed > 0
    if (this.params.exciter_type === 1 && (force > 0.05 && speed > 0.05)) {
      const level = Math.min(0.8, force * speed * 0.9);
      this.frictionGain.gain.setTargetAtTime(level, now, 0.05);
      const filterFreq = 400 + speed * 3500;
      this.frictionFilter.frequency.setTargetAtTime(filterFreq, now, 0.05);
    } else {
      this.frictionGain.gain.setTargetAtTime(0.0, now, 0.05);
    }
  }

  updateDynamics() {
    if (!this.ctx) return;
    const now = this.ctx.currentTime;
    const dryWetNorm = this.params.dry_wet_mix / 100;
    this.dryGain.gain.setTargetAtTime(Math.cos(dryWetNorm * Math.PI * 0.5) * 0.6, now, 0.02);
    this.wetGain.gain.setTargetAtTime(Math.sin(dryWetNorm * Math.PI * 0.5), now, 0.02);
  }

  setPower(isPowered) {
    this.isPowered = Boolean(isPowered);
    this.params.power_state = this.isPowered;
    if (!this.ctx) return;
    const now = this.ctx.currentTime;
    if (!this.isPowered) {
      this.masterGain.gain.setTargetAtTime(0.0, now, 0.05);
      this.stopPoissonRain();
    } else {
      if (this.ctx.state === 'suspended') {
        this.ctx.resume();
      }
      const gainLinear = Math.pow(10, this.params.master_trim_db / 20);
      this.masterGain.gain.setTargetAtTime(gainLinear, now, 0.05);
      if (this.params.poisson_density > 0) {
        this.setPoissonDensity(this.params.poisson_density);
      }
    }
    if (typeof this.onPowerStateChanged === 'function') {
      this.onPowerStateChanged(this.isPowered);
    }
  }

  // --- Deck 03: Chaotic Lorenz Modulation & Tri-Phase Chorus Sweep Loop ---
  _startLorenzModulationLoop() {
    let lastTime = performance.now();

    const loop = (nowTime) => {
      const dt = Math.min(0.05, (nowTime - lastTime) / 1000);
      lastTime = nowTime;

      if (this.ctx && this.isPowered) {
        this._stepLorenz(dt);
        this._stepChorusLfo(dt);
        this._collectTelemetry();
      }

      if (typeof requestAnimationFrame === 'function') {
        requestAnimationFrame(loop);
      }
    };

    if (typeof requestAnimationFrame === 'function') {
      requestAnimationFrame(loop);
    }
  }

  _stepLorenz(dt) {
    const sigma = 10.0;
    const rho = 12.0 + (this.params.lorenz_chaos || 0.5) * 33.0; // [12, 45]
    const beta = 8.0 / 3.0;
    const speed = (this.params.lorenz_rate || 0.85) * 1.5;

    let x = this.lorenzState.x;
    let y = this.lorenzState.y;
    let z = this.lorenzState.z;

    const dx = sigma * (y - x);
    const dy = x * (rho - z) - y;
    const dz = x * y - beta * z;

    x += dx * dt * speed;
    y += dy * dt * speed;
    z += dz * dt * speed;

    // Anti-blowup safety guard: check radius ||u|| > 120.0 (r2 > 14400.0)
    const r2 = x * x + y * y + z * z;
    if (!Number.isFinite(r2) || r2 > 14400.0) {
      x = 0.1;
      y = 0.0;
      z = 0.0;
    }

    this.lorenzState.x = x;
    this.lorenzState.y = y;
    this.lorenzState.z = z;
    this.lorenzState.rate = this.params.lorenz_rate;
    this.lorenzState.chaos = rho;

    // Normalize Lorenz variables for modulation
    const normX = Math.max(-1, Math.min(1, x / 20.0));
    const normY = Math.max(-1, Math.min(1, y / 28.0));
    const normZ = Math.max(-1, Math.min(1, (z - 25.0) / 25.0));

    // Dynamic modulation of modal frequencies & Q
    const freqModDepth = (this.params.lorenz_freq_mod / 100) * 0.025; // +/- 2.5%
    const qModDepth = (this.params.lorenz_q_mod / 100) * 0.35; // +/- 35%

    const curTime = this.ctx.currentTime;
    const maxNyquist = (this.ctx && this.ctx.sampleRate) ? (this.ctx.sampleRate * 0.49) : 22000;

    if (this.customProfileActive && this.morpher) {
      const morphed = this.morpher.morph(
        this.params.lorenz_morph_alpha !== undefined ? this.params.lorenz_morph_alpha : 0.5,
        this.lorenzState,
        this.params.lorenz_morph_steer !== undefined ? this.params.lorenz_morph_steer : 0.0
      );
      for (let i = 0; i < 16; i++) {
        if (this.modes[i]) {
          const safeF = Math.max(20, Math.min(maxNyquist, morphed.frequencies[i]));
          this.modalFreqs[i] = safeF;
          this.modes[i].frequency.setTargetAtTime(safeF, curTime, 0.03);
          this.modes[i].Q.setTargetAtTime(morphed.qFactors[i], curTime, 0.03);
        }
        if (this.modalGains[i]) {
          this.modalGains[i].gain.setTargetAtTime(morphed.gains[i], curTime, 0.03);
        }
        if (this.modalPanners[i]) {
          const spread = (this.params.golden_pan_spread || 85.0) / 100.0;
          this.modalPanners[i].pan.setTargetAtTime(
            Math.max(-1.0, Math.min(1.0, morphed.panning[i] * spread)),
            curTime,
            0.02
          );
        }
      }
      return;
    }

    for (let i = 0; i < 16; i++) {
      if (this.modes[i]) {
        const baseF = this.modalFreqs[i] || 220;
        const modF = (i % 2 === 0) ? (baseF * (1 + normX * freqModDepth)) : (baseF * (1 + normY * freqModDepth));
        const safeModF = Math.max(20, Math.min(maxNyquist, modF));
        this.modes[i].frequency.setTargetAtTime(safeModF, curTime, 0.03);
      }
    }
  }

  _stepChorusLfo(dt) {
    if (!this.params.chorus_enable) return;

    const rateHz = this.params.chorus_rate_hz || 0.65;
    const depthSec = ((this.params.chorus_depth_ms || 1.40) / 1000) * 0.8;

    this.chorusPhase = (this.chorusPhase + 2 * Math.PI * rateHz * dt) % (2 * Math.PI);

    // Tri-phase offsets: 0 deg, 120 deg (2pi/3), 240 deg (4pi/3)
    const p1 = this.chorusPhase;
    const p2 = this.chorusPhase + (2 * Math.PI) / 3;
    const p3 = this.chorusPhase + (4 * Math.PI) / 3;

    const t1 = this.baseDelaySec + depthSec * Math.sin(p1);
    const t2 = this.baseDelaySec + depthSec * Math.sin(p2);
    const t3 = this.baseDelaySec + depthSec * Math.sin(p3);

    const curTime = this.ctx.currentTime;
    if (this.delay1) this.delay1.delayTime.setTargetAtTime(Math.max(0.0005, t1), curTime, 0.01);
    if (this.delay2) this.delay2.delayTime.setTargetAtTime(Math.max(0.0005, t2), curTime, 0.01);
    if (this.delay3) this.delay3.delayTime.setTargetAtTime(Math.max(0.0005, t3), curTime, 0.01);
  }

  _collectTelemetry() {
    // Mode energies for CRT Modal FFT & Chladni figure
    const scratch = new Uint8Array(64);
    for (let i = 0; i < 16; i++) {
      if (this.modeAnalysers[i]) {
        this.modeAnalysers[i].getByteTimeDomainData(scratch);
        let peak = 0;
        for (let j = 0; j < 64; j++) {
          const val = Math.abs(scratch[j] - 128) / 128;
          if (val > peak) peak = val;
        }
        // Smooth peak decay
        this.modalEnergies[i] = Math.max(peak, this.modalEnergies[i] * 0.92);
      }
    }

    // Time domain data for CRT
    if (this.analyser) {
      this.analyser.getFloatTimeDomainData(this.timeData);
    }
  }

  getTelemetry() {
    const kick = this.transientKick;
    this.transientKick = 0;
    return {
      modalEnergies: this.modalEnergies,
      modalFreqs: this.modalFreqs,
      lorenzState: this.lorenzState,
      timeData: this.timeData,
      transientKick: kick
    };
  }

  // --- Deck 01: Tactile Performance Excitations ---

  /**
   * Mass-Spring Viscoelastic Contact Strike
   * Generates a true broadband viscoelastic contact impulse (steep asymmetric attack with
   * exponential decay tail, acoustic dipole differentiation, and bandlimited noise click)
   * that simultaneously excites all 16 inharmonic Chladni / Beam / Vocal / Horn modes.
   */
  triggerStrike(hardness = null, velocity = null, pitchHz = null) {
    this._ensureAudioReady();
    if (!this.ctx) return;
    const ctx = this.ctx;
    const now = ctx.currentTime;

    const h = Math.max(0.0, Math.min(1.0, hardness ?? this.params.strike_hardness));
    const v = Math.max(0.01, Math.min(1.0, velocity ?? this.params.strike_velocity));

    if (pitchHz && pitchHz >= 20.0 && Math.abs(this.params.modal_frequency - pitchHz) > 0.05) {
      this.params.modal_frequency = pitchHz;
      this.updateModalFrequencies(true);
    }

    // Viscoelastic contact parameters:
    // High hardness -> short contact time (crisp click), low hardness -> longer contact time (felt mallet)
    const contactDurationSec = 0.0005 + Math.pow(1.0 - h, 1.5) * 0.0025;
    const numSamples = Math.max(48, Math.ceil(contactDurationSec * 3.0 * ctx.sampleRate));
    const buffer = ctx.createBuffer(1, numSamples, ctx.sampleRate);
    const data = buffer.getChannelData(0);

    const riseSec = Math.max(0.00008, contactDurationSec * 0.22);
    const decaySec = Math.max(0.00015, (contactDurationSec - riseSec) / 2.5);

    // Hertzian stiffness peak scaling: hard mallet delivers higher peak force
    const stiffnessAmp = 0.60 + 0.40 * Math.pow(h, 0.6);

    let prevSample = 0.0;
    for (let i = 0; i < numSamples; i++) {
      const t = i / ctx.sampleRate;
      let fContact = 0.0;
      if (t < riseSec) {
        fContact = Math.pow(Math.sin((t / riseSec) * Math.PI * 0.5), 1.8);
      } else {
        fContact = Math.exp(-(t - riseSec) / decaySec);
      }

      // Bandlimited surface micro-contact click
      const ramp = Math.min(1.0, t / 0.0002);
      const fClick = (Math.random() * 2 - 1) * Math.exp(-t / 0.0006) * (0.10 + 0.50 * h) * ramp;
      const rawForce = (fContact + fClick) * stiffnessAmp;

      // Acoustic pressure dipole differentiation: dF/dt (removes DC, yields pure broadband wave)
      const dipoleSample = rawForce - 0.90 * prevSample;
      prevSample = rawForce;
      data[i] = dipoleSample;
    }

    const source = ctx.createBufferSource();
    source.buffer = buffer;

    // Hardness filter shaping for contact stiffness
    const filter = ctx.createBiquadFilter();
    filter.type = 'lowpass';
    const cutoff = 1200 + Math.pow(h, 1.8) * 18800; // 1.2 kHz to 20 kHz
    filter.frequency.setValueAtTime(cutoff, now);
    filter.Q.setValueAtTime(0.7, now);

    const gain = ctx.createGain();
    // Acoustic velocity curve: maintains punch across full dynamic range
    const peakAmp = (0.55 + 0.45 * Math.pow(v, 1.0)) * 13.0;
    gain.gain.setValueAtTime(peakAmp, now);

    source.connect(filter);
    filter.connect(gain);
    gain.connect(this.exciterBus);

    source.start(now);
    this.transientKick = v;
  }

  /**
   * Dirac Single-Sample Mathematical Impulse
   * Injects an ultra-broadband bandlimited unit impulse doublet into exciterBus.
   */
  triggerDirac() {
    this._ensureAudioReady();
    if (!this.ctx) return;
    const ctx = this.ctx;
    const buffer = ctx.createBuffer(1, 32, ctx.sampleRate);
    const data = buffer.getChannelData(0);
    // Bandlimited mathematical Dirac doublet with zero DC
    data[0] = 1.0;
    data[1] = -0.7;
    data[2] = 0.25;
    data[3] = -0.08;
    data[4] = 0.02;

    const source = ctx.createBufferSource();
    source.buffer = buffer;
    const gain = ctx.createGain();
    gain.gain.setValueAtTime(12.0, ctx.currentTime);

    source.connect(gain);
    gain.connect(this.exciterBus);
    source.start();

    this.transientKick = 1.0;
  }

  /**
   * Felt Mallet Heavy Impact
   * Combines low-frequency core compression with broadband felt surface transient.
   */
  triggerFeltHammer() {
    this._ensureAudioReady();
    if (!this.ctx) return;
    const ctx = this.ctx;
    const now = ctx.currentTime;

    const numSamples = Math.floor(ctx.sampleRate * 0.035);
    const buffer = ctx.createBuffer(1, numSamples, ctx.sampleRate);
    const data = buffer.getChannelData(0);
    const riseSec = 0.0022;
    const decaySec = 0.011;

    for (let i = 0; i < numSamples; i++) {
      const t = i / ctx.sampleRate;
      let env = 0.0;
      if (t < riseSec) {
        env = Math.sin((t / riseSec) * Math.PI * 0.5);
      } else {
        env = Math.exp(-(t - riseSec) / decaySec);
      }
      const f0 = this.params.modal_frequency || 220;
      const thud = Math.sin(2 * Math.PI * f0 * t);
      const texture = (Math.random() * 2 - 1) * 0.22 * Math.exp(-t / 0.004);
      data[i] = (env * 0.85 * thud + texture * env);
    }

    const source = ctx.createBufferSource();
    source.buffer = buffer;

    const filter = ctx.createBiquadFilter();
    filter.type = 'lowpass';
    filter.frequency.setValueAtTime(2200, now);
    filter.Q.setValueAtTime(1.0, now);

    const gain = ctx.createGain();
    gain.gain.setValueAtTime(0.35, now);

    source.connect(filter);
    filter.connect(gain);
    gain.connect(this.exciterBus);

    source.start(now);
    this.transientKick = 0.9;
  }

  /**
   * Bowed Friction Burst (stick-slip scrape)
   * Injects wide broadband stick-slip friction energy that continuously feeds all 16 modes.
   */
  triggerBowedFriction(durationSec = 0.6) {
    this._ensureAudioReady();
    if (!this.ctx) return;
    const ctx = this.ctx;
    const now = ctx.currentTime;

    const bufferSize = Math.floor(ctx.sampleRate * durationSec);
    const noiseBuffer = ctx.createBuffer(1, bufferSize, ctx.sampleRate);
    const data = noiseBuffer.getChannelData(0);

    const slipFreq = Math.max(80, Math.min(500, this.params.modal_frequency * 0.5));
    const slipPeriodSamples = Math.floor(ctx.sampleRate / slipFreq);

    for (let i = 0; i < bufferSize; i++) {
      const phaseInPeriod = (i % slipPeriodSamples) / slipPeriodSamples;
      const stickSlip = phaseInPeriod < 0.85 ? (phaseInPeriod / 0.85) : (1.0 - (phaseInPeriod - 0.85) / 0.15 * 2.0);
      const grit = (Math.random() * 2 - 1) * 0.4;
      data[i] = stickSlip * 0.65 + grit;
    }

    const source = ctx.createBufferSource();
    source.buffer = noiseBuffer;

    const filter = ctx.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.setValueAtTime(Math.max(400, Math.min(3600, this.params.modal_frequency * 1.5)), now);
    filter.Q.setValueAtTime(0.7, now);

    const gain = ctx.createGain();
    gain.gain.setValueAtTime(0.01, now);
    gain.gain.linearRampToValueAtTime(0.28, now + 0.08);
    gain.gain.setValueAtTime(0.26, now + durationSec * 0.7);
    gain.gain.exponentialRampToValueAtTime(0.001, now + durationSec);

    source.connect(filter);
    filter.connect(gain);
    gain.connect(this.exciterBus);

    source.start(now);
    source.stop(now + durationSec + 0.01);

    this.transientKick = 0.6;
  }

  /**
   * Optical Air Jet Turbulence Burst with Vactrol Sag
   * Injects rich broadband pink turbulence across all 16 modes with dynamic lowpass decay.
   */
  triggerAirJet(durationSec = 0.45) {
    this._ensureAudioReady();
    if (!this.ctx) return;
    const ctx = this.ctx;
    const now = ctx.currentTime;

    const bufferSize = Math.floor(ctx.sampleRate * durationSec);
    const noiseBuffer = ctx.createBuffer(1, bufferSize, ctx.sampleRate);
    const data = noiseBuffer.getChannelData(0);

    let b0 = 0, b1 = 0, b2 = 0;
    for (let i = 0; i < bufferSize; i++) {
      const white = Math.random() * 2 - 1;
      b0 = 0.99765 * b0 + white * 0.0990460;
      b1 = 0.96300 * b1 + white * 0.2965164;
      b2 = 0.57000 * b2 + white * 1.0526913;
      data[i] = (b0 + b1 + b2 + white * 0.1848) * 0.4;
    }

    const source = ctx.createBufferSource();
    source.buffer = noiseBuffer;

    const filter = ctx.createBiquadFilter();
    filter.type = 'lowpass';
    filter.frequency.setValueAtTime(4200, now);
    filter.frequency.exponentialRampToValueAtTime(350, now + durationSec);

    const gain = ctx.createGain();
    gain.gain.setValueAtTime(0.01, now);
    gain.gain.linearRampToValueAtTime(0.13, now + 0.02);
    gain.gain.exponentialRampToValueAtTime(0.045, now + 0.10);
    gain.gain.exponentialRampToValueAtTime(0.0001, now + durationSec);

    source.connect(filter);
    filter.connect(gain);
    gain.connect(this.exciterBus);

    source.start(now);
    source.stop(now + durationSec + 0.01);

    this.transientKick = 0.8;
  }

  /**
   * Play Microtonal Chime Key
   * Tunes resonator fundamental to key frequency and strikes with viscoelastic contact impulse.
   */
  playChimeKey(pitchHz = 220, velocity = 0.8, hardness = null) {
    this._ensureAudioReady();
    if (!this.ctx) return;
    let targetPitch = pitchHz;
    if (typeof targetPitch === 'number' && targetPitch < 20) {
      targetPitch = 220 * Math.pow(2, targetPitch / 12);
    }
    this.triggerStrike(hardness, velocity, targetPitch);
  }

  // --- Autonomous Trigger Engines ---

  /**
   * Poisson Stochastic Rain: delta_t = -ln(1 - U) / lambda
   */
  setPoissonDensity(densityHz) {
    this.stopPoissonRain();
    if (densityHz <= 0.05) return;

    const scheduleNext = () => {
      if (!this.isPowered || this.params.poisson_density <= 0.05) return;
      const u = Math.random();
      const intervalSec = -Math.log(Math.max(0.0001, 1 - u)) / this.params.poisson_density;
      const delayMs = Math.max(10, Math.min(5000, intervalSec * 1000));

      this.poissonTimer = setTimeout(() => {
        const vel = 0.3 + Math.random() * 0.6;
        const hardness = 0.4 + Math.random() * 0.5;
        this.triggerStrike(hardness, vel);
        scheduleNext();
      }, delayMs);
    };

    scheduleNext();
  }

  stopPoissonRain() {
    if (this.poissonTimer) {
      clearTimeout(this.poissonTimer);
      this.poissonTimer = null;
    }
  }

  /**
   * Euclidean Polyrhythm Clock E(k, n)
   */
  setEuclidean(pulses, steps, bpm = 120) {
    this.stopEuclidean();
    const k = Math.max(1, Math.min(steps, pulses));
    const n = Math.max(2, steps);

    // Bjorklund Euclidean rhythm generator
    const pattern = new Array(n).fill(false);
    for (let i = 0; i < k; i++) {
      pattern[Math.floor((i * n) / k)] = true;
    }

    const stepIntervalMs = (60000 / bpm) / 4; // 16th note subdivisions
    this.euclideanStepIndex = 0;

    this.euclideanTimer = setInterval(() => {
      if (!this.isPowered) return;
      if (pattern[this.euclideanStepIndex]) {
        this.triggerStrike(0.75, 0.85);
      }
      this.euclideanStepIndex = (this.euclideanStepIndex + 1) % n;
    }, stepIntervalMs);
  }

  stopEuclidean() {
    if (this.euclideanTimer) {
      clearInterval(this.euclideanTimer);
      this.euclideanTimer = null;
    }
  }
}
