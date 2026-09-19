#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "PluginEditor.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

#if JUCE_WINDOWS
#include <windows.h>
#endif

// BinaryData inclusion for embedded web assets
#if __has_include(<BinaryData.h>)
#include <BinaryData.h>
#define MR16_HAS_BINARY_DATA 1
#elif __has_include("BinaryData.h")
#include "BinaryData.h"
#define MR16_HAS_BINARY_DATA 1
#else
#define MR16_HAS_BINARY_DATA 0
#endif

namespace {

// Clean embedded fallback HTML page adhering to Dieter Rams Braun design language
static const char* kEmbeddedBraunFallbackHtml = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>BRAUN MR-16 · Modaler Resonator & Kinetischer Impulssynthesizer</title>
<style>
  :root {
    --chassis-bg: #141517;
    --card-bg: #1C1D20;
    --border-color: #2E3035;
    --text-main: #ECEBE4;
    --text-dim: #7E8085;
    --braun-orange: #EE592B;
    --phosphor-green: #24FF6A;
    --phosphor-amber: #FFB000;
    --knob-cap: #26282C;
    --knob-border: #3A3C42;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
  body {
    background: var(--chassis-bg);
    color: var(--text-main);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "DIN 1451", Helvetica, Arial, sans-serif;
    padding: 16px;
    height: 100vh;
    overflow-y: auto;
  }
  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 2px solid var(--border-color);
    padding-bottom: 12px;
    margin-bottom: 16px;
  }
  .title-group h1 { font-size: 18px; font-weight: 700; letter-spacing: 2px; }
  .title-group p { font-size: 10px; color: var(--text-dim); letter-spacing: 1px; }
  .badge { background: var(--braun-orange); color: #fff; padding: 4px 10px; font-size: 10px; font-weight: 700; border-radius: 2px; letter-spacing: 1px; }
  
  .crt-container {
    background: #090B0A;
    border: 2px solid #202622;
    border-radius: 4px;
    padding: 12px;
    margin-bottom: 16px;
    display: flex;
    gap: 16px;
    align-items: center;
  }
  canvas { background: #000; border: 1px solid #162419; border-radius: 2px; }
  .meters { display: flex; flex-direction: column; gap: 4px; flex: 1; font-size: 10px; font-family: monospace; }
  .meter-row { display: flex; align-items: center; gap: 8px; }
  .meter-bar { flex: 1; height: 6px; background: #18201B; border-radius: 2px; overflow: hidden; }
  .meter-fill { height: 100%; width: 0%; background: var(--phosphor-green); transition: width 0.05s; }
  
  .audition-bar {
    background: var(--card-bg);
    border: 1px solid var(--border-color);
    border-radius: 4px;
    padding: 10px;
    margin-bottom: 16px;
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
    align-items: center;
  }
  .audition-btn {
    background: var(--knob-cap);
    color: var(--text-main);
    border: 1px solid var(--border-color);
    padding: 6px 14px;
    font-size: 11px;
    font-weight: 600;
    cursor: pointer;
    border-radius: 2px;
  }
  .audition-btn:active { background: var(--braun-orange); color: #fff; }

  .rack-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 12px;
  }
  .deck {
    background: var(--card-bg);
    border: 1px solid var(--border-color);
    border-radius: 4px;
    padding: 12px;
  }
  .deck-header {
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 1.5px;
    color: var(--braun-orange);
    border-bottom: 1px solid var(--border-color);
    padding-bottom: 6px;
    margin-bottom: 10px;
    text-transform: uppercase;
  }
  .knob-row {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(65px, 1fr));
    gap: 10px;
    text-align: center;
  }
  .knob-unit { display: flex; flex-direction: column; align-items: center; gap: 4px; }
  .knob-label { font-size: 9px; color: var(--text-dim); letter-spacing: 0.5px; text-transform: uppercase; }
  .knob-val { font-size: 10px; font-weight: 600; color: var(--text-main); }
  input[type=range] {
    width: 100%;
    accent-color: var(--braun-orange);
    cursor: pointer;
  }
  select, button {
    background: var(--knob-cap);
    color: var(--text-main);
    border: 1px solid var(--border-color);
    padding: 4px 8px;
    border-radius: 2px;
    font-size: 11px;
    cursor: pointer;
  }
  select option {
    background: var(--card-bg);
    color: var(--text-main);
  }
</style>
</head>
<body>
<header>
  <div class="title-group">
    <h1>BRAUN MR-16</h1>
    <p>MODAL RESONATOR &amp; KINETIC IMPULSE SYNTHESIZER</p>
  </div>
  <div style="display:flex;gap:8px;align-items:center;">
    <button id="btnPower" class="badge">POWER ON</button>
    <button id="btnRec" class="audition-btn">REC WAV</button>
  </div>
</header>

<div class="audition-bar">
  <span style="font-size:10px;color:var(--text-dim);letter-spacing:1px;margin-right:4px;">AUDITION:</span>
  <button class="audition-btn" onmousedown="emitExciter({type:'strike',vel:0.8,hard:0.65})">DIRAC STRIKE</button>
  <button class="audition-btn" onmousedown="emitExciter({type:'strike',vel:0.9,hard:0.9})">HARD HAMMER</button>
  <button class="audition-btn" onmousedown="emitExciter({type:'friction',speed:0.7,force:0.6})">BOW FRICTION</button>
  <button class="audition-btn" onmousedown="emitExciter({type:'vactrol',amt:0.8})">OPTICAL PLUCK</button>
  <button class="audition-btn" id="btnPoisson" onclick="togglePoisson()">POISSON RAIN</button>
</div>

<div class="crt-container">
  <canvas id="scopeCanvas" width="300" height="120"></canvas>
  <div class="meters" id="modalMeters">
    <!-- 16 Modal Energy Bars -->
  </div>
</div>

<div class="rack-grid" id="deckGrid">
  <!-- Decks 01 through 05 will be populated dynamically -->
</div>

<script>
const paramsMeta = [
  // Deck 01
  { id: 'exciter_type', webId: 'exciterType', name: 'Exciter Model', deck: 1, min: 0, max: 3, def: 0, isChoice: true, choices: ['Strike', 'Friction', 'Vactrol', 'Ext In'] },
  { id: 'strike_hardness', webId: 'strikeHardness', name: 'Strike Hardness', deck: 1, min: 0, max: 1, def: 0.5, unit: '%' },
  { id: 'strike_velocity', webId: 'strikeVelocity', name: 'Strike Velocity', deck: 1, min: 0, max: 1, def: 0.8, unit: '%' },
  { id: 'friction_force', webId: 'frictionForce', name: 'Friction Force', deck: 1, min: 0, max: 1, def: 0.35, unit: '%' },
  { id: 'friction_speed', webId: 'frictionSpeed', name: 'Friction Speed', deck: 1, min: 0, max: 1, def: 0.4, unit: '%' },
  { id: 'vactrol_sag', webId: 'vactrolSag', name: 'Vactrol Sag', deck: 1, min: 0, max: 1, def: 0.6, unit: '%' },
  { id: 'ext_input_gain', webId: 'extInputGain', name: 'Ext Input Gain', deck: 1, min: -24, max: 12, def: 0, unit: 'dB' },
  { id: 'poisson_density', webId: 'poissonDensity', name: 'Poisson Density', deck: 1, min: 0, max: 25, def: 0, unit: 'Hz' },
  { id: 'euclidean_pulses', webId: 'euclideanPulses', name: 'Euclidean Pulses', deck: 1, min: 0, max: 32, def: 4, unit: '' },
  { id: 'euclidean_steps', webId: 'euclideanSteps', name: 'Euclidean Steps', deck: 1, min: 1, max: 32, def: 16, unit: '' },

  // Deck 02
  { id: 'manifold_type', webId: 'manifoldType', name: 'Manifold', deck: 2, min: 0, max: 3, def: 0, isChoice: true, choices: ['Chladni', 'Beam', 'Formant', 'Poincare'] },
  { id: 'modal_frequency', webId: 'modalFrequency', name: 'Fundamental', deck: 2, min: 20, max: 5000, def: 440, unit: 'Hz' },
  { id: 'modal_damping', webId: 'modalDamping', name: 'Damping', deck: 2, min: 0.005, max: 1, def: 0.15, unit: '%' },
  { id: 'material_profile', webId: 'materialProfile', name: 'Material', deck: 2, min: 0, max: 4, def: 0, isChoice: true, choices: ['Wood', 'Glass', 'Steel', 'Brass', 'Nylon'] },
  { id: 'modal_coupling', webId: 'modalCoupling', name: 'Coupling', deck: 2, min: 0, max: 1, def: 0.25, unit: '%' },
  { id: 'modal_spread', webId: 'modalSpread', name: 'Harmonic Spread', deck: 2, min: 0.2, max: 3, def: 1.0, unit: 'x' },

  // Deck 03
  { id: 'lorenz_rate', webId: 'lorenzRate', name: 'Chaos Rate', deck: 3, min: 0.01, max: 10, def: 0.5, unit: 'Hz' },
  { id: 'lorenz_chaos', webId: 'lorenzChaos', name: 'Chaos Depth', deck: 3, min: 0, max: 1, def: 0.5, unit: '%' },
  { id: 'lorenz_freq_mod', webId: 'lorenzFreqMod', name: 'Freq Mod', deck: 3, min: 0, max: 1, def: 0.2, unit: '%' },
  { id: 'lorenz_q_mod', webId: 'lorenzQMod', name: 'Damping Mod', deck: 3, min: 0, max: 1, def: 0.15, unit: '%' },

  // Deck 04
  { id: 'chorus_enable', webId: 'chorusEnable', name: 'Chorus Enable', deck: 4, min: 0, max: 1, def: 1, isBool: true },
  { id: 'chorus_rate_hz', webId: 'chorusRateHz', name: 'Chorus Rate', deck: 4, min: 0.05, max: 5, def: 0.8, unit: 'Hz' },
  { id: 'chorus_depth_ms', webId: 'chorusDepthMs', name: 'Chorus Depth', deck: 4, min: 0.1, max: 10, def: 2.5, unit: 'ms' },
  { id: 'chorus_dimension', webId: 'chorusDimension', name: 'Dimension Spread', deck: 4, min: 0, max: 1, def: 0.75, unit: '%' },
  { id: 'chorus_mix', webId: 'chorusMix', name: 'Chorus Mix', deck: 4, min: 0, max: 1, def: 0.5, unit: '%' },

  // Deck 05
  { id: 'golden_pan_spread', webId: 'goldenPanSpread', name: 'Spatial Pan', deck: 5, min: 0, max: 1, def: 0.8, unit: '%' },
  { id: 'vactrol_lpg_cutoff', webId: 'vactrolLpgCutoff', name: 'LPG Cutoff', deck: 5, min: 100, max: 20000, def: 12000, unit: 'Hz' },
  { id: 'drive_saturation', webId: 'driveSaturation', name: 'Drive Saturation', deck: 5, min: 0, max: 24, def: 0, unit: 'dB' },
  { id: 'master_trim_db', webId: 'masterTrimDb', name: 'Master Trim', deck: 5, min: -24, max: 12, def: 0, unit: 'dB' },
  { id: 'dry_wet_mix', webId: 'dryWetMix', name: 'Dry / Wet', deck: 5, min: 0, max: 1, def: 0.65, unit: '%' }
];

const deckNames = {
  1: 'Deck 01 · Kinetic Exciter',
  2: 'Deck 02 · 16-Pole Modal Matrix',
  3: 'Deck 03 · 3D Lorenz Attractor',
  4: 'Deck 04 · Tri-Phase BBD Chorus',
  5: 'Deck 05 · Spatial & Master Dynamics'
};

// Build Deck DOM
const deckGrid = document.getElementById('deckGrid');
const decks = {};
for (let d = 1; d <= 5; ++d) {
  const el = document.createElement('div');
  el.className = 'deck';
  el.innerHTML = `<div class="deck-header">${deckNames[d]}</div><div class="knob-row" id="deckRow_${d}"></div>`;
  deckGrid.appendChild(el);
  decks[d] = el.querySelector(`#deckRow_${d}`);
}

paramsMeta.forEach(p => {
  const ku = document.createElement('div');
  ku.className = 'knob-unit';
  ku.innerHTML = `<div class="knob-label">${p.name}</div>`;

  if (p.isChoice) {
    const sel = document.createElement('select');
    sel.id = 'ctrl_' + p.id;
    p.choices.forEach((c, idx) => {
      const opt = document.createElement('option');
      opt.value = idx;
      opt.textContent = c;
      if (idx === p.def) opt.selected = true;
      sel.appendChild(opt);
    });
    sel.onchange = () => emitParam(p.id, parseFloat(sel.value));
    ku.appendChild(sel);
  } else if (p.isBool) {
    const btn = document.createElement('button');
    btn.id = 'ctrl_' + p.id;
    btn.textContent = p.def > 0.5 ? 'ON' : 'OFF';
    btn.onclick = () => {
      const cur = btn.textContent === 'ON';
      btn.textContent = cur ? 'OFF' : 'ON';
      btn.style.color = cur ? 'var(--text-dim)' : 'var(--braun-orange)';
      emitParam(p.id, cur ? 0.0 : 1.0);
    };
    ku.appendChild(btn);
  } else {
    const valDisp = document.createElement('div');
    valDisp.className = 'knob-val';
    valDisp.id = 'val_' + p.id;
    valDisp.textContent = p.def.toFixed(2) + ' ' + (p.unit || '');
    ku.appendChild(valDisp);

    const rng = document.createElement('input');
    rng.type = 'range';
    rng.id = 'ctrl_' + p.id;
    rng.min = p.min;
    rng.max = p.max;
    rng.step = (p.max - p.min) / 100;
    rng.value = p.def;
    rng.oninput = () => {
      valDisp.textContent = parseFloat(rng.value).toFixed(2) + ' ' + (p.unit || '');
      emitParam(p.id, parseFloat(rng.value));
    };
    ku.appendChild(rng);
  }

  decks[p.deck].appendChild(ku);
});

// Setup 16 Modal Energy Bars
const metersContainer = document.getElementById('modalMeters');
for (let i = 0; i < 16; ++i) {
  const row = document.createElement('div');
  row.className = 'meter-row';
  row.innerHTML = `<span style="width:24px;">M${i+1}</span><div class="meter-bar"><div class="meter-fill" id="meter_${i}"></div></div>`;
  metersContainer.appendChild(row);
}

function emitParam(id, value) {
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.emitEvent('paramChange', { id, value });
  }
}

function emitExciter(data) {
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.emitEvent('exciterTrigger', data);
  }
}

let isPowered = true;
const pBtn = document.getElementById('btnPower');
if (pBtn) {
  pBtn.onclick = () => {
    isPowered = !isPowered;
    pBtn.textContent = isPowered ? 'POWER ON' : 'STANDBY';
    pBtn.style.background = isPowered ? 'var(--braun-orange)' : 'var(--knob-cap)';
    emitParam('power_state', isPowered ? 1.0 : 0.0);
  };
}

let isRecording = false;
const recBtn = document.getElementById('btnRec');
if (recBtn) {
  recBtn.onclick = () => {
    if (window.__JUCE__ && window.__JUCE__.backend) {
      if (!isRecording) {
        window.__JUCE__.backend.emitEvent('startRecording', {});
      } else {
        window.__JUCE__.backend.emitEvent('stopRecording', {});
      }
    }
  };
}

let poissonRunning = false;
function togglePoisson() {
  poissonRunning = !poissonRunning;
  const btn = document.getElementById('btnPoisson');
  if (btn) {
    btn.style.color = poissonRunning ? 'var(--braun-orange)' : 'var(--text-main)';
    btn.style.borderColor = poissonRunning ? 'var(--braun-orange)' : 'var(--border-color)';
  }
  emitParam('poisson_density', poissonRunning ? 8.0 : 0.0);
}

// Oscilloscope Canvas
const canvas = document.getElementById('scopeCanvas');
const ctx = canvas.getContext('2d');
let scopeData = new Float32Array(256);

function drawScope() {
  ctx.fillStyle = '#090B0A';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.strokeStyle = '#18241C';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(0, canvas.height / 2);
  ctx.lineTo(canvas.width, canvas.height / 2);
  ctx.stroke();

  ctx.strokeStyle = '#24FF6A';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  for (let i = 0; i < canvas.width; ++i) {
    const idx = Math.floor((i / canvas.width) * scopeData.length);
    const y = (canvas.height / 2) - (scopeData[idx] * (canvas.height / 2) * 0.9);
    if (i === 0) ctx.moveTo(i, y);
    else ctx.lineTo(i, y);
  }
  ctx.stroke();
  requestAnimationFrame(drawScope);
}
requestAnimationFrame(drawScope);

// JUCE Bridge Listeners
if (window.__JUCE__ && window.__JUCE__.backend) {
  window.__JUCE__.backend.addEventListener('paramUpdate', (data) => {
    if (!data || !data.id) return;
    const p = paramsMeta.find(m => m.id === data.id || m.webId === data.id);
    if (!p) return;
    const ctrl = document.getElementById('ctrl_' + p.id);
    const valDisp = document.getElementById('val_' + p.id);
    if (ctrl) {
      if (p.isChoice) ctrl.value = Math.round(data.value);
      else if (p.isBool) {
        ctrl.textContent = data.value > 0.5 ? 'ON' : 'OFF';
        ctrl.style.color = data.value > 0.5 ? 'var(--braun-orange)' : 'var(--text-dim)';
      }
      else ctrl.value = data.value;
    }
    if (valDisp) {
      valDisp.textContent = parseFloat(data.value).toFixed(2) + ' ' + (p.unit || '');
    }
  });

  window.__JUCE__.backend.addEventListener('recordingState', (data) => {
    isRecording = data.recording;
    if (recBtn) {
      recBtn.textContent = isRecording ? 'STOP REC' : 'REC WAV';
      recBtn.style.color = isRecording ? '#fff' : 'var(--text-main)';
      recBtn.style.background = isRecording ? 'var(--braun-orange)' : 'var(--knob-cap)';
    }
  });

  window.__JUCE__.backend.addEventListener('telemetryFrame', (data) => {
    if (!data) return;
    if (data.scopeL && data.scopeL.length > 0) {
      scopeData = new Float32Array(data.scopeL);
    }
    if (data.modalEnergies && data.modalEnergies.length >= 16) {
      for (let i = 0; i < 16; ++i) {
        const m = document.getElementById('meter_' + i);
        if (m) {
          const pct = Math.min(100, Math.max(0, data.modalEnergies[i] * 120));
          m.style.width = pct + '%';
        }
      }
    }
  });
}
</script>
</body>
</html>
)html";

} // namespace

//==============================================================================
juce::File BRAUN_MR16AudioProcessorEditor::getSettingsFile()
{
    auto appData = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    return appData.getChildFile("Braun").getChildFile("MR16_settings.xml");
}

bool BRAUN_MR16AudioProcessorEditor::loadPersistedNativeUIPreference()
{
    auto file = getSettingsFile();
    if (file.existsAsFile())
    {
        auto xml = juce::parseXML(file);
        if (xml != nullptr && xml->hasTagName("MR16_SETTINGS"))
        {
            return xml->getBoolAttribute("useNativeUI", false);
        }
    }
    return false;
}

void BRAUN_MR16AudioProcessorEditor::savePersistedNativeUIPreference(bool native)
{
    auto file = getSettingsFile();
    file.getParentDirectory().createDirectory();
    juce::XmlElement xml("MR16_SETTINGS");
    xml.setAttribute("useNativeUI", native);
    xml.writeTo(file);
}

//==============================================================================
BRAUN_MR16AudioProcessorEditor::BRAUN_MR16AudioProcessorEditor(BRAUN_MR16AudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    setLookAndFeel(&braunLookAndFeel);
    setOpaque(true);

    setupNativeControls();

    const bool defaultToNative = loadPersistedNativeUIPreference();

#if JUCE_WEB_BROWSER
    for (size_t i = 0; i < kNumParams; ++i)
    {
        pendingParamValues[i].store(0.0f, std::memory_order_relaxed);
        paramDirty[i].store(false, std::memory_order_relaxed);
    }

    webComponent = std::make_unique<juce::WebBrowserComponent>(createWebOptions(*this));
    webComponent->setOpaque(true);
    addAndMakeVisible(*webComponent);
    useNativeUI = defaultToNative;
    webComponent->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#else
    juce::ignoreUnused(defaultToNative);
    useNativeUI = true;
#endif

    setNativeMode(useNativeUI);
    registerParameterListeners();

    // Set editor default size to 1360x780 (resizable with limits 960x600 to 2560x1440) matching the wide 19" 2U rack.
    setSize(1360, 780);
    setResizable(true, true);
    setResizeLimits(960, 600, 2560, 1440);

    startTimerHz(60);
}

BRAUN_MR16AudioProcessorEditor::~BRAUN_MR16AudioProcessorEditor()
{
    stopTimer();
    unregisterParameterListeners();
    setLookAndFeel(nullptr);
}

void BRAUN_MR16AudioProcessorEditor::registerParameterListeners()
{
    for (const auto& meta : mr16::getParameterMetadataTable())
    {
        processorRef.getAPVTS().addParameterListener(meta.apvtsId, this);
    }
}

void BRAUN_MR16AudioProcessorEditor::unregisterParameterListeners()
{
    for (const auto& meta : mr16::getParameterMetadataTable())
    {
        processorRef.getAPVTS().removeParameterListener(meta.apvtsId, this);
    }
}

void BRAUN_MR16AudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
#if JUCE_WEB_BROWSER
    const auto& table = mr16::getParameterMetadataTable();
    for (size_t i = 0; i < table.size(); ++i)
    {
        if (parameterID == table[i].apvtsId)
        {
            pendingParamValues[i].store(newValue, std::memory_order_relaxed);
            paramDirty[i].store(true, std::memory_order_release);
            break;
        }
    }
#else
    juce::ignoreUnused(parameterID, newValue);
#endif
}

#if JUCE_WEB_BROWSER
juce::WebBrowserComponent::Options BRAUN_MR16AudioProcessorEditor::createWebOptions(BRAUN_MR16AudioProcessorEditor& editor)
{
#if JUCE_WINDOWS
    // Configure WebView2 Chromium flags for host DAW embedding (low latency, no audio contention, UI stability):
    // - Mute browser audio output (C++ DSP engine handles all audio synthesis)
    // - Disable Web MIDI in Chromium (prevents WinMM device contention with DAW MIDI inputs)
    // - Disable background Chromium features that create unneeded threads / network queries
    // - Disable CalculateNativeWinOcclusion to eliminate global SetWinEventHook desktop dragging lag
    // - Disable backgrounding and timer throttling for occluded windows to prevent dirty rect stalls
    _wputenv_s(
        L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS",
        L"--mute-audio "
        L"--disable-audio-output "
        L"--disable-web-midi "
        L"--disable-background-timer-throttling "
        L"--disable-backgrounding-occluded-windows "
        L"--disable-renderer-backgrounding "
        L"--disable-features=Translate,OptimizationHints,MediaRouter,InterestFeedContentSuggestions,CalculateNativeWinOcclusion"
    );
#endif

    auto options = juce::WebBrowserComponent::Options{}
#if JUCE_WINDOWS
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("BraunMR16_WebView2"))
                .withBackgroundColour(juce::Colour(0xff141517)))
#endif
        .withUserScript("window.__IS_JUCE__ = true; window.addEventListener('contextmenu', function(e) { if (!e.defaultPrevented) e.preventDefault(); }, false);")
        .withNativeIntegrationEnabled()
        .withResourceProvider([&editor](const juce::String& url) {
            return editor.getResource(url);
        })
        .withEventListener("paramChange", [&editor](const juce::var& data) {
            editor.handleParamChangeFromWeb(data);
        })
        .withEventListener("exciterTrigger", [&editor](const juce::var& data) {
            editor.handleExciterTriggerFromWeb(data);
        })
        .withEventListener("startRecording", [&editor](const juce::var& /*data*/) {
            editor.handleStartRecordingFromWeb();
        })
        .withEventListener("stopRecording", [&editor](const juce::var& /*data*/) {
            editor.handleStopRecordingFromWeb();
        })
        .withEventListener("showContextMenu", [&editor](const juce::var& data) {
            if (data.isObject())
            {
                const juce::String id = data.getProperty("id", "").toString();
                const int x = static_cast<int>(data.getProperty("x", 0));
                const int y = static_cast<int>(data.getProperty("y", 0));
                if (auto* slot = editor.findKnob(id))
                {
                    editor.showKnobContextMenu(*slot, { x, y });
                }
            }
        });

    return options;
}

std::optional<juce::WebBrowserComponent::Resource> BRAUN_MR16AudioProcessorEditor::getResource(const juce::String& url)
{
    juce::String path = url;

    // Strip virtual hostname (both with and without trailing slash, supporting https, http, and juce protocols)
    if (path.startsWithIgnoreCase("https://juce.backend/"))
        path = path.substring(21);
    else if (path.startsWithIgnoreCase("http://juce.backend/"))
        path = path.substring(20);
    else if (path.startsWithIgnoreCase("juce://juce.backend/"))
        path = path.substring(20);
    else if (path.startsWithIgnoreCase("https://juce.backend"))
        path = path.substring(20);
    else if (path.startsWithIgnoreCase("http://juce.backend"))
        path = path.substring(19);
    else if (path.startsWithIgnoreCase("juce://juce.backend"))
        path = path.substring(19);

    const int queryIdx = path.indexOfChar('?');
    if (queryIdx >= 0) path = path.substring(0, queryIdx);
    const int hashIdx = path.indexOfChar('#');
    if (hashIdx >= 0) path = path.substring(0, hashIdx);

    while (path.startsWithChar('/') || path.startsWithChar('\\') || path.startsWith("./"))
    {
        if (path.startsWithChar('/') || path.startsWithChar('\\'))
            path = path.substring(1);
        else if (path.startsWith("./"))
            path = path.substring(2);
    }

    if (path.startsWithIgnoreCase("web/"))
        path = path.substring(4);
    else if (path.startsWithIgnoreCase("ui/"))
        path = path.substring(3);

    while (path.startsWithChar('/') || path.startsWithChar('\\'))
        path = path.substring(1);

    if (path.isEmpty())
        path = "index.html";

    juce::String mimeType = "application/octet-stream";
    if (path.endsWithIgnoreCase(".html") || path.endsWithIgnoreCase(".htm")) mimeType = "text/html; charset=utf-8";
    else if (path.endsWithIgnoreCase(".css")) mimeType = "text/css; charset=utf-8";
    else if (path.endsWithIgnoreCase(".js") || path.endsWithIgnoreCase(".mjs")) mimeType = "text/javascript; charset=utf-8";
    else if (path.endsWithIgnoreCase(".json")) mimeType = "application/json; charset=utf-8";
    else if (path.endsWithIgnoreCase(".svg")) mimeType = "image/svg+xml";
    else if (path.endsWithIgnoreCase(".png")) mimeType = "image/png";
    else if (path.endsWithIgnoreCase(".jpg") || path.endsWithIgnoreCase(".jpeg")) mimeType = "image/jpeg";
    else if (path.endsWithIgnoreCase(".woff2")) mimeType = "font/woff2";
    else if (path.endsWithIgnoreCase(".woff")) mimeType = "font/woff";
    else if (path.endsWithIgnoreCase(".ttf")) mimeType = "font/ttf";
    else if (path.endsWithIgnoreCase(".wasm")) mimeType = "application/wasm";

    // 1. Search local filesystem (for live dev iteration and standalone execution)
    auto checkDiskFile = [&](const juce::File& file) -> std::optional<juce::WebBrowserComponent::Resource> {
        if (file.existsAsFile())
        {
            juce::MemoryBlock mb;
            if (file.loadFileAsData(mb))
            {
                std::vector<std::byte> bytes(mb.getSize());
                std::memcpy(bytes.data(), mb.getData(), mb.getSize());
                return juce::WebBrowserComponent::Resource { std::move(bytes), mimeType };
            }
        }
        return std::nullopt;
    };

    const juce::File cwd = juce::File::getCurrentWorkingDirectory();
    if (auto res = checkDiskFile(cwd.getChildFile("web").getChildFile(path))) return res;
    if (auto res = checkDiskFile(cwd.getChildFile("ui").getChildFile(path))) return res;
    if (auto res = checkDiskFile(cwd.getChildFile("braun_mr-16/web").getChildFile(path))) return res;
    if (auto res = checkDiskFile(cwd.getChildFile("mr-16/web").getChildFile(path))) return res;

    // Search relative to executable
    auto dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentExecutableFile).getParentDirectory();
    for (int depth = 0; depth < 5; ++depth)
    {
        if (auto res = checkDiskFile(dir.getChildFile("web").getChildFile(path))) return res;
        if (auto res = checkDiskFile(dir.getChildFile("ui").getChildFile(path))) return res;
        if (auto res = checkDiskFile(dir.getChildFile("braun_mr-16/web").getChildFile(path))) return res;
        if (auto res = checkDiskFile(dir.getChildFile("mr-16/web").getChildFile(path))) return res;
        dir = dir.getParentDirectory();
    }

    // 2. Unpack from embedded binary zip archive (BraunMr16WebAssets)
#if MR16_HAS_BINARY_DATA
    int zipSize = 0;
    const char* zipData = nullptr;
    #if defined(BinaryData_web_assets_mr16_zip) || defined(BINARYDATA_H_INCLUDED)
    if (BinaryData::web_assets_mr16_zipSize > 0 && BinaryData::web_assets_mr16_zip != nullptr)
    {
        zipData = BinaryData::web_assets_mr16_zip;
        zipSize = BinaryData::web_assets_mr16_zipSize;
    }
    #endif
    if (zipData == nullptr)
    {
        zipData = BinaryData::getNamedResource("web_assets_mr16_zip", zipSize);
    }

    if (zipData != nullptr && zipSize > 0)
    {
        juce::MemoryInputStream memStream(zipData, static_cast<size_t>(zipSize), false);
        juce::ZipFile zip(memStream);

        juce::String normalizedPath = path.replaceCharacter('\\', '/');
        while (normalizedPath.startsWithChar('/') || normalizedPath.startsWith("./"))
        {
            if (normalizedPath.startsWithChar('/')) normalizedPath = normalizedPath.substring(1);
            else if (normalizedPath.startsWith("./")) normalizedPath = normalizedPath.substring(2);
        }
        if (normalizedPath.startsWithIgnoreCase("web/")) normalizedPath = normalizedPath.substring(4);
        else if (normalizedPath.startsWithIgnoreCase("ui/")) normalizedPath = normalizedPath.substring(3);

        int entryIndex = zip.getIndexOfFileName(normalizedPath);
        if (entryIndex < 0)
        {
            for (int i = 0; i < zip.getNumEntries(); ++i)
            {
                const auto* entry = zip.getEntry(i);
                if (entry != nullptr)
                {
                    juce::String name = entry->filename.replaceCharacter('\\', '/');
                    while (name.startsWithChar('/') || name.startsWith("./"))
                    {
                        if (name.startsWithChar('/')) name = name.substring(1);
                        else if (name.startsWith("./")) name = name.substring(2);
                    }
                    if (name.startsWithIgnoreCase("web/")) name = name.substring(4);
                    else if (name.startsWithIgnoreCase("ui/")) name = name.substring(3);

                    if (name.equalsIgnoreCase(normalizedPath))
                    {
                        entryIndex = i;
                        break;
                    }
                }
            }
        }

        if (entryIndex >= 0)
        {
            const auto* entry = zip.getEntry(entryIndex);
            if (entry != nullptr)
            {
                std::unique_ptr<juce::InputStream> stream(zip.createStreamForEntry(*entry));
                if (stream != nullptr)
                {
                    juce::MemoryBlock mb;
                    stream->readIntoMemoryBlock(mb, -1);
                    std::vector<std::byte> data(mb.getSize());
                    std::memcpy(data.data(), mb.getData(), mb.getSize());
                    return juce::WebBrowserComponent::Resource { std::move(data), mimeType };
                }
            }
        }
    }
#endif

    // 3. Embedded fallback HTML (only if disk and binary assets both unavailable)
    if (path.equalsIgnoreCase("index.html"))
    {
        const size_t len = std::strlen(kEmbeddedBraunFallbackHtml);
        std::vector<std::byte> bytes(len);
        std::memcpy(bytes.data(), kEmbeddedBraunFallbackHtml, len);
        return juce::WebBrowserComponent::Resource { std::move(bytes), mimeType };
    }

    return std::nullopt;
}

void BRAUN_MR16AudioProcessorEditor::handleParamChangeFromWeb(const juce::var& data)
{
    if (!data.isObject()) return;
    const juce::String id = data.getProperty("id", "").toString();
    const float val = static_cast<float>(data.getProperty("value", 0.0));

    if (id.isEmpty()) return;

    // Handle switching to native JUCE UI
    if (id.equalsIgnoreCase("toggleNativeUI") ||
        id.equalsIgnoreCase("nativeUI") ||
        id.equalsIgnoreCase("switchUI"))
    {
        setNativeMode(true);
        return;
    }

    if (id == "power_state")
    {
        processorRef.setPower(val > 0.5f);
        return;
    }

    const auto& table = mr16::getParameterMetadataTable();
    for (const auto& meta : table)
    {
        if (id == meta.apvtsId || id == meta.webId)
        {
            if (auto* param = processorRef.getAPVTS().getParameter(meta.apvtsId))
            {
                const float norm = meta.isChoice
                                     ? param->convertTo0to1(std::round(val))
                                     : param->convertTo0to1(val);
                param->setValueNotifyingHost(std::clamp(norm, 0.0f, 1.0f));
            }
            break;
        }
    }
}

void BRAUN_MR16AudioProcessorEditor::handleExciterTriggerFromWeb(const juce::var& data)
{
    if (!data.isObject()) return;
    const juce::String type = data.getProperty("type", "").toString();

    if (type == "strike")
    {
        float vel = static_cast<float>(data.getProperty("vel", 0.8));
        float hard = static_cast<float>(data.getProperty("hard", 0.65));
        processorRef.triggerStrike(vel, hard);
    }
    else if (type == "chime")
    {
        int key = static_cast<int>(data.getProperty("key", 0));
        float vel = static_cast<float>(data.getProperty("vel", 1.0));
        processorRef.triggerChimeKey(key, vel);
    }
    else if (type == "note")
    {
        int midi = static_cast<int>(data.getProperty("midi", 60));
        float vel = static_cast<float>(data.getProperty("velocity", 0.8));
        processorRef.triggerMidiNote(midi, vel);
    }
}

void BRAUN_MR16AudioProcessorEditor::handleStartRecordingFromWeb()
{
    processorRef.startRecording();
    sendRecordingStateUpdateToWeb(true);
}

void BRAUN_MR16AudioProcessorEditor::handleStopRecordingFromWeb()
{
    processorRef.stopRecording();
    sendRecordingStateUpdateToWeb(false);
}

void BRAUN_MR16AudioProcessorEditor::sendParameterUpdateToWeb(const juce::String& apvtsId, const juce::String& webId, float newValue)
{
    if (!webComponent) return;
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("id", apvtsId);
    obj->setProperty("webId", webId);
    obj->setProperty("value", newValue);
    webComponent->emitEventIfBrowserIsVisible("paramUpdate", juce::var(obj.get()));
}

void BRAUN_MR16AudioProcessorEditor::sendRecordingStateUpdateToWeb(bool isRecording)
{
    if (!webComponent) return;
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("recording", isRecording);
    webComponent->emitEventIfBrowserIsVisible("recordingState", juce::var(obj.get()));
}

void BRAUN_MR16AudioProcessorEditor::syncAllParametersToWeb()
{
    const auto& table = mr16::getParameterMetadataTable();
    for (const auto& meta : table)
    {
        if (auto* param = processorRef.getAPVTS().getParameter(meta.apvtsId))
        {
            sendParameterUpdateToWeb(meta.apvtsId, meta.webId, param->getValue());
        }
    }
    sendRecordingStateUpdateToWeb(processorRef.isRecording());
}

void BRAUN_MR16AudioProcessorEditor::sendTelemetryToWeb()
{
    if (!webComponent) return;

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    juce::Array<juce::var> energies;
    for (size_t i = 0; i < 16; ++i)
        energies.add(latestTelemetryFrame.modalEnergies[i]);
    obj->setProperty("modalEnergies", energies);

    obj->setProperty("lorenzX", latestTelemetryFrame.lorenzX);
    obj->setProperty("lorenzY", latestTelemetryFrame.lorenzY);
    obj->setProperty("lorenzZ", latestTelemetryFrame.lorenzZ);
    obj->setProperty("exciterActivity", latestTelemetryFrame.exciterActivity);

    webComponent->emitEventIfBrowserIsVisible("telemetryFrame", juce::var(obj.get()));
}

void BRAUN_MR16AudioProcessorEditor::sendScopeDataToWeb()
{
    if (!webComponent) return;

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    juce::Array<juce::var> samples;
    for (size_t i = 0; i < 256; ++i)
        samples.add(latestTelemetryFrame.scopeSamplesL[i]);
    obj->setProperty("scopeL", samples);

    webComponent->emitEventIfBrowserIsVisible("telemetryFrame", juce::var(obj.get()));
}
#endif

void BRAUN_MR16AudioProcessorEditor::setNativeMode(bool native)
{
    useNativeUI = native;
    savePersistedNativeUIPreference(useNativeUI);

#if JUCE_WEB_BROWSER
    if (webComponent != nullptr)
    {
        if (useNativeUI)
        {
            webComponent->setVisible(false);
            webComponent->setBounds(0, 0, 0, 0);
            webComponent->toBack();
        }
        else
        {
            webComponent->setVisible(true);
            webComponent->setBounds(getLocalBounds());
            webComponent->toFront(false);
        }
    }
    viewModeButton.setVisible(useNativeUI);
    viewModeButton.setButtonText("SWITCH TO WEB UI");
#endif

    updateNativeControlVisibility();
    resized();
    repaint();
}

void BRAUN_MR16AudioProcessorEditor::timerCallback()
{
#if JUCE_WINDOWS
    if (!useNativeUI)
    {
        if (!hwndStylesConfigured || ++hwndCheckCounter >= 25)
        {
            hwndCheckCounter = 0;
            ensureHwndStyles();
        }
    }
#endif

    if (processorRef.popVisualizerFrame(latestTelemetryFrame))
    {
#if JUCE_WEB_BROWSER
        if (!useNativeUI)
        {
            sendTelemetryToWeb();
            sendScopeDataToWeb();
        }
#endif
    }

#if JUCE_WEB_BROWSER
    if (!useNativeUI && webComponent != nullptr && webComponent->isVisible())
    {
        if (!initialSyncDone)
        {
            syncAllParametersToWeb();
            initialSyncDone = true;
        }

        const auto& table = mr16::getParameterMetadataTable();
        for (size_t i = 0; i < table.size(); ++i)
        {
            if (paramDirty[i].exchange(false, std::memory_order_acq_rel))
            {
                sendParameterUpdateToWeb(table[i].apvtsId, table[i].webId, pendingParamValues[i].load(std::memory_order_relaxed));
            }
        }
    }
#endif

    if (processorRef.consumeRecordingSavedDirty())
    {
        recordButton.setButtonText("REC WAV");
        recordButton.setColour(juce::TextButton::buttonColourId, findColour(mr16::BraunColours::bgPanelInsetColourId));
    }

    if (useNativeUI)
    {
        powerButton.setButtonText(processorRef.isPower() ? "POWER ON" : "STANDBY");
        powerButton.setToggleState(processorRef.isPower(), juce::dontSendNotification);

        recordButton.setButtonText(processorRef.isRecording() ? "STOP REC" : "REC WAV");
        recordButton.setColour(juce::TextButton::buttonColourId,
                               processorRef.isRecording() ? findColour(mr16::BraunColours::braunOrangeColourId)
                                                          : findColour(mr16::BraunColours::bgPanelInsetColourId));

        const int currentProg = processorRef.getCurrentProgram();
        if (presetComboBox.getSelectedId() != currentProg + 1)
        {
            presetComboBox.setSelectedId(currentProg + 1, juce::dontSendNotification);
        }

        auto crtArea = getLocalBounds().withTrimmedTop(54).removeFromTop(130).reduced(16, 4);
        repaint(crtArea);
    }
}

//==============================================================================
// Native Control Construction & Layout
//==============================================================================
void BRAUN_MR16AudioProcessorEditor::setupNativeControls()
{
    addMouseListener(this, true);

    // Header controls
    powerButton.setButtonText(processorRef.isPower() ? "POWER ON" : "STANDBY");
    powerButton.setClickingTogglesState(true);
    powerButton.setToggleState(processorRef.isPower(), juce::dontSendNotification);
    powerButton.onClick = [this] {
        const bool p = powerButton.getToggleState();
        processorRef.setPower(p);
        powerButton.setButtonText(p ? "POWER ON" : "STANDBY");
    };
    addChildComponent(powerButton);

    themeButton.setButtonText("THEME");
    themeButton.onClick = [this] {
        braunLookAndFeel.setDarkTheme(!braunLookAndFeel.isDarkTheme());
        repaint();
    };
    addChildComponent(themeButton);

    recordButton.setButtonText(processorRef.isRecording() ? "STOP REC" : "REC WAV");
    recordButton.onClick = [this] {
        if (!processorRef.isRecording())
        {
            processorRef.startRecording();
            recordButton.setButtonText("STOP REC");
            recordButton.setColour(juce::TextButton::buttonColourId, findColour(mr16::BraunColours::braunOrangeColourId));
        }
        else
        {
            processorRef.stopRecording();
            recordButton.setButtonText("REC WAV");
            recordButton.setColour(juce::TextButton::buttonColourId, findColour(mr16::BraunColours::bgPanelInsetColourId));
        }
    };
    addChildComponent(recordButton);

#if JUCE_WEB_BROWSER
    viewModeButton.setButtonText("SWITCH TO WEB UI");
    viewModeButton.onClick = [this] {
        setNativeMode(false);
    };
    addChildComponent(viewModeButton);
#endif

    // Presets
    presetLabel.setText("PRESET:", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addChildComponent(presetLabel);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetComboBox.addItem(processorRef.getProgramName(i), i + 1);
    presetComboBox.setSelectedId(processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetComboBox.onChange = [this] {
        processorRef.setCurrentProgram(presetComboBox.getSelectedId() - 1);
    };
    addChildComponent(presetComboBox);

    prevPresetBtn.setButtonText("<");
    prevPresetBtn.onClick = [this] {
        int cur = presetComboBox.getSelectedId() - 1;
        if (cur > 0) presetComboBox.setSelectedId(cur);
    };
    addChildComponent(prevPresetBtn);

    nextPresetBtn.setButtonText(">");
    nextPresetBtn.onClick = [this] {
        int cur = presetComboBox.getSelectedId() + 1;
        if (cur <= processorRef.getNumPrograms()) presetComboBox.setSelectedId(cur);
    };
    addChildComponent(nextPresetBtn);

    // Audition buttons
    strikeTriggerBtn.setButtonText("STRIKE");
    strikeTriggerBtn.onClick = [this] { processorRef.triggerStrike(0.85f, 0.65f); };
    addChildComponent(strikeTriggerBtn);

    frictionTriggerBtn.setButtonText("BOW");
    frictionTriggerBtn.onClick = [this] { processorRef.triggerStrike(0.60f, 0.40f); };
    addChildComponent(frictionTriggerBtn);

    vactrolTriggerBtn.setButtonText("PLUCK");
    vactrolTriggerBtn.onClick = [this] { processorRef.triggerStrike(0.90f, 0.85f); };
    addChildComponent(vactrolTriggerBtn);

    poissonTriggerBtn.setButtonText("POISSON RAIN");
    poissonTriggerBtn.setClickingTogglesState(true);
    poissonTriggerBtn.onClick = [this] {
        if (auto* p = processorRef.getAPVTS().getParameter(mr16::ParamIDs::poissonDensity.getParamID()))
            p->setValueNotifyingHost(poissonTriggerBtn.getToggleState() ? 0.35f : 0.0f);
    };
    addChildComponent(poissonTriggerBtn);

    diracTriggerBtn.setButtonText("DIRAC");
    diracTriggerBtn.onClick = [this] { processorRef.triggerStrike(1.0f, 1.0f); };
    addChildComponent(diracTriggerBtn);

    // Build all 32 APVTS controls
    auto& apvts = processorRef.getAPVTS();
    const auto& table = mr16::getParameterMetadataTable();

    for (const auto& meta : table)
    {
        if (meta.isChoice)
        {
            auto slot = std::make_unique<ComboSlot>();
            slot->paramId = meta.apvtsId;
            slot->label.setText(meta.name, juce::dontSendNotification);
            slot->label.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));

            if (juce::String(meta.apvtsId) == mr16::ParamIDs::exciterType.getParamID())
            {
                const auto& c = mr16::getExciterTypeChoices();
                for (int i = 0; i < c.size(); ++i) slot->comboBox.addItem(c[i], i + 1);
            }
            else if (juce::String(meta.apvtsId) == mr16::ParamIDs::manifoldType.getParamID())
            {
                const auto& c = mr16::getManifoldTypeChoices();
                for (int i = 0; i < c.size(); ++i) slot->comboBox.addItem(c[i], i + 1);
            }
            else if (juce::String(meta.apvtsId) == mr16::ParamIDs::materialProfile.getParamID())
            {
                const auto& c = mr16::getMaterialProfileChoices();
                for (int i = 0; i < c.size(); ++i) slot->comboBox.addItem(c[i], i + 1);
            }
            else if (juce::String(meta.apvtsId) == mr16::ParamIDs::displayMode.getParamID())
            {
                const auto& c = mr16::getDisplayModeChoices();
                for (int i = 0; i < c.size(); ++i) slot->comboBox.addItem(c[i], i + 1);
            }

            slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, meta.apvtsId, slot->comboBox);
            addChildComponent(slot->label);
            addChildComponent(slot->comboBox);
            comboSlots.push_back(std::move(slot));
        }
        else if (meta.isBool)
        {
            auto slot = std::make_unique<ButtonSlot>();
            slot->paramId = meta.apvtsId;
            slot->button.setButtonText(meta.name);
            slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, meta.apvtsId, slot->button);
            addChildComponent(slot->button);
            buttonSlots.push_back(std::move(slot));
        }
        else
        {
            auto slot = std::make_unique<KnobSlot>();
            slot->paramId = meta.apvtsId;
            slot->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slot->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
            if (std::strlen(meta.unit) > 0)
                slot->slider.setTextValueSuffix(juce::String(" ") + meta.unit);

            slot->nameLabel.setText(meta.name, juce::dontSendNotification);
            slot->nameLabel.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
            slot->nameLabel.setJustificationType(juce::Justification::centred);

            slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, meta.apvtsId, slot->slider);

            slot->slider.addMouseListener(this, false);
            slot->nameLabel.addMouseListener(this, false);

            addChildComponent(slot->nameLabel);
            addChildComponent(slot->slider);
            knobSlots.push_back(std::move(slot));
        }
    }
}

void BRAUN_MR16AudioProcessorEditor::updateNativeControlVisibility()
{
    const bool show = useNativeUI;

    powerButton.setVisible(show);
    themeButton.setVisible(show);
    recordButton.setVisible(show);
#if JUCE_WEB_BROWSER
    viewModeButton.setVisible(show);
#endif
    presetLabel.setVisible(show);
    presetComboBox.setVisible(show);
    prevPresetBtn.setVisible(show);
    nextPresetBtn.setVisible(show);

    strikeTriggerBtn.setVisible(show);
    frictionTriggerBtn.setVisible(show);
    vactrolTriggerBtn.setVisible(show);
    poissonTriggerBtn.setVisible(show);
    diracTriggerBtn.setVisible(show);

    for (auto& s : knobSlots)
    {
        s->slider.setVisible(show);
        s->nameLabel.setVisible(show);
        if (show)
        {
            s->slider.toFront(false);
            s->nameLabel.toFront(false);
        }
    }
    for (auto& s : buttonSlots)
    {
        s->button.setVisible(show);
        if (show)
            s->button.toFront(false);
    }
    for (auto& s : comboSlots)
    {
        s->comboBox.setVisible(show);
        s->label.setVisible(show);
        if (show)
        {
            s->comboBox.toFront(false);
            s->label.toFront(false);
        }
    }

    if (show)
    {
        powerButton.toFront(false);
        themeButton.toFront(false);
        recordButton.toFront(false);
        presetLabel.toFront(false);
        presetComboBox.toFront(false);
        prevPresetBtn.toFront(false);
        nextPresetBtn.toFront(false);
        strikeTriggerBtn.toFront(false);
        frictionTriggerBtn.toFront(false);
        vactrolTriggerBtn.toFront(false);
        poissonTriggerBtn.toFront(false);
        diracTriggerBtn.toFront(false);
#if JUCE_WEB_BROWSER
        viewModeButton.toFront(true);
#endif
    }
}

BRAUN_MR16AudioProcessorEditor::KnobSlot* BRAUN_MR16AudioProcessorEditor::findKnob(const juce::ParameterID& id)
{
    return findKnob(id.getParamID());
}

BRAUN_MR16AudioProcessorEditor::KnobSlot* BRAUN_MR16AudioProcessorEditor::findKnob(const juce::String& paramId)
{
    for (auto& k : knobSlots)
        if (k->paramId == paramId) return k.get();
    return nullptr;
}

BRAUN_MR16AudioProcessorEditor::ButtonSlot* BRAUN_MR16AudioProcessorEditor::findButton(const juce::ParameterID& id)
{
    for (auto& b : buttonSlots)
        if (b->paramId == id.getParamID()) return b.get();
    return nullptr;
}

BRAUN_MR16AudioProcessorEditor::ComboSlot* BRAUN_MR16AudioProcessorEditor::findCombo(const juce::ParameterID& id)
{
    for (auto& c : comboSlots)
        if (c->paramId == id.getParamID()) return c.get();
    return nullptr;
}

void BRAUN_MR16AudioProcessorEditor::paint(juce::Graphics& g)
{
    if (useNativeUI)
    {
        drawBraunChassis(g, getLocalBounds());
    }
    else
    {
        g.fillAll(juce::Colour(0xff141517));
    }
}

void BRAUN_MR16AudioProcessorEditor::drawBraunChassis(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.fillAll(findColour(mr16::BraunColours::bgAppColourId));

    // Outer precision border
    g.setColour(findColour(mr16::BraunColours::borderLineColourId));
    g.drawRect(bounds.toFloat(), 1.5f);

    // 1. Top Header Deck (54px)
    auto headerArea = bounds.removeFromTop(54);
    g.setColour(findColour(mr16::BraunColours::bgPanelColourId));
    g.fillRect(headerArea);
    g.setColour(findColour(mr16::BraunColours::borderLineColourId));
    g.drawHorizontalLine(headerArea.getBottom(), 0.0f, static_cast<float>(bounds.getWidth()));

    // Typography: Dieter Rams Braun styling
    g.setColour(findColour(mr16::BraunColours::textPrimaryColourId));
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    g.drawText("BRAUN MR-16", headerArea.removeFromLeft(160).reduced(16, 0), juce::Justification::centredLeft);

    g.setColour(findColour(mr16::BraunColours::textMutedColourId));
    g.setFont(juce::Font(juce::FontOptions(9.5f, juce::Font::plain)));
    g.drawText(juce::String("MODAL RESONATOR & KINETIC SYNTHESIZER ") + juce::String::charToString(0x00B7) + " DIN 1451",
               headerArea.removeFromLeft(330).reduced(4, 0), juce::Justification::centredLeft);

    // 2. Central CRT Phosphor Visualizer Scope (130px)
    auto crtArea = bounds.removeFromTop(130).reduced(16, 4);
    drawCrtDisplay(g, crtArea);

    // 3. Audition Strip (34px)
    auto auditionArea = bounds.removeFromTop(34).reduced(16, 2);
    g.setColour(findColour(mr16::BraunColours::bgPanelColourId));
    g.fillRoundedRectangle(auditionArea.toFloat(), 3.0f);
    g.setColour(findColour(mr16::BraunColours::borderLineColourId));
    g.drawRoundedRectangle(auditionArea.toFloat(), 3.0f, 1.0f);

    auto auditionLabelArea = auditionArea.removeFromLeft(130);
    g.setColour(findColour(mr16::BraunColours::textMutedColourId));
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText("AUDITION EXCITER:", auditionLabelArea.reduced(8, 0), juce::Justification::centredLeft);

    // 4. 5 Signal-Flow Decks Grid
    auto gridArea = bounds.reduced(16, 6);
    const int numCols = 5;
    const int colWidth = gridArea.getWidth() / numCols;

    const char* deckTitles[5] = {
        "1. KINETIC EXCITER",
        "2. MODAL MATRIX",
        "3. LORENZ ATTRACTOR",
        "4. BBD CHORUS",
        "5. SPATIAL & DYNAMICS"
    };

    for (int c = 0; c < numCols; ++c)
    {
        auto cell = juce::Rectangle<int>(gridArea.getX() + c * colWidth, gridArea.getY(), colWidth, gridArea.getHeight()).reduced(4);

        g.setColour(findColour(mr16::BraunColours::bgPanelColourId));
        g.fillRoundedRectangle(cell.toFloat(), 3.0f);
        g.setColour(findColour(mr16::BraunColours::borderLineColourId));
        g.drawRoundedRectangle(cell.toFloat(), 3.0f, 1.0f);

        // Deck header banner
        auto deckHeader = cell.removeFromTop(24);
        g.setColour(findColour(mr16::BraunColours::braunOrangeColourId));
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.drawText(deckTitles[c], deckHeader.reduced(8, 0), juce::Justification::centredLeft);
    }
}

void BRAUN_MR16AudioProcessorEditor::drawCrtDisplay(juce::Graphics& g, juce::Rectangle<int> crtBounds)
{
    // CRT outer bezel
    g.setColour(findColour(mr16::BraunColours::bgBezelColourId));
    g.fillRoundedRectangle(crtBounds.toFloat(), 5.0f);
    g.setColour(findColour(mr16::BraunColours::borderLineColourId));
    g.drawRoundedRectangle(crtBounds.toFloat(), 5.0f, 1.0f);

    auto screen = crtBounds.reduced(6);
    g.setColour(juce::Colour(0xFF070A08));
    g.fillRoundedRectangle(screen.toFloat(), 3.0f);

    // Split screen: Left 42% for Vector Scope & Waveform, Right 58% for 16 Modal Energy Bars
    const int scopeWidth = juce::jlimit(340, 520, static_cast<int>(screen.getWidth() * 0.42f));
    auto scopeScreen = screen.removeFromLeft(scopeWidth);
    auto meterScreen = screen.reduced(10, 4);

    // Subtle CRT raster scanlines on scope screen
    g.setColour(juce::Colours::black.withAlpha(0.20f));
    for (int y = scopeScreen.getY(); y < scopeScreen.getBottom(); y += 3)
        g.drawHorizontalLine(y, (float)scopeScreen.getX(), (float)scopeScreen.getRight());

    // Center graticule crosshair
    g.setColour(juce::Colour(0xFF142218));
    g.drawHorizontalLine(scopeScreen.getCentreY(), (float)scopeScreen.getX(), (float)scopeScreen.getRight());
    g.drawVerticalLine(scopeScreen.getCentreX(), (float)scopeScreen.getY(), (float)scopeScreen.getBottom());

    // Vertical separator between scope and modal meters
    g.setColour(juce::Colour(0xFF162419));
    g.drawVerticalLine(meterScreen.getX() - 6, (float)screen.getY(), (float)screen.getBottom());

    // Determine current display mode
    int mode = 0;
    if (auto* dispParam = processorRef.getAPVTS().getRawParameterValue(mr16::ParamIDs::displayMode.getParamID()))
    {
        mode = static_cast<int>(std::round(dispParam->load(std::memory_order_relaxed)));
    }

    if (mode == 0)
    {
        // Mode 0: 2D Chladni Nodal Geometry Simulation
        g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.85f));

        const float cx = (float)scopeScreen.getCentreX();
        const float cy = (float)scopeScreen.getCentreY();
        const float maxR = (float)juce::jmin(scopeScreen.getWidth(), scopeScreen.getHeight()) * 0.40f;

        static float phase = 0.0f;
        phase += 0.02f;

        juce::Path chladniCurve;
        const int steps = 120;
        for (int i = 0; i <= steps; ++i)
        {
            float theta = (float)i / (float)steps * juce::MathConstants<float>::twoPi;
            float r = maxR * (0.65f + 0.35f * std::sin(3.0f * theta + phase) * std::cos(2.0f * theta - phase * 0.5f));
            float px = cx + r * std::cos(theta);
            float py = cy + r * std::sin(theta);
            if (i == 0) chladniCurve.startNewSubPath(px, py);
            else chladniCurve.lineTo(px, py);
        }
        chladniCurve.closeSubPath();
        g.strokePath(chladniCurve, juce::PathStrokeType(1.5f));
    }
    else if (mode == 1)
    {
        // Mode 1: 3D Lorenz Attractor Orbit
        g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.85f));
        const float cx = (float)scopeScreen.getCentreX();
        const float cy = (float)scopeScreen.getCentreY();

        juce::Path lorenzPath;
        const int nPts = 100;
        float lx = latestTelemetryFrame.lorenzX * 0.05f;
        float ly = latestTelemetryFrame.lorenzY * 0.05f;
        float lz = latestTelemetryFrame.lorenzZ * 0.05f;

        for (int i = 0; i < nPts; ++i)
        {
            float t = (float)i * 0.1f;
            float px = cx + (lx * std::cos(t) - ly * std::sin(t)) * 40.0f;
            float py = cy + (lz - 1.2f) * 35.0f;
            if (i == 0) lorenzPath.startNewSubPath(px, py);
            else lorenzPath.lineTo(px, py);
        }
        g.strokePath(lorenzPath, juce::PathStrokeType(1.2f));
    }
    else
    {
        // Mode 2: Kinetic Exciter Waveform Glow
        g.setColour(findColour(mr16::BraunColours::braunOrangeColourId).withAlpha(0.80f));
        const float cx = (float)scopeScreen.getCentreX();
        const float cy = (float)scopeScreen.getCentreY();
        const float radius = juce::jmin(scopeScreen.getWidth(), scopeScreen.getHeight()) * 0.36f;
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.2f);
    }

    // Overlay real-time oscilloscope waveform trace on scope screen
    float scopeL[256];
    processorRef.getScopeSamples(scopeL, nullptr, 256);

    juce::Path wave;
    for (int i = 0; i < scopeScreen.getWidth(); ++i)
    {
        int idx = (i * 256) / juce::jmax(1, scopeScreen.getWidth());
        float y = (float)scopeScreen.getCentreY() - (scopeL[idx] * (float)scopeScreen.getHeight() * 0.35f);
        if (i == 0) wave.startNewSubPath((float)scopeScreen.getX() + (float)i, y);
        else wave.lineTo((float)scopeScreen.getX() + (float)i, y);
    }
    g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.50f));
    g.strokePath(wave, juce::PathStrokeType(1.0f));

    // Corner CRT legend on scope screen
    g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.65f));
    g.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
    juce::String modeName = (mode == 0) ? "CHLADNI 2D" : (mode == 1) ? "ATTRACTOR 3D" : "MODAL FFT";
    g.drawText(modeName, scopeScreen.reduced(6), juce::Justification::topRight);

    // Right side: 16 Modal Energy Bars
    const int nBars = 16;
    const float totalW = (float)meterScreen.getWidth();
    const float barSpacing = 4.0f;
    const float barW = (totalW - barSpacing * (nBars + 1)) / (float)nBars;
    const float maxH = (float)meterScreen.getHeight() - 16.0f;

    for (int i = 0; i < nBars; ++i)
    {
        const float bx = (float)meterScreen.getX() + barSpacing + (float)i * (barW + barSpacing);
        const float by = (float)meterScreen.getY() + 2.0f;

        // Background track
        g.setColour(juce::Colour(0xFF101612));
        g.fillRect(bx, by, barW, maxH);

        // Energy fill
        const float energy = std::clamp(latestTelemetryFrame.modalEnergies[i] * 120.0f, 0.0f, 1.0f);
        const float fillH = energy * maxH;
        const float fillY = by + maxH - fillH;

        // Color: phosphor green with amber peak for high energy
        const juce::Colour barCol = (energy > 0.85f)
            ? findColour(mr16::BraunColours::braunOrangeColourId)
            : findColour(mr16::BraunColours::phosphorColourId);
        g.setColour(barCol);
        g.fillRect(bx, fillY, barW, fillH);

        // Label "M1", "M2", ...
        g.setColour(juce::Colour(0xFF7E8085));
        g.setFont(juce::Font(juce::FontOptions(7.5f, juce::Font::plain)));
        g.drawText("M" + juce::String(i + 1),
                   (int)bx - 2, (int)(by + maxH + 1.0f), (int)barW + 4, 12,
                   juce::Justification::centred);
    }
}

void BRAUN_MR16AudioProcessorEditor::resized()
{
#if JUCE_WEB_BROWSER
    if (webComponent != nullptr)
    {
        if (!useNativeUI)
            webComponent->setBounds(getLocalBounds());
        else
            webComponent->setBounds(0, 0, 0, 0);
    }
#endif

    updateNativeControlVisibility();
    layoutNativeControls();
}

void BRAUN_MR16AudioProcessorEditor::layoutNativeControls()
{
    if (!useNativeUI)
        return;

    auto bounds = getLocalBounds();
    if (bounds.isEmpty()) return;

    // 1. Top Header Bar (54px)
    auto header = bounds.removeFromTop(54);
    auto rightHeader = header.removeFromRight(header.getWidth() - 480).reduced(8, 10);

#if JUCE_WEB_BROWSER
    viewModeButton.setBounds(rightHeader.removeFromRight(140).reduced(4, 2));
#endif
    recordButton.setBounds(rightHeader.removeFromRight(100).reduced(4, 2));
    themeButton.setBounds(rightHeader.removeFromRight(80).reduced(4, 2));
    powerButton.setBounds(rightHeader.removeFromRight(95).reduced(4, 2));

    // Preset selector in header space
    if (rightHeader.getWidth() >= 160)
    {
        nextPresetBtn.setBounds(rightHeader.removeFromRight(26).reduced(2, 3));
        const int comboW = juce::jlimit(100, 180, rightHeader.getWidth() - 85);
        presetComboBox.setBounds(rightHeader.removeFromRight(comboW).reduced(2, 3));
        prevPresetBtn.setBounds(rightHeader.removeFromRight(26).reduced(2, 3));
        presetLabel.setBounds(rightHeader.removeFromRight(juce::jmin(65, rightHeader.getWidth())).reduced(2, 3));
    }

    // 2. Skip Central CRT Phosphor Visualizer Scope (130px)
    bounds.removeFromTop(130);

    // 3. Audition Exciter Bar (34px)
    auto audition = bounds.removeFromTop(34).reduced(16, 2);
    audition.removeFromLeft(130); // Skip label drawn by chassis
    strikeTriggerBtn.setBounds(audition.removeFromLeft(110).reduced(4, 2));
    frictionTriggerBtn.setBounds(audition.removeFromLeft(110).reduced(4, 2));
    vactrolTriggerBtn.setBounds(audition.removeFromLeft(110).reduced(4, 2));
    poissonTriggerBtn.setBounds(audition.removeFromLeft(130).reduced(4, 2));
    diracTriggerBtn.setBounds(audition.removeFromLeft(110).reduced(4, 2));

    // 4. 5 Signal-Flow Decks Grid
    auto gridArea = bounds.reduced(16, 6);
    const int numCols = 5;
    const int colW = gridArea.getWidth() / numCols;

    auto deck1Area = juce::Rectangle<int>(gridArea.getX() + 0 * colW, gridArea.getY(), colW, gridArea.getHeight()).reduced(4);
    auto deck2Area = juce::Rectangle<int>(gridArea.getX() + 1 * colW, gridArea.getY(), colW, gridArea.getHeight()).reduced(4);
    auto deck3Area = juce::Rectangle<int>(gridArea.getX() + 2 * colW, gridArea.getY(), colW, gridArea.getHeight()).reduced(4);
    auto deck4Area = juce::Rectangle<int>(gridArea.getX() + 3 * colW, gridArea.getY(), colW, gridArea.getHeight()).reduced(4);
    auto deck5Area = juce::Rectangle<int>(gridArea.getX() + 4 * colW, gridArea.getY(), colW, gridArea.getHeight()).reduced(4);

    auto layoutKnobInArea = [](KnobSlot* slot, juce::Rectangle<int> area) {
        if (slot == nullptr) return;
        auto labelArea = area.removeFromBottom(16);
        slot->nameLabel.setBounds(labelArea);
        slot->slider.setBounds(area);
    };

    auto layoutComboInArea = [](ComboSlot* slot, juce::Rectangle<int> area) {
        if (slot == nullptr) return;
        auto labelArea = area.removeFromTop(14);
        slot->label.setBounds(labelArea);
        slot->comboBox.setBounds(area.reduced(2, 1));
    };

    auto layout2Knobs = [&](KnobSlot* k1, KnobSlot* k2, juce::Rectangle<int>& area, int rowH) {
        auto row = area.removeFromTop(rowH).reduced(2);
        const int w = row.getWidth() / 2;
        if (k1) layoutKnobInArea(k1, row.removeFromLeft(w).reduced(2));
        if (k2) layoutKnobInArea(k2, row.reduced(2));
    };

    auto layout1Knob = [&](KnobSlot* k, juce::Rectangle<int>& area, int rowH) {
        auto row = area.removeFromTop(rowH).reduced(2);
        if (k) layoutKnobInArea(k, row.withSizeKeepingCentre(juce::jmin(110, row.getWidth()), rowH - 4));
    };

    // Skip deck headers (24px)
    deck1Area.removeFromTop(24);
    deck2Area.removeFromTop(24);
    deck3Area.removeFromTop(24);
    deck4Area.removeFromTop(24);
    deck5Area.removeFromTop(24);

    // Deck 01 (Kinetic Exciter)
    layoutComboInArea(findCombo(mr16::ParamIDs::exciterType), deck1Area.removeFromTop(38).reduced(4, 2));
    layout2Knobs(findKnob(mr16::ParamIDs::strikeHardness), findKnob(mr16::ParamIDs::strikeVelocity), deck1Area, 72);
    layout2Knobs(findKnob(mr16::ParamIDs::frictionForce), findKnob(mr16::ParamIDs::frictionSpeed), deck1Area, 72);
    layout2Knobs(findKnob(mr16::ParamIDs::vactrolSag), findKnob(mr16::ParamIDs::extInputGain), deck1Area, 72);
    layout2Knobs(findKnob(mr16::ParamIDs::poissonDensity), findKnob(mr16::ParamIDs::euclideanPulses), deck1Area, 72);
    layout1Knob(findKnob(mr16::ParamIDs::euclideanSteps), deck1Area, 72);

    // Deck 02 (Modal Matrix)
    layoutComboInArea(findCombo(mr16::ParamIDs::manifoldType), deck2Area.removeFromTop(38).reduced(4, 2));
    layoutComboInArea(findCombo(mr16::ParamIDs::materialProfile), deck2Area.removeFromTop(38).reduced(4, 2));
    layout2Knobs(findKnob(mr16::ParamIDs::modalFrequency), findKnob(mr16::ParamIDs::modalDamping), deck2Area, 75);
    layout2Knobs(findKnob(mr16::ParamIDs::modalCoupling), findKnob(mr16::ParamIDs::modalSpread), deck2Area, 75);

    // Deck 03 (Lorenz Attractor)
    layout2Knobs(findKnob(mr16::ParamIDs::lorenzRate), findKnob(mr16::ParamIDs::lorenzChaos), deck3Area, 80);
    layout2Knobs(findKnob(mr16::ParamIDs::lorenzFreqMod), findKnob(mr16::ParamIDs::lorenzQMod), deck3Area, 80);

    // Deck 04 (Chorus)
    if (auto* b = findButton(mr16::ParamIDs::chorusEnable))
    {
        b->button.setBounds(deck4Area.removeFromTop(32).reduced(12, 2));
    }
    layout2Knobs(findKnob(mr16::ParamIDs::chorusRateHz), findKnob(mr16::ParamIDs::chorusDepthMs), deck4Area, 80);
    layout2Knobs(findKnob(mr16::ParamIDs::chorusDimension), findKnob(mr16::ParamIDs::chorusMix), deck4Area, 80);

    // Deck 05 (Spatial & Dynamics)
    layoutComboInArea(findCombo(mr16::ParamIDs::displayMode), deck5Area.removeFromTop(38).reduced(4, 2));
    layout2Knobs(findKnob(mr16::ParamIDs::goldenPanSpread), findKnob(mr16::ParamIDs::vactrolLpgCutoff), deck5Area, 75);
    layout2Knobs(findKnob(mr16::ParamIDs::driveSaturation), findKnob(mr16::ParamIDs::masterTrimDb), deck5Area, 75);
    layout1Knob(findKnob(mr16::ParamIDs::dryWetMix), deck5Area, 75);
}

void BRAUN_MR16AudioProcessorEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();
    hwndStylesConfigured = false;
    if (!useNativeUI)
    {
        ensureHwndStyles();
    }
}

void BRAUN_MR16AudioProcessorEditor::ensureHwndStyles()
{
#if JUCE_WINDOWS
    if (useNativeUI)
        return;

    if (auto* peer = getPeer())
    {
        HWND hwnd = static_cast<HWND>(peer->getNativeHandle());
        if (hwnd == nullptr || !::IsWindow(hwnd))
            return;

        // Apply WS_CLIPCHILDREN | WS_CLIPSIBLINGS to our own plugin HWND only.
        // We NEVER touch ancestor/parent windows to avoid corrupting FL Studio or host DAW title bars/frames.
        LONG_PTR style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
        if ((style & (WS_CLIPCHILDREN | WS_CLIPSIBLINGS)) != (WS_CLIPCHILDREN | WS_CLIPSIBLINGS))
        {
            ::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        }

        // Also ensure child windows (WebView2 host HWNDs and render widget) enforce clipping
        int childCount = 0;
        ::EnumChildWindows(hwnd, [](HWND child, LPARAM lParam) -> BOOL {
            if (child == nullptr || !::IsWindow(child))
                return TRUE;
            auto* count = reinterpret_cast<int*>(lParam);
            (*count)++;
            LONG_PTR childStyle = ::GetWindowLongPtr(child, GWL_STYLE);
            if ((childStyle & (WS_CLIPCHILDREN | WS_CLIPSIBLINGS)) != (WS_CLIPCHILDREN | WS_CLIPSIBLINGS))
            {
                ::SetWindowLongPtr(child, GWL_STYLE, childStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&childCount));

        if (childCount > 0)
            hwndStylesConfigured = true;
    }
#endif
}

void BRAUN_MR16AudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!useNativeUI)
        return;

    if (e.mods.isPopupMenu())
    {
        for (auto& slot : knobSlots)
        {
            if (e.eventComponent == &slot->slider || slot->slider.isParentOf(e.eventComponent)
                || e.eventComponent == &slot->nameLabel || slot->nameLabel.isParentOf(e.eventComponent))
            {
                showKnobContextMenu(*slot, e.getScreenPosition());
                return;
            }
        }
    }
}

void BRAUN_MR16AudioProcessorEditor::showKnobContextMenu(KnobSlot& slot, juce::Point<int> screenPos)
{
    juce::PopupMenu m;
    m.addItem("Reset to Default", [this, &slot] {
        if (auto* p = processorRef.getAPVTS().getParameter(slot.paramId))
            p->setValueNotifyingHost(p->getDefaultValue());
    });

    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)));
}
