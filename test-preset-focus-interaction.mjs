/**
 * BRAUN MR-16 Preset Keyboard Focus & Hotkey Interaction Test Suite
 *
 * Validates that:
 * 1. Selecting an option in #select-preset blurs the select element immediately.
 * 2. Pressing chime hotkeys (a, w, s, e, ...) or Spacebar while #select-preset or any
 *    select dropdown is focused triggers the chime/Dirac sound and does NOT change
 *    the preset value or cycle dropdown options.
 * 3. #select-theme, #select-scale, and #select-root immediately blur on change.
 * 4. Dedicated keydown and global keydown interceptors prevent focus entrapment.
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
const WEB_DIR = __dirname;

describe('BRAUN MR-16 Preset Focus & Hotkey Interaction Suite', () => {
  const appJsPath = path.join(WEB_DIR, 'js', 'app.js');
  const appJs = fs.readFileSync(appJsPath, 'utf8');

  //----------------------------------------------------------------------------
  describe('1. Static Code Analysis & Contract Verification', () => {
    it('verifies selectPreset change listener blurs select and document.activeElement', () => {
      assert.ok(
        appJs.includes("this.dom.selectPreset?.addEventListener('change'"),
        'Must register change listener on selectPreset'
      );
      assert.ok(
        appJs.includes('this.dom.selectPreset.blur()'),
        'Must call selectPreset.blur() on change'
      );
    });

    it('verifies selectPreset does not close prematurely on pointerup or click, but retains blur and keydown listeners', () => {
      assert.strictEqual(
        appJs.includes("this.dom.selectPreset?.addEventListener('pointerup'"),
        false,
        'Must NOT register premature pointerup blur listener on selectPreset'
      );
      assert.strictEqual(
        appJs.includes("this.dom.selectPreset?.addEventListener('click'"),
        false,
        'Must NOT register premature click blur listener on selectPreset'
      );
      assert.ok(
        appJs.includes("this.dom.selectPreset?.addEventListener('blur'"),
        'Must register blur listener on selectPreset'
      );
      assert.ok(
        appJs.includes("this.dom.selectPreset?.addEventListener('keydown'"),
        'Must register keydown listener on selectPreset'
      );
    });

    it('verifies dedicated keydown listener on selectPreset intercepts Space and CHIME_HOTKEYS', () => {
      assert.ok(
        appJs.includes("e.code === 'Space' || CHIME_HOTKEYS.includes(keyLower)"),
        'selectPreset keydown must detect Space and CHIME_HOTKEYS'
      );
      assert.ok(
        appJs.includes('this.engine.triggerDirac()'),
        'Must trigger Dirac impulse on Space'
      );
      assert.ok(
        appJs.includes('this.triggerChimeKey(idx, 0.85)'),
        'Must trigger chime key on CHIME_HOTKEYS'
      );
    });

    it('verifies global keydown listener allows SELECT elements to trigger hotkeys', () => {
      assert.ok(
        appJs.includes("if (e.target.tagName === 'SELECT')"),
        'Global keydown listener must inspect SELECT elements'
      );
      assert.ok(
        appJs.includes("e.code === 'Space' || CHIME_HOTKEYS.includes(keyLower)"),
        'Global keydown listener must detect Space or CHIME_HOTKEYS on SELECT'
      );
    });

    it('verifies global keydown listener allows BUTTON elements to trigger hotkeys', () => {
      assert.ok(
        appJs.includes("else if (e.target.tagName === 'BUTTON')"),
        'Global keydown listener must inspect BUTTON elements'
      );
      assert.ok(
        appJs.includes('clearButtonFocus'),
        'Must register auto-blur handlers for buttons and toggles'
      );
    });

    it('verifies selectScale, selectRoot, and selectTheme blur on change', () => {
      assert.ok(
        appJs.includes('this.dom.selectScale.blur()'),
        'selectScale must blur on change'
      );
      assert.ok(
        appJs.includes('this.dom.selectRoot.blur()'),
        'selectRoot must blur on change'
      );
      assert.ok(
        appJs.includes('this.dom.selectTheme.blur()'),
        'selectTheme must blur on change'
      );
    });

    it('verifies repeat keydown events are ignored in global and selectPreset listeners', () => {
      const globalKeydownMatch = appJs.match(/window\.addEventListener\('keydown',\s*\(e\)\s*=>\s*\{\s*if\s*\(e\.repeat\)\s*return;/);
      assert.ok(globalKeydownMatch, 'Global keydown listener must immediately return on repeat events');

      const selectKeydownMatch = appJs.match(/this\.dom\.selectPreset\?\.addEventListener\('keydown',\s*\(e\)\s*=>\s*\{\s*if\s*\(e\.repeat\)\s*return;/);
      assert.ok(selectKeydownMatch, 'selectPreset keydown listener must immediately return on repeat events');
    });
  });

  //----------------------------------------------------------------------------
  describe('2. Live Headless Browser E2E Interaction Test', () => {
    it('verifies hotkeys trigger sound and do not cycle presets when select is focused', async () => {
      const edgePath = 'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe';
      if (!fs.existsSync(edgePath)) {
        console.log('[PresetTest] Edge not found; skipping headless browser live session.');
        return;
      }

      const PORT = 3819;
      const CDP_PORT = 9225;

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

      const userDataDir = path.join(os.tmpdir(), `mr16_preset_audit_${Date.now()}`);
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
        let targetTab = null;
        for (let i = 0; i < 25; i++) {
          await new Promise((r) => setTimeout(r, 200));
          try {
            const res = await fetch(`http://127.0.0.1:${CDP_PORT}/json/list`);
            const tabs = await res.json();
            targetTab = tabs.find((t) => t.url.includes(`localhost:${PORT}`)) || tabs[0];
            if (targetTab && targetTab.webSocketDebuggerUrl) break;
          } catch (_) {}
        }

        assert.ok(targetTab && targetTab.webSocketDebuggerUrl, 'Must acquire Edge CDP WebSocket debugger URL');

        ws = new WebSocket(targetTab.webSocketDebuggerUrl);
        await new Promise((resolve, reject) => {
          ws.onopen = resolve;
          ws.onerror = reject;
        });

        let msgId = 1;
        const pending = new Map();
        ws.onmessage = (event) => {
          const msg = JSON.parse(event.data);
          if (msg.id && pending.has(msg.id)) {
            pending.get(msg.id)(msg);
            pending.delete(msg.id);
          }
        };

        const call = (method, params = {}) => {
          const id = msgId++;
          return new Promise((resolve, reject) => {
            pending.set(id, (res) => {
              if (res.error) reject(new Error(`CDP Error [${method}]: ${res.error.message}`));
              else resolve(res.result);
            });
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

        // Allow app initialization
        await new Promise((r) => setTimeout(r, 1200));

        // Instrument chime triggers and dirac triggers
        await evaluate(`
          window.__test_chime_calls = [];
          window.__test_dirac_calls = 0;
          const origTriggerChimeKey = window.__MR16__.triggerChimeKey.bind(window.__MR16__);
          window.__MR16__.triggerChimeKey = function(keyIndex, velocity) {
            window.__test_chime_calls.push({ keyIndex, velocity });
            return origTriggerChimeKey(keyIndex, velocity);
          };
          const origTriggerDirac = window.__MR16__.engine.triggerDirac.bind(window.__MR16__.engine);
          window.__MR16__.engine.triggerDirac = function() {
            window.__test_dirac_calls++;
            return origTriggerDirac();
          };
        `);

        // Test 1: Verify selecting an option blurs select-preset immediately
        const changeResult = await evaluate(`
          (() => {
            const sel = document.getElementById('select-preset');
            sel.focus();
            const initialFocus = (document.activeElement === sel);
            sel.value = 'MARIMBA_ROSEWOOD_BAR';
            sel.dispatchEvent(new Event('change', { bubbles: true }));
            const blurredAfterChange = (document.activeElement !== sel);
            return { initialFocus, blurredAfterChange, activeValue: sel.value };
          })()
        `);
        assert.strictEqual(changeResult.initialFocus, true, 'select-preset was successfully focused');
        assert.strictEqual(changeResult.blurredAfterChange, true, 'select-preset must blur immediately on change');
        assert.strictEqual(changeResult.activeValue, 'MARIMBA_ROSEWOOD_BAR', 'Preset was updated to MARIMBA_ROSEWOOD_BAR');

        // Test 2: Focus select-preset and press chime hotkey 'a' (keyIndex 0)
        const hotkeyResultA = await evaluate(`
          (() => {
            const sel = document.getElementById('select-preset');
            sel.focus();
            const initialPreset = sel.value;
            const event = new KeyboardEvent('keydown', {
              key: 'a',
              code: 'KeyA',
              bubbles: true,
              cancelable: true
            });
            sel.dispatchEvent(event);
            return {
              presetAfter: sel.value,
              presetUnchanged: sel.value === initialPreset,
              blurred: (document.activeElement !== sel),
              chimeCalls: window.__test_chime_calls.length,
              lastChimeKey: window.__test_chime_calls[window.__test_chime_calls.length - 1]
            };
          })()
        `);
        assert.strictEqual(hotkeyResultA.presetUnchanged, true, 'Preset must NOT change when typing hotkey "a"');
        assert.strictEqual(hotkeyResultA.blurred, true, 'select-preset must be blurred when hotkey "a" is pressed');
        assert.strictEqual(hotkeyResultA.chimeCalls, 1, 'Chime trigger must be called once');
        assert.strictEqual(hotkeyResultA.lastChimeKey.keyIndex, 0, 'Chime hotkey "a" corresponds to keyIndex 0');

        // Test 3: Focus select-preset and press Spacebar (Dirac impulse)
        const spaceResult = await evaluate(`
          (() => {
            const sel = document.getElementById('select-preset');
            sel.focus();
            const initialPreset = sel.value;
            const event = new KeyboardEvent('keydown', {
              key: ' ',
              code: 'Space',
              bubbles: true,
              cancelable: true
            });
            sel.dispatchEvent(event);
            return {
              presetAfter: sel.value,
              presetUnchanged: sel.value === initialPreset,
              blurred: (document.activeElement !== sel),
              diracCalls: window.__test_dirac_calls
            };
          })()
        `);
        assert.strictEqual(spaceResult.presetUnchanged, true, 'Preset must NOT change when pressing Spacebar');
        assert.strictEqual(spaceResult.blurred, true, 'select-preset must be blurred when Spacebar is pressed');
        assert.strictEqual(spaceResult.diracCalls, 1, 'Dirac impulse must be triggered once on Spacebar');

        // Test 4: Focus select-scale and press chime hotkey 's' (keyIndex 1)
        const scaleHotkeyResult = await evaluate(`
          (() => {
            const sel = document.getElementById('select-scale');
            sel.focus();
            const event = new KeyboardEvent('keydown', {
              key: 's',
              code: 'KeyS',
              bubbles: true,
              cancelable: true
            });
            sel.dispatchEvent(event);
            return {
              activeElementId: document.activeElement ? document.activeElement.id : null,
              blurred: (document.activeElement !== sel),
              lastChimeKey: window.__test_chime_calls[window.__test_chime_calls.length - 1],
              totalChimes: window.__test_chime_calls.length
            };
          })()
        `);
        assert.strictEqual(scaleHotkeyResult.blurred, true, 'select-scale must blur on chime hotkey');
        assert.strictEqual(scaleHotkeyResult.lastChimeKey.keyIndex, 1, 'Hotkey "s" corresponds to keyIndex 1');

        // Test 5: Verify select-scale and select-root blur on change
        const otherSelectsBlurResult = await evaluate(`
          (() => {
            const scaleSel = document.getElementById('select-scale');
            scaleSel.focus();
            scaleSel.value = 'JUST_INTONATION';
            scaleSel.dispatchEvent(new Event('change', { bubbles: true }));
            const scaleBlurred = (document.activeElement !== scaleSel);

            const rootSel = document.getElementById('select-root');
            rootSel.focus();
            rootSel.value = 'C';
            rootSel.dispatchEvent(new Event('change', { bubbles: true }));
            const rootBlurred = (document.activeElement !== rootSel);

            const themeSel = document.getElementById('select-theme');
            themeSel.focus();
            themeSel.value = 'dark';
            themeSel.dispatchEvent(new Event('change', { bubbles: true }));
            const themeBlurred = (document.activeElement !== themeSel);

            return { scaleBlurred, rootBlurred, themeBlurred };
          })()
        `);
        assert.strictEqual(otherSelectsBlurResult.scaleBlurred, true, 'select-scale must blur on change');
        assert.strictEqual(otherSelectsBlurResult.rootBlurred, true, 'select-root must blur on change');
        assert.strictEqual(otherSelectsBlurResult.themeBlurred, true, 'select-theme must blur on change');

        // Test 6: Verify clicking chorus toggle blurs it and subsequent hotkey 'd' plays note immediately without window click
        const chorusToggleResult = await evaluate(`
          (() => {
            const chorusBtn = document.getElementById('btn-chorus-enable');
            chorusBtn.click();
            const blurredAfterClick = (document.activeElement !== chorusBtn);

            // Now dispatch hotkey 'd' (keyIndex 2)
            const event = new KeyboardEvent('keydown', {
              key: 'd',
              code: 'KeyD',
              bubbles: true,
              cancelable: true
            });
            window.dispatchEvent(event);

            return {
              blurredAfterClick,
              lastChimeKey: window.__test_chime_calls[window.__test_chime_calls.length - 1],
              totalChimes: window.__test_chime_calls.length
            };
          })()
        `);
        assert.strictEqual(chorusToggleResult.blurredAfterClick, true, 'Chorus toggle button must blur immediately after click');
        assert.strictEqual(chorusToggleResult.lastChimeKey.keyIndex, 2, 'Hotkey "d" corresponds to keyIndex 2');

        // Test 7: Verify that if a button is explicitly focused, pressing chime hotkey 'f' triggers note and blurs
        const focusedButtonResult = await evaluate(`
          (() => {
            const limiterBtn = document.getElementById('btn-soft-limit');
            limiterBtn.focus();
            const initialFocus = (document.activeElement === limiterBtn);

            const event = new KeyboardEvent('keydown', {
              key: 'f',
              code: 'KeyF',
              bubbles: true,
              cancelable: true
            });
            limiterBtn.dispatchEvent(event);

            return {
              initialFocus,
              blurredAfterKey: (document.activeElement !== limiterBtn),
              lastChimeKey: window.__test_chime_calls[window.__test_chime_calls.length - 1]
            };
          })()
        `);
        assert.strictEqual(focusedButtonResult.initialFocus, true, 'Limiter button was successfully focused');
        assert.strictEqual(focusedButtonResult.blurredAfterKey, true, 'Limiter button must blur when hotkey is pressed');
        assert.strictEqual(focusedButtonResult.lastChimeKey.keyIndex, 3, 'Hotkey "f" corresponds to keyIndex 3');

        // Test 8: Verify that if a button is explicitly focused, pressing Spacebar triggers Dirac and blurs
        const focusedButtonSpaceResult = await evaluate(`
          (() => {
            const powerBtn = document.getElementById('btn-power');
            powerBtn.focus();
            const initialFocus = (document.activeElement === powerBtn);

            const event = new KeyboardEvent('keydown', {
              key: ' ',
              code: 'Space',
              bubbles: true,
              cancelable: true
            });
            powerBtn.dispatchEvent(event);

            return {
              initialFocus,
              blurredAfterKey: (document.activeElement !== powerBtn),
              diracCalls: window.__test_dirac_calls
            };
          })()
        `);
        assert.strictEqual(focusedButtonSpaceResult.initialFocus, true, 'Power button was successfully focused');
        assert.strictEqual(focusedButtonSpaceResult.blurredAfterKey, true, 'Power button must blur when Spacebar is pressed');
        assert.strictEqual(focusedButtonSpaceResult.diracCalls >= 2, true, 'Dirac impulse must be called on Spacebar');

        // Test 9: Verify repeat keydown events on window are ignored and do not re-trigger strikes
        const repeatEventsResult = await evaluate(`
          (() => {
            const initialDiracCalls = window.__test_dirac_calls;
            const initialChimeCalls = window.__test_chime_calls.length;

            // Dispatch repeat Space keydown
            const repeatSpaceEvent = new KeyboardEvent('keydown', {
              code: 'Space',
              repeat: true,
              bubbles: true,
              cancelable: true
            });
            window.dispatchEvent(repeatSpaceEvent);

            // Dispatch repeat '1' keydown
            const repeatOneEvent = new KeyboardEvent('keydown', {
              key: '1',
              repeat: true,
              bubbles: true,
              cancelable: true
            });
            window.dispatchEvent(repeatOneEvent);

            // Dispatch repeat chime hotkey 'a' keydown
            const repeatAEvent = new KeyboardEvent('keydown', {
              key: 'a',
              repeat: true,
              bubbles: true,
              cancelable: true
            });
            window.dispatchEvent(repeatAEvent);

            return {
              diracCallsAfter: window.__test_dirac_calls,
              chimeCallsAfter: window.__test_chime_calls.length,
              initialDiracCalls,
              initialChimeCalls
            };
          })()
        `);
        assert.strictEqual(
          repeatEventsResult.diracCallsAfter,
          repeatEventsResult.initialDiracCalls,
          'Dirac impulse must not re-trigger on repeat Space keydown'
        );
        assert.strictEqual(
          repeatEventsResult.chimeCallsAfter,
          repeatEventsResult.initialChimeCalls,
          'Chime strikes must not re-trigger on repeat keydown events'
        );

        // Test 10: Verify repeat keydown events when selectPreset is focused are also ignored
        const repeatPresetEventsResult = await evaluate(`
          (() => {
            const sel = document.getElementById('select-preset');
            sel.focus();

            const initialDiracCalls = window.__test_dirac_calls;
            const initialChimeCalls = window.__test_chime_calls.length;

            const repeatSpaceOnSelect = new KeyboardEvent('keydown', {
              code: 'Space',
              repeat: true,
              bubbles: true,
              cancelable: true
            });
            sel.dispatchEvent(repeatSpaceOnSelect);

            const repeatOneOnSelect = new KeyboardEvent('keydown', {
              key: '1',
              repeat: true,
              bubbles: true,
              cancelable: true
            });
            sel.dispatchEvent(repeatOneOnSelect);

            const repeatAOnSelect = new KeyboardEvent('keydown', {
              key: 'a',
              repeat: true,
              bubbles: true,
              cancelable: true
            });
            sel.dispatchEvent(repeatAOnSelect);

            return {
              diracCallsAfter: window.__test_dirac_calls,
              chimeCallsAfter: window.__test_chime_calls.length,
              initialDiracCalls,
              initialChimeCalls
            };
          })()
        `);
        assert.strictEqual(
          repeatPresetEventsResult.diracCallsAfter,
          repeatPresetEventsResult.initialDiracCalls,
          'Dirac impulse must not trigger on repeat Space keydown when preset select is focused'
        );
        assert.strictEqual(
          repeatPresetEventsResult.chimeCallsAfter,
          repeatPresetEventsResult.initialChimeCalls,
          'Chime strikes must not trigger on repeat keydown when preset select is focused'
        );

      } finally {
        if (ws) {
          try { ws.close(); } catch (_) {}
        }
        edgeProc.kill();
        server.close();
      }
    });
  });
});
