/**
 * @file verify.mjs
 * @brief Automated Verification Suite for BRAUN MR-16 Web Showcase
 */

import { describe, it } from 'node:test';
import assert from 'node:assert';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

describe('BRAUN MR-16 Verification Suite', () => {

  //----------------------------------------------------------------------------
  describe('1. Dieter Rams Sibling Aesthetic Parity Check', () => {
    it('verifies exact AS-42 & RB-26 color token parity in style.css', () => {
      const cssPath = path.join(__dirname, 'css', 'style.css');
      const css = fs.readFileSync(cssPath, 'utf8');

      assert.ok(css.includes('#ECEBE4'), 'Light chassis #ECEBE4 must be defined');
      assert.ok(css.includes('#E2E0D8'), 'Panel surface #E2E0D8 must be defined');
      assert.ok(css.includes('#141517'), 'Dark anthracite chassis #141517 must be defined');
      assert.ok(css.includes('#EE592B'), 'Braun accent orange #EE592B must be defined');
      assert.ok(css.includes('#24FF6A'), 'P1 Phosphor green #24FF6A must be defined');
      assert.ok(css.includes('#FFB000') || css.includes('#E5A93C'), 'P3 Amber phosphor / accent must be defined');
      assert.ok(css.includes('--font-sans'), 'DIN 1451 typography font family must be defined');
      assert.ok(css.includes('--font-mono'), 'Monospace readout font family must be defined');
    });

    it('verifies tablet touch-scrolling rules in html and body', () => {
      const cssPath = path.join(__dirname, 'css', 'style.css');
      const css = fs.readFileSync(cssPath, 'utf8');

      assert.ok(css.includes('overscroll-behavior-y: auto'), 'html/body must allow vertical overscroll');
      assert.ok(css.includes('-webkit-overflow-scrolling: touch'), 'iOS momentum scrolling must be enabled');
      assert.ok(css.includes('touch-action: pan-y'), 'touch-action pan-y must be set on body');
    });

    it('verifies 19-inch 2U rack enclosure styling in rack.css', () => {
      const rackPath = path.join(__dirname, 'css', 'rack.css');
      const rack = fs.readFileSync(rackPath, 'utf8');

      assert.ok(rack.includes('.braun-rack-ear'), 'Rack ears must be styled');
      assert.ok(rack.includes('.braun-rack-screw'), 'Hex countersunk rack screws must be styled');
      assert.ok(rack.includes('.braun-rack-grid'), 'Multi-deck rack grid layout must be defined');
      assert.ok(rack.includes('clip-path: polygon'), 'Hex countersunk screw socket must be clipped');
      assert.ok(rack.includes('@media (max-width: 1100px)') || rack.includes('@media (max-width: 1040px)'), 'Tablet breakpoint must be defined');
      assert.ok(rack.includes('@media (max-width: 820px)'), 'Mobile/portrait breakpoint must be defined');
    });
  });

  //----------------------------------------------------------------------------
  describe('2. 19" Rackmount 7-Deck Chassis & HTML DOM Hierarchy', () => {
    it('verifies all 7 decks exist in index.html', () => {
      const htmlPath = path.join(__dirname, 'index.html');
      const html = fs.readFileSync(htmlPath, 'utf8');

      assert.ok(html.includes('01</span>'), 'Deck 01 number must be present');
      assert.ok(html.includes('KINETIC EXCITER ENGINE'), 'Deck 01 title must be present');

      assert.ok(html.includes('02</span>'), 'Deck 02 number must be present');
      assert.ok(html.includes('16-POLE MODAL RESONATOR'), 'Deck 02 title must be present');

      assert.ok(html.includes('03</span>'), 'Deck 03 number must be present');
      assert.ok(html.includes('KINETIC MORPH &amp; 3D ATTRACTOR') || html.includes('KINETIC MORPH & 3D ATTRACTOR'), 'Deck 03 title must be present');

      assert.ok(html.includes('04</span>'), 'Deck 04 number must be present');
      assert.ok(html.includes('TRI-PHASE SPATIAL BBD CHORUS'), 'Deck 04 title must be present');

      assert.ok(html.includes('05</span>'), 'Deck 05 number must be present');
      assert.ok(html.includes('SPATIAL DISPERSION &amp; DYNAMICS') || html.includes('SPATIAL DISPERSION & DYNAMICS'), 'Deck 05 title must be present');

      assert.ok(html.includes('06</span>'), 'Deck 06 number must be present');
      assert.ok(html.includes('VECTOR PHOSPHOR CRT'), 'Deck 06 title must be present');

      assert.ok(html.includes('BRAUN') && html.includes('MR-16'), 'Deck 07 top brand header must be present');
    });

    it('verifies all rotary knob containers exist in index.html', () => {
      const htmlPath = path.join(__dirname, 'index.html');
      const html = fs.readFileSync(htmlPath, 'utf8');

      const expectedKnobs = [
        // Deck 01
        'knob-exciter-velocity', 'knob-exciter-hardness', 'knob-vactrol-sag',
        'knob-friction-velocity', 'knob-friction-force', 'knob-ext-input-gain',
        'knob-poisson-density', 'knob-euclidean-pulses', 'knob-euclidean-steps',
        // Deck 02
        'knob-modal-frequency', 'knob-modal-spread', 'knob-modal-damping',
        'knob-modal-q', 'knob-modal-coupling',
        // Deck 03
        'knob-attractor-rate', 'knob-attractor-chaos',
        'knob-attractor-freq-mod', 'knob-attractor-q-mod',
        // Deck 04
        'knob-chorus-rate', 'knob-chorus-depth', 'knob-chorus-dimension', 'knob-chorus-mix',
        // Deck 05
        'knob-spatial-pan', 'knob-dynamics-lpg', 'knob-dynamics-drive',
        'knob-master-trim', 'knob-master-mix',
        // Deck 06
        'knob-crt-intensity'
      ];

      for (const k of expectedKnobs) {
        assert.ok(html.includes(`id="${k}"`), `DOM container id="${k}" must exist`);
      }
    });

    it('verifies CRT canvas and controls exist in index.html', () => {
      const htmlPath = path.join(__dirname, 'index.html');
      const html = fs.readFileSync(htmlPath, 'utf8');

      assert.ok(html.includes('id="crt-canvas"'), 'Canvas #crt-canvas must exist');
      assert.ok(html.includes('id="group-crt-mode"'), 'CRT mode selector group must exist');
      assert.ok(html.includes('id="group-phosphor-type"'), 'Phosphor type selector must exist');
      assert.ok(html.includes('id="btn-power"'), 'Power switch must exist');
    });

    it('verifies tactile performance controls and audition bar in index.html', () => {
      const htmlPath = path.join(__dirname, 'index.html');
      const html = fs.readFileSync(htmlPath, 'utf8');

      assert.ok(html.includes('id="btn-audition-impulse"'), 'Dirac impulse audition button must exist');
      assert.ok(html.includes('id="btn-audition-hammer"'), 'Felt hammer audition button must exist');
      assert.ok(html.includes('id="btn-audition-friction"'), 'Bowed friction audition button must exist');
      assert.ok(html.includes('id="btn-audition-air"'), 'Optical air jet audition button must exist');
      assert.ok(html.includes('id="btn-audition-poisson"'), 'Poisson rain audition button must exist');

      assert.ok(html.includes('id="btn-strike-dirac"'), 'Strike dirac button must exist');
      assert.ok(html.includes('id="btn-strike-hammer"'), 'Strike hammer button must exist');
      assert.ok(html.includes('id="btn-strike-friction"'), 'Strike friction button must exist');
      assert.ok(html.includes('id="btn-strike-air"'), 'Strike air jet button must exist');

      assert.ok(html.includes('id="chime-strip"'), '16-key microtonal chime strip container must exist');
      assert.ok(html.includes('id="select-scale"'), 'Scale tuning select must exist');
      assert.ok(html.includes('id="select-root"'), 'Root key select must exist');
    });

    it('verifies master utilities exist in index.html', () => {
      const htmlPath = path.join(__dirname, 'index.html');
      const html = fs.readFileSync(htmlPath, 'utf8');

      assert.ok(html.includes('id="select-theme"'), 'Finish theme select must exist');
      assert.ok(html.includes('id="select-preset"'), 'Preset select must exist');
      assert.ok(html.includes('id="btn-save-patch"'), 'Save patch button must exist');
      assert.ok(html.includes('id="btn-export-patch"'), 'Export patch button must exist');
      assert.ok(html.includes('id="btn-load-patch"'), 'Load patch button must exist');
      assert.ok(html.includes('id="btn-ab-toggle"'), 'A/B toggle button must exist');
      assert.ok(html.includes('id="btn-ab-copy"'), 'A/B copy button must exist');
      assert.ok(html.includes('id="btn-record-wav"'), 'Lossless REC WAV button must exist');
      assert.ok(html.includes('id="btn-reset-all"'), 'Reset All button must exist');
    });
  });

  //----------------------------------------------------------------------------
  describe('3. Technical Typography & Character Encoding Audit', () => {
    it('verifies standard technical character encoding across all web assets', () => {
      const nonAsciiSymbolRegex = /[\u{1F300}-\u{1F9FF}]|[\u{2600}-\u{26FF}]|[\u{2700}-\u{27BF}]/u;
      const filesToCheck = [
        path.join(__dirname, 'index.html'),
        path.join(__dirname, 'css', 'style.css'),
        path.join(__dirname, 'css', 'rack.css'),
        path.join(__dirname, 'js', 'app.js'),
        path.join(__dirname, 'js', 'ui', 'knob.js'),
        path.join(__dirname, 'js', 'ui', 'crt-display.js'),
        path.join(__dirname, 'js', 'audio', 'mr16_web_engine.js'),
        path.join(__dirname, 'factory_presets.json')
      ];

      for (const filePath of filesToCheck) {
        if (fs.existsSync(filePath)) {
          const content = fs.readFileSync(filePath, 'utf8');
          const hasInvalidChar = nonAsciiSymbolRegex.test(content);
          assert.strictEqual(hasInvalidChar, false, `File ${path.basename(filePath)} adheres to technical character encoding`);
        }
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('4. Factory Presets Compliance & Parameter Bounds', () => {
    it('verifies factory_presets.json conforms to RFC 8259 with 16 presets', () => {
      const presetsPath = path.join(__dirname, 'factory_presets.json');
      assert.ok(fs.existsSync(presetsPath), 'factory_presets.json must exist');

      const raw = fs.readFileSync(presetsPath, 'utf8');
      const data = JSON.parse(raw);

      assert.strictEqual(data.format, 'BRAUN_MR16_PRESET_COLLECTION');
      assert.strictEqual(data.device, 'BRAUN_MR16');
      assert.ok(Array.isArray(data.presets), 'presets must be an array');
      assert.strictEqual(data.presets.length, 16, 'Exactly 16 curated factory presets must exist');

      for (const preset of data.presets) {
        assert.ok(preset.id, 'Preset must have an id');
        assert.ok(preset.name, 'Preset must have a human-readable name');
        assert.ok(preset.params, `Preset ${preset.id} must have params`);

        const p = preset.params;
        // Deck 01
        assert.ok(p.exciter_type >= 0 && p.exciter_type <= 3, 'exciter_type in [0, 3]');
        assert.ok(p.strike_hardness >= 0.0 && p.strike_hardness <= 1.0, 'strike_hardness in [0, 1]');
        assert.ok(p.strike_velocity >= 0.0 && p.strike_velocity <= 1.0, 'strike_velocity in [0, 1]');
        // Deck 02
        assert.ok(p.manifold_type >= 0 && p.manifold_type <= 3, 'manifold_type in [0, 3]');
        assert.ok(p.modal_frequency >= 20.0 && p.modal_frequency <= 2000.0, 'modal_frequency in [20, 2000]');
        assert.ok(p.modal_damping >= 0.05 && p.modal_damping <= 10.0, 'modal_damping in [0.05, 10]');
        assert.ok(p.material_profile >= 0 && p.material_profile <= 4, 'material_profile in [0, 4]');
        // Deck 04
        assert.ok(typeof p.chorus_enable === 'boolean', 'chorus_enable must be boolean');
        assert.ok(p.chorus_rate_hz >= 0.05 && p.chorus_rate_hz <= 8.0, 'chorus_rate_hz in [0.05, 8.0]');
        assert.ok(p.chorus_mix >= 0.0 && p.chorus_mix <= 100.0, 'chorus_mix in [0, 100]');
        // Deck 05
        assert.ok(p.golden_pan_spread >= 0.0 && p.golden_pan_spread <= 100.0, 'golden_pan_spread in [0, 100]');
        assert.ok(p.drive_saturation >= 0.0 && p.drive_saturation <= 100.0, 'drive_saturation in [0, 100]');
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('5. Web Audio DSP Mathematical Constants & Transfer Curves', () => {
    it('verifies 4 acoustic manifolds contain 16 monotonic ratios', async () => {
      const engineModule = await import('./js/audio/mr16_web_engine.js');
      const ratios = engineModule.MANIFOLD_RATIOS;

      assert.ok(ratios.CHLADNI.length === 16, 'CHLADNI must have 16 modes');
      assert.ok(ratios.BEAM.length === 16, 'BEAM must have 16 modes');
      assert.ok(ratios.VOCAL.length === 16, 'VOCAL must have 16 modes');
      assert.ok(ratios.HORN.length === 16, 'HORN must have 16 modes');

      for (const key of ['CHLADNI', 'BEAM', 'VOCAL', 'HORN']) {
        const arr = ratios[key];
        assert.strictEqual(arr[0], 1.0, `${key} fundamental mode ratio must be 1.0`);
        for (let i = 1; i < arr.length; i++) {
          assert.ok(arr[i] > arr[i - 1], `${key} mode ratios must increase monotonically: ${arr[i]} > ${arr[i-1]}`);
        }
      }
    });

    it('verifies Hermite soft-knee wave shaper curve monotonicity and bounds', async () => {
      const engineModule = await import('./js/audio/mr16_web_engine.js');
      const curve = engineModule.generateHermiteCurve(1024);

      assert.strictEqual(curve.length, 1024);
      assert.ok(Math.abs(curve[512]) < 0.01, 'Center index must be close to 0');

      for (let i = 1; i < curve.length; i++) {
        assert.ok(curve[i] >= curve[i - 1] - 1e-6, `Hermite curve must be monotonically non-decreasing at index ${i}`);
        assert.ok(Math.abs(curve[i]) <= 1.06, `Hermite curve must be bounded within [-1.06, 1.06] at index ${i}`);
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('6. Master Utilities: Lossless 16-Bit 48kHz WAV Recording & A/B Comparison Buffer', () => {
    it('verifies MasterWavRecorder generates bit-exact 44-byte RIFF/WAVE header and 16-bit PCM encoding', async () => {
      const appModule = await import('./js/app.js');
      assert.ok(typeof appModule.MasterWavRecorder === 'function', 'MasterWavRecorder must be exported from app.js');

      const recorder = new appModule.MasterWavRecorder({ ctx: { sampleRate: 48000 } });
      assert.ok(typeof recorder.encodeWAV === 'function', 'MasterWavRecorder must have encodeWAV method');

      const numSamples = 500;
      const left = new Float32Array(numSamples);
      const right = new Float32Array(numSamples);
      for (let i = 0; i < numSamples; i++) {
        left[i] = Math.sin((i / numSamples) * Math.PI * 2) * 0.8;
        right[i] = Math.cos((i / numSamples) * Math.PI * 2) * 0.8;
      }

      const blob = recorder.encodeWAV(left, right, 48000);
      assert.strictEqual(blob.type, 'audio/wav', 'Blob type must be audio/wav');

      const arrayBuf = await blob.arrayBuffer();
      const view = new DataView(arrayBuf);

      // 1. RIFF Chunk Descriptor
      const riff = String.fromCharCode(view.getUint8(0), view.getUint8(1), view.getUint8(2), view.getUint8(3));
      assert.strictEqual(riff, 'RIFF', 'RIFF chunk identifier must match');
      const wave = String.fromCharCode(view.getUint8(8), view.getUint8(9), view.getUint8(10), view.getUint8(11));
      assert.strictEqual(wave, 'WAVE', 'WAVE format identifier must match');

      // 2. "fmt " Sub-chunk
      const fmt = String.fromCharCode(view.getUint8(12), view.getUint8(13), view.getUint8(14), view.getUint8(15));
      assert.strictEqual(fmt, 'fmt ', 'fmt sub-chunk identifier must match');
      assert.strictEqual(view.getUint32(16, true), 16, 'Subchunk1Size must be 16 for PCM');
      assert.strictEqual(view.getUint16(20, true), 1, 'AudioFormat must be 1 (linear PCM)');
      assert.strictEqual(view.getUint16(22, true), 2, 'NumChannels must be 2 (Stereo)');
      assert.strictEqual(view.getUint32(24, true), 48000, 'SampleRate must be 48000 Hz');
      assert.strictEqual(view.getUint32(28, true), 48000 * 2 * 2, 'ByteRate must be 192000 B/s');
      assert.strictEqual(view.getUint16(32, true), 4, 'BlockAlign must be 4 (2 channels * 2 bytes)');
      assert.strictEqual(view.getUint16(34, true), 16, 'BitsPerSample must be 16');

      // 3. "data" Sub-chunk
      const dataStr = String.fromCharCode(view.getUint8(36), view.getUint8(37), view.getUint8(38), view.getUint8(39));
      assert.strictEqual(dataStr, 'data', 'data sub-chunk identifier must match');
      const dataSize = numSamples * 2 * 2; // 2 channels * 2 bytes per sample
      assert.strictEqual(view.getUint32(40, true), dataSize, 'Subchunk2Size must match payload length');
      assert.strictEqual(arrayBuf.byteLength, 44 + dataSize, 'Total file byte length must be exactly 44 header + data');
    });
  });

  //----------------------------------------------------------------------------
  describe('7. Zero-Dependency Static Server & Launcher Script', () => {
    it('verifies root server.js config and port 3816', () => {
      const serverPath = path.join(__dirname, '..', 'server.js');
      assert.ok(fs.existsSync(serverPath), 'server.js must exist in project root');

      const content = fs.readFileSync(serverPath, 'utf8');
      assert.ok(content.includes('3816'), 'server.js must listen on port 3816');
      assert.ok(content.includes('http.createServer'), 'server.js must create standard HTTP server');
      assert.ok(content.includes('MIME_TYPES'), 'server.js must define static MIME types');
    });

    it('verifies root start.bat launcher script', () => {
      const batPath = path.join(__dirname, '..', 'start.bat');
      assert.ok(fs.existsSync(batPath), 'start.bat must exist in project root');

      const content = fs.readFileSync(batPath, 'utf8');
      assert.ok(content.includes('3816'), 'start.bat must target port 3816');
      assert.ok(content.includes('node server.js'), 'start.bat must invoke node server.js');
      assert.ok(content.includes('http://localhost:3816'), 'start.bat must launch browser at port 3816');
    });
  });
});
