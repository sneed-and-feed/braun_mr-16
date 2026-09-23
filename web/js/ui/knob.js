/**
 * @file knob.js
 * @brief Precision Dieter Rams / Braun rotary control component for the MR-16.
 * Features turned aluminum styling, touch disambiguation for tablet scrolling,
 * multi-touch isolation, logarithmic/linear scaling, double-click direct entry,
 * keyboard accessibility, and fine-tuning modifiers.
 */

export class BraunKnob {
  /**
   * @param {HTMLElement} container
   * @param {Object} options
   */
  constructor(container, options = {}) {
    this.container = container;
    this.id = options.id || `knob-${Math.random().toString(36).substr(2, 9)}`;
    this.label = options.label || 'CONTROL';
    this.min = options.min ?? 0;
    this.max = options.max ?? 100;
    this.step = options.step ?? 1;
    this.unit = options.unit || '';
    this.isLog = options.isLog ?? false;
    this.defaultValue = options.value ?? (this.min + (this.max - this.min) / 2);
    this.value = this.defaultValue;
    this.precision = options.precision ?? (this.step < 1 ? (options.precision !== undefined ? options.precision : 2) : 0);
    this.size = options.size || 'medium'; // 'small', 'medium', 'large'
    this.color = options.color || 'var(--knob-fill)';
    this.onChange = options.onChange || null;
    this.onDragEnd = options.onDragEnd || null;
    this.paramId = options.paramId || this.id;
    this.title = options.title || (container && container.getAttribute && container.getAttribute('title')) || '';
    this.onContextMenu = options.onContextMenu || null;

    this.isDragging = false;
    this.lastUserInteractionTime = 0;
    this._lastFormatted = '';
    this._lastAria = null;

    this.startAngle = -140; // degrees
    this.endAngle = 140;    // degrees
    this.angleRange = this.endAngle - this.startAngle; // 280 deg
    this._cleanups = [];

    if (this.container && typeof document !== 'undefined') {
      this.container.innerHTML = '';
      this._render();
      this._attachEvents();
    }
    this.setValue(this.value, false);
  }

  _render() {
    let sizeClass = 'braun-knob-medium';
    if (this.size === 'hero' || this.size === 'large') {
      sizeClass = 'braun-knob-hero braun-knob-large';
    } else if (this.size === 'secondary' || this.size === 'medium') {
      sizeClass = 'braun-knob-secondary braun-knob-medium';
    } else if (this.size === 'standard' || this.size === 'small') {
      sizeClass = 'braun-knob-standard braun-knob-small';
    } else if (this.size === 'compact' || this.size === 'mini') {
      sizeClass = 'braun-knob-compact braun-knob-mini';
    }

    this.element = document.createElement('div');
    this.element.className = `braun-knob-wrapper ${sizeClass}`;
    this.element.tabIndex = 0;
    this.element.setAttribute('role', 'slider');
    this.element.setAttribute('aria-label', this.label);
    this.element.setAttribute('aria-valuemin', this.min);
    this.element.setAttribute('aria-valuemax', this.max);
    this.element.setAttribute('aria-valuenow', this.value);
    if (this.title) {
      this.element.title = this.title;
    }

    this.element.innerHTML = `
      <div class="braun-knob-label">${this.label}</div>
      <div class="braun-knob-assembly">
        <svg class="braun-knob-scale" viewBox="0 0 100 100">
          <circle class="braun-knob-track" cx="50" cy="50" r="42" />
          <circle class="braun-knob-fill" cx="50" cy="50" r="42" />
        </svg>
        <div class="braun-knob-cap">
          <div class="braun-knob-indicator"></div>
        </div>
      </div>
      <div class="braun-knob-value-display">
        <span class="braun-knob-value-text">${this.formatValue(this.value)}</span><span class="braun-knob-unit">${this.unit}</span>
      </div>
      <input type="text" class="braun-knob-direct-input" style="display:none;" />
    `;

    this.container.appendChild(this.element);

    this.cap = this.element.querySelector('.braun-knob-cap');
    this.fillCircle = this.element.querySelector('.braun-knob-fill');
    this.valueText = this.element.querySelector('.braun-knob-value-text');
    this.directInput = this.element.querySelector('.braun-knob-direct-input');
    this.assembly = this.element.querySelector('.braun-knob-assembly');
    this.labelEl = this.element.querySelector('.braun-knob-label');
    this.valueDisplay = this.element.querySelector('.braun-knob-value-display');

    // Arc length: radius 42 -> circumference ~ 263.89
    this.circumference = 2 * Math.PI * 42;
    this.arcLength = (this.angleRange / 360) * this.circumference;
    if (this.fillCircle) {
      this.fillCircle.style.strokeDasharray = `${this.arcLength} ${this.circumference}`;
      this.fillCircle.style.strokeDashoffset = `${this.arcLength}`;
      this.fillCircle.setAttribute('stroke-dasharray', `${this.arcLength} ${this.circumference}`);
      this.fillCircle.setAttribute('stroke-dashoffset', `${this.arcLength}`);
    }
  }

  _attachEvents() {
    let startY = 0;
    let startVal = 0;
    let lastShift = false;
    let activePointerId = null;

    const onPointerDown = (e) => {
      if (e.target === this.directInput) return;
      if (this.isDragging) return;
      if (e.button !== undefined && e.button !== 0) {
        return;
      }

      // Touch Disambiguation: Labels and value readouts allow native vertical momentum scrolling
      const isLabelOrValue = Boolean(
        e.target && (
          (typeof e.target.closest === 'function' && (
            e.target.closest('.braun-knob-label') ||
            e.target.closest('.braun-knob-value-display') ||
            e.target.closest('.braun-knob-direct-input')
          )) ||
          (e.target.classList && (
            e.target.classList.contains('braun-knob-label') ||
            e.target.classList.contains('braun-knob-value-display') ||
            e.target.classList.contains('braun-knob-direct-input')
          ))
        )
      );

      if (isLabelOrValue) {
        return; // Allow native page scroll
      }

      // Only touches specifically on the rotary assembly capture drag
      const isKnobAssembly = Boolean(
        (e.target && typeof e.target.closest === 'function' && e.target.closest('.braun-knob-assembly')) ||
        (e.target && e.target.classList && e.target.classList.contains('braun-knob-assembly')) ||
        (this.assembly && (e.target === this.assembly || (typeof this.assembly.contains === 'function' && this.assembly.contains(e.target))))
      );

      if (!isKnobAssembly) {
        return;
      }

      if (typeof e.preventDefault === 'function') {
        e.preventDefault();
      }

      // Multi-touch isolation
      activePointerId = e.pointerId !== undefined ? e.pointerId : null;
      if (activePointerId !== null && this.element.setPointerCapture) {
        try {
          this.element.setPointerCapture(activePointerId);
        } catch (_) {}
      }

      this.isDragging = true;
      this.lastUserInteractionTime = Date.now();
      startY = e.clientY;
      startVal = this.value;
      lastShift = Boolean(e.shiftKey);
      this.element.classList.add('is-dragging');
      this.element.classList.add('is-active');

      window.addEventListener('pointermove', onPointerMove, { passive: false });
      window.addEventListener('pointerup', onPointerUp);
      window.addEventListener('pointercancel', onPointerUp);
      window.addEventListener('blur', onPointerUp);
      if (this.element && this.element.addEventListener) {
        this.element.addEventListener('lostpointercapture', onPointerUp);
      }
    };

    const onPointerMove = (e) => {
      if (!this.isDragging) return;
      if (activePointerId !== null && e.pointerId !== undefined && e.pointerId !== activePointerId) {
        return;
      }

      if (e.cancelable && typeof e.preventDefault === 'function') {
        e.preventDefault();
      }

      // Shift modifier dynamic origin re-anchoring to prevent jump artifacts
      if (Boolean(e.shiftKey) !== lastShift) {
        startVal = this.value;
        startY = e.clientY;
        lastShift = Boolean(e.shiftKey);
      }

      const deltaY = startY - e.clientY;
      const dragScale = e.shiftKey ? 0.15 : 1.0;
      const pixelRange = 180; // 180px for full sweep

      const startNorm = this._valueToNormalized(startVal);
      const rawNorm = startNorm + (deltaY / pixelRange) * dragScale;
      const norm = Math.max(0, Math.min(1, rawNorm));

      // Re-anchor at bounds to eliminate deadzone lag when reversing drag direction
      if (rawNorm > 1.0 || rawNorm < 0.0) {
        startY = e.clientY;
        startVal = this._normalizedToValue(norm);
      }

      const newVal = this._normalizedToValue(norm);
      this.setValue(newVal, true);
    };

    const onPointerUp = (e) => {
      if (!this.isDragging) return;
      if (e && activePointerId !== null && e.pointerId !== undefined && e.pointerId !== activePointerId) {
        return;
      }

      this.isDragging = false;
      this.element.classList.remove('is-dragging');
      this.element.classList.remove('is-active');
      this.lastUserInteractionTime = Date.now();

      if (activePointerId !== null && this.element.releasePointerCapture) {
        try {
          this.element.releasePointerCapture(activePointerId);
        } catch (_) {}
      }
      activePointerId = null;

      window.removeEventListener('pointermove', onPointerMove);
      window.removeEventListener('pointerup', onPointerUp);
      window.removeEventListener('pointercancel', onPointerUp);
      window.removeEventListener('blur', onPointerUp);
      if (this.element && this.element.removeEventListener) {
        this.element.removeEventListener('lostpointercapture', onPointerUp);
      }

      if (typeof this.onDragEnd === 'function') {
        this.onDragEnd(this.value, this.paramId);
      }
    };

    // Double-click to reset to default
    this.element.addEventListener('dblclick', (e) => {
      if (e.target === this.directInput) return;
      e.preventDefault();
      this.setValue(this.defaultValue, true);
    });

    // Right-click context menu / reset support
    this.element.addEventListener('contextmenu', (e) => {
      if (typeof this.onContextMenu === 'function') {
        e.preventDefault();
        this.onContextMenu(e, this);
      }
    });

    // Value display click opens direct numeric text input
    if (this.valueDisplay) {
      this.valueDisplay.addEventListener('click', (e) => {
        e.stopPropagation();
        this._showDirectInput();
      });
    }

    // Direct input key handling
    if (this.directInput) {
      this.directInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          this._applyDirectInput();
        } else if (e.key === 'Escape') {
          this._hideDirectInput();
        }
      });
      this.directInput.addEventListener('blur', () => {
        this._applyDirectInput();
      });
    }

    // Keyboard accessibility
    this.element.addEventListener('keydown', (e) => {
      let delta = 0;
      const mult = e.shiftKey ? 0.1 : 1.0;
      if (e.key === 'ArrowUp' || e.key === 'ArrowRight') {
        delta = this.step * mult;
      } else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') {
        delta = -this.step * mult;
      } else if (e.key === 'Home') {
        e.preventDefault();
        this.setValue(this.min, true);
        return;
      } else if (e.key === 'End') {
        e.preventDefault();
        this.setValue(this.max, true);
        return;
      }

      if (delta !== 0) {
        e.preventDefault();
        this.setValue(this.value + delta, true);
      }
    });

    // Mouse wheel support (parity with BRAUN AS-42 and RB-26)
    this.element.addEventListener('wheel', (e) => {
      e.preventDefault();
      const direction = e.deltaY < 0 ? 1 : -1;
      const stepFactor = e.shiftKey ? 0.2 : 1.0;
      if (this.isLog) {
        const deltaNorm = direction * 0.025 * stepFactor;
        const normVal = Math.max(0, Math.min(1, this._valueToNormalized(this.value) + deltaNorm));
        this.setValue(this._normalizedToValue(normVal), true);
      } else {
        const stepSize = (this.step || (this.max - this.min) / 100) * stepFactor;
        this.setValue(this.value + direction * stepSize, true);
      }
    }, { passive: false });

    this.element.addEventListener('pointerdown', onPointerDown);
  }

  _showDirectInput() {
    if (!this.directInput) return;
    this.directInput.value = this.valueText ? this.valueText.textContent : this.value;
    this.directInput.style.display = 'block';
    if (this.valueDisplay) this.valueDisplay.style.display = 'none';
    this.directInput.focus();
    this.directInput.select();
  }

  _hideDirectInput() {
    if (!this.directInput) return;
    this.directInput.style.display = 'none';
    if (this.valueDisplay) this.valueDisplay.style.display = 'flex';
  }

  _applyDirectInput() {
    if (!this.directInput || this.directInput.style.display === 'none') return;
    const parsed = parseFloat(this.directInput.value);
    if (!isNaN(parsed)) {
      this.setValue(parsed, true);
    }
    this._hideDirectInput();
  }

  _valueToNormalized(val) {
    const clamped = Math.max(this.min, Math.min(this.max, val));
    if (this.isLog) {
      const minLog = Math.log(Math.max(0.0001, this.min));
      const maxLog = Math.log(this.max);
      return (Math.log(Math.max(0.0001, clamped)) - minLog) / (maxLog - minLog);
    }
    return (clamped - this.min) / (this.max - this.min);
  }

  _normalizedToValue(norm) {
    let val;
    if (this.isLog) {
      const minLog = Math.log(Math.max(0.0001, this.min));
      const maxLog = Math.log(this.max);
      val = Math.exp(minLog + norm * (maxLog - minLog));
    } else {
      val = this.min + norm * (this.max - this.min);
    }
    // Round to step
    if (this.step > 0) {
      val = Math.round(val / this.step) * this.step;
    }
    return Math.max(this.min, Math.min(this.max, val));
  }

  setValue(val, notify = false) {
    const clamped = Math.max(this.min, Math.min(this.max, val));
    this.value = clamped;

    const norm = this._valueToNormalized(this.value);
    const angle = this.startAngle + norm * this.angleRange;

    if (this.cap) {
      this.cap.style.transform = `rotate(${angle}deg)`;
    }

    if (this.fillCircle) {
      // Stroke dash offset calculation
      const offset = this.arcLength * (1 - norm);
      this.fillCircle.style.strokeDashoffset = `${offset}`;
      this.fillCircle.setAttribute('stroke-dashoffset', `${offset}`);
    }

    const formatted = this.formatValue(this.value);
    if (this.valueText && this._lastFormatted !== formatted) {
      this.valueText.textContent = formatted;
      this._lastFormatted = formatted;
    }

    if (this.element && this._lastAria !== this.value) {
      this.element.setAttribute('aria-valuenow', this.value);
      this._lastAria = this.value;
    }

    if (notify && typeof this.onChange === 'function') {
      this.onChange(this.value, this.paramId);
    }
  }

  formatValue(val) {
    if (this.precision === 0) {
      return Math.round(val).toString();
    }
    return val.toFixed(this.precision);
  }

  destroy() {
    this.isDragging = false;
    if (this.element) {
      this.element.classList.remove('is-dragging');
      this.element.classList.remove('is-active');
    }
    if (this.container && this.element && this.element.parentNode === this.container) {
      this.container.removeChild(this.element);
    }
    this.element = null;
    this.assembly = null;
    this.cap = null;
    this.fillCircle = null;
    this.valueText = null;
    this.directInput = null;
  }
}
