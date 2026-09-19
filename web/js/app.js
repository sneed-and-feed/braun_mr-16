/**
 * @file app.js
 * @brief Application Coordinator & Master Controller for BRAUN MR-16 Web Showcase
 * Features:
 * - AudioContext Lifecycle Guard & Unlock on first user interaction
 * - 16 Curated Factory Presets + LocalStorage User Presets
 * - RFC 8259 JSON Patch Export, Load & Drag-and-Drop Chassis Dropzone
 * - Instant A/B Parameter Comparison Buffer with Copy Utilities
 * - Lossless 16-bit 48kHz WAV Master Bus Recorder with Auto-Download
 * - Complete 2-Way Binding for all 25 precision rotary knobs and segmented buttons
 * - Audition Sound Bar & 4 Machined Strike Triggers
 * - 16-Key Microtonal Chime Performance Strip with Velocity, Scales, and Hotkeys
 * - 60 FPS Vector CRT Oscilloscope Telemetry Bridge
 * - Dieter Rams functionalist laboratory aesthetics (100% Technical English, zero emojis)
 */

import { BraunKnob } from './ui/knob.js';
import { BraunCrtDisplay } from './ui/crt-display.js';
import { Mr16WebEngine } from './audio/mr16_web_engine.js';
import { ModalExtractor } from './audio/modal_extractor.js';
import { LorenzMorpher, createDefaultProfile } from './audio/lorenz_morpher.js';

// --- Modal Scales for Microtonal Performance Strip ---
export const SCALES = {
  BUDD_PENTATONIC: { name: 'BUDD PENTATONIC', intervals: [0, 2, 4, 7, 9] },
  LYDIAN_DREAM: { name: 'LYDIAN DREAM', intervals: [0, 2, 4, 6, 7, 9, 11] },
  DORIAN_MYSTIC: { name: 'DORIAN MYSTIC', intervals: [0, 2, 3, 5, 7, 9, 10] },
  YOSHIMURA_AMBIENT: { name: 'YOSHIMURA AMBIENT', intervals: [0, 2, 5, 7, 9] },
  AEOLIAN_MIDNIGHT: { name: 'AEOLIAN MIDNIGHT', intervals: [0, 2, 3, 5, 7, 8, 10] },
  PYTHAGOREAN_JUST: { name: 'PYTHAGOREAN JUST', intervals: [0, 2, 4, 5, 7, 9, 11] }
};

export const ROOT_FREQS = {
  'A': 220.00,
  'C': 130.81,
  'D': 146.83,
  'E': 164.81,
  'F': 174.61,
  'G': 196.00
};

export const CHIME_HOTKEYS = ['a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', "'", 'z', 'x', 'c', 'v', 'b'];

/**
 * Lossless 16-bit 48kHz PCM Stereo WAV Recorder
 */
export class MasterWavRecorder {
  constructor(engine) {
    this.engine = engine;
    this.isRecording = false;
    this.recordingStartTime = 0;
    this.leftChannelData = [];
    this.rightChannelData = [];
    this.scriptNode = null;
  }

  start() {
    if (this.isRecording || !this.engine.ctx) return;
    this.isRecording = true;
    this.recordingStartTime = Date.now();
    this.leftChannelData = [];
    this.rightChannelData = [];

    const ctx = this.engine.ctx;
    // Buffer size 4096 gives ~85 ms chunks at 48kHz
    this.scriptNode = ctx.createScriptProcessor(4096, 2, 2);
    this.scriptNode.onaudioprocess = (e) => {
      if (!this.isRecording) return;
      const inL = e.inputBuffer.getChannelData(0);
      const inR = e.inputBuffer.getChannelData(1);
      this.leftChannelData.push(new Float32Array(inL));
      this.rightChannelData.push(new Float32Array(inR));
      if (e.outputBuffer) {
        if (e.outputBuffer.numberOfChannels > 0) e.outputBuffer.getChannelData(0).fill(0);
        if (e.outputBuffer.numberOfChannels > 1) e.outputBuffer.getChannelData(1).fill(0);
      }
    };

    // Tap master output before analyser
    this.engine.masterGain.connect(this.scriptNode);
    this.scriptNode.connect(ctx.destination);
  }

  stop() {
    if (!this.isRecording) return;
    this.isRecording = false;

    if (this.scriptNode) {
      this.scriptNode.onaudioprocess = null;
      try {
        this.engine.masterGain.disconnect(this.scriptNode);
        this.scriptNode.disconnect();
      } catch (_) {}
      this.scriptNode = null;
    }

    this._exportWav();
  }

  _exportWav() {
    let totalSamples = 0;
    for (let i = 0; i < this.leftChannelData.length; i++) {
      totalSamples += this.leftChannelData[i].length;
    }

    if (totalSamples === 0) return;

    const sampleRate = (this.engine.ctx && this.engine.ctx.sampleRate) || 48000;
    const numChannels = 2;
    const bytesPerSample = 2; // 16-bit
    const blockAlign = numChannels * bytesPerSample;
    const byteRate = sampleRate * blockAlign;
    const dataSize = totalSamples * blockAlign;
    const bufferSize = 44 + dataSize;

    const arrayBuffer = new ArrayBuffer(bufferSize);
    const view = new DataView(arrayBuffer);

    // RIFF header
    this._writeString(view, 0, 'RIFF');
    view.setUint32(4, 36 + dataSize, true);
    this._writeString(view, 8, 'WAVE');

    // fmt sub-chunk
    this._writeString(view, 12, 'fmt ');
    view.setUint32(16, 16, true); // Subchunk1Size = 16 for PCM
    view.setUint16(20, 1, true);  // AudioFormat = 1 (PCM)
    view.setUint16(22, numChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, byteRate, true);
    view.setUint16(32, blockAlign, true);
    view.setUint16(34, 16, true); // BitsPerSample = 16

    // data sub-chunk
    this._writeString(view, 36, 'data');
    view.setUint32(40, dataSize, true);

    // Write interleaved 16-bit PCM samples with [-1, 1] clamping
    let offset = 44;
    for (let chunk = 0; chunk < this.leftChannelData.length; chunk++) {
      const l = this.leftChannelData[chunk];
      const r = this.rightChannelData[chunk];
      for (let i = 0; i < l.length; i++) {
        const rawL = l[i];
        const rawR = r[i];
        const valL = Number.isFinite(rawL) ? rawL : 0;
        const valR = Number.isFinite(rawR) ? rawR : 0;
        const sL = Math.max(-1.0, Math.min(1.0, valL));
        const sR = Math.max(-1.0, Math.min(1.0, valR));
        view.setInt16(offset, sL < 0 ? sL * 0x8000 : sL * 0x7FFF, true);
        offset += 2;
        view.setInt16(offset, sR < 0 ? sR * 0x8000 : sR * 0x7FFF, true);
        offset += 2;
      }
    }

    const blob = new Blob([arrayBuffer], { type: 'audio/wav' });
    if (typeof document !== 'undefined' && typeof URL !== 'undefined' && URL.createObjectURL) {
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
      a.download = `BRAUN_MR16_${timestamp}.wav`;
      a.href = url;
      document.body.appendChild(a);
      a.click();
      setTimeout(() => {
        if (a.parentNode) {
          a.parentNode.removeChild(a);
        }
        URL.revokeObjectURL(url);
      }, 2000);
    }

    this.leftChannelData = [];
    this.rightChannelData = [];
  }

  _writeString(view, offset, string) {
    for (let i = 0; i < string.length; i++) {
      view.setUint8(offset + i, string.charCodeAt(i));
    }
  }

  encodeWAV(left, right, sampleRate = 48000) {
    const numSamples = Math.min(left.length, right.length);
    const numChannels = 2;
    const bytesPerSample = 2;
    const blockAlign = numChannels * bytesPerSample;
    const byteRate = sampleRate * blockAlign;
    const dataSize = numSamples * blockAlign;
    const bufferSize = 44 + dataSize;

    const arrayBuffer = new ArrayBuffer(bufferSize);
    const view = new DataView(arrayBuffer);

    this._writeString(view, 0, 'RIFF');
    view.setUint32(4, 36 + dataSize, true);
    this._writeString(view, 8, 'WAVE');

    this._writeString(view, 12, 'fmt ');
    view.setUint32(16, 16, true);
    view.setUint16(20, 1, true); // PCM
    view.setUint16(22, numChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, byteRate, true);
    view.setUint16(32, blockAlign, true);
    view.setUint16(34, 16, true);

    this._writeString(view, 36, 'data');
    view.setUint32(40, dataSize, true);

    let offset = 44;
    for (let i = 0; i < numSamples; i++) {
      const rawL = left[i];
      const rawR = right[i];
      const valL = Number.isFinite(rawL) ? rawL : 0;
      const valR = Number.isFinite(rawR) ? rawR : 0;
      const sL = Math.max(-1.0, Math.min(1.0, valL));
      const sR = Math.max(-1.0, Math.min(1.0, valR));
      view.setInt16(offset, sL < 0 ? sL * 0x8000 : sL * 0x7FFF, true);
      offset += 2;
      view.setInt16(offset, sR < 0 ? sR * 0x8000 : sR * 0x7FFF, true);
      offset += 2;
    }

    return new Blob([arrayBuffer], { type: 'audio/wav' });
  }
}

/**
 * Main BRAUN MR-16 Application Class
 */
class BraunMr16App {
  constructor() {
    this.engine = new Mr16WebEngine();
    this.recorder = new MasterWavRecorder(this.engine);
    this.crt = null;
    this.knobs = {};
    this.presets = {};

    // A/B Comparison State
    this.activeBuffer = 'A';
    this.bufferA = null;
    this.bufferB = null;
    this._abCopyFeedbackTimer = null;

    // Performance Strip State
    this.activeScaleId = 'BUDD_PENTATONIC';
    this.activeRootKey = 'A';
    this.chimePitches = new Float32Array(16);

    // Modal Extraction & Morphing
    this.modalExtractor = new ModalExtractor();
    this.activeProfileSlot = 'A';

    this.telemetryRafId = null;
    this._isJuceRecording = false;
    this._juceBridgeInitialized = false;

    this._setupDOMReferences();
  }

  get isJuce() {
    return Boolean(
      (typeof window !== 'undefined' && (window.__IS_JUCE__ || window.__JUCE__?.backend || window.__JUCE__)) ||
      (typeof window !== 'undefined' && window.location && (window.location.protocol === 'juce:' || window.location.hostname === 'juce.backend'))
    );
  }

  _emitJuceParam(id, value) {
    if (this.isJuce && typeof window !== 'undefined' && window.__JUCE__?.backend) {
      try {
        window.__JUCE__.backend.emitEvent('paramChange', { id, value });
      } catch (err) {
        console.warn('[JUCE] emitEvent paramChange error:', err);
      }
    }
  }

  _emitJuceExciter(data) {
    if (this.isJuce && typeof window !== 'undefined' && window.__JUCE__?.backend) {
      try {
        window.__JUCE__.backend.emitEvent('exciterTrigger', data);
      } catch (err) {
        console.warn('[JUCE] emitEvent exciterTrigger error:', err);
      }
    }
  }

  _initJuceBridge() {
    if (typeof window === 'undefined') return;

    const setupBackend = () => {
      const backend = window.__JUCE__?.backend;
      if (!backend || this._juceBridgeInitialized) return;
      this._juceBridgeInitialized = true;

      if (typeof backend.addEventListener === 'function') {
        // 1. Listen for paramUpdate events from JUCE C++ APVTS / host automation
        backend.addEventListener('paramUpdate', (data) => {
          if (!data || typeof data !== 'object') return;
          const { id, webId, value } = data;
          const targetId = id || webId;
          if (targetId === 'power_state' || targetId === 'power' || targetId === 'powerState') {
            const isPowered = (value > 0.5);
            if (this.engine.isPowered !== isPowered) {
              this.engine.setPower(isPowered);
            }
            if (this.dom.btnPower) {
              this.dom.btnPower.classList.toggle('is-active', isPowered);
              const statusText = this.dom.btnPower.querySelector('.braun-status-text');
              const led = this.dom.btnPower.querySelector('.braun-led');
              if (statusText) statusText.textContent = isPowered ? 'ACTIVE' : 'STANDBY';
              if (led) led.classList.toggle('is-active-green', isPowered);
            }
            if (this.crt) this.crt.setPower(isPowered);
            return;
          }

          if (typeof this.engine.params[targetId] !== 'undefined') {
            this.engine.params[targetId] = value;
          }

          let knob = this.knobs[targetId] || (webId && this.knobs[webId]);
          if (knob && typeof value === 'number') {
            let knobVal = value;
            if ((targetId === 'modal_coupling' || targetId === 'modalCoupling') && value <= 1.0) {
              knobVal = value * 100;
            } else if ((targetId === 'chorus_dimension' || targetId === 'chorusDimension') && value <= 1.0) {
              knobVal = value * 100;
            }
            knob.setValue(knobVal, false);
          }
        });

        // 2. Listen for telemetryFrame events for 60 FPS CRT scope & meters
        backend.addEventListener('telemetryFrame', (data) => {
          if (!data || typeof data !== 'object') return;

          const scope = data.scopeL || data.scopeSamplesL;
          if (this.crt) {
            const telemetry = {};
            if (scope && Array.isArray(scope)) {
              telemetry.timeData = scope;
            }
            if (data.modalEnergies && Array.isArray(data.modalEnergies)) {
              telemetry.modalEnergies = data.modalEnergies;
            }
            if (typeof data.lorenzX === 'number') {
              telemetry.lorenzState = {
                x: data.lorenzX,
                y: data.lorenzY || 0,
                z: data.lorenzZ || 0,
                rate: this.engine.params.lorenz_rate || 0.85,
                chaos: (this.engine.params.lorenz_chaos || 0.5) * 28.0
              };
            }
            if (typeof data.exciterActivity === 'number' && data.exciterActivity > 0.05) {
              telemetry.transientKick = data.exciterActivity;
            }
            this.crt.updateTelemetry(telemetry);
          }

          if (data.modalEnergies && Array.isArray(data.modalEnergies)) {
            for (let i = 0; i < 16; i++) {
              const bar = document.getElementById(`meter_${i}`);
              if (bar) {
                const energy = data.modalEnergies[i] || 0;
                bar.style.width = `${Math.min(100, Math.max(0, energy * 100))}%`;
              }
            }
          }
        });

        // 3. Listen for recordingState events
        backend.addEventListener('recordingState', (data) => {
          if (!data) return;
          const isRec = Boolean(data.recording);
          this._isJuceRecording = isRec;
          if (this.dom.btnRecordWav) {
            this.dom.btnRecordWav.classList.toggle('is-active', isRec);
            const recText = this.dom.btnRecordWav.querySelector('.braun-rec-text');
            const led = this.dom.btnRecordWav.querySelector('.braun-led');
            if (recText) recText.textContent = isRec ? 'STOP & SAVE' : 'REC WAV';
            if (led) led.classList.toggle('is-recording', isRec);
          }
        });
      }

      // 4. Wire knob context menus to showContextMenu
      for (const [id, knob] of Object.entries(this.knobs)) {
        if (!knob) continue;
        knob.onContextMenu = (e) => {
          try {
            backend.emitEvent('showContextMenu', {
              id: knob.paramId || id,
              x: Math.round(e.screenX || e.clientX || 0),
              y: Math.round(e.screenY || e.clientY || 0)
            });
          } catch (err) {
            console.warn('[JUCE] showContextMenu error:', err);
          }
        };
      }
    };

    if (window.__JUCE__?.backend) {
      setupBackend();
    } else {
      let attempts = 0;
      const poll = setInterval(() => {
        attempts++;
        if (window.__JUCE__?.backend) {
          clearInterval(poll);
          setupBackend();
        } else if (attempts >= 50) {
          clearInterval(poll);
        }
      }, 100);
      if (typeof poll.unref === 'function') {
        poll.unref();
      }
    }
  }

  triggerDirac() {
    this._ensureActiveAudio();
    this.engine.triggerDirac();
    this._emitJuceExciter({ type: 'strike', vel: 1.0, hard: 1.0 });
  }

  triggerFeltHammer() {
    this._ensureActiveAudio();
    this.engine.triggerFeltHammer();
    this._emitJuceExciter({ type: 'strike', vel: 0.8, hard: 0.65 });
  }

  triggerStickSlip(speed = 0.6, force = 0.5) {
    this._ensureActiveAudio();
    this.engine.triggerBowedFriction(speed);
    this._emitJuceExciter({ type: 'friction', speed, force });
  }

  triggerAirJet(vel = 0.8) {
    this._ensureActiveAudio();
    this.engine.triggerAirJet(0.45);
    this._emitJuceExciter({ type: 'vactrol', vel });
  }

  async init() {
    if (typeof window !== 'undefined') {
      window.__MR16__ = this;
    }
    this._initKnobs();
    this._initCrtDisplay();
    this._initPerformanceStrip();
    this._initEventListeners();
    await this._loadPresets();
    this._initTheme();
    this._initJuceBridge();

    // Snapshot Initial Buffer A & B
    this.bufferA = JSON.parse(JSON.stringify(this.engine.params));
    this.bufferB = JSON.parse(JSON.stringify(this.engine.params));
    this._updateAbUi();

    // Synchronize Power button state with engine
    this.engine.onPowerStateChanged = (isPowered) => {
      if (this.dom.btnPower) {
        this.dom.btnPower.classList.toggle('is-active', isPowered);
        const statusText = this.dom.btnPower.querySelector('.braun-status-text');
        const led = this.dom.btnPower.querySelector('.braun-led');
        if (statusText) statusText.textContent = isPowered ? 'ACTIVE' : 'STANDBY';
        if (led) led.classList.toggle('is-active-green', isPowered);
      }
      if (this.crt) this.crt.setPower(isPowered);
    };

    if (this.dom.btnPower) {
      this.dom.btnPower.classList.toggle('is-active', this.engine.isPowered);
      const statusText = this.dom.btnPower.querySelector('.braun-status-text');
      const led = this.dom.btnPower.querySelector('.braun-led');
      if (statusText) statusText.textContent = this.engine.isPowered ? 'ACTIVE' : 'STANDBY';
      if (led) led.classList.toggle('is-active-green', this.engine.isPowered);
    }

    // Unlock AudioContext on first gesture
    const unlock = async () => {
      await this._ensureActiveAudio();
      window.removeEventListener('pointerdown', unlock);
      window.removeEventListener('keydown', unlock);
    };
    window.addEventListener('pointerdown', unlock, { once: true });
    window.addEventListener('keydown', unlock, { once: true });
  }

  async _ensureActiveAudio() {
    if (!this.engine.isInitialized) {
      await this.engine.init();
    }
    if (this.engine.ctx && this.engine.ctx.state === 'suspended') {
      await this.engine.ctx.resume();
    }
    if (!this.engine.isPowered) {
      this.engine.setPower(true);
    }
    if (this.crt && !this.crt.isRunning) {
      this.crt.start();
    }
  }

  _setupDOMReferences() {
    this.dom = {
      chassis: document.getElementById('braun-chassis'),
      dropOverlay: document.getElementById('drop-overlay'),
      selectTheme: document.getElementById('select-theme'),
      selectPreset: document.getElementById('select-preset'),
      userPresetsGroup: document.getElementById('user-presets-group'),
      btnUiMode: document.getElementById('btn-ui-mode'),
      btnSavePatch: document.getElementById('btn-save-patch'),
      btnExportPatch: document.getElementById('btn-export-patch'),
      btnLoadPatch: document.getElementById('btn-load-patch'),
      inputLoadPatch: document.getElementById('input-load-patch'),
      btnAbToggle: document.getElementById('btn-ab-toggle'),
      abStatusText: document.getElementById('ab-status-text'),
      btnAbCopy: document.getElementById('btn-ab-copy'),
      btnRecordWav: document.getElementById('btn-record-wav'),
      btnResetAll: document.getElementById('btn-reset-all'),
      btnPower: document.getElementById('btn-power'),
      btnChorusEnable: document.getElementById('btn-chorus-enable'),
      btnSoftLimit: document.getElementById('btn-soft-limit'),
      selectScale: document.getElementById('select-scale'),
      selectRoot: document.getElementById('select-root'),
      chimeStrip: document.getElementById('chime-strip'),
      crtCanvas: document.getElementById('crt-canvas'),
      sampleDropzone: document.getElementById('sample-dropzone'),
      dropzoneTarget: document.getElementById('dropzone-target'),
      btnSlotA: document.getElementById('btn-slot-a'),
      btnSlotB: document.getElementById('btn-slot-b'),
      btnLoadSample: document.getElementById('btn-load-sample'),
      inputLoadSample: document.getElementById('input-load-sample')
    };
  }

  _initKnobs() {
    const create = (id, options) => {
      const container = document.getElementById(id);
      if (!container) return;
      const knob = new BraunKnob(container, {
        ...options,
        onChange: (val) => {
          this.engine.setParam(options.paramId, val);
          this._emitJuceParam(options.paramId, val);
        },
        onContextMenu: (e) => {
          if (this.isJuce && typeof window !== 'undefined' && window.__JUCE__?.backend) {
            try {
              window.__JUCE__.backend.emitEvent('showContextMenu', {
                id: options.paramId,
                x: Math.round(e.screenX || e.clientX || 0),
                y: Math.round(e.screenY || e.clientY || 0)
              });
            } catch (err) {
              console.warn('[JUCE] showContextMenu error:', err);
            }
          }
        }
      });
      this.knobs[options.paramId] = knob;
    };

    // --- Deck 01: Kinetic Exciter Knobs ---
    create('knob-exciter-velocity', {
      label: 'STRIKE VEL',
      paramId: 'strike_velocity',
      min: 0.0, max: 1.0, step: 0.01, value: 0.80, unit: '', precision: 2
    });
    create('knob-exciter-hardness', {
      label: 'HARDNESS',
      paramId: 'strike_hardness',
      min: 0.0, max: 1.0, step: 0.01, value: 0.65, unit: '%', precision: 2
    });
    create('knob-vactrol-sag', {
      label: 'VACTROL SAG',
      paramId: 'vactrol_sag',
      min: 0.0, max: 1.0, step: 0.01, value: 0.35, unit: '%', precision: 2
    });
    create('knob-friction-velocity', {
      label: 'FRICTION VEL',
      paramId: 'friction_speed',
      min: 0.0, max: 1.0, step: 0.01, value: 0.40, unit: '%', precision: 2
    });
    create('knob-friction-force', {
      label: 'FRICTION FORCE',
      paramId: 'friction_force',
      min: 0.0, max: 1.0, step: 0.01, value: 0.50, unit: '%', precision: 2
    });
    create('knob-ext-input-gain', {
      label: 'EXT IN GAIN',
      paramId: 'ext_input_gain',
      min: -24.0, max: 12.0, step: 0.5, value: 0.0, unit: 'dB', precision: 1
    });
    create('knob-poisson-density', {
      label: 'POISSON DENS',
      paramId: 'poisson_density',
      min: 0.0, max: 50.0, step: 0.5, value: 0.0, unit: 'Hz', precision: 1
    });
    create('knob-euclidean-pulses', {
      label: 'EUCLID HITS',
      paramId: 'euclidean_pulses',
      min: 1, max: 16, step: 1, value: 4, unit: '', precision: 0
    });
    create('knob-euclidean-steps', {
      label: 'EUCLID STEPS',
      paramId: 'euclidean_steps',
      min: 2, max: 16, step: 1, value: 16, unit: '', precision: 0
    });

    // --- Deck 02: 16-Pole Modal Resonator Knobs ---
    create('knob-modal-frequency', {
      label: 'FUNDAMENTAL',
      paramId: 'modal_frequency',
      min: 20.0, max: 2000.0, step: 0.5, value: 220.0, unit: 'Hz', isLog: true, precision: 1, size: 'hero'
    });
    create('knob-modal-spread', {
      label: 'OVERTONE SPREAD',
      paramId: 'modal_spread',
      min: 0.25, max: 2.00, step: 0.01, value: 1.00, unit: 'x', precision: 2
    });
    create('knob-modal-damping', {
      label: 'DAMPING RT60',
      paramId: 'modal_damping',
      min: 0.05, max: 10.00, step: 0.05, value: 1.80, unit: 's', isLog: true, precision: 2
    });
    create('knob-modal-q', {
      label: 'RESONANCE Q',
      paramId: 'modal_q',
      min: 5.0, max: 500.0, step: 1.0, value: 85.0, unit: 'Q', isLog: true, precision: 0
    });
    create('knob-modal-coupling', {
      label: 'COUPLING',
      paramId: 'modal_coupling',
      min: 0.0, max: 100.0, step: 0.5, value: 40.0, unit: '%', precision: 1
    });

    // --- Deck 03: Kinetic Morph & 3D Chaotic Attractor Knobs ---
    create('knob-attractor-rate', {
      label: 'ORBIT RATE',
      paramId: 'lorenz_rate',
      min: 0.01, max: 10.00, step: 0.02, value: 0.85, unit: 'Hz', isLog: true, precision: 2
    });
    create('knob-attractor-chaos', {
      label: 'CHAOS RHO',
      paramId: 'lorenz_chaos',
      min: 0.0, max: 1.0, step: 0.01, value: 0.50, unit: '%', precision: 2
    });
    create('knob-attractor-freq-mod', {
      label: 'FREQ MOD',
      paramId: 'lorenz_freq_mod',
      min: 0.0, max: 100.0, step: 0.5, value: 15.0, unit: '%', precision: 1
    });
    create('knob-attractor-q-mod', {
      label: 'Q MOD',
      paramId: 'lorenz_q_mod',
      min: 0.0, max: 100.0, step: 0.5, value: 20.0, unit: '%', precision: 1
    });
    create('knob-morph-alpha', {
      label: 'MORPH A/B',
      paramId: 'lorenz_morph_alpha',
      min: 0.0, max: 1.0, step: 0.01, value: 0.50, unit: '', precision: 2, size: 'secondary'
    });
    create('knob-lorenz-steer', {
      label: 'LORENZ STEER',
      paramId: 'lorenz_morph_steer',
      min: 0.0, max: 100.0, step: 0.5, value: 0.0, unit: '%', precision: 1
    });

    // --- Deck 04: Tri-Phase Spatial BBD Chorus Knobs ---
    create('knob-chorus-rate', {
      label: 'SWEEP RATE',
      paramId: 'chorus_rate_hz',
      min: 0.05, max: 8.00, step: 0.02, value: 0.65, unit: 'Hz', isLog: true, precision: 2
    });
    create('knob-chorus-depth', {
      label: 'BBD DEPTH',
      paramId: 'chorus_depth_ms',
      min: 0.1, max: 5.0, step: 0.05, value: 1.40, unit: 'ms', precision: 2
    });
    create('knob-chorus-dimension', {
      label: 'DIMENSION',
      paramId: 'chorus_dimension',
      min: 0.0, max: 100.0, step: 1.0, value: 75.0, unit: '%', precision: 0
    });
    create('knob-chorus-mix', {
      label: 'CHORUS MIX',
      paramId: 'chorus_mix',
      min: 0.0, max: 100.0, step: 0.5, value: 45.0, unit: '%', precision: 1
    });

    // --- Deck 05: Spatial Dispersion & Master Dynamics Knobs ---
    create('knob-spatial-pan', {
      label: 'GOLDEN PAN',
      paramId: 'golden_pan_spread',
      min: 0.0, max: 100.0, step: 0.5, value: 85.0, unit: '%', precision: 1
    });
    create('knob-dynamics-lpg', {
      label: 'VACTROL LPG',
      paramId: 'vactrol_lpg_cutoff',
      min: 20.0, max: 20000.0, step: 10.0, value: 14000.0, unit: 'Hz', isLog: true, precision: 0
    });
    create('knob-dynamics-drive', {
      label: 'SATURATION',
      paramId: 'drive_saturation',
      min: 0.0, max: 100.0, step: 0.5, value: 25.0, unit: '%', precision: 1
    });
    create('knob-master-trim', {
      label: 'MASTER TRIM',
      paramId: 'master_trim_db',
      min: -24.0, max: 12.0, step: 0.2, value: 0.0, unit: 'dB', precision: 1
    });
    create('knob-master-mix', {
      label: 'DRY / WET',
      paramId: 'dry_wet_mix',
      min: 0.0, max: 100.0, step: 0.5, value: 65.0, unit: '%', precision: 1
    });

    // --- Deck 06: CRT Visualizer Knobs ---
    create('knob-crt-intensity', {
      label: 'PERSISTENCE',
      paramId: 'crt_intensity',
      min: 0.0, max: 100.0, step: 1.0, value: 85.0, unit: '%', precision: 0, size: 'compact'
    });
    if (this.knobs['crt_intensity']) {
      this.knobs['crt_intensity'].onChange = (val) => {
        if (this.crt) this.crt.setIntensity(val);
        this._emitJuceParam('crt_intensity', val);
      };
    }
  }

  _initCrtDisplay() {
    if (!this.dom.crtCanvas) return;
    this.crt = new BraunCrtDisplay(this.dom.crtCanvas, {
      mode: 'CHLADNI',
      phosphorType: 'GREEN_P1',
      intensity: 85.0,
      isPowered: true
    });

    // CRT 60 FPS Telemetry Update Loop
    const telemetryLoop = () => {
      if (this.crt && this.engine.isInitialized && this.crt.isRunning) {
        const telemetry = this.engine.getTelemetry();
        this.crt.updateTelemetry(telemetry);
      }
      this.telemetryRafId = requestAnimationFrame(telemetryLoop);
    };
    this.telemetryRafId = requestAnimationFrame(telemetryLoop);
  }

  _initPerformanceStrip() {
    this._recomputeChimePitches();
    this._renderChimeKeys();
  }

  _recomputeChimePitches() {
    const rootFreq = ROOT_FREQS[this.activeRootKey] || 220.0;
    const scale = SCALES[this.activeScaleId] || SCALES.BUDD_PENTATONIC;
    const intervals = scale.intervals;
    const count = intervals.length;

    for (let i = 0; i < 16; i++) {
      const octave = Math.floor(i / count);
      const interval = intervals[i % count];
      const semitones = octave * 12 + interval;
      this.chimePitches[i] = rootFreq * Math.pow(2, semitones / 12);
    }
  }

  _renderChimeKeys() {
    if (!this.dom.chimeStrip) return;
    this.dom.chimeStrip.innerHTML = '';

    for (let i = 0; i < 16; i++) {
      const key = document.createElement('div');
      key.className = 'braun-chime-key';
      key.dataset.index = i;

      const badge = document.createElement('span');
      badge.className = 'chime-key-badge';
      badge.textContent = (CHIME_HOTKEYS[i] || '').toUpperCase();

      const note = document.createElement('span');
      note.className = 'chime-key-note';
      const freq = Math.round(this.chimePitches[i]);
      note.textContent = freq < 1000 ? `${freq}` : `${(freq / 1000).toFixed(1)}k`;

      key.appendChild(badge);
      key.appendChild(note);

      // Touch / Pointer Event with Vertical Y Velocity Detection
      key.addEventListener('pointerdown', (e) => {
        e.preventDefault();
        key.classList.add('is-active');

        // Y coordinate velocity: bottom = soft (0.3), top = hard (1.0)
        const rect = key.getBoundingClientRect();
        const normY = 1.0 - Math.max(0, Math.min(1, (e.clientY - rect.top) / rect.height));
        const velocity = 0.3 + normY * 0.7;

        this.triggerChimeKey(i, velocity);
      });

      const clearActive = () => key.classList.remove('is-active');
      key.addEventListener('pointerup', clearActive);
      key.addEventListener('pointerleave', clearActive);
      key.addEventListener('pointercancel', clearActive);

      this.dom.chimeStrip.appendChild(key);
    }
  }

  triggerChimeKey(index, velocity = 0.8) {
    if (index < 0 || index >= 16) return;
    this._ensureActiveAudio();
    const freq = this.chimePitches[index];
    this.engine.triggerStrike(this.engine.params.strike_hardness, velocity, freq);
    this._emitJuceExciter({ type: 'chime', key: index, vel: velocity });

    // Visual feedback
    if (this.dom.chimeStrip) {
      const keys = this.dom.chimeStrip.children;
      if (keys[index]) {
        keys[index].classList.add('is-active');
        setTimeout(() => keys[index].classList.remove('is-active'), 120);
      }
    }
  }

  _initEventListeners() {
    // --- Audition Sound Bar ---
    document.getElementById('btn-audition-impulse')?.addEventListener('click', () => {
      this.triggerDirac();
    });
    document.getElementById('btn-audition-hammer')?.addEventListener('click', () => {
      this.triggerFeltHammer();
    });
    document.getElementById('btn-audition-friction')?.addEventListener('click', () => {
      this.triggerStickSlip(0.6, 0.5);
    });
    document.getElementById('btn-audition-air')?.addEventListener('click', () => {
      this.triggerAirJet(0.8);
    });
    document.getElementById('btn-audition-poisson')?.addEventListener('click', (e) => {
      this._ensureActiveAudio();
      const btn = e.currentTarget;
      if (this.engine.params.poisson_density > 0) {
        this.engine.setParam('poisson_density', 0);
        if (this.knobs['poisson_density']) this.knobs['poisson_density'].setValue(0);
        btn.classList.remove('is-active');
        this._emitJuceParam('poisson_density', 0.0);
      } else {
        this.engine.setParam('poisson_density', 18.0);
        if (this.knobs['poisson_density']) this.knobs['poisson_density'].setValue(18.0);
        btn.classList.add('is-active');
        this._emitJuceParam('poisson_density', 18.0);
      }
    });

    // Spacebar triggers Dirac Impulse, Escape cancels inputs and overlays
    window.addEventListener('keydown', (e) => {
      if (e.target && (
        e.target.tagName === 'INPUT' ||
        e.target.tagName === 'SELECT' ||
        e.target.tagName === 'TEXTAREA' ||
        e.target.isContentEditable ||
        e.target.tagName === 'BUTTON'
      )) {
        if (e.key === 'Escape') {
          if (typeof e.target.blur === 'function') e.target.blur();
        }
        if (e.target.tagName === 'SELECT') {
          const keyLower = e.key ? e.key.toLowerCase() : '';
          if (e.code === 'Space' || CHIME_HOTKEYS.includes(keyLower)) {
            e.preventDefault();
            if (typeof e.target.blur === 'function') e.target.blur();
            if (document.activeElement && typeof document.activeElement.blur === 'function') {
              document.activeElement.blur();
            }
            // Proceed directly to execute Space or chime key trigger
          } else {
            return;
          }
        } else if (e.target.tagName === 'BUTTON') {
          const keyLower = e.key ? e.key.toLowerCase() : '';
          if (e.code === 'Space' || CHIME_HOTKEYS.includes(keyLower)) {
            e.preventDefault();
            if (typeof e.target.blur === 'function') e.target.blur();
            if (document.activeElement && typeof document.activeElement.blur === 'function') {
              document.activeElement.blur();
            }
            // Proceed directly to execute Space or chime key trigger
          } else {
            return;
          }
        } else {
          return;
        }
      }

      if (e.key === 'Escape') {
        for (const id in this.knobs) {
          if (typeof this.knobs[id]._hideDirectInput === 'function') {
            this.knobs[id]._hideDirectInput();
          }
        }
        if (this.dom.chassis) this.dom.chassis.classList.remove('is-drag-over');
        if (this.dom.sampleDropzone) this.dom.sampleDropzone.classList.remove('is-drag-over');
        if (typeof document !== 'undefined' && document.activeElement && typeof document.activeElement.blur === 'function') {
          document.activeElement.blur();
        }
        return;
      }

      if (e.code === 'Space') {
        e.preventDefault();
        this.triggerDirac();
        return;
      }

      // Hotkeys for 16-key chime performance strip
      const keyIndex = CHIME_HOTKEYS.indexOf(e.key.toLowerCase());
      if (keyIndex !== -1) {
        e.preventDefault();
        this.triggerChimeKey(keyIndex, 0.85);
      }
    });

    // Ensure clicking any button or toggle immediately clears focus to keep keyboard note triggers active
    const clearButtonFocus = (e) => {
      const btn = e.target?.closest?.('button, .braun-btn, .braun-toggle, .braun-segment-btn, .braun-strike-btn, .braun-audition-btn');
      if (btn && typeof btn.blur === 'function') {
        btn.blur();
      }
      if (document.activeElement && (document.activeElement.tagName === 'BUTTON' || document.activeElement.classList?.contains('braun-btn'))) {
        document.activeElement.blur();
      }
    };
    document.addEventListener('pointerup', clearButtonFocus);
    document.addEventListener('click', clearButtonFocus);

    // --- Tactile Strike Buttons ---
    const btnStrikeDirac = document.getElementById('btn-strike-dirac');
    btnStrikeDirac?.addEventListener('click', () => this.triggerDirac());

    const btnStrikeHammer = document.getElementById('btn-strike-hammer');
    btnStrikeHammer?.addEventListener('click', () => this.triggerFeltHammer());

    const btnStrikeFriction = document.getElementById('btn-strike-friction');
    btnStrikeFriction?.addEventListener('click', () => this.triggerStickSlip(0.6, 0.5));

    const btnStrikeAir = document.getElementById('btn-strike-air');
    btnStrikeAir?.addEventListener('click', () => this.triggerAirJet(0.8));

    // --- Performance Scale & Root Selectors ---
    this.dom.selectScale?.addEventListener('change', (e) => {
      this.activeScaleId = e.target.value;
      this._recomputeChimePitches();
      this._renderChimeKeys();
      this.dom.selectScale.blur();
      if (document.activeElement && typeof document.activeElement.blur === 'function') {
        document.activeElement.blur();
      }
    });
    this.dom.selectRoot?.addEventListener('change', (e) => {
      this.activeRootKey = e.target.value;
      this._recomputeChimePitches();
      this._renderChimeKeys();
      this.dom.selectRoot.blur();
      if (document.activeElement && typeof document.activeElement.blur === 'function') {
        document.activeElement.blur();
      }
    });

    // --- Deck 01: Exciter Mode Segment Buttons ---
    document.getElementById('group-exciter-mode')?.addEventListener('click', (e) => {
      if (e.target.tagName === 'BUTTON') {
        const val = parseInt(e.target.dataset.val, 10);
        this._updateSegmentActive('group-exciter-mode', e.target);
        this.engine.setParam('exciter_type', val);
        this._emitJuceParam('exciter_type', val);
      }
    });

    // --- Deck 02: Manifold & Material Segment Buttons ---
    document.getElementById('group-manifold')?.addEventListener('click', (e) => {
      if (e.target.tagName === 'BUTTON') {
        const val = parseInt(e.target.dataset.val, 10);
        this._updateSegmentActive('group-manifold', e.target);
        this.engine.setParam('manifold_type', val);
        this._emitJuceParam('manifold_type', val);
      }
    });

    document.getElementById('group-material')?.addEventListener('click', (e) => {
      if (e.target.tagName === 'BUTTON') {
        const val = parseInt(e.target.dataset.val, 10);
        this._updateSegmentActive('group-material', e.target);
        this.engine.setParam('material_profile', val);
        this._emitJuceParam('material_profile', val);
      }
    });

    // --- Deck 04: Chorus Enable & Dimension Mode Buttons ---
    this.dom.btnChorusEnable?.addEventListener('click', () => {
      const active = !this.engine.params.chorus_enable;
      this.engine.setParam('chorus_enable', active);
      this._emitJuceParam('chorus_enable', active ? 1.0 : 0.0);
      this.dom.btnChorusEnable.classList.toggle('is-active', active);
      const text = this.dom.btnChorusEnable.querySelector('span:last-child');
      if (text) text.textContent = active ? 'CHORUS ACTIVE' : 'CHORUS BYPASS';
      const led = this.dom.btnChorusEnable.querySelector('.braun-led');
      if (led) {
        led.classList.toggle('is-active-orange', active);
      }
      if (typeof this.dom.btnChorusEnable.blur === 'function') {
        this.dom.btnChorusEnable.blur();
      }
    });

    document.getElementById('group-dimension-mode')?.addEventListener('click', (e) => {
      if (e.target.tagName === 'BUTTON') {
        const val = parseInt(e.target.dataset.val, 10);
        this._updateSegmentActive('group-dimension-mode', e.target);
        const dimensionWidths = { 1: 35.0, 2: 65.0, 3: 85.0, 4: 100.0 };
        const w = dimensionWidths[val] || 75.0;
        this.engine.setParam('chorus_dimension', w);
        if (this.knobs['chorus_dimension']) this.knobs['chorus_dimension'].setValue(w);
        this._emitJuceParam('chorus_dimension', w);
      }
    });

    // --- Deck 05: Soft Limiter Toggle ---
    this.dom.btnSoftLimit?.addEventListener('click', () => {
      const active = !this.engine.params.soft_limiter;
      this.engine.setParam('soft_limiter', active);
      this._emitJuceParam('soft_limiter', active ? 1.0 : 0.0);
      this.dom.btnSoftLimit.classList.toggle('is-active', active);
      const text = this.dom.btnSoftLimit.querySelector('span:last-child');
      if (text) text.textContent = active ? 'LIMITER ACTIVE' : 'LIMITER BYPASS';
      const led = this.dom.btnSoftLimit.querySelector('.braun-led');
      if (led) {
        led.classList.toggle('is-active-green', active);
      }
      if (typeof this.dom.btnSoftLimit.blur === 'function') {
        this.dom.btnSoftLimit.blur();
      }
    });

    // --- Deck 06: CRT Mode & Phosphor Segment Buttons ---
    document.getElementById('group-crt-mode')?.addEventListener('click', (e) => {
      if (e.target.tagName === 'BUTTON') {
        const mode = e.target.dataset.val;
        this._updateSegmentActive('group-crt-mode', e.target);
        if (this.crt) this.crt.setMode(mode);
        const modeIdx = mode === 'CHLADNI' ? 0 : (mode === 'ATTRACTOR' ? 1 : 2);
        this._emitJuceParam('display_mode', modeIdx);
      }
    });

    document.getElementById('group-phosphor-type')?.addEventListener('click', (e) => {
      if (e.target.tagName === 'BUTTON') {
        const type = e.target.dataset.val;
        this._updateSegmentActive('group-phosphor-type', e.target);
        if (this.crt) this.crt.setPhosphorType(type);
      }
    });

    // --- Header Utilities: Master Power Button ---
    this.dom.btnPower?.addEventListener('click', async () => {
      const isPowered = !this.engine.isPowered;
      if (isPowered) {
        await this._ensureActiveAudio();
      } else {
        this.engine.setPower(false);
      }
      this._emitJuceParam('power_state', isPowered ? 1.0 : 0.0);
    });

    // --- Header Utilities: Lossless WAV Recorder ---
    this.dom.btnRecordWav?.addEventListener('click', () => {
      const isRec = !this.recorder.isRecording && !this._isJuceRecording;
      const recText = this.dom.btnRecordWav.querySelector('.braun-rec-text');
      const led = this.dom.btnRecordWav.querySelector('.braun-led');

      if (this.isJuce && typeof window !== 'undefined' && window.__JUCE__?.backend) {
        if (isRec) {
          window.__JUCE__.backend.emitEvent('startRecording', {});
        } else {
          window.__JUCE__.backend.emitEvent('stopRecording', {});
        }
      } else {
        if (isRec) {
          this.recorder.start();
          this.dom.btnRecordWav.classList.add('is-active');
          if (recText) recText.textContent = 'STOP & SAVE';
          if (led) led.classList.add('is-recording');
        } else {
          this.recorder.stop();
          this.dom.btnRecordWav.classList.remove('is-active');
          if (recText) recText.textContent = 'REC WAV';
          if (led) led.classList.remove('is-recording');
        }
      }
    });

    // --- Header Utilities: A/B Comparison Buffer ---
    this.dom.btnAbToggle?.addEventListener('click', () => {
      if (this.activeBuffer === 'A') {
        // Save current into A, switch to B
        this.bufferA = JSON.parse(JSON.stringify(this.engine.params));
        this.activeBuffer = 'B';
        this.applyParamTree(this.bufferB || this.bufferA);
      } else {
        // Save current into B, switch to A
        this.bufferB = JSON.parse(JSON.stringify(this.engine.params));
        this.activeBuffer = 'A';
        this.applyParamTree(this.bufferA || this.bufferB);
      }
      this._updateAbUi();
    });

    this.dom.btnAbCopy?.addEventListener('click', () => {
      const current = JSON.parse(JSON.stringify(this.engine.params));
      if (this.activeBuffer === 'A') {
        this.bufferB = current;
      } else {
        this.bufferA = current;
      }
      if (this._abCopyFeedbackTimer) clearTimeout(this._abCopyFeedbackTimer);
      this.dom.btnAbCopy.innerHTML = '<span>COPIED!</span>';
      this._abCopyFeedbackTimer = setTimeout(() => {
        this._abCopyFeedbackTimer = null;
        this._updateAbUi();
      }, 1000);
    });

    // --- Header Utilities: Reset All ---
    this.dom.btnResetAll?.addEventListener('click', () => {
      const presetId = this.dom.selectPreset ? this.dom.selectPreset.value : 'DEFAULT_CONCERT_CHLADNI';
      if (this.presets[presetId]) {
        this.applyPreset(this.presets[presetId]);
      }
    });

    // --- Header Utilities: UI Mode Toggle ---
    this.dom.btnUiMode?.addEventListener('click', () => {
      const isNative = this.dom.btnUiMode.textContent.includes('NATIVE');
      this.dom.btnUiMode.innerHTML = isNative ? '<span>UI: WEB</span>' : '<span>UI: NATIVE</span>';
      this._emitJuceParam('toggleNativeUI', 1.0);
    });

    // --- Preset Selector ---
    this.dom.selectPreset?.addEventListener('change', (e) => {
      const presetId = e.target.value;
      if (this.presets[presetId]) {
        this.applyPreset(this.presets[presetId]);
      }
      this.dom.selectPreset.blur();
      if (document.activeElement && typeof document.activeElement.blur === 'function') {
        document.activeElement.blur();
      }
    });

    this.dom.selectPreset?.addEventListener('pointerup', () => {
      setTimeout(() => {
        if (this.dom.selectPreset) this.dom.selectPreset.blur();
        if (document.activeElement && typeof document.activeElement.blur === 'function') {
          document.activeElement.blur();
        }
      }, 0);
    });

    this.dom.selectPreset?.addEventListener('click', () => {
      setTimeout(() => {
        if (this.dom.selectPreset) this.dom.selectPreset.blur();
        if (document.activeElement && typeof document.activeElement.blur === 'function') {
          document.activeElement.blur();
        }
      }, 0);
    });

    this.dom.selectPreset?.addEventListener('blur', () => {
      if (document.activeElement === this.dom.selectPreset && typeof this.dom.selectPreset.blur === 'function') {
        this.dom.selectPreset.blur();
      }
    });

    this.dom.selectPreset?.addEventListener('keydown', (e) => {
      const keyLower = e.key ? e.key.toLowerCase() : '';
      if (e.code === 'Space' || CHIME_HOTKEYS.includes(keyLower)) {
        e.preventDefault();
        e.stopPropagation();
        this.dom.selectPreset.blur();
        if (document.activeElement && typeof document.activeElement.blur === 'function') {
          document.activeElement.blur();
        }
        if (e.code === 'Space') {
          this._ensureActiveAudio();
          this.engine.triggerDirac();
        } else {
          const idx = CHIME_HOTKEYS.indexOf(keyLower);
          if (idx !== -1) {
            this.triggerChimeKey(idx, 0.85);
          }
        }
      } else if (e.key === 'Escape') {
        this.dom.selectPreset.blur();
      }
    });

    // --- Patch Save / Export / Load ---
    this.dom.btnSavePatch?.addEventListener('click', () => {
      this._saveUserPreset();
    });
    this.dom.btnExportPatch?.addEventListener('click', () => {
      this._exportPatchJson();
    });
    this.dom.btnLoadPatch?.addEventListener('click', () => {
      if (this.dom.inputLoadPatch) this.dom.inputLoadPatch.click();
    });
    this.dom.inputLoadPatch?.addEventListener('change', (e) => {
      const file = e.target.files && e.target.files[0];
      if (file) this._loadPatchFile(file);
      e.target.value = '';
    });

    // --- Profile Slot Selection & Sample Extraction ---
    document.getElementById('group-profile-slot')?.addEventListener('click', (e) => {
      const btn = e.target.closest('button');
      if (btn && btn.dataset.slot) {
        this.activeProfileSlot = btn.dataset.slot;
        this._updateSegmentActive('group-profile-slot', btn);
      }
    });

    this.dom.btnLoadSample?.addEventListener('click', () => {
      if (this.dom.inputLoadSample) this.dom.inputLoadSample.click();
    });
    this.dom.dropzoneTarget?.addEventListener('click', () => {
      if (this.dom.inputLoadSample) this.dom.inputLoadSample.click();
    });
    this.dom.inputLoadSample?.addEventListener('change', async (e) => {
      const file = e.target.files && e.target.files[0];
      if (file) await this._handleAudioFileExtraction(file);
      e.target.value = '';
    });

    // --- Dedicated Deck 03 Dropzone Events ---
    const sampleDropzone = this.dom.sampleDropzone;
    if (sampleDropzone) {
      sampleDropzone.addEventListener('dragover', (e) => {
        e.preventDefault();
        e.stopPropagation();
        sampleDropzone.classList.add('is-drag-over');
      });
      sampleDropzone.addEventListener('dragleave', (e) => {
        e.preventDefault();
        e.stopPropagation();
        sampleDropzone.classList.remove('is-drag-over');
      });
      sampleDropzone.addEventListener('drop', async (e) => {
        e.preventDefault();
        e.stopPropagation();
        sampleDropzone.classList.remove('is-drag-over');
        if (this.dom.chassis) this.dom.chassis.classList.remove('is-drag-over');
        const file = e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0];
        if (file) {
          if (file.name.match(/\.(wav|mp3|ogg|flac|aiff|m4a)$/i) || file.type.startsWith('audio/')) {
            await this._handleAudioFileExtraction(file);
          } else {
            this._loadPatchFile(file);
          }
        }
      });
    }

    // --- Chassis Drag & Drop ---
    if (this.dom.chassis) {
      window.addEventListener('dragover', (e) => {
        e.preventDefault();
        this.dom.chassis.classList.add('is-drag-over');
      });
      window.addEventListener('dragleave', (e) => {
        if (e.relatedTarget === null) {
          this.dom.chassis.classList.remove('is-drag-over');
        }
      });
      window.addEventListener('dragend', () => {
        this.dom.chassis.classList.remove('is-drag-over');
      });
      window.addEventListener('drop', async (e) => {
        e.preventDefault();
        this.dom.chassis.classList.remove('is-drag-over');
        const file = e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0];
        if (file) {
          if (file.name.match(/\.(wav|mp3|ogg|flac|aiff|m4a)$/i) || file.type.startsWith('audio/')) {
            await this._handleAudioFileExtraction(file);
          } else {
            this._loadPatchFile(file);
          }
        }
      });
    }

    // --- Theme Finish Selector ---
    this.dom.selectTheme?.addEventListener('change', (e) => {
      this.setTheme(e.target.value);
      this.dom.selectTheme.blur();
      if (document.activeElement && typeof document.activeElement.blur === 'function') {
        document.activeElement.blur();
      }
    });
  }

  _updateSegmentActive(groupId, targetBtn) {
    const group = document.getElementById(groupId);
    if (!group) return;
    const btns = group.querySelectorAll('.braun-segment-btn');
    btns.forEach(b => b.classList.remove('is-selected'));
    targetBtn.classList.add('is-selected');
    if (typeof targetBtn?.blur === 'function') {
      targetBtn.blur();
    }
    if (document.activeElement && (document.activeElement.tagName === 'BUTTON' || document.activeElement.classList?.contains('braun-btn'))) {
      document.activeElement.blur();
    }
  }

  _initTheme() {
    const saved = localStorage.getItem('braun_mr16_theme') || 'light';
    this.setTheme(saved);
    if (this.dom.selectTheme) this.dom.selectTheme.value = saved;
  }

  setTheme(theme) {
    if (theme === 'dark' || theme === 'matte-black') {
      document.documentElement.setAttribute('data-theme', 'dark');
    } else {
      document.documentElement.removeAttribute('data-theme');
    }
    localStorage.setItem('braun_mr16_theme', theme);
  }

  async _loadPresets() {
    try {
      const resp = await fetch('factory_presets.json');
      if (resp.ok) {
        const data = await resp.json();
        if (data.presets && Array.isArray(data.presets)) {
          data.presets.forEach(p => {
            this.presets[p.id] = p;
          });
        }
      }
    } catch (_) {
      console.warn('Could not fetch factory_presets.json via HTTP; fallback to embedded defaults.');
    }

    this._populateUserPresets();
  }

  _populateUserPresets() {
    if (!this.dom.userPresetsGroup) return;
    this.dom.userPresetsGroup.innerHTML = '';
    const userJson = localStorage.getItem('braun_mr16_user_presets');
    if (userJson) {
      try {
        const userPresets = JSON.parse(userJson);
        for (const id in userPresets) {
          this.presets[id] = userPresets[id];
          const opt = document.createElement('option');
          opt.value = id;
          opt.textContent = userPresets[id].name || id;
          this.dom.userPresetsGroup.appendChild(opt);
        }
      } catch (_) {}
    }
  }

  _saveUserPreset() {
    const name = prompt('Enter a name for this custom MR-16 preset:', 'MY_ACOUSTIC_RESONANCE');
    if (!name) return;

    const id = 'USER_' + name.toUpperCase().replace(/[^A-Z0-9]/g, '_');
    const patch = {
      $schema: 'https://braun-audio.de/schemas/mr16-patch-v1.json',
      format: 'BRAUN_MR16_PATCH',
      version: 1,
      device: 'BRAUN_MR16',
      id: id,
      name: name,
      category: 'User Preset',
      timestamp: new Date().toISOString(),
      theme: document.documentElement.getAttribute('data-theme') || 'light',
      params: JSON.parse(JSON.stringify(this.engine.params)),
      ui: {
        vector_crt_mode: this.crt ? this.crt.mode : 'CHLADNI',
        phosphor_type: this.crt ? this.crt.phosphorType : 'GREEN_P1',
        scale_id: this.activeScaleId,
        root_key: this.activeRootKey
      }
    };

    let userPresets = {};
    const userJson = localStorage.getItem('braun_mr16_user_presets');
    if (userJson) {
      try { userPresets = JSON.parse(userJson); } catch (_) {}
    }
    userPresets[id] = patch;
    localStorage.setItem('braun_mr16_user_presets', JSON.stringify(userPresets));

    this.presets[id] = patch;
    this._populateUserPresets();
    if (this.dom.selectPreset) this.dom.selectPreset.value = id;
  }

  _exportPatchJson() {
    const patch = {
      $schema: 'https://braun-audio.de/schemas/mr16-patch-v1.json',
      format: 'BRAUN_MR16_PATCH',
      version: 1,
      device: 'BRAUN_MR16',
      id: `PATCH_${Date.now()}`,
      name: 'BRAUN MR-16 CUSTOM PATCH',
      timestamp: new Date().toISOString(),
      theme: document.documentElement.getAttribute('data-theme') || 'light',
      params: JSON.parse(JSON.stringify(this.engine.params)),
      ui: {
        vector_crt_mode: this.crt ? this.crt.mode : 'CHLADNI',
        phosphor_type: this.crt ? this.crt.phosphorType : 'GREEN_P1',
        scale_id: this.activeScaleId,
        root_key: this.activeRootKey
      }
    };

    const blob = new Blob([JSON.stringify(patch, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.download = `BRAUN_MR16_PATCH_${new Date().toISOString().slice(0, 10)}.json`;
    a.href = url;
    document.body.appendChild(a);
    a.click();
    setTimeout(() => {
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }, 2000);
  }

  async _handleAudioFileExtraction(file) {
    if (!file) return;
    if (file.size > 50 * 1024 * 1024) {
      if (typeof alert === 'function') alert('Audio file exceeds maximum size limit of 50 MB for modal extraction.');
      return;
    }
    try {
      await this.engine.init();
      const arrayBuffer = await file.arrayBuffer();
      const audioBuffer = await this.engine.ctx.decodeAudioData(arrayBuffer);
      const profile = await this.modalExtractor.extractFromAudioBuffer(audioBuffer, file.name);

      const slot = this.activeProfileSlot || 'A';
      this.engine.loadModalProfile(profile, slot);

      const targetBtn = slot === 'B' ? this.dom.btnSlotB : this.dom.btnSlotA;
      if (targetBtn) {
        const orig = targetBtn.textContent;
        targetBtn.textContent = `${slot}: ${profile.manifoldClassification.type}`;
        setTimeout(() => {
          targetBtn.textContent = orig;
        }, 3000);
      }

      if (this.dom.dropzoneTarget) {
        const textEl = this.dom.dropzoneTarget.querySelector('.braun-dropzone-text');
        if (textEl) {
          const origText = textEl.textContent;
          textEl.textContent = `EXTRACTED INTO ${slot}: ${profile.manifoldClassification.type} (${profile.modes.length} MODES)`;
          setTimeout(() => {
            textEl.textContent = origText;
          }, 3500);
        }
      }
    } catch (err) {
      console.error('Modal extraction failed:', err);
      if (typeof alert === 'function') {
        alert('Failed to extract modal parameters from audio file: ' + err.message);
      }
    }
  }

  _loadPatchFile(file) {
    if (!file) return;
    if (file.size > 5 * 1024 * 1024) {
      if (typeof alert === 'function') alert('Patch file exceeds maximum size limit of 5 MB.');
      return;
    }
    const reader = new FileReader();
    reader.onerror = () => {
      if (typeof alert === 'function') alert('Failed to read patch file.');
    };
    reader.onload = (e) => {
      try {
        const patch = JSON.parse(e.target.result);
        if (patch.format === 'BRAUN_MR16_MODAL_PROFILE' || patch.modes) {
          const slot = this.activeProfileSlot || 'A';
          this.engine.loadModalProfile(patch, slot);
          const targetBtn = slot === 'B' ? this.dom.btnSlotB : this.dom.btnSlotA;
          if (targetBtn) {
            const orig = targetBtn.textContent;
            targetBtn.textContent = `${slot}: ${patch.manifoldClassification?.type || 'PROFILE'}`;
            setTimeout(() => {
              targetBtn.textContent = orig;
            }, 3000);
          }
          if (this.dom.dropzoneTarget) {
            const textEl = this.dom.dropzoneTarget.querySelector('.braun-dropzone-text');
            if (textEl) {
              const origText = textEl.textContent;
              textEl.textContent = `LOADED INTO ${slot}: ${patch.manifoldClassification?.type || 'MODAL PROFILE'}`;
              setTimeout(() => {
                textEl.textContent = origText;
              }, 3500);
            }
          }
          return;
        }
        if (patch.params) {
          this.applyPreset(patch);
          return;
        }
        if (typeof alert === 'function') {
          alert('Unrecognized patch format. Expected BRAUN MR-16 patch or modal profile.');
        }
      } catch (err) {
        if (typeof alert === 'function') {
          alert('Invalid JSON file: ' + err.message);
        }
      }
    };
    reader.readAsText(file);
  }

  applyPreset(preset) {
    if (!preset || !preset.params) return;

    // Apply parameters to engine & knobs
    this.applyParamTree(preset.params);

    // Snapshot current state into active buffer
    if (this.activeBuffer === 'A') {
      this.bufferA = JSON.parse(JSON.stringify(this.engine.params));
    } else {
      this.bufferB = JSON.parse(JSON.stringify(this.engine.params));
    }

    // Apply UI state if present
    if (preset.ui) {
      if (preset.ui.vector_crt_mode && this.crt) {
        this.crt.setMode(preset.ui.vector_crt_mode);
        const group = document.getElementById('group-crt-mode');
        if (group) {
          const btn = group.querySelector(`button[data-val="${preset.ui.vector_crt_mode}"]`);
          if (btn) this._updateSegmentActive('group-crt-mode', btn);
        }
      }
      if (preset.ui.phosphor_type && this.crt) {
        this.crt.setPhosphorType(preset.ui.phosphor_type);
        const group = document.getElementById('group-phosphor-type');
        if (group) {
          const btn = group.querySelector(`button[data-val="${preset.ui.phosphor_type}"]`);
          if (btn) this._updateSegmentActive('group-phosphor-type', btn);
        }
      }
      if (preset.ui.scale_id) {
        this.activeScaleId = preset.ui.scale_id;
        if (this.dom.selectScale) this.dom.selectScale.value = preset.ui.scale_id;
        this._recomputeChimePitches();
        this._renderChimeKeys();
      }
      if (preset.ui.root_key) {
        this.activeRootKey = preset.ui.root_key;
        if (this.dom.selectRoot) this.dom.selectRoot.value = preset.ui.root_key;
        this._recomputeChimePitches();
        this._renderChimeKeys();
      }
    }
  }

  applyParamTree(params) {
    for (const id in params) {
      const val = params[id];
      this.engine.setParam(id, val);
      this._emitJuceParam(id, val);
      let knobVal = val;
      if (id === 'modal_coupling' && val <= 1.0) {
        knobVal = val * 100;
      } else if (id === 'chorus_dimension' && val <= 1.0) {
        knobVal = val * 100;
      }
      if (this.knobs[id]) {
        this.knobs[id].setValue(knobVal, false);
      }
    }

    // Synchronize discrete segment selectors
    if (params.exciter_type !== undefined) {
      const group = document.getElementById('group-exciter-mode');
      const btn = group?.querySelector(`button[data-val="${params.exciter_type}"]`);
      if (btn) this._updateSegmentActive('group-exciter-mode', btn);
    }
    if (params.manifold_type !== undefined) {
      const group = document.getElementById('group-manifold');
      const btn = group?.querySelector(`button[data-val="${params.manifold_type}"]`);
      if (btn) this._updateSegmentActive('group-manifold', btn);
    }
    if (params.material_profile !== undefined) {
      const group = document.getElementById('group-material');
      const btn = group?.querySelector(`button[data-val="${params.material_profile}"]`);
      if (btn) this._updateSegmentActive('group-material', btn);
    }
    if (params.chorus_dimension !== undefined) {
      const dim = params.chorus_dimension > 1.0 ? params.chorus_dimension : params.chorus_dimension * 100;
      let modeVal = 2;
      if (dim <= 45) modeVal = 1;
      else if (dim <= 75) modeVal = 2;
      else if (dim <= 90) modeVal = 3;
      else modeVal = 4;
      const group = document.getElementById('group-dimension-mode');
      const btn = group?.querySelector(`button[data-val="${modeVal}"]`);
      if (btn) this._updateSegmentActive('group-dimension-mode', btn);
    }
    if (params.chorus_enable !== undefined && this.dom.btnChorusEnable) {
      const active = Boolean(params.chorus_enable);
      this.dom.btnChorusEnable.classList.toggle('is-active', active);
      const text = this.dom.btnChorusEnable.querySelector('span:last-child');
      if (text) text.textContent = active ? 'CHORUS ACTIVE' : 'CHORUS BYPASS';
      const led = this.dom.btnChorusEnable.querySelector('.braun-led');
      if (led) led.classList.toggle('is-active-orange', active);
    }
    if (params.soft_limiter !== undefined && this.dom.btnSoftLimit) {
      const active = Boolean(params.soft_limiter);
      this.dom.btnSoftLimit.classList.toggle('is-active', active);
      const text = this.dom.btnSoftLimit.querySelector('span:last-child');
      if (text) text.textContent = active ? 'LIMITER ACTIVE' : 'LIMITER BYPASS';
      const led = this.dom.btnSoftLimit.querySelector('.braun-led');
      if (led) led.classList.toggle('is-active-green', active);
    }
  }

  _updateAbUi() {
    if (this.dom.abStatusText) {
      this.dom.abStatusText.textContent = this.activeBuffer;
    }
    if (this.dom.btnAbCopy && !this._abCopyFeedbackTimer) {
      const target = this.activeBuffer === 'A' ? 'B' : 'A';
      this.dom.btnAbCopy.innerHTML = `<span>COPY ${this.activeBuffer}&rarr;${target}</span>`;
    }
  }

  destroy() {
    if (this.telemetryRafId && typeof cancelAnimationFrame === 'function') {
      cancelAnimationFrame(this.telemetryRafId);
      this.telemetryRafId = null;
    }
    if (this._abCopyFeedbackTimer) {
      clearTimeout(this._abCopyFeedbackTimer);
      this._abCopyFeedbackTimer = null;
    }
    if (this.crt) {
      this.crt.destroy();
      this.crt = null;
    }
    if (this.recorder) {
      this.recorder.stop();
    }
    for (const id in this.knobs) {
      if (typeof this.knobs[id].destroy === 'function') {
        this.knobs[id].destroy();
      }
    }
    this.knobs = {};
  }
}

// Instantiate and start app on DOM ready
if (typeof window !== 'undefined') {
  window.addEventListener('DOMContentLoaded', () => {
    window.braunMr16App = new BraunMr16App();
    window.braunMr16App.init();
  });
}
