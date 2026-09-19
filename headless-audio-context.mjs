/**
 * @file headless-audio-context.mjs
 * @brief Zero-Dependency Web Audio API Headless DSP Simulator for Node.js
 *
 * Implements block-rate (128 samples) Web Audio processing graph for offline rendering:
 * - HeadlessOfflineAudioContext
 * - HeadlessAudioParam (setValue, setTarget, linearRamp, exponentialRamp)
 * - HeadlessAudioNode
 * - HeadlessGainNode
 * - HeadlessBiquadFilterNode (RBJ Biquad filters: lowpass, highpass, bandpass)
 * - HeadlessDelayNode (circular buffer with fractional linear interpolation)
 * - HeadlessWaveShaperNode (piecewise-linear shaper lookup)
 * - HeadlessStereoPannerNode (constant-power sine/cosine panning)
 * - HeadlessChannelSplitterNode & HeadlessChannelMergerNode
 * - HeadlessAnalyserNode
 * - HeadlessAudioBufferSourceNode & HeadlessAudioBuffer
 * - HeadlessAudioDestinationNode
 */

export class HeadlessAudioParam {
  constructor(defaultValue = 0.0) {
    this._defaultValue = defaultValue;
    this._value = defaultValue;
    this._timeline = [];
  }

  get value() {
    return this._value;
  }

  set value(v) {
    this._value = v;
    this._timeline = [{ type: 'setValue', time: 0, value: v }];
  }

  setValueAtTime(value, time) {
    this._value = value;
    this._timeline.push({ type: 'setValue', time, value });
    if (this._timeline.length > 8) this._timeline = this._timeline.slice(-8);
    return this;
  }

  setTargetAtTime(target, time, timeConstant) {
    const startVal = this.getValueAtTime(time);
    this._timeline.push({ type: 'setTarget', time, target, timeConstant, startVal });
    if (this._timeline.length > 8) this._timeline = this._timeline.slice(-8);
    return this;
  }

  linearRampToValueAtTime(value, endTime) {
    const startVal = this.getValueAtTime(endTime);
    this._timeline.push({ type: 'linearRamp', endTime, value, startVal });
    if (this._timeline.length > 8) this._timeline = this._timeline.slice(-8);
    return this;
  }

  exponentialRampToValueAtTime(value, endTime) {
    const startVal = this.getValueAtTime(endTime);
    this._timeline.push({ type: 'exponentialRamp', endTime, value: Math.max(1e-6, value), startVal });
    if (this._timeline.length > 8) this._timeline = this._timeline.slice(-8);
    return this;
  }

  getValueAtTime(t) {
    if (this._timeline.length === 0) return this._value;

    let activeIdx = -1;
    for (let i = this._timeline.length - 1; i >= 0; i--) {
      const ev = this._timeline[i];
      const evTime = ev.time ?? ev.endTime;
      if (t >= evTime) {
        activeIdx = i;
        break;
      }
    }

    if (activeIdx === -1) {
      return this._defaultValue;
    }

    const ev = this._timeline[activeIdx];
    if (ev.type === 'setValue') {
      return ev.value;
    } else if (ev.type === 'setTarget') {
      const dt = t - ev.time;
      return ev.target + (ev.startVal - ev.target) * Math.exp(-dt / ev.timeConstant);
    } else if (ev.type === 'linearRamp') {
      return ev.value;
    } else if (ev.type === 'exponentialRamp') {
      return ev.value;
    }
    return this._value;
  }
}

export class HeadlessAudioNode {
  constructor(context, numInputs = 1, numOutputs = 1, channelCount = 2) {
    this.context = context;
    this.numberOfInputs = numInputs;
    this.numberOfOutputs = numOutputs;
    this.channelCount = channelCount;
    this.inputs = [];
    this.outputs = [];

    this._currentBlockIndex = -1;
    this._outputBuffers = [];
    for (let o = 0; o < numOutputs; o++) {
      this._outputBuffers.push([
        new Float32Array(context.blockSize),
        new Float32Array(context.blockSize)
      ]);
    }
  }

  connect(destination, outputIndex = 0, inputIndex = 0) {
    const conn = { sourceNode: this, destNode: destination, sourceOutput: outputIndex, targetInput: inputIndex };
    this.outputs.push(conn);
    destination.inputs.push(conn);
    return destination;
  }

  disconnect(destination) {
    if (!destination) {
      for (const conn of this.outputs) {
        const idx = conn.destNode.inputs.indexOf(conn);
        if (idx !== -1) conn.destNode.inputs.splice(idx, 1);
      }
      this.outputs = [];
    } else {
      this.outputs = this.outputs.filter(c => {
        if (c.destNode === destination) {
          const idx = destination.inputs.indexOf(c);
          if (idx !== -1) destination.inputs.splice(idx, 1);
          return false;
        }
        return true;
      });
    }
  }

  getSummedInput(inputIndex, blockSize) {
    const inL = new Float32Array(blockSize);
    const inR = new Float32Array(blockSize);

    for (const conn of this.inputs) {
      if (conn.targetInput === inputIndex) {
        conn.sourceNode.processBlock(this.context.currentBlockIndex, blockSize);
        const srcBufs = conn.sourceNode._outputBuffers[conn.sourceOutput];
        const sL = srcBufs[0];
        const sR = srcBufs.length > 1 ? srcBufs[1] : srcBufs[0];
        for (let i = 0; i < blockSize; i++) {
          inL[i] += sL[i];
          inR[i] += sR[i];
        }
      }
    }
    return [inL, inR];
  }

  processBlock(blockIndex, blockSize) {
    if (this._currentBlockIndex === blockIndex) return;
    this._currentBlockIndex = blockIndex;
    this._process(blockIndex, blockSize);
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    const out = this._outputBuffers[0];
    out[0].set(inL);
    out[1].set(inR);
  }
}

export class HeadlessGainNode extends HeadlessAudioNode {
  constructor(context) {
    super(context, 1, 1, 2);
    this.gain = new HeadlessAudioParam(1.0);
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    const out = this._outputBuffers[0];
    const t0 = (blockIndex * blockSize) / this.context.sampleRate;
    const g = this.gain.getValueAtTime(t0);

    const outL = out[0];
    const outR = out[1];
    for (let i = 0; i < blockSize; i++) {
      outL[i] = inL[i] * g;
      outR[i] = inR[i] * g;
    }
  }
}

export class HeadlessBiquadFilterNode extends HeadlessAudioNode {
  constructor(context) {
    super(context, 1, 1, 2);
    this.type = 'lowpass';
    this.frequency = new HeadlessAudioParam(350);
    this.Q = new HeadlessAudioParam(1);

    this.s1L = 0; this.s2L = 0;
    this.s1R = 0; this.s2R = 0;
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    const out = this._outputBuffers[0];
    const fs = this.context.sampleRate;
    const t0 = (blockIndex * blockSize) / fs;

    const f = Math.max(10, Math.min(fs * 0.49, this.frequency.getValueAtTime(t0)));
    const q = Math.max(0.001, this.Q.getValueAtTime(t0));

    const w0 = (2 * Math.PI * f) / fs;
    const cosw0 = Math.cos(w0);
    const sinw0 = Math.sin(w0);
    const alpha = sinw0 / (2 * q);

    let b0 = 0, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    if (this.type === 'lowpass') {
      b0 = (1 - cosw0) / 2;
      b1 = 1 - cosw0;
      b2 = (1 - cosw0) / 2;
      a0 = 1 + alpha;
      a1 = -2 * cosw0;
      a2 = 1 - alpha;
    } else if (this.type === 'highpass') {
      b0 = (1 + cosw0) / 2;
      b1 = -(1 + cosw0);
      b2 = (1 + cosw0) / 2;
      a0 = 1 + alpha;
      a1 = -2 * cosw0;
      a2 = 1 - alpha;
    } else if (this.type === 'bandpass') {
      b0 = alpha;
      b1 = 0;
      b2 = -alpha;
      a0 = 1 + alpha;
      a1 = -2 * cosw0;
      a2 = 1 - alpha;
    }

    const invA0 = 1.0 / a0;
    const nb0 = b0 * invA0;
    const nb1 = b1 * invA0;
    const nb2 = b2 * invA0;
    const na1 = a1 * invA0;
    const na2 = a2 * invA0;

    let s1L = this.s1L, s2L = this.s2L;
    let s1R = this.s1R, s2R = this.s2R;

    const outL = out[0];
    const outR = out[1];

    for (let i = 0; i < blockSize; i++) {
      const xL = inL[i];
      const yL = nb0 * xL + s1L;
      s1L = nb1 * xL - na1 * yL + s2L;
      s2L = nb2 * xL - na2 * yL;
      outL[i] = Number.isFinite(yL) ? yL : 0;

      const xR = inR[i];
      const yR = nb0 * xR + s1R;
      s1R = nb1 * xR - na1 * yR + s2R;
      s2R = nb2 * xR - na2 * yR;
      outR[i] = Number.isFinite(yR) ? yR : 0;
    }

    this.s1L = s1L; this.s2L = s2L;
    this.s1R = s1R; this.s2R = s2R;
  }
}

export class HeadlessDelayNode extends HeadlessAudioNode {
  constructor(context, maxDelay = 1.0) {
    super(context, 1, 1, 2);
    this.delayTime = new HeadlessAudioParam(0.0);
    const maxSamples = Math.ceil(maxDelay * context.sampleRate) + 128;
    this.bufL = new Float32Array(maxSamples);
    this.bufR = new Float32Array(maxSamples);
    this.bufSize = maxSamples;
    this.writeIdx = 0;
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    const out = this._outputBuffers[0];
    const fs = this.context.sampleRate;
    const t0 = (blockIndex * blockSize) / fs;

    const dSec = Math.max(0, this.delayTime.getValueAtTime(t0));
    const dSamples = dSec * fs;

    const outL = out[0];
    const outR = out[1];

    for (let i = 0; i < blockSize; i++) {
      this.bufL[this.writeIdx] = inL[i];
      this.bufR[this.writeIdx] = inR[i];

      let readPos = this.writeIdx - dSamples;
      while (readPos < 0) readPos += this.bufSize;
      while (readPos >= this.bufSize) readPos -= this.bufSize;

      const idx0 = Math.floor(readPos);
      const frac = readPos - idx0;
      const idx1 = (idx0 + 1) % this.bufSize;

      outL[i] = this.bufL[idx0] * (1 - frac) + this.bufL[idx1] * frac;
      outR[i] = this.bufR[idx0] * (1 - frac) + this.bufR[idx1] * frac;

      this.writeIdx = (this.writeIdx + 1) % this.bufSize;
    }
  }
}

export class HeadlessWaveShaperNode extends HeadlessAudioNode {
  constructor(context) {
    super(context, 1, 1, 2);
    this.curve = null;
    this.oversample = 'none';
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    const out = this._outputBuffers[0];

    if (!this.curve || this.curve.length < 2) {
      out[0].set(inL);
      out[1].set(inR);
      return;
    }

    const n = this.curve.length;
    for (let i = 0; i < blockSize; i++) {
      for (let ch = 0; ch < 2; ch++) {
        const x = ch === 0 ? inL[i] : inR[i];
        const clamped = Math.max(-1.0, Math.min(1.0, x));
        const idxFloat = ((clamped + 1.0) / 2.0) * (n - 1);
        const idx0 = Math.floor(idxFloat);
        const idx1 = Math.min(n - 1, idx0 + 1);
        const frac = idxFloat - idx0;
        out[ch][i] = this.curve[idx0] * (1.0 - frac) + this.curve[idx1] * frac;
      }
    }
  }
}

export class HeadlessStereoPannerNode extends HeadlessAudioNode {
  constructor(context) {
    super(context, 1, 1, 2);
    this.pan = new HeadlessAudioParam(0.0);
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    const out = this._outputBuffers[0];
    const fs = this.context.sampleRate;
    const t0 = (blockIndex * blockSize) / fs;

    const p = Math.max(-1.0, Math.min(1.0, this.pan.getValueAtTime(t0)));
    const panNorm = (p + 1.0) / 2.0;
    const gainL = Math.cos(panNorm * Math.PI * 0.5) * Math.SQRT2;
    const gainR = Math.sin(panNorm * Math.PI * 0.5) * Math.SQRT2;

    const outL = out[0];
    const outR = out[1];
    for (let i = 0; i < blockSize; i++) {
      const mono = (inL[i] + inR[i]) * 0.5;
      outL[i] = mono * gainL;
      outR[i] = mono * gainR;
    }
  }
}

export class HeadlessChannelSplitterNode extends HeadlessAudioNode {
  constructor(context, numOutputs = 2) {
    super(context, 1, numOutputs, 1);
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    this._outputBuffers[0][0].set(inL);
    this._outputBuffers[0][1].set(inL);
    if (this._outputBuffers.length > 1) {
      this._outputBuffers[1][0].set(inR);
      this._outputBuffers[1][1].set(inR);
    }
  }
}

export class HeadlessChannelMergerNode extends HeadlessAudioNode {
  constructor(context, numInputs = 2) {
    super(context, numInputs, 1, 2);
  }

  _process(blockIndex, blockSize) {
    const out = this._outputBuffers[0];
    const [inL] = this.getSummedInput(0, blockSize);
    const [inR] = this.getSummedInput(1, blockSize);
    out[0].set(inL);
    out[1].set(inR);
  }
}

export class HeadlessAnalyserNode extends HeadlessAudioNode {
  constructor(context) {
    super(context, 1, 1, 2);
    this.fftSize = 512;
    this.smoothingTimeConstant = 0.8;
    this._timeBuffer = new Float32Array(512);
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    this._outputBuffers[0][0].set(inL);
    this._outputBuffers[0][1].set(inR);

    const copyLen = Math.min(blockSize, 512);
    this._timeBuffer.copyWithin(0, copyLen);
    for (let i = 0; i < copyLen; i++) {
      this._timeBuffer[512 - copyLen + i] = 0.5 * (inL[i] + inR[i]);
    }
  }

  getFloatTimeDomainData(array) {
    array.set(this._timeBuffer.subarray(0, array.length));
  }

  getByteTimeDomainData(array) {
    for (let i = 0; i < array.length; i++) {
      const v = Math.max(-1.0, Math.min(1.0, this._timeBuffer[i] || 0));
      array[i] = Math.round((v + 1.0) * 127.5);
    }
  }
}

export class HeadlessAudioBufferSourceNode extends HeadlessAudioNode {
  constructor(context) {
    super(context, 0, 1, 1);
    this.buffer = null;
    this.loop = false;
    this._startTime = Infinity;
    this._stopTime = Infinity;
    this._started = false;
    this._playhead = 0;
  }

  start(when = 0) {
    this._startTime = Math.max(0, when);
    this._started = true;
  }

  stop(when = 0) {
    this._stopTime = Math.max(0, when);
  }

  _process(blockIndex, blockSize) {
    const out = this._outputBuffers[0];
    out[0].fill(0);
    out[1].fill(0);

    if (!this.buffer || !this._started) return;

    const fs = this.context.sampleRate;
    const t0 = (blockIndex * blockSize) / fs;
    const dt = 1.0 / fs;
    const data = this.buffer.getChannelData(0);
    const dataLen = data.length;

    for (let i = 0; i < blockSize; i++) {
      const curTime = t0 + i * dt;
      if (curTime >= this._startTime && curTime < this._stopTime) {
        if (this._playhead < dataLen) {
          const val = data[this._playhead++];
          out[0][i] = val;
          out[1][i] = val;
        } else if (this.loop && dataLen > 0) {
          this._playhead = 0;
          const val = data[this._playhead++];
          out[0][i] = val;
          out[1][i] = val;
        }
      }
    }
  }
}

export class HeadlessAudioBuffer {
  constructor(numberOfChannels, length, sampleRate) {
    this.numberOfChannels = numberOfChannels;
    this.length = length;
    this.sampleRate = sampleRate;
    this.duration = length / sampleRate;
    this._channels = [];
    for (let c = 0; c < numberOfChannels; c++) {
      this._channels.push(new Float32Array(length));
    }
  }

  getChannelData(c) {
    return this._channels[c];
  }
}

export class HeadlessAudioDestinationNode extends HeadlessAudioNode {
  constructor(context, totalSamples) {
    super(context, 1, 0, 2);
    this.totalSamples = totalSamples;
    this.recordedL = new Float32Array(totalSamples);
    this.recordedR = new Float32Array(totalSamples);
  }

  _process(blockIndex, blockSize) {
    const [inL, inR] = this.getSummedInput(0, blockSize);
    const startIdx = blockIndex * blockSize;
    for (let i = 0; i < blockSize && (startIdx + i) < this.totalSamples; i++) {
      this.recordedL[startIdx + i] = inL[i];
      this.recordedR[startIdx + i] = inR[i];
    }
  }
}

export class HeadlessOfflineAudioContext {
  constructor(numberOfChannels = 2, length = 96000, sampleRate = 48000) {
    this.numberOfChannels = numberOfChannels;
    this.length = length;
    this.sampleRate = sampleRate;
    this.blockSize = 128;
    this.currentBlockIndex = 0;
    this.currentTime = 0.0;
    this.destination = new HeadlessAudioDestinationNode(this, length);
    this.allNodes = [this.destination];
  }

  createGain() {
    const node = new HeadlessGainNode(this);
    this.allNodes.push(node);
    return node;
  }

  createBiquadFilter() {
    const node = new HeadlessBiquadFilterNode(this);
    this.allNodes.push(node);
    return node;
  }

  createDelay(maxDelay = 1.0) {
    const node = new HeadlessDelayNode(this, maxDelay);
    this.allNodes.push(node);
    return node;
  }

  createWaveShaper() {
    const node = new HeadlessWaveShaperNode(this);
    this.allNodes.push(node);
    return node;
  }

  createStereoPanner() {
    const node = new HeadlessStereoPannerNode(this);
    this.allNodes.push(node);
    return node;
  }

  createChannelSplitter(outputs = 2) {
    const node = new HeadlessChannelSplitterNode(this, outputs);
    this.allNodes.push(node);
    return node;
  }

  createChannelMerger(inputs = 2) {
    const node = new HeadlessChannelMergerNode(this, inputs);
    this.allNodes.push(node);
    return node;
  }

  createAnalyser() {
    const node = new HeadlessAnalyserNode(this);
    this.allNodes.push(node);
    return node;
  }

  createBufferSource() {
    const node = new HeadlessAudioBufferSourceNode(this);
    this.allNodes.push(node);
    return node;
  }

  createBuffer(channels, length, sampleRate) {
    return new HeadlessAudioBuffer(channels, length, sampleRate);
  }

  async resume() {
    return Promise.resolve();
  }

  async startRendering() {
    const numBlocks = Math.ceil(this.length / this.blockSize);
    for (let b = 0; b < numBlocks; b++) {
      this.currentBlockIndex = b;
      this.currentTime = (b * this.blockSize) / this.sampleRate;
      this.destination.processBlock(b, this.blockSize);
    }

    const renderedBuffer = new HeadlessAudioBuffer(this.numberOfChannels, this.length, this.sampleRate);
    renderedBuffer.getChannelData(0).set(this.destination.recordedL);
    if (this.numberOfChannels > 1) {
      renderedBuffer.getChannelData(1).set(this.destination.recordedR);
    }
    return renderedBuffer;
  }
}
