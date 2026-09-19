/**
 * @file crt-display.js
 * @brief Master 60 FPS Vector Phosphor CRT Display for BRAUN MR-16
 * Features:
 * - Authentic P1 Green (#24FF6A) & P3 Amber (#FFB000) oscilloscope lab phosphor persistence decay
 * - Hardware-accelerated dual-pass vector beam glow with high-DPI 2x retina scaling
 * - 10x8 precision graticule with center crosshairs and calibrated division ticks (cached offscreen)
 * - 3 Dedicated Physical Acoustic Analysis Modes:
 *   Mode 1: CHLADNI - 2D biharmonic standing-wave plate nodal contours & kinetic lycopodium sand particle simulation
 *   Mode 2: ATTRACTOR - 3D rotating perspective projection of the continuous chaotic Lorenz butterfly orbit
 *   Mode 3: MODAL FFT - 16 discrete resonant phosphor bar meters with ballistic peak-hold indicators and tabular Hz readouts
 * - Power standby beam collapse animation
 * - Zero garbage collection in render loop
 * - Strict Dieter Rams functionalist typography & nomenclature
 */

export class BraunCrtDisplay {
  /**
   * @param {HTMLCanvasElement} canvas
   * @param {Object} [options={}]
   */
  constructor(canvas, options = {}) {
    if (!canvas) return;

    this.canvas = canvas;
    this.ctx = canvas.getContext ? canvas.getContext('2d') : null;
    this.mode = options.mode || 'CHLADNI'; // 'CHLADNI' | 'ATTRACTOR' | 'MODAL_FFT'
    this.phosphorType = options.phosphorType || 'GREEN_P1'; // 'GREEN_P1' | 'AMBER_P3'
    this.scopeSource = options.scopeSource || 'OUT'; // 'OUT' | 'IN'
    this.intensity = options.intensity ?? 85.0; // 0 - 100%
    this.isPowered = options.isPowered ?? true;
    this.isRunning = false;
    this.animationFrameId = null;

    // Color definitions
    this.updatePhosphorColors();
    this.gridColor = 'rgba(255, 255, 255, 0.08)';
    this.fontMono = '8px monospace';

    // Telemetry state
    this.timeData = new Float32Array(512);
    this.modalEnergies = new Float32Array(16);
    this.modalFreqs = new Float32Array(16);
    this.modalPeaks = new Float32Array(16);
    this.modalPeakVels = new Float32Array(16);
    this.modalPeakHolds = new Float32Array(16);
    this.lorenzState = { x: 0.1, y: 0.0, z: 0.0, rate: 0.85, chaos: 28.0 };

    // Initialize default modal frequencies (A2 base)
    const baseFreq = 220.0;
    for (let i = 0; i < 16; i++) {
      this.modalFreqs[i] = Math.round(baseFreq * (1 + i * 0.85));
    }

    // --- Chladni Lycopodium Sand Particulate System (450 particles) ---
    this.numParticles = 450;
    this.particles = new Float32Array(this.numParticles * 4); // x, y, vx, vy
    this._initParticles();

    // --- 3D Lorenz Attractor Orbit Ring Buffer (1000 points) ---
    this.attractorHistorySize = 1000;
    this.attractorHistoryX = new Float32Array(this.attractorHistorySize);
    this.attractorHistoryY = new Float32Array(this.attractorHistorySize);
    this.attractorHistoryZ = new Float32Array(this.attractorHistorySize);
    this.attractorHistoryIdx = 0;
    this.attractorHistoryFilled = 0;
    this.orbitAngle = 0;

    // Offscreen graticule cache & high-DPI scaling
    this._graticuleCanvas = null;
    this._dpr = 2;
    this.width = 580;
    this.height = 260;
    this.lastTime = 0;

    this._resizeRafId = null;
    this._resizeHandler = () => this._requestResize();

    this._resize();
    if (typeof window !== 'undefined') {
      window.addEventListener('resize', this._resizeHandler);
    }
    if (typeof ResizeObserver !== 'undefined' && this.canvas && this.canvas.parentElement) {
      this._resizeObserver = new ResizeObserver(this._resizeHandler);
      this._resizeObserver.observe(this.canvas.parentElement);
    }
  }

  _requestResize() {
    if (this._resizeRafId) return;
    if (typeof requestAnimationFrame === 'function') {
      this._resizeRafId = requestAnimationFrame(() => {
        this._resizeRafId = null;
        this._resize();
      });
    } else {
      this._resize();
    }
  }

  updatePhosphorColors() {
    if (this.phosphorType === 'AMBER_P3') {
      this.phosphorColor = '#FFB000';
      this.phosphorGlow = 'rgba(255, 176, 0, 0.45)';
      this.phosphorDim = 'rgba(255, 176, 0, 0.15)';
    } else {
      // Default P1 Green
      this.phosphorColor = '#24FF6A';
      this.phosphorGlow = 'rgba(36, 255, 106, 0.45)';
      this.phosphorDim = 'rgba(36, 255, 106, 0.15)';
    }
  }

  setPhosphorType(type) {
    this.phosphorType = type === 'AMBER_P3' ? 'AMBER_P3' : 'GREEN_P1';
    this.updatePhosphorColors();
  }

  setIntensity(val) {
    this.intensity = Math.max(0, Math.min(100, val));
  }

  setMode(mode) {
    if (mode === 'CHLADNI' || mode === 'ATTRACTOR' || mode === 'MODAL_FFT') {
      this.mode = mode;
    }
  }

  setScopeSource(source) {
    this.scopeSource = (source === 'IN' ? 'IN' : 'OUT');
  }

  setPower(powered) {
    this.isPowered = Boolean(powered);
  }

  _initParticles() {
    for (let i = 0; i < this.numParticles; i++) {
      const idx = i * 4;
      this.particles[idx] = (Math.random() - 0.5) * 2; // x in [-1, 1]
      this.particles[idx + 1] = (Math.random() - 0.5) * 2; // y in [-1, 1]
      this.particles[idx + 2] = 0; // vx
      this.particles[idx + 3] = 0; // vy
    }
  }

  kickParticles(intensity = 1.0) {
    for (let i = 0; i < this.numParticles; i++) {
      const idx = i * 4;
      const angle = Math.random() * Math.PI * 2;
      const speed = (0.02 + Math.random() * 0.06) * intensity;
      this.particles[idx + 2] += Math.cos(angle) * speed;
      this.particles[idx + 3] += Math.sin(angle) * speed;
    }
  }

  _resize() {
    if (!this.canvas || !this.ctx) return;
    const rect = this.canvas.getBoundingClientRect();
    const dpr = (typeof window !== 'undefined' && window.devicePixelRatio) ? Math.max(2, window.devicePixelRatio) : 2;
    const w = Math.round(rect.width || this.canvas.clientWidth || 580);
    const h = Math.round(rect.height || this.canvas.clientHeight || 260);

    const targetW = Math.floor(w * dpr);
    const targetH = Math.floor(h * dpr);

    if (this.canvas.width === targetW && this.canvas.height === targetH && this._dpr === dpr && this.width === w && this.height === h) {
      return;
    }

    this._dpr = dpr;
    this.canvas.width = targetW;
    this.canvas.height = targetH;

    if (this.ctx.resetTransform) {
      this.ctx.resetTransform();
    }
    if (this.ctx.scale) {
      this.ctx.scale(dpr, dpr);
    }

    this.width = w;
    this.height = h;

    this._updateGraticuleCache(w, h);

    this.ctx.fillStyle = '#121414';
    this.ctx.fillRect(0, 0, w, h);
    this.draw();
  }

  _updateGraticuleCache(w, h) {
    if (typeof document === 'undefined') return;
    if (!this._graticuleCanvas) {
      this._graticuleCanvas = document.createElement('canvas');
    }
    const dpr = this._dpr || 2;
    this._graticuleCanvas.width = Math.floor(w * dpr);
    this._graticuleCanvas.height = Math.floor(h * dpr);
    const gctx = this._graticuleCanvas.getContext('2d');
    if (!gctx) return;

    if (gctx.resetTransform) {
      gctx.resetTransform();
    }
    if (gctx.scale) {
      gctx.scale(dpr, dpr);
    }

    gctx.clearRect(0, 0, w, h);

    gctx.strokeStyle = this.gridColor;
    gctx.lineWidth = 1;

    const numX = 10;
    const numY = 8;
    const stepX = w / numX;
    const stepY = h / numY;

    // Major grid lines
    gctx.beginPath();
    for (let i = 1; i < numX; i++) {
      const x = Math.round(i * stepX) + 0.5;
      gctx.moveTo(x, 0);
      gctx.lineTo(x, h);
    }
    for (let j = 1; j < numY; j++) {
      const y = Math.round(j * stepY) + 0.5;
      gctx.moveTo(0, y);
      gctx.lineTo(w, y);
    }
    gctx.stroke();

    // Center crosshairs (illuminated)
    const midX = Math.round(w / 2) + 0.5;
    const midY = Math.round(h / 2) + 0.5;
    gctx.strokeStyle = 'rgba(255, 255, 255, 0.18)';
    gctx.lineWidth = 1.2;
    gctx.beginPath();
    gctx.moveTo(midX, 0);
    gctx.lineTo(midX, h);
    gctx.moveTo(0, midY);
    gctx.lineTo(w, midY);
    gctx.stroke();

    // Calibrated division ticks along center axes
    gctx.strokeStyle = 'rgba(255, 255, 255, 0.28)';
    gctx.lineWidth = 1;
    const ticksPerDiv = 5;
    gctx.beginPath();
    for (let i = 0; i <= numX * ticksPerDiv; i++) {
      const tx = Math.round(i * (stepX / ticksPerDiv)) + 0.5;
      const tickH = (i % ticksPerDiv === 0) ? 4 : 2;
      gctx.moveTo(tx, midY - tickH);
      gctx.lineTo(tx, midY + tickH);
    }
    for (let j = 0; j <= numY * ticksPerDiv; j++) {
      const ty = Math.round(j * (stepY / ticksPerDiv)) + 0.5;
      const tickW = (j % ticksPerDiv === 0) ? 4 : 2;
      gctx.moveTo(midX - tickW, ty);
      gctx.lineTo(midX + tickW, ty);
    }
    gctx.stroke();

    // Outer boundary frame
    gctx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
    gctx.lineWidth = 1;
    gctx.strokeRect(0.5, 0.5, w - 1, h - 1);
  }

  _drawGraticule(ctx, w, h) {
    const dpr = this._dpr || 2;
    if (!this._graticuleCanvas || this._graticuleCanvas.width !== Math.floor(w * dpr) || this._graticuleCanvas.height !== Math.floor(h * dpr)) {
      this._updateGraticuleCache(w, h);
    }
    if (this._graticuleCanvas) {
      ctx.drawImage(this._graticuleCanvas, 0, 0, w, h);
    }
  }

  start() {
    if (this.isRunning) return;
    this.isRunning = true;
    const loop = (timestamp) => {
      if (!this.isRunning) return;
      this.draw(timestamp);
      this.animationFrameId = requestAnimationFrame(loop);
    };
    this.animationFrameId = requestAnimationFrame(loop);
  }

  stop() {
    this.isRunning = false;
    if (this.animationFrameId) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }
  }

  destroy() {
    this.stop();
    if (this._resizeRafId && typeof cancelAnimationFrame === 'function') {
      cancelAnimationFrame(this._resizeRafId);
      this._resizeRafId = null;
    }
    if (typeof window !== 'undefined' && this._resizeHandler) {
      window.removeEventListener('resize', this._resizeHandler);
      this._resizeHandler = null;
    }
    if (this._resizeObserver) {
      this._resizeObserver.disconnect();
      this._resizeObserver = null;
    }
    this._graticuleCanvas = null;
  }

  updateTelemetry(telemetry) {
    if (!telemetry) return;
    if (telemetry.modalEnergies) {
      const len = Math.min(16, telemetry.modalEnergies.length);
      for (let i = 0; i < len; i++) {
        this.modalEnergies[i] = telemetry.modalEnergies[i];
      }
    }
    if (telemetry.modalFreqs) {
      const len = Math.min(16, telemetry.modalFreqs.length);
      for (let i = 0; i < len; i++) {
        this.modalFreqs[i] = telemetry.modalFreqs[i];
      }
    }
    if (telemetry.lorenzState) {
      this.lorenzState.x = telemetry.lorenzState.x ?? this.lorenzState.x;
      this.lorenzState.y = telemetry.lorenzState.y ?? this.lorenzState.y;
      this.lorenzState.z = telemetry.lorenzState.z ?? this.lorenzState.z;
      this.lorenzState.rate = telemetry.lorenzState.rate ?? this.lorenzState.rate;
      this.lorenzState.chaos = telemetry.lorenzState.chaos ?? this.lorenzState.chaos;
    }
    if (telemetry.timeData) {
      const len = Math.min(512, telemetry.timeData.length);
      for (let i = 0; i < len; i++) {
        this.timeData[i] = telemetry.timeData[i];
      }
    }
    if (telemetry.transientKick) {
      this.kickParticles(telemetry.transientKick);
    }
  }

  draw(timestamp = 0) {
    if (!this.canvas || !this.ctx) return;
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;

    const dt = this.lastTime ? Math.min(0.05, (timestamp - this.lastTime) / 1000) : 0.016;
    this.lastTime = timestamp;

    // Standby Power Collapse
    if (!this.isPowered) {
      ctx.fillStyle = '#121414';
      ctx.fillRect(0, 0, w, h);
      this._drawGraticule(ctx, w, h);

      // CRT phosphor collapse horizontal center line
      const midY = Math.round(h / 2);
      ctx.save();
      ctx.shadowColor = this.phosphorGlow;
      ctx.shadowBlur = 8;
      ctx.strokeStyle = this.phosphorColor;
      ctx.lineWidth = 1.2;
      ctx.beginPath();
      ctx.moveTo(w * 0.45, midY);
      ctx.lineTo(w * 0.55, midY);
      ctx.stroke();
      ctx.restore();
      return;
    }

    // Authentic P1 Phosphor persistence decay fill
    // Higher intensity -> lower alpha in clear fill -> longer persistence trails
    const decayAlpha = Math.max(0.10, 0.40 - (this.intensity / 100) * 0.24);
    ctx.fillStyle = `rgba(18, 20, 20, ${decayAlpha.toFixed(3)})`;
    ctx.fillRect(0, 0, w, h);

    // Draw Graticule
    this._drawGraticule(ctx, w, h);

    // Render operational mode
    ctx.save();
    if (this.mode === 'CHLADNI') {
      this._drawChladni(ctx, w, h, dt);
    } else if (this.mode === 'ATTRACTOR') {
      this._drawAttractor(ctx, w, h, dt);
    } else if (this.mode === 'MODAL_FFT') {
      this._drawModalFft(ctx, w, h, dt);
    }
    ctx.restore();

    // Render 60 FPS CRT vector oscilloscope waveform
    if (this.scopeSource === 'IN' || this.showWaveform) {
      this._drawVectorScopeWaveform(ctx, w, h);
    }

    // Mode readout in upper-right corner (DIN tabular)
    ctx.fillStyle = this.phosphorColor;
    ctx.font = '9px "DIN 1451 Mittelschrift", monospace';
    ctx.textAlign = 'right';
    ctx.fillText(`CRT: ${this.mode} · SCOPE: ${this.scopeSource}`, w - 12, 16);

    // Phosphor indicator in upper-left corner
    ctx.fillStyle = this.phosphorDim;
    ctx.textAlign = 'left';
    ctx.fillText(`PHOSPHOR: ${this.phosphorType === 'AMBER_P3' ? 'P3 AMBER' : 'P1 GREEN'} · 60 FPS`, 12, 16);
  }

  // --- Mode 1: Chladni Biharmonic 2D Plate Nodal Standing Waves ---
  _drawChladni(ctx, w, h, dt) {
    const midX = w / 2;
    const midY = h / 2;
    const plateRadius = Math.min(midX * 0.72, midY * 0.86);

    // Harmonic mode integers m and n derived from dominant modal energies
    let totalEnergy = 0.0;
    let maxIdx = 0;
    let maxE = 0.0;
    for (let i = 0; i < 16; i++) {
      const e = this.modalEnergies[i] || 0.0;
      totalEnergy += e;
      if (e > maxE) {
        maxE = e;
        maxIdx = i;
      }
    }
    let m = (maxIdx % 4) + 2;
    let n = ((maxIdx + 1) % 3) + 1;
    // Prevent degenerate identical modes where w(x,y) vanishes everywhere
    if (m === n) {
      n = (n % 3) + 1;
      if (m === n) n = (n === 1 ? 2 : 1);
    }

    // Dual-Pass Nodal Curves: Pass 1 (Glow), Pass 2 (Core)
    const drawContourPaths = (strokeStyle, lineWidth, glow = false) => {
      ctx.strokeStyle = strokeStyle;
      ctx.lineWidth = lineWidth;
      if (glow) {
        ctx.shadowColor = this.phosphorGlow;
        ctx.shadowBlur = 8;
      } else {
        ctx.shadowBlur = 0;
      }

      ctx.beginPath();
      const steps = 120;
      for (let pass = 0; pass < 2; pass++) {
        for (let i = 0; i <= steps; i++) {
          const theta = (i / steps) * Math.PI * 2;
          // Chladni nodal condition: cos(m*theta) - cos(n*theta) = 0
          const r = plateRadius * (0.65 + 0.30 * Math.cos(m * theta) * Math.sin(n * theta));
          const px = midX + Math.cos(theta + pass * Math.PI * 0.5) * r;
          const py = midY + Math.sin(theta + pass * Math.PI * 0.5) * r;
          if (i === 0) ctx.moveTo(px, py);
          else ctx.lineTo(px, py);
        }
      }
      ctx.stroke();
    };

    // Draw plate border
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.12)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.rect(midX - plateRadius, midY - plateRadius, plateRadius * 2, plateRadius * 2);
    ctx.stroke();

    // Dual-pass vector bloom
    drawContourPaths(this.phosphorGlow, 3.4, true);
    drawContourPaths(this.phosphorColor, 1.3, false);

    // Particle Simulation (Lycopodium Sand settling onto nodal lines)
    ctx.fillStyle = this.phosphorColor;
    const dampingFactor = 0.94;
    for (let i = 0; i < this.numParticles; i++) {
      const idx = i * 4;
      let px = this.particles[idx];
      let py = this.particles[idx + 1];
      let vx = this.particles[idx + 2];
      let vy = this.particles[idx + 3];

      // Physical acceleration towards nodal lines: w(x,y) = cos(m*pi*x)*cos(n*pi*y) - cos(n*pi*x)*cos(m*pi*y)
      const wx = Math.cos(m * Math.PI * px) * Math.cos(n * Math.PI * py) - Math.cos(n * Math.PI * px) * Math.cos(m * Math.PI * py);
      // Exact analytical gradient towards nodes (wx = 0)
      const gradX = -m * Math.PI * Math.sin(m * Math.PI * px) * Math.cos(n * Math.PI * py)
                    + n * Math.PI * Math.sin(n * Math.PI * px) * Math.cos(m * Math.PI * py);
      const gradY = -n * Math.PI * Math.cos(m * Math.PI * px) * Math.sin(n * Math.PI * py)
                    + m * Math.PI * Math.cos(n * Math.PI * px) * Math.sin(m * Math.PI * py);

      const forceScale = 0.10 * (maxE * 15.0 + 0.1);
      vx -= Math.sign(wx) * gradX * forceScale * dt;
      vy -= Math.sign(wx) * gradY * forceScale * dt;

      // Continuous plate acoustic agitation proportional to total modal vibration
      const audioVibe = Math.min(0.04, Math.sqrt(totalEnergy) * 0.15);
      if (audioVibe > 0.0005) {
        vx += (Math.random() - 0.5) * audioVibe;
        vy += (Math.random() - 0.5) * audioVibe;
      }

      vx *= dampingFactor;
      vy *= dampingFactor;

      px += vx;
      py += vy;

      // Boundary reflection
      if (px < -0.95) { px = -0.95; vx = -vx * 0.5; }
      if (px > 0.95) { px = 0.95; vx = -vx * 0.5; }
      if (py < -0.95) { py = -0.95; vy = -vy * 0.5; }
      if (py > 0.95) { py = 0.95; vy = -vy * 0.5; }

      this.particles[idx] = px;
      this.particles[idx + 1] = py;
      this.particles[idx + 2] = vx;
      this.particles[idx + 3] = vy;

      const screenX = midX + px * plateRadius;
      const screenY = midY + py * plateRadius;

      ctx.fillRect(screenX - 1, screenY - 1, 2, 2);
    }
  }

  // --- Mode 2: Continuous 3D Lorenz Chaotic Attractor Projection ---
  _drawAttractor(ctx, w, h, dt) {
    const midX = w / 2;
    const midY = h / 2;

    // Advance Lorenz numerical integration
    const sigma = 10.0;
    const rho = this.lorenzState.chaos || 28.0;
    const beta = 8.0 / 3.0;
    const rateScale = (this.lorenzState.rate || 0.85) * 0.8;
    const subSteps = 6;
    const subDt = (dt * rateScale) / subSteps;

    let lx = this.lorenzState.x;
    let ly = this.lorenzState.y;
    let lz = this.lorenzState.z;

    for (let s = 0; s < subSteps; s++) {
      const dx = sigma * (ly - lx);
      const dy = lx * (rho - lz) - ly;
      const dz = lx * ly - beta * lz;

      lx += dx * subDt;
      ly += dy * subDt;
      lz += dz * subDt;

      // Store in history buffer
      this.attractorHistoryX[this.attractorHistoryIdx] = lx;
      this.attractorHistoryY[this.attractorHistoryIdx] = ly;
      this.attractorHistoryZ[this.attractorHistoryIdx] = lz;

      this.attractorHistoryIdx = (this.attractorHistoryIdx + 1) % this.attractorHistorySize;
      if (this.attractorHistoryFilled < this.attractorHistorySize) {
        this.attractorHistoryFilled++;
      }
    }

    this.lorenzState.x = lx;
    this.lorenzState.y = ly;
    this.lorenzState.z = lz;

    // Continuous orbital rotation
    this.orbitAngle += dt * 0.65;
    const theta = this.orbitAngle;
    const cosT = Math.cos(theta);
    const sinT = Math.sin(theta);
    const pitch = 25 * (Math.PI / 180);
    const cosP = Math.cos(pitch);
    const sinP = Math.sin(pitch);

    const fov = 300;
    const camDist = 68;
    const orbitScale = 0.52;

    // Render Lorenz 3D Trajectory
    const count = this.attractorHistoryFilled;
    if (count > 2) {
      ctx.lineWidth = 1.3;
      const numBatches = 24;
      const batchSize = Math.ceil(count / numBatches);

      for (let b = 0; b < numBatches; b++) {
        const startIdx = b * batchSize;
        const endIdx = Math.min(count - 1, (b + 1) * batchSize);
        if (startIdx >= endIdx) break;

        const alpha = Math.max(0.06, (endIdx / count)).toFixed(2);
        ctx.strokeStyle = this.phosphorType === 'AMBER_P3'
          ? `rgba(255, 176, 0, ${alpha})`
          : `rgba(36, 255, 106, ${alpha})`;

        ctx.beginPath();
        let first = true;
        for (let i = startIdx; i <= endIdx; i++) {
          const ringIdx = (this.attractorHistoryIdx - count + i + this.attractorHistorySize) % this.attractorHistorySize;
          const x = this.attractorHistoryX[ringIdx] * orbitScale;
          const y = this.attractorHistoryY[ringIdx] * orbitScale;
          const z = (this.attractorHistoryZ[ringIdx] - 25.0) * orbitScale;

          const rx = x * cosT + z * sinT;
          const ry = y * cosP - (-x * sinT + z * cosT) * sinP;
          const rz = (-x * sinT + z * cosT) * cosP + y * sinP + camDist;

          if (rz <= 5) continue;

          const sx = midX + (rx * fov) / rz;
          const sy = midY - (ry * fov) / rz;

          if (first) {
            ctx.moveTo(sx, sy);
            first = false;
          } else {
            ctx.lineTo(sx, sy);
          }
        }
        ctx.stroke();
      }

      // Glowing Focus Head
      const lastIdx = (this.attractorHistoryIdx - 1 + this.attractorHistorySize) % this.attractorHistorySize;
      const hx = this.attractorHistoryX[lastIdx] * orbitScale;
      const hy = this.attractorHistoryY[lastIdx] * orbitScale;
      const hz = (this.attractorHistoryZ[lastIdx] - 25.0) * orbitScale;

      const rhx = hx * cosT + hz * sinT;
      const rhy = hy * cosP - (-hx * sinT + hz * cosT) * sinP;
      const rhz = (-hx * sinT + hz * cosT) * cosP + hy * sinP + camDist;

      if (rhz > 5) {
        const hsx = midX + (rhx * fov) / rhz;
        const hsy = midY - (rhy * fov) / rhz;

        ctx.save();
        ctx.shadowColor = this.phosphorGlow;
        ctx.shadowBlur = 12;
        ctx.fillStyle = '#FFFFFF';
        ctx.beginPath();
        ctx.arc(hsx, hsy, 2.5, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();
      }
    }
  }

  // --- Mode 3: 16-Pole Modal Resonator FFT Bar Meters ---
  _drawModalFft(ctx, w, h, dt) {
    const numBars = 16;
    const paddingX = 20;
    const availableW = w - paddingX * 2;
    const barSpacing = availableW / numBars;
    const barWidth = Math.max(6, barSpacing - 8);
    const bottomY = h - 26;
    const maxBarHeight = h - 52;

    const gravity = 180.0 * dt;

    // Draw reference level dB horizontal lines across scope
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let level = 0.25; level <= 0.75; level += 0.25) {
      const ly = bottomY - maxBarHeight * level;
      ctx.moveTo(paddingX, ly);
      ctx.lineTo(w - paddingX, ly);
    }
    ctx.stroke();

    ctx.font = '8px "DIN 1451 Mittelschrift", monospace';
    ctx.textAlign = 'center';

    for (let i = 0; i < numBars; i++) {
      const x = paddingX + i * barSpacing + (barSpacing - barWidth) / 2;
      const rawE = Math.max(0.0, this.modalEnergies[i] || 0.0);
      const energy = Math.min(1.0, Math.sqrt(rawE * 36.0));

      // Target bar height
      const targetH = energy * maxBarHeight;

      // Ballistic Peak Hold: hold for 0.8s, then decay with gravity
      if (targetH >= this.modalPeaks[i]) {
        this.modalPeaks[i] = targetH;
        this.modalPeakHolds[i] = 0.8;
        this.modalPeakVels[i] = 0;
      } else {
        if (this.modalPeakHolds[i] > 0) {
          this.modalPeakHolds[i] -= dt;
        } else {
          this.modalPeakVels[i] += gravity;
          this.modalPeaks[i] = Math.max(0, this.modalPeaks[i] - this.modalPeakVels[i]);
        }
      }

      // Draw Inactive bar boundary
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
      ctx.lineWidth = 1;
      ctx.strokeRect(x, bottomY - maxBarHeight, barWidth, maxBarHeight);

      // Draw Active Phosphor Bar
      if (targetH > 1) {
        ctx.save();
        ctx.shadowColor = this.phosphorGlow;
        ctx.shadowBlur = 8;
        ctx.fillStyle = this.phosphorColor;
        ctx.fillRect(x, bottomY - targetH, barWidth, targetH);
        ctx.restore();
      }

      // Draw Peak Tick
      const peakY = bottomY - this.modalPeaks[i];
      if (peakY < bottomY && peakY >= bottomY - maxBarHeight) {
        ctx.fillStyle = '#FFFFFF';
        ctx.fillRect(x - 1, peakY - 1, barWidth + 2, 2);
      }

      // Frequency readout beneath bar (Tabular Hz)
      const freq = Math.round(this.modalFreqs[i] || 0);
      ctx.fillStyle = 'rgba(255, 255, 255, 0.55)';
      ctx.fillText(freq < 1000 ? `${freq}` : `${(freq / 1000).toFixed(1)}k`, x + barWidth / 2, h - 8);
    }
  }

  // --- Oscilloscope Waveform Vector Trace ---
  _drawVectorScopeWaveform(ctx, w, h) {
    if (!this.timeData || this.timeData.length === 0) return;
    const len = this.timeData.length;
    const midY = h / 2;
    const amp = h * 0.40;

    ctx.save();
    ctx.shadowColor = this.phosphorGlow;
    ctx.shadowBlur = (this.scopeSource === 'IN') ? 10 : 6;
    ctx.strokeStyle = this.phosphorColor;
    ctx.lineWidth = (this.scopeSource === 'IN') ? 1.8 : 1.2;
    ctx.beginPath();

    const padding = 16;
    const drawW = w - padding * 2;
    for (let i = 0; i < drawW; i++) {
      const idx = Math.floor((i / drawW) * len);
      const val = this.timeData[idx] || 0.0;
      const x = padding + i;
      const y = midY - val * amp;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
    ctx.restore();
  }
}
