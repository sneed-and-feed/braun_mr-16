/**
 * @file test-ui-audit.mjs
 * @brief Automated UI Layout & CRT Display Verification Suite for BRAUN MR-16
 * Strictly adheres to Dieter Rams functionalist principles and DIN 1451 technical English.
 * Zero emojis in tests, assertions, and console logs.
 */

import { describe, it } from 'node:test';
import assert from 'node:assert';
import fs from 'node:fs';
import path from 'node:path';
import http from 'node:http';
import { spawn } from 'node:child_process';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const ROOT_DIR = path.resolve(__dirname, '..');
const WEB_DIR = __dirname;

describe('BRAUN MR-16 UI Layout & CRT Display Audit Suite', () => {

  //----------------------------------------------------------------------------
  describe('1. 19-Inch 2U Studio Rack & DOM Hierarchy Constraints', () => {
    const htmlPath = path.join(WEB_DIR, 'index.html');
    const html = fs.readFileSync(htmlPath, 'utf8');

    it('verifies 19-inch rack outer assembly, chassis faceplate, and hex screws', () => {
      assert.ok(html.includes('class="braun-rack-outer"'), 'Chassis outer container must exist');
      assert.ok(html.includes('class="braun-rack-ear ear-left"'), 'Left rack mounting ear must exist');
      assert.ok(html.includes('class="braun-rack-ear ear-right"'), 'Right rack mounting ear must exist');
      assert.ok(html.includes('class="braun-rack-screw"'), 'Precision hex socket screws must exist');
      assert.ok(html.includes('class="braun-chassis" id="braun-chassis"'), 'Main 2U faceplate chassis must exist');
    });

    it('verifies 7-deck signal flow architecture and master header', () => {
      assert.ok(html.includes('KINETIC EXCITER ENGINE'), 'Deck 01 title must exist');
      assert.ok(html.includes('16-POLE MODAL RESONATOR'), 'Deck 02 title must exist');
      assert.ok(html.includes('KINETIC MORPH &amp; 3D ATTRACTOR') || html.includes('KINETIC MORPH & 3D ATTRACTOR'), 'Deck 03 title must exist');
      assert.ok(html.includes('TRI-PHASE SPATIAL BBD CHORUS'), 'Deck 04 title must exist');
      assert.ok(html.includes('SPATIAL DISPERSION &amp; DYNAMICS') || html.includes('SPATIAL DISPERSION & DYNAMICS'), 'Deck 05 title must exist');
      assert.ok(html.includes('VECTOR PHOSPHOR CRT'), 'Deck 06 title must exist');
      assert.ok(html.includes('class="braun-header"'), 'Master header / Deck 07 must exist');
      assert.ok(html.includes('class="braun-performance-section"'), 'Tactile performance section must exist');
      assert.ok(html.includes('class="braun-footer"'), 'Dieter Rams manifesto footer must exist');
    });

    it('verifies all 28 rotary knob DOM containers exist', () => {
      const knobIds = [
        // Deck 01 (9 knobs)
        'knob-exciter-velocity', 'knob-exciter-hardness', 'knob-vactrol-sag',
        'knob-friction-velocity', 'knob-friction-force', 'knob-ext-input-gain',
        'knob-poisson-density', 'knob-euclidean-pulses', 'knob-euclidean-steps',
        // Deck 02 (5 knobs)
        'knob-modal-frequency', 'knob-modal-spread', 'knob-modal-damping',
        'knob-modal-q', 'knob-modal-coupling',
        // Deck 03 (6 knobs)
        'knob-attractor-rate', 'knob-attractor-chaos',
        'knob-attractor-freq-mod', 'knob-attractor-q-mod',
        'knob-morph-alpha', 'knob-lorenz-steer',
        // Deck 04 (4 knobs)
        'knob-chorus-rate', 'knob-chorus-depth', 'knob-chorus-dimension', 'knob-chorus-mix',
        // Deck 05 (5 knobs)
        'knob-spatial-pan', 'knob-dynamics-lpg', 'knob-dynamics-drive',
        'knob-master-trim', 'knob-master-mix',
        // Deck 06 (1 knob)
        'knob-crt-intensity'
      ];

      for (const id of knobIds) {
        assert.ok(html.includes(`id="${id}"`), `DOM container id="${id}" must exist`);
      }
      assert.strictEqual(knobIds.length, 30, 'Total knobs accounted for across all 6 operational decks');
    });

    it('verifies Deck 03 Sample Extraction dropzone and Slot A/B selectors', () => {
      assert.ok(html.includes('id="sample-dropzone"'), 'Sample extraction dropzone container must exist in Deck 03');
      assert.ok(html.includes('id="dropzone-target"'), 'Clickable and droppable target must exist in dropzone');
      assert.ok(html.includes('id="group-profile-slot"'), 'Profile slot segment selector group must exist');
      assert.ok(html.includes('id="btn-slot-a"'), 'Slot A button must exist');
      assert.ok(html.includes('id="btn-slot-b"'), 'Slot B button must exist');
      assert.ok(html.includes('id="btn-load-sample"'), 'Extract sample button must exist');
      assert.ok(html.includes('id="input-load-sample"'), 'File input for audio extraction must exist');
      assert.ok(html.includes('id="knob-morph-alpha"'), 'Morph A/B knob must exist in Deck 03');
      assert.ok(html.includes('id="knob-lorenz-steer"'), 'Lorenz steer knob must exist in Deck 03');
    });

    it('verifies Deck 06 CRT canvas and visualizer control elements', () => {
      assert.ok(html.includes('id="crt-canvas"'), 'Canvas #crt-canvas must exist');
      assert.ok(html.includes('width="580"'), 'Canvas logical width must be 580');
      assert.ok(html.includes('height="260"'), 'Canvas logical height must be 260');
      assert.ok(html.includes('id="group-crt-mode"'), 'CRT mode selector group must exist');
      assert.ok(html.includes('id="group-phosphor-type"'), 'Phosphor type selector must exist');
      assert.ok(html.includes('id="knob-crt-intensity"'), 'Persistence intensity knob container must exist');
    });

    it('verifies tactile performance controls and audition bar elements', () => {
      assert.ok(html.includes('id="chime-strip"'), '16-key microtonal chime strip container must exist');
      assert.ok(html.includes('id="select-scale"'), 'Scale tuning select must exist');
      assert.ok(html.includes('id="select-root"'), 'Scale root select must exist');
      assert.ok(html.includes('id="btn-strike-dirac"'), 'Dirac impulse strike button must exist');
      assert.ok(html.includes('id="btn-strike-hammer"'), 'Felt hammer strike button must exist');
      assert.ok(html.includes('id="btn-strike-friction"'), 'Bowed friction strike button must exist');
      assert.ok(html.includes('id="btn-strike-air"'), 'Optical air jet strike button must exist');
    });
  });

  //----------------------------------------------------------------------------
  describe('2. CSS Properties, Token Parity & Touch Action Specifications', () => {
    const styleCss = fs.readFileSync(path.join(WEB_DIR, 'css', 'style.css'), 'utf8');
    const rackCss = fs.readFileSync(path.join(WEB_DIR, 'css', 'rack.css'), 'utf8');

    it('verifies exact Dieter Rams color tokens (#ECEBE4, #141517, #EE592B, #24FF6A, #FFB000)', () => {
      assert.ok(styleCss.includes('#ECEBE4'), 'Chassis light token #ECEBE4 must be defined');
      assert.ok(styleCss.includes('#141517'), 'Anthracite dark token #141517 must be defined');
      assert.ok(styleCss.includes('#EE592B'), 'Braun accent orange #EE592B must be defined');
      assert.ok(styleCss.includes('#24FF6A'), 'P1 Phosphor green #24FF6A must be defined');
      assert.ok(styleCss.includes('#FFB000'), 'P3 Phosphor amber #FFB000 must be defined');
    });

    it('verifies DIN 1451 font family declarations', () => {
      assert.ok(styleCss.includes('--font-sans'), 'Sans font variable must be defined');
      assert.ok(styleCss.includes('DIN 1451'), 'DIN 1451 Mittelschrift must be referenced');
      assert.ok(styleCss.includes('--font-mono'), 'Mono font variable must be defined');
    });

    it('verifies touch actions: pan-y on scrollable containers and none on interactive targets', () => {
      assert.ok(styleCss.includes('touch-action: pan-y'), 'body / wrappers must declare touch-action: pan-y');
      assert.ok(rackCss.includes('touch-action: pan-y'), 'rack chassis must declare touch-action: pan-y');
      assert.ok(styleCss.includes('.braun-knob-assembly'), 'Knob assembly selector must exist');
      assert.ok(styleCss.includes('.braun-chime-key'), 'Chime key selector must exist');
      assert.ok(styleCss.includes('touch-action: none'), 'Knob assembly and chime keys must declare touch-action: none');
    });

    it('verifies 4 distinct knob sizing classes: Hero 64px, Secondary 52px, Standard 42px, Compact 32px', () => {
      assert.ok(styleCss.includes('.braun-knob-hero .braun-knob-assembly') && styleCss.includes('64px'), 'Hero knob 64px must be defined');
      assert.ok(styleCss.includes('.braun-knob-secondary .braun-knob-assembly') && styleCss.includes('52px'), 'Secondary knob 52px must be defined');
      assert.ok(styleCss.includes('.braun-knob-standard .braun-knob-assembly') && styleCss.includes('42px'), 'Standard knob 42px must be defined');
      assert.ok(styleCss.includes('.braun-knob-compact .braun-knob-assembly') && styleCss.includes('32px'), 'Compact knob 32px must be defined');
    });

    it('verifies hero knob orange indicator tick and active hover states', () => {
      assert.ok(styleCss.includes('.braun-knob-hero .braun-knob-indicator'), 'Hero knob indicator rule must be defined');
      assert.ok(styleCss.includes('.braun-knob-wrapper.is-active .braun-knob-indicator'), 'Active knob indicator state must be defined');
    });

    it('verifies 1360px chassis centering and responsive breakpoints', () => {
      assert.ok(rackCss.includes('max-width: 1360px') || rackCss.includes('max-width: 1420px'), 'Chassis max-width constraint must be defined');
      assert.ok(rackCss.includes('@media (max-width: 1100px)'), 'Tablet breakpoint @media (max-width: 1100px) must be defined');
      assert.ok(rackCss.includes('@media (max-width: 820px)'), 'Mobile breakpoint @media (max-width: 820px) must be defined');
    });

    it('verifies Deck 03 Sample Extraction dropzone styling in style.css', () => {
      assert.ok(styleCss.includes('.braun-sample-dropzone'), '.braun-sample-dropzone must be styled');
      assert.ok(styleCss.includes('.braun-dropzone-target'), '.braun-dropzone-target must be styled');
      assert.ok(styleCss.includes('.braun-dropzone-text'), '.braun-dropzone-text must be styled');
      assert.ok(styleCss.includes('.braun-sample-dropzone.is-drag-over'), 'Drag-over visual highlight must be styled');
    });
  });

  //----------------------------------------------------------------------------
  describe('3. CRT Phosphor Display Engine Specifications', () => {
    const crtJsPath = path.join(WEB_DIR, 'js', 'ui', 'crt-display.js');
    const crtJs = fs.readFileSync(crtJsPath, 'utf8');

    it('verifies 2x Retina pixel density scaling implementation', () => {
      assert.ok(crtJs.includes('window.devicePixelRatio'), 'Must inspect window.devicePixelRatio');
      assert.ok(crtJs.includes('Math.max(2,'), 'Must scale at least 2x for high-DPI displays');
      assert.ok(crtJs.includes('ctx.scale(dpr, dpr)'), 'Canvas context must be scaled by DPR');
    });

    it('verifies 10x8 precision graticule with center crosshair calibration ticks', () => {
      assert.ok(crtJs.includes('const numX = 10;'), 'Graticule must declare 10 horizontal divisions');
      assert.ok(crtJs.includes('const numY = 8;'), 'Graticule must declare 8 vertical divisions');
      assert.ok(crtJs.includes('const ticksPerDiv = 5;'), 'Center crosshair must declare 5 ticks per division');
      assert.ok(crtJs.includes('_updateGraticuleCache'), 'Graticule must be cached offscreen');
    });

    it('verifies Chladni biharmonic particle physics with 450 particles and analytical gradient', () => {
      assert.ok(crtJs.includes('this.numParticles = 450;'), 'Must allocate exactly 450 particles');
      assert.ok(crtJs.includes('this.particles = new Float32Array(this.numParticles * 4);'), 'Float32Array for particle states');
      assert.ok(crtJs.includes('gradX') && crtJs.includes('gradY'), 'Analytical gradient must compute nodal acceleration');
      assert.ok(crtJs.includes('m === n'), 'Degenerate mode pairs must be eliminated');
    });

    it('verifies continuous 3D Lorenz attractor projection and 1000-point ring buffer', () => {
      assert.ok(crtJs.includes('this.attractorHistorySize = 1000;'), 'Must maintain 1000-point ring buffer');
      assert.ok(crtJs.includes('this.orbitAngle +='), 'Orbit angle must continuously advance');
      assert.ok(crtJs.includes('fov = 300;') || crtJs.includes('fov = 300'), 'Perspective projection must declare 300 fov');
      assert.ok(crtJs.includes('camDist = 68;') || crtJs.includes('camDist = 68'), 'Camera distance must declare 68 units');
      assert.ok(crtJs.includes('batchSize'), 'Trajectory must be rendered in continuous smooth batches');
    });

    it('verifies 16-pole modal FFT spectrum with ballistic peak-hold needles', () => {
      assert.ok(crtJs.includes('const numBars = 16;'), 'Spectrum meter must declare 16 bars');
      assert.ok(crtJs.includes('this.modalPeaks = new Float32Array(16);'), 'Must track 16 modal peaks');
      assert.ok(crtJs.includes('this.modalPeakHolds = new Float32Array(16);'), 'Must track 16 peak-hold timers');
      assert.ok(crtJs.includes('this.modalPeakHolds[i] = 0.8;'), 'Peak hold duration must be 0.8 seconds');
    });
  });

  //----------------------------------------------------------------------------
  describe('4. Strict Austerity: Zero Emoji Compliance Audit', () => {
    it('verifies 0 emojis in all HTML, CSS, JS, and JSON files', () => {
      const emojiRegex = /[\u{1F300}-\u{1F9FF}]|[\u{2600}-\u{26FF}]|[\u{2700}-\u{27BF}]/u;
      const filesToCheck = [
        path.join(WEB_DIR, 'index.html'),
        path.join(WEB_DIR, 'css', 'style.css'),
        path.join(WEB_DIR, 'css', 'rack.css'),
        path.join(WEB_DIR, 'js', 'app.js'),
        path.join(WEB_DIR, 'js', 'ui', 'knob.js'),
        path.join(WEB_DIR, 'js', 'ui', 'crt-display.js'),
        path.join(WEB_DIR, 'js', 'audio', 'mr16_web_engine.js'),
        path.join(WEB_DIR, 'js', 'audio', 'modal_extractor.js'),
        path.join(WEB_DIR, 'factory_presets.json'),
        path.join(WEB_DIR, 'test-ui-audit.mjs')
      ];

      for (const filePath of filesToCheck) {
        if (fs.existsSync(filePath)) {
          const content = fs.readFileSync(filePath, 'utf8');
          const hasEmoji = emojiRegex.test(content);
          assert.strictEqual(hasEmoji, false, `File ${path.basename(filePath)} must contain ZERO emojis`);
        }
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('5. Headless Browser Live Viewport & DOM E2E Audit', () => {
    it('runs headless browser inspection across 4K, Laptop, and Tablet viewports via Edge CDP', async () => {
      const edgePath = 'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe';
      if (process.env.CI || !fs.existsSync(edgePath)) {
        console.log('[BrowserAudit] Skipping headless CDP browser session in CI environment.');
        return;
      }

      const PORT = 3817;
      const CDP_PORT = 9224;

      // Start static HTTP server for web directory
      const mimeMap = {
        '.html': 'text/html; charset=utf-8',
        '.js': 'application/javascript; charset=utf-8',
        '.mjs': 'application/javascript; charset=utf-8',
        '.css': 'text/css; charset=utf-8',
        '.json': 'application/json; charset=utf-8',
        '.svg': 'image/svg+xml'
      };

      const server = http.createServer((req, res) => {
        let reqPath = req.url.split('?')[0];
        if (reqPath === '/' || reqPath === '') reqPath = '/index.html';
        const safePath = path.normalize(reqPath).replace(/^(\.\.[\/\\])+/, '');
        const filePath = path.join(WEB_DIR, safePath);

        fs.stat(filePath, (err, stats) => {
          if (err || !stats.isFile()) {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            res.end('Not Found');
            return;
          }
          const ext = path.extname(filePath).toLowerCase();
          const contentType = mimeMap[ext] || 'application/octet-stream';
          res.writeHead(200, {
            'Content-Type': contentType,
            'Access-Control-Allow-Origin': '*'
          });
          fs.createReadStream(filePath).pipe(res);
        });
      });

      await new Promise((resolve, reject) => {
        server.once('error', reject);
        server.listen(PORT, resolve);
      });

      const userDataDir = path.join(os.tmpdir(), `mr16_edge_audit_${Date.now()}`);
      const edgeProc = spawn(edgePath, [
        '--headless',
        `--remote-debugging-port=${CDP_PORT}`,
        `--user-data-dir=${userDataDir}`,
        '--no-first-run',
        '--no-default-browser-check',
        '--autoplay-policy=no-user-gesture-required',
        `http://localhost:${PORT}/index.html`
      ], { stdio: 'ignore' });

      let ws = null;
      try {
        let connected = false;
        let targetTab = null;
        for (let i = 0; i < 25; i++) {
          await new Promise((r) => setTimeout(r, 200));
          try {
            const res = await fetch(`http://127.0.0.1:${CDP_PORT}/json/list`);
            const tabs = await res.json();
            targetTab = tabs.find((t) => t.url.includes(`localhost:${PORT}`)) || tabs[0];
            if (targetTab && targetTab.webSocketDebuggerUrl) {
              connected = true;
              break;
            }
          } catch (_) {}
        }

        assert.ok(connected && targetTab, 'Connected to headless Edge via CDP');

        ws = new WebSocket(targetTab.webSocketDebuggerUrl);
        await new Promise((resolve) => {
          ws.onopen = resolve;
        });

        let msgId = 100;
        const pending = new Map();
        const pageExceptions = [];

        ws.onmessage = (event) => {
          const msg = JSON.parse(event.data);
          if (msg.method === 'Runtime.exceptionThrown') {
            pageExceptions.push(msg.params.exceptionDetails);
          }
          if (msg.id && pending.has(msg.id)) {
            const { resolve, reject } = pending.get(msg.id);
            pending.delete(msg.id);
            if (msg.error) reject(msg.error);
            else resolve(msg.result);
          }
        };

        const call = (method, params = {}) => {
          return new Promise((resolve, reject) => {
            const id = ++msgId;
            pending.set(id, { resolve, reject });
            ws.send(JSON.stringify({ id, method, params }));
          });
        };

        const evaluate = async (expr) => {
          const res = await call('Runtime.evaluate', {
            expression: expr,
            returnByValue: true,
            awaitPromise: true
          });
          if (res.exceptionDetails) {
            throw new Error('Eval Exception: ' + JSON.stringify(res.exceptionDetails));
          }
          return res.result.value;
        };

        await call('Page.enable');
        await call('Runtime.enable');

        // Allow DOM to settle
        await new Promise((r) => setTimeout(r, 1200));

        // 1. Document title & zero errors
        const title = await evaluate('document.title');
        assert.ok(title.includes('BRAUN'), 'Page title must reference BRAUN');
        assert.strictEqual(pageExceptions.length, 0, 'Zero runtime exceptions thrown on initial load');

        // 2. window.__MR16__ instance verification
        const appInitialized = await evaluate('Boolean(window.__MR16__)');
        assert.strictEqual(appInitialized, true, 'window.__MR16__ must be initialized');

        const knobCount = await evaluate('Object.keys(window.__MR16__.knobs).length');
        assert.ok(knobCount >= 28, `Expected at least 28 instantiated knobs, got ${knobCount}`);

        // 3. Viewport 1: 4K Desktop (2560x1440, DPR 2)
        await call('Emulation.setDeviceMetricsOverride', {
          width: 2560,
          height: 1440,
          deviceScaleFactor: 2,
          mobile: false
        });
        await new Promise((r) => setTimeout(r, 200));

        const metrics4K = await evaluate(`({
          chassisWidth: document.getElementById('braun-chassis').getBoundingClientRect().width,
          outerWidth: document.querySelector('.braun-rack-outer').getBoundingClientRect().width,
          bodyScrollHeight: document.body.scrollHeight,
          canvasWidth: document.getElementById('crt-canvas').width,
          canvasHeight: document.getElementById('crt-canvas').height,
          canvasClientWidth: document.getElementById('crt-canvas').clientWidth,
          canvasClientHeight: document.getElementById('crt-canvas').clientHeight,
          dpr: window.devicePixelRatio
        })`);
        assert.ok(metrics4K.chassisWidth <= 1361, `4K chassis width (${metrics4K.chassisWidth}px) must be <= 1361px`);
        assert.ok(metrics4K.bodyScrollHeight <= 1300, `Natural vertical flow (${metrics4K.bodyScrollHeight}px) should be <= 1300px`);
        assert.strictEqual(metrics4K.canvasWidth, Math.floor(metrics4K.canvasClientWidth * metrics4K.dpr), 'CRT canvas width scaled exactly by devicePixelRatio');
        assert.strictEqual(metrics4K.canvasHeight, Math.floor(metrics4K.canvasClientHeight * metrics4K.dpr), 'CRT canvas height scaled exactly by devicePixelRatio');

        // 4. Viewport 2: Laptop (1366x768, DPR 1)
        await call('Emulation.setDeviceMetricsOverride', {
          width: 1366,
          height: 768,
          deviceScaleFactor: 1,
          mobile: false
        });
        await new Promise((r) => setTimeout(r, 200));

        const metricsLaptop = await evaluate(`({
          chassisWidth: document.getElementById('braun-chassis').getBoundingClientRect().width,
          hasHorizontalOverflow: document.documentElement.scrollWidth > document.documentElement.clientWidth
        })`);
        assert.ok(metricsLaptop.chassisWidth <= 1361, 'Laptop chassis must be <= 1361px');
        assert.strictEqual(metricsLaptop.hasHorizontalOverflow, false, 'No horizontal overflow on 1366x768 laptop');

        // 5. Viewport 3: iPad Landscape (1024x768, DPR 2, Mobile touch)
        await call('Emulation.setDeviceMetricsOverride', {
          width: 1024,
          height: 768,
          deviceScaleFactor: 2,
          mobile: true
        });
        await new Promise((r) => setTimeout(r, 200));

        const metricsTabletLandscape = await evaluate(`({
          hasHorizontalOverflow: document.documentElement.scrollWidth > document.documentElement.clientWidth,
          dropzoneVisible: Boolean(document.getElementById('sample-dropzone')),
          chassisWidth: document.getElementById('braun-chassis').getBoundingClientRect().width
        })`);
        assert.strictEqual(metricsTabletLandscape.hasHorizontalOverflow, false, 'No horizontal overflow on iPad landscape');
        assert.strictEqual(metricsTabletLandscape.dropzoneVisible, true, 'Sample dropzone must be present on iPad');

        // 6. Viewport 4: iPad Portrait (768x1024, DPR 2, Mobile touch)
        await call('Emulation.setDeviceMetricsOverride', {
          width: 768,
          height: 1024,
          deviceScaleFactor: 2,
          mobile: true
        });
        await new Promise((r) => setTimeout(r, 200));

        const metricsTabletPortrait = await evaluate(`({
          hasHorizontalOverflow: document.documentElement.scrollWidth > document.documentElement.clientWidth,
          earsHidden: window.getComputedStyle(document.querySelector('.braun-rack-ear')).display === 'none'
        })`);
        assert.strictEqual(metricsTabletPortrait.hasHorizontalOverflow, false, 'No horizontal overflow on iPad portrait');
        assert.strictEqual(metricsTabletPortrait.earsHidden, true, 'Rack ears must hide on narrow portrait viewports');

        // 7. Live zero emoji audit in rendered DOM
        const emojiCount = await evaluate(`
          (() => {
            const text = document.body.innerText;
            const emojiRegex = /[\\u{1F300}-\\u{1F9FF}]|[\\u{2600}-\\u{26FF}]|[\\u{2700}-\\u{27BF}]/u;
            const matches = text.match(new RegExp(emojiRegex, 'gu'));
            return matches ? matches.length : 0;
          })()
        `);
        assert.strictEqual(emojiCount, 0, 'Zero emojis detected in live rendered DOM text');

      } finally {
        if (ws) {
          try { ws.close(); } catch (_) {}
        }
        edgeProc.kill();
        server.close();
      }
    });
  });

  //----------------------------------------------------------------------------
  describe('6. State Synchronization, Pointer Capture & Accessibility Audit', () => {
    const appJsPath = path.join(WEB_DIR, 'js', 'app.js');
    const appJs = fs.readFileSync(appJsPath, 'utf8');
    const knobJsPath = path.join(WEB_DIR, 'js', 'ui', 'knob.js');
    const knobJs = fs.readFileSync(knobJsPath, 'utf8');
    const crtJsPath = path.join(WEB_DIR, 'js', 'ui', 'crt-display.js');
    const crtJs = fs.readFileSync(crtJsPath, 'utf8');

    it('verifies BraunCrtDisplay lifecycle methods, resize throttling, and listener cleanup', () => {
      assert.ok(crtJs.includes('this._resizeHandler'), 'Must store bound resize handler for unregistration');
      assert.ok(crtJs.includes('_requestResize'), 'Must throttle resize handling through requestAnimationFrame');
      assert.ok(crtJs.includes('destroy()'), 'Must declare destroy lifecycle method');
      assert.ok(crtJs.includes("window.removeEventListener('resize'"), 'Must remove window resize listener on destroy');
      assert.ok(crtJs.includes('this._resizeObserver.disconnect()'), 'Must disconnect ResizeObserver on destroy');
    });

    it('verifies BraunKnob pointer capture release on blur and lostpointercapture', () => {
      assert.ok(knobJs.includes("window.addEventListener('blur', onPointerUp)"), 'Must listen to window blur to prevent stuck dragging');
      assert.ok(knobJs.includes('lostpointercapture'), 'Must listen to lostpointercapture event');
      assert.ok(knobJs.includes("window.removeEventListener('blur', onPointerUp)"), 'Must remove window blur listener on pointer release');
      assert.ok(knobJs.includes('destroy()'), 'Must implement destroy method for DOM and state cleanup');
    });

    it('verifies MasterWavRecorder RIFF header bit-exactness, PCM clamping, and NaN safety', async () => {
      const { MasterWavRecorder } = await import('./js/app.js');
      const recorder = new MasterWavRecorder({ ctx: { sampleRate: 48000 } });

      // Test extreme inputs: clipping beyond [-1, 1], NaN, and +/- Infinity
      const testSamples = [1.5, -2.5, NaN, Infinity, -Infinity, 0.5, -0.5, 0.0];
      const left = new Float32Array(testSamples);
      const right = new Float32Array(testSamples);

      const blob = recorder.encodeWAV(left, right, 48000);
      assert.strictEqual(blob.type, 'audio/wav', 'Exported mime type must be audio/wav');

      const arrayBuf = await blob.arrayBuffer();
      const view = new DataView(arrayBuf);

      // Verify 44-byte RIFF header
      assert.strictEqual(String.fromCharCode(view.getUint8(0), view.getUint8(1), view.getUint8(2), view.getUint8(3)), 'RIFF');
      assert.strictEqual(view.getUint32(4, true), 36 + testSamples.length * 4);
      assert.strictEqual(String.fromCharCode(view.getUint8(8), view.getUint8(9), view.getUint8(10), view.getUint8(11)), 'WAVE');
      assert.strictEqual(String.fromCharCode(view.getUint8(12), view.getUint8(13), view.getUint8(14), view.getUint8(15)), 'fmt ');
      assert.strictEqual(view.getUint32(16, true), 16); // Subchunk1Size
      assert.strictEqual(view.getUint16(20, true), 1);  // PCM format
      assert.strictEqual(view.getUint16(22, true), 2);  // Stereo 2 channels
      assert.strictEqual(view.getUint32(24, true), 48000);
      assert.strictEqual(view.getUint32(28, true), 48000 * 4); // ByteRate
      assert.strictEqual(view.getUint16(32, true), 4);  // BlockAlign
      assert.strictEqual(view.getUint16(34, true), 16); // BitsPerSample
      assert.strictEqual(String.fromCharCode(view.getUint8(36), view.getUint8(37), view.getUint8(38), view.getUint8(39)), 'data');
      assert.strictEqual(view.getUint32(40, true), testSamples.length * 4);

      // Verify clamped PCM values (clamped to [-32768, 32767], NaN/Infinity mapped to safe values)
      let offset = 44;
      for (let i = 0; i < testSamples.length; i++) {
        const valL = view.getInt16(offset, true);
        const valR = view.getInt16(offset + 2, true);
        assert.ok(valL >= -32768 && valL <= 32767, `PCM left sample ${i} (${valL}) must be within 16-bit range`);
        assert.ok(valR >= -32768 && valR <= 32767, `PCM right sample ${i} (${valR}) must be within 16-bit range`);
        offset += 4;
      }

      // Sample 0: 1.5 -> 32767
      assert.strictEqual(view.getInt16(44, true), 32767);
      // Sample 1: -2.5 -> -32768
      assert.strictEqual(view.getInt16(48, true), -32768);
      // Sample 2: NaN -> 0
      assert.strictEqual(view.getInt16(52, true), 0);
      // Sample 3: Infinity -> 32767 (clamped to +1.0) or 0
      assert.ok(view.getInt16(56, true) === 0 || view.getInt16(56, true) === 32767);
    });

    it('verifies A/B buffer state synchronization, dynamic copy button label, and parameter preservation', () => {
      assert.ok(appJs.includes('_updateAbUi'), 'Must declare _updateAbUi method');
      assert.ok(appJs.includes('COPY ${this.activeBuffer}&rarr;${target}'), 'Copy button must dynamically update direction');
      assert.ok(appJs.includes("id === 'modal_coupling' && val <= 1.0"), 'Must scale normalized modal_coupling to percentage for knob visual sync');
      assert.ok(appJs.includes("id === 'chorus_dimension' && val <= 1.0"), 'Must scale normalized chorus_dimension to percentage for knob visual sync');
      assert.ok(appJs.includes('group-dimension-mode'), 'Must synchronize dimension mode segment button cluster on parameter update');
    });

    it('verifies patch JSON parsing, 5 MB file size limit enforcement, and error resilience', () => {
      assert.ok(appJs.includes('5 * 1024 * 1024'), 'Must enforce 5 MB file size limit on patch upload');
      assert.ok(appJs.includes('50 * 1024 * 1024'), 'Must enforce 50 MB file size limit on audio sample extraction');
      assert.ok(appJs.includes('reader.onerror'), 'Must handle file reader IO errors');
      assert.ok(appJs.includes('Unrecognized patch format'), 'Must notify user when JSON is not a valid MR-16 patch or profile');
      assert.ok(appJs.includes("e.target.value = ''"), 'Must clear patch input value after loading to allow reloading same file');
    });

    it('verifies Spacebar Dirac impulse guard and Escape key cancellation in app.js', () => {
      assert.ok(appJs.includes("e.key === 'Escape'"), 'Must handle Escape key globally');
      assert.ok(appJs.includes('_hideDirectInput'), 'Must hide direct knob text inputs on Escape');
      assert.ok(appJs.includes("e.target.tagName === 'TEXTAREA'"), 'Spacebar handler must ignore TEXTAREA elements');
      assert.ok(appJs.includes('isContentEditable'), 'Spacebar handler must ignore contentEditable elements');
      assert.ok(appJs.includes("e.target.tagName === 'BUTTON'"), 'Spacebar handler must ignore focused buttons to allow standard keyboard activation');
    });
  });

});
