/**
 * @file modal_extractor.js
 * @brief High-Resolution Sample-to-Modal Parameter Extractor for BRAUN MR-16
 *
 * Decomposes arbitrary audio buffers into a 16-pole modal acoustic profile:
 * - Windowed 4096-point Radix-2 FFT with 4-term Blackman-Harris windowing
 * - Parabolic / quadratic sub-bin spectral peak interpolation
 * - Multi-slice temporal envelope regression for modal damping factors (Q_1 ... Q_16)
 * - Fundamental frequency extraction and overtone ratio derivation (r_k = f_k / f_1)
 * - Acoustic manifold classification against Chladni, Beam, Vocal, and Horn geometries
 * - Returns RFC 8259 compliant ModalProfile data structures
 *
 * Strict real-time safety, zero emojis, DIN 1451 technical English nomenclature.
 */

import { MANIFOLD_RATIOS } from './mr16_web_engine.js';

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
 * In-place Radix-2 Decimation-in-Time Fast Fourier Transform
 * @param {Float32Array} real - Real components array (length must be power of 2)
 * @param {Float32Array} imag - Imaginary components array (length must be power of 2)
 */
export function radix2FFT(real, imag) {
  const n = real.length;
  if ((n & (n - 1)) !== 0) {
    throw new Error(`FFT size must be a power of 2, received ${n}`);
  }

  // Bit-reversal permutation
  let j = 0;
  for (let i = 0; i < n - 1; i++) {
    if (i < j) {
      const tr = real[i];
      real[i] = real[j];
      real[j] = tr;
      const ti = imag[i];
      imag[i] = imag[j];
      imag[j] = ti;
    }
    let k = n >> 1;
    while (k <= j) {
      j -= k;
      k >>= 1;
    }
    j += k;
  }

  // Cooley-Tukey butterfly computations
  for (let len = 2; len <= n; len <<= 1) {
    const halfLen = len >> 1;
    const angleStep = (-2.0 * Math.PI) / len;
    const wStepReal = Math.cos(angleStep);
    const wStepImag = Math.sin(angleStep);

    for (let i = 0; i < n; i += len) {
      let wReal = 1.0;
      let wImag = 0.0;

      for (let k = 0; k < halfLen; k++) {
        const uReal = real[i + k];
        const uImag = imag[i + k];

        const vReal = real[i + k + halfLen] * wReal - imag[i + k + halfLen] * wImag;
        const vImag = real[i + k + halfLen] * wImag + imag[i + k + halfLen] * wReal;

        real[i + k] = uReal + vReal;
        imag[i + k] = uImag + vImag;
        real[i + k + halfLen] = uReal - vReal;
        imag[i + k + halfLen] = uImag - vImag;

        const nextWReal = wReal * wStepReal - wImag * wStepImag;
        wImag = wReal * wStepImag + wImag * wStepReal;
        wReal = nextWReal;
      }
    }
  }
}

/**
 * Generate 4-term Blackman-Harris window coefficients
 * Side-lobe attenuation: -92 dB
 * @param {number} size
 * @returns {Float32Array}
 */
export function createBlackmanHarrisWindow(size) {
  const win = new Float32Array(size);
  const a0 = 0.35875;
  const a1 = 0.48829;
  const a2 = 0.14128;
  const a3 = 0.01168;

  for (let i = 0; i < size; i++) {
    const phi = (2.0 * Math.PI * i) / (size - 1);
    win[i] = a0 - a1 * Math.cos(phi) + a2 * Math.cos(2.0 * phi) - a3 * Math.cos(3.0 * phi);
  }
  return win;
}

export class ModalExtractor {
  /**
   * @param {Object} options
   * @param {number} [options.fftSize=4096] - Analysis FFT length (power of 2)
   * @param {number} [options.numModes=16] - Target modal poles to extract
   * @param {number} [options.minFreq=20.0] - Lower frequency limit (Hz)
   * @param {number} [options.maxFreq=20000.0] - Upper frequency limit (Hz)
   * @param {number} [options.peakThresholdDb=-72.0] - Peak detection floor (dBFS)
   * @param {number} [options.minPeakDistanceHz=18.0] - Minimum inter-peak spacing (Hz)
   */
  constructor(options = {}) {
    this.fftSize = options.fftSize || 4096;
    this.numModes = options.numModes || 16;
    this.minFreq = options.minFreq || 20.0;
    this.maxFreq = options.maxFreq || 20000.0;
    this.peakThresholdDb = options.peakThresholdDb !== undefined ? options.peakThresholdDb : -72.0;
    this.minPeakDistanceHz = options.minPeakDistanceHz || 18.0;

    this.window = createBlackmanHarrisWindow(this.fftSize);
    this.real = new Float32Array(this.fftSize);
    this.imag = new Float32Array(this.fftSize);
    this.magnitude = new Float32Array(this.fftSize >> 1);
  }

  /**
   * Decompose an AudioBuffer or mock audio container into a 16-pole ModalProfile
   * @param {AudioBuffer|Object} audioBuffer
   * @param {string} [name='Extracted Profile']
   * @returns {Promise<Object>} RFC 8259 ModalProfile object
   */
  async extractFromAudioBuffer(audioBuffer, name = 'Extracted Profile') {
    if (!audioBuffer) {
      throw new Error('ModalExtractor: Invalid audio buffer provided');
    }

    const sampleRate = audioBuffer.sampleRate || 48000;
    const length = audioBuffer.length || (audioBuffer.getChannelData ? audioBuffer.getChannelData(0).length : 0);

    if (length < 256) {
      throw new Error(`ModalExtractor: Audio buffer length too short (${length} samples)`);
    }

    // 1. Channel down-mix to mono Float32Array
    const mono = this._downmixToMono(audioBuffer, length);

    // 2. Compute initial onset spectrum using Blackman-Harris window
    const onsetSpectrum = this._computeWindowedMagnitude(mono, 0, sampleRate);

    // 3. Locate candidate spectral peaks with sub-bin parabolic refinement
    const detectedPeaks = this._detectPeaks(onsetSpectrum, sampleRate);

    // 4. Select top 16 dominant resonant modes (sorted by ascending frequency)
    const selectedModes = this._selectAndSortModes(detectedPeaks);

    // 5. Fit modal damping quality factors (Q_1 ... Q_16) across temporal slices
    this._fitModalDamping(selectedModes, mono, sampleRate);

    // 6. Derive fundamental frequency f_1 and overtone ratios r_k = f_k / f_1
    const fundamental = selectedModes[0].frequency;
    for (let i = 0; i < selectedModes.length; i++) {
      selectedModes[i].ratio = Number((selectedModes[i].frequency / fundamental).toFixed(4));
    }

    // 7. Classify against theoretical geometric manifolds (Chladni, Beam, Vocal, Horn)
    const manifoldClassification = this._classifyManifold(selectedModes);

    // 8. Package RFC 8259 compliant ModalProfile
    const profileId = 'MODAL_' + Date.now().toString(36).toUpperCase() + '_' + Math.floor(Math.random() * 1000);
    const timestamp = new Date().toISOString();

    const profile = {
      $schema: 'https://braun-audio.de/schemas/mr16-modal-profile-v1.json',
      format: 'BRAUN_MR16_MODAL_PROFILE',
      version: 1,
      id: profileId,
      name: name,
      timestamp: timestamp,
      sampleRate: sampleRate,
      fundamental: Number(fundamental.toFixed(2)),
      manifoldClassification: manifoldClassification,
      modes: selectedModes.map((m, idx) => ({
        index: idx,
        frequency: Number(m.frequency.toFixed(2)),
        ratio: m.ratio,
        q: Number(m.q.toFixed(1)),
        gain: Number(m.gain.toFixed(4)),
        decayTimeSec: Number(m.decayTimeSec.toFixed(3)),
        decayRateAlpha: Number(m.decayRateAlpha.toFixed(2))
      })),
      ratios: selectedModes.map(m => m.ratio),
      frequencies: selectedModes.map(m => Number(m.frequency.toFixed(2))),
      qFactors: selectedModes.map(m => Number(m.q.toFixed(1))),
      gains: selectedModes.map(m => Number(m.gain.toFixed(4)))
    };

    return profile;
  }

  /**
   * Mix down multi-channel audio buffer to mono Float32Array with DC offset removal
   * @private
   */
  _downmixToMono(audioBuffer, length) {
    const mono = new Float32Array(length);
    const numChannels = audioBuffer.numberOfChannels || 1;

    for (let c = 0; c < numChannels; c++) {
      const chData = audioBuffer.getChannelData(c);
      const weight = 1.0 / numChannels;
      for (let i = 0; i < length; i++) {
        mono[i] += chData[i] * weight;
      }
    }

    // DC Blocking filter (1st-order highpass at ~15 Hz)
    let prevX = 0.0;
    let prevY = 0.0;
    const r = 0.998;
    for (let i = 0; i < length; i++) {
      const x = mono[i];
      const y = x - prevX + r * prevY;
      prevX = x;
      prevY = y;
      mono[i] = flushDenormal(y);
    }

    return mono;
  }

  /**
   * Compute windowed FFT magnitude spectrum at sample offset
   * @private
   */
  _computeWindowedMagnitude(signal, offset, sampleRate) {
    const n = this.fftSize;
    const halfN = n >> 1;
    const sigLen = signal.length;

    for (let i = 0; i < n; i++) {
      const srcIdx = offset + i;
      const val = srcIdx < sigLen ? signal[srcIdx] : 0.0;
      this.real[i] = val * this.window[i];
      this.imag[i] = 0.0;
    }

    radix2FFT(this.real, this.imag);

    const mag = new Float32Array(halfN);
    const norm = 2.0 / n;

    for (let k = 0; k < halfN; k++) {
      const r = this.real[k];
      const im = this.imag[k];
      mag[k] = flushDenormal(Math.sqrt(r * r + im * im) * norm);
    }

    return mag;
  }

  /**
   * Detect spectral peak local maxima with sub-bin parabolic interpolation
   * @private
   */
  _detectPeaks(mag, sampleRate) {
    const halfN = mag.length;
    const binHz = sampleRate / this.fftSize;
    const minBin = Math.max(2, Math.floor(this.minFreq / binHz));
    const maxBin = Math.min(halfN - 2, Math.ceil(this.maxFreq / binHz));

    // Find global peak magnitude for dBFS reference
    let globalPeak = 1.0e-9;
    for (let k = minBin; k <= maxBin; k++) {
      if (mag[k] > globalPeak) globalPeak = mag[k];
    }
    const peakFloorLinear = globalPeak * Math.pow(10.0, this.peakThresholdDb / 20.0);

    const rawPeaks = [];

    for (let k = minBin; k <= maxBin; k++) {
      const y0 = mag[k];
      if (y0 < peakFloorLinear) continue;

      const ym1 = mag[k - 1];
      const yp1 = mag[k + 1];

      // Local maximum condition
      if (y0 > ym1 && y0 > yp1) {
        // Parabolic interpolation on logarithmic magnitude
        const alpha = 20.0 * Math.log10(Math.max(1.0e-9, ym1));
        const beta = 20.0 * Math.log10(Math.max(1.0e-9, y0));
        const gamma = 20.0 * Math.log10(Math.max(1.0e-9, yp1));

        const denom = alpha - 2.0 * beta + gamma;
        let delta = 0.0;
        if (Math.abs(denom) > 1.0e-6) {
          delta = 0.5 * ((alpha - gamma) / denom);
          delta = Math.max(-0.5, Math.min(0.5, delta));
        }

        const trueBin = k + delta;
        const peakFreq = trueBin * binHz;
        const peakDb = beta - 0.25 * (alpha - gamma) * delta;
        const peakAmp = Math.pow(10.0, peakDb / 20.0);

        rawPeaks.push({
          bin: trueBin,
          frequency: peakFreq,
          amplitude: peakAmp,
          db: peakDb
        });
      }
    }

    // Filter peaks by minimum frequency distance (suppress side-lobes)
    rawPeaks.sort((a, b) => b.amplitude - a.amplitude);
    const filteredPeaks = [];

    for (const p of rawPeaks) {
      let isTooClose = false;
      for (const accepted of filteredPeaks) {
        if (Math.abs(p.frequency - accepted.frequency) < this.minPeakDistanceHz) {
          isTooClose = true;
          break;
        }
      }
      if (!isTooClose) {
        filteredPeaks.push(p);
      }
    }

    return filteredPeaks;
  }

  /**
   * Select top 16 dominant modes, pad if necessary, and sort by ascending frequency
   * @private
   */
  _selectAndSortModes(detectedPeaks) {
    const targetCount = this.numModes;
    let modes = [];

    if (detectedPeaks.length >= targetCount) {
      modes = detectedPeaks.slice(0, targetCount);
    } else if (detectedPeaks.length > 0) {
      modes = detectedPeaks.slice();
      // Extrapolate missing modes based on highest detected mode
      const lastPeak = modes[modes.length - 1];
      const prevPeak = modes.length > 1 ? modes[modes.length - 2] : null;
      const stepFactor = prevPeak ? Math.max(1.1, lastPeak.frequency / prevPeak.frequency) : 1.4142;
      let synthFreq = lastPeak.frequency;

      while (modes.length < targetCount) {
        synthFreq = Math.min(20000.0, synthFreq * stepFactor);
        modes.push({
          bin: 0,
          frequency: synthFreq,
          amplitude: lastPeak.amplitude * Math.pow(0.55, modes.length - detectedPeaks.length),
          db: lastPeak.db - 6.0
        });
      }
    } else {
      // Default fallback: 16-mode Chladni series rooted at 220 Hz
      const f0 = 220.0;
      const ratios = MANIFOLD_RATIOS.CHLADNI;
      for (let i = 0; i < targetCount; i++) {
        modes.push({
          bin: 0,
          frequency: f0 * ratios[i],
          amplitude: 1.0 / (1.0 + i * 0.35),
          db: -6.0 * i
        });
      }
    }

    // Sort by ascending frequency
    modes.sort((a, b) => a.frequency - b.frequency);

    // Normalize amplitude weights relative to dominant mode
    let maxAmp = 1.0e-6;
    for (let i = 0; i < modes.length; i++) {
      if (modes[i].amplitude > maxAmp) maxAmp = modes[i].amplitude;
    }

    return modes.map((m, idx) => ({
      index: idx,
      frequency: m.frequency,
      amplitude: m.amplitude,
      gain: m.amplitude / maxAmp,
      q: 85.0, // Initial default Q, refined in _fitModalDamping
      decayTimeSec: 1.5,
      decayRateAlpha: 4.6
    }));
  }

  /**
   * Fit modal damping coefficients (alpha_k) and Q_k via multi-slice temporal envelope regression
   * @private
   */
  _fitModalDamping(modes, signal, sampleRate) {
    const numSlices = 6;
    const sliceHopSamples = Math.min(Math.floor(signal.length / numSlices), this.fftSize >> 1);

    if (sliceHopSamples < 128 || signal.length < this.fftSize + sliceHopSamples * 2) {
      // Buffer too short for multi-slice regression; apply analytical material heuristic
      for (const m of modes) {
        const f = m.frequency;
        // Wood/Steel hybrid dispersion: Q scales moderately with sqrt(f)
        const estimatedQ = Math.max(15.0, Math.min(420.0, 35.0 * Math.sqrt(f / 100.0)));
        m.q = estimatedQ;
        m.decayRateAlpha = (Math.PI * f) / estimatedQ;
        m.decayTimeSec = Math.log(1000.0) / m.decayRateAlpha;
      }
      return;
    }

    const timesSec = new Float32Array(numSlices);
    const spectra = [];

    for (let s = 0; s < numSlices; s++) {
      const offset = s * sliceHopSamples;
      timesSec[s] = offset / sampleRate;
      spectra.push(this._computeWindowedMagnitude(signal, offset, sampleRate));
    }

    const binHz = sampleRate / this.fftSize;

    for (const m of modes) {
      const targetBin = Math.round(m.frequency / binHz);
      const logAmps = new Float32Array(numSlices);
      let validCount = 0;

      for (let s = 0; s < numSlices; s++) {
        // Track local peak around target bin
        let localMax = 1.0e-9;
        const searchMin = Math.max(1, targetBin - 2);
        const searchMax = Math.min(spectra[s].length - 1, targetBin + 2);

        for (let b = searchMin; b <= searchMax; b++) {
          if (spectra[s][b] > localMax) localMax = spectra[s][b];
        }

        logAmps[s] = Math.log(Math.max(1.0e-9, localMax));
        validCount++;
      }

      // Ordinary Least Squares Linear Regression: ln(A(t)) = ln(A_0) - alpha * t
      let sumT = 0.0;
      let sumY = 0.0;
      let sumTT = 0.0;
      let sumTY = 0.0;

      for (let s = 0; s < numSlices; s++) {
        const t = timesSec[s];
        const y = logAmps[s];
        sumT += t;
        sumY += y;
        sumTT += t * t;
        sumTY += t * y;
      }

      const meanT = sumT / numSlices;
      const meanY = sumY / numSlices;
      const denom = sumTT - numSlices * meanT * meanT;

      let slope = -2.0; // Default gentle decay
      if (Math.abs(denom) > 1.0e-7) {
        slope = (sumTY - numSlices * meanT * meanY) / denom;
      }

      // Ensure positive decay rate alpha = -slope
      const alpha = Math.max(0.2, Math.min(800.0, -slope));
      const qCalc = (Math.PI * m.frequency) / alpha;
      const clampedQ = Math.max(8.0, Math.min(480.0, qCalc));

      m.q = clampedQ;
      m.decayRateAlpha = alpha;
      m.decayTimeSec = Math.log(1000.0) / alpha;
    }
  }

  /**
   * Classify extracted 16-mode ratio vector against theoretical physical manifolds
   * @private
   */
  _classifyManifold(modes) {
    const ratios = modes.map(m => m.ratio);
    const manifoldTypes = ['CHLADNI', 'BEAM', 'VOCAL', 'HORN'];
    const scores = {};
    let bestType = 'CHLADNI';
    let lowestMse = 1.0e9;

    for (const type of manifoldTypes) {
      const theory = MANIFOLD_RATIOS[type];
      let mse = 0.0;

      for (let i = 0; i < 16; i++) {
        const measuredLog = Math.log2(Math.max(0.1, ratios[i]));
        const theoryLog = Math.log2(Math.max(0.1, theory[i]));
        const diff = measuredLog - theoryLog;
        mse += diff * diff;
      }

      mse /= 16.0;
      scores[type] = Number(mse.toFixed(4));

      if (mse < lowestMse) {
        lowestMse = mse;
        bestType = type;
      }
    }

    // Compute softmax confidence score (temperature tau = 0.35)
    let sumExp = 0.0;
    for (const type of manifoldTypes) {
      sumExp += Math.exp(-scores[type] / 0.35);
    }
    const confidence = Number((Math.exp(-lowestMse / 0.35) / sumExp).toFixed(3));

    return {
      type: bestType,
      confidence: Math.max(0.01, Math.min(0.99, confidence)),
      mse: Number(lowestMse.toFixed(4)),
      scores: scores
    };
  }

  /**
   * Synthesize a test AudioBuffer with known ground-truth damped sinusoidal modes
   * @static
   * @param {number} [sampleRate=48000]
   * @param {number} [durationSec=1.0]
   * @param {Array<number>} [modeFreqs]
   * @param {Array<number>} [decayTimes]
   * @param {Array<number>} [gains]
   * @returns {Object} Mock AudioBuffer with getChannelData
   */
  static synthesizeMockAudioBuffer(
    sampleRate = 48000,
    durationSec = 1.0,
    modeFreqs = [220.0, 458.26, 751.08, 856.24],
    decayTimes = [1.8, 1.4, 1.1, 0.9],
    gains = [1.0, 0.7, 0.5, 0.3]
  ) {
    const totalSamples = Math.floor(sampleRate * durationSec);
    const pcm = new Float32Array(totalSamples);
    const numModes = modeFreqs.length;

    for (let m = 0; m < numModes; m++) {
      const f = modeFreqs[m];
      const tau60 = decayTimes[m] || 1.0;
      const alpha = Math.log(1000.0) / tau60;
      const amp = gains[m] || 0.5;
      const omega = (2.0 * Math.PI * f) / sampleRate;

      for (let i = 0; i < totalSamples; i++) {
        const t = i / sampleRate;
        pcm[i] += amp * Math.exp(-alpha * t) * Math.sin(omega * i);
      }
    }

    // Normalize peak to -1.0 dBFS (0.891)
    let peak = 1.0e-9;
    for (let i = 0; i < totalSamples; i++) {
      const absVal = Math.abs(pcm[i]);
      if (absVal > peak) peak = absVal;
    }
    const scale = 0.891 / peak;
    for (let i = 0; i < totalSamples; i++) {
      pcm[i] *= scale;
    }

    return {
      sampleRate: sampleRate,
      length: totalSamples,
      duration: durationSec,
      numberOfChannels: 1,
      getChannelData: () => pcm
    };
  }
}
