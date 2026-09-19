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

    useNativeUI = loadPersistedNativeUIPreference();

#if JUCE_WEB_BROWSER
    for (size_t i = 0; i < kNumParams; ++i)
    {
        pendingParamValues[i].store(0.0f, std::memory_order_relaxed);
        paramDirty[i].store(false, std::memory_order_relaxed);
    }

    if (!useNativeUI)
    {
        auto options = createWebOptions(*this);
        webComponent = std::make_unique<juce::WebBrowserComponent>(options);
        addAndMakeVisible(*webComponent);
        webComponent->goToURL("https://juce.backend/index.html");
    }
#else
    useNativeUI = true;
#endif

    setupNativeControls();
    updateNativeControlVisibility();

    setSize(1180, 680);
    setResizable(true, true);
    setResizeLimits(960, 560, 1920, 1100);

    registerParameterListeners();
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
    return juce::WebBrowserComponent::Options()
        .withResourceProvider([&editor](const juce::String& url) {
            return editor.getResource(url);
        })
        .withNativeIntegrationEnabled()
        .withEventListener("paramChange", [&editor](const juce::var& data) {
            editor.handleParamChangeFromWeb(data);
        })
        .withEventListener("exciterTrigger", [&editor](const juce::var& data) {
            editor.handleExciterTriggerFromWeb(data);
        })
        .withEventListener("startRecording", [&editor](const juce::var&) {
            editor.handleStartRecordingFromWeb();
        })
        .withEventListener("stopRecording", [&editor](const juce::var&) {
            editor.handleStopRecordingFromWeb();
        });
}

std::optional<juce::WebBrowserComponent::Resource> BRAUN_MR16AudioProcessorEditor::getResource(const juce::String& url)
{
    juce::String cleanUrl = url;
    if (cleanUrl.startsWith("https://juce.backend/"))
        cleanUrl = cleanUrl.substring(21);
    else if (cleanUrl.startsWith("/"))
        cleanUrl = cleanUrl.substring(1);

    if (cleanUrl.isEmpty() || cleanUrl == "index.html")
    {
#if MR16_HAS_BINARY_DATA
        int size = 0;
        if (const char* zipData = BinaryData::getNamedResource("web_assets_mr16_zip", size))
        {
            juce::MemoryInputStream zipStream(zipData, static_cast<size_t>(size), false);
            juce::ZipFile zip(zipStream);
            if (auto* entry = zip.getEntry("index.html"))
            {
                if (auto stream = std::unique_ptr<juce::InputStream>(zip.createStreamForEntry(*entry)))
                {
                    std::vector<std::byte> bytes(static_cast<size_t>(entry->uncompressedSize));
                    stream->read(bytes.data(), bytes.size());
                    return juce::WebBrowserComponent::Resource { std::move(bytes), "text/html" };
                }
            }
        }
#endif
        const size_t len = std::strlen(kEmbeddedBraunFallbackHtml);
        std::vector<std::byte> bytes(len);
        std::memcpy(bytes.data(), kEmbeddedBraunFallbackHtml, len);
        return juce::WebBrowserComponent::Resource { std::move(bytes), "text/html" };
    }

#if MR16_HAS_BINARY_DATA
    int size = 0;
    if (const char* zipData = BinaryData::getNamedResource("web_assets_mr16_zip", size))
    {
        juce::MemoryInputStream zipStream(zipData, static_cast<size_t>(size), false);
        juce::ZipFile zip(zipStream);
        if (auto* entry = zip.getEntry(cleanUrl))
        {
            if (auto stream = std::unique_ptr<juce::InputStream>(zip.createStreamForEntry(*entry)))
            {
                std::vector<std::byte> bytes(static_cast<size_t>(entry->uncompressedSize));
                stream->read(bytes.data(), bytes.size());

                juce::String mime = "application/octet-stream";
                if (cleanUrl.endsWith(".html")) mime = "text/html";
                else if (cleanUrl.endsWith(".css")) mime = "text/css";
                else if (cleanUrl.endsWith(".js") || cleanUrl.endsWith(".mjs")) mime = "application/javascript";
                else if (cleanUrl.endsWith(".json")) mime = "application/json";
                else if (cleanUrl.endsWith(".png")) mime = "image/png";
                else if (cleanUrl.endsWith(".svg")) mime = "image/svg+xml";

                return juce::WebBrowserComponent::Resource { std::move(bytes), mime };
            }
        }
    }
#endif

    return std::nullopt;
}

void BRAUN_MR16AudioProcessorEditor::handleParamChangeFromWeb(const juce::var& data)
{
    if (!data.isObject()) return;
    const juce::String id = data.getProperty("id", "").toString();
    const float val = static_cast<float>(data.getProperty("value", 0.0));

    if (id.isEmpty()) return;

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
    if (useNativeUI == native) return;

    useNativeUI = native;
    savePersistedNativeUIPreference(useNativeUI);

#if JUCE_WEB_BROWSER
    if (useNativeUI)
    {
        if (webComponent)
            webComponent->setVisible(false);
    }
    else
    {
        if (!webComponent)
        {
            auto options = createWebOptions(*this);
            webComponent = std::make_unique<juce::WebBrowserComponent>(options);
            addAndMakeVisible(*webComponent);
            webComponent->goToURL("https://juce.backend/index.html");
        }
        else
        {
            webComponent->setVisible(true);
        }
    }
#endif

    updateNativeControlVisibility();
    resized();
    repaint();
}

void BRAUN_MR16AudioProcessorEditor::timerCallback()
{
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
    if (!useNativeUI)
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
        repaint();
    }
}

//==============================================================================
// Native Control Construction & Layout
//==============================================================================
void BRAUN_MR16AudioProcessorEditor::setupNativeControls()
{
    // Header controls
    powerButton.setButtonText(processorRef.isPower() ? "POWER ON" : "STANDBY");
    powerButton.setClickingTogglesState(true);
    powerButton.setToggleState(processorRef.isPower(), juce::dontSendNotification);
    powerButton.onClick = [this] {
        const bool p = powerButton.getToggleState();
        processorRef.setPower(p);
        powerButton.setButtonText(p ? "POWER ON" : "STANDBY");
    };
    addAndMakeVisible(powerButton);

    themeButton.setButtonText("THEME");
    themeButton.onClick = [this] {
        braunLookAndFeel.setDarkTheme(!braunLookAndFeel.isDarkTheme());
        repaint();
    };
    addAndMakeVisible(themeButton);

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
    addAndMakeVisible(recordButton);

#if JUCE_WEB_BROWSER
    viewModeButton.setButtonText(useNativeUI ? "WEB UI" : "NATIVE UI");
    viewModeButton.onClick = [this] {
        setNativeMode(!useNativeUI);
        viewModeButton.setButtonText(useNativeUI ? "WEB UI" : "NATIVE UI");
    };
    addAndMakeVisible(viewModeButton);
#endif

    // Presets
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    addAndMakeVisible(presetLabel);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetComboBox.addItem(processorRef.getProgramName(i), i + 1);
    presetComboBox.setSelectedId(processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetComboBox.onChange = [this] {
        processorRef.setCurrentProgram(presetComboBox.getSelectedId() - 1);
    };
    addAndMakeVisible(presetComboBox);

    prevPresetBtn.setButtonText("<");
    prevPresetBtn.onClick = [this] {
        int cur = presetComboBox.getSelectedId() - 1;
        if (cur > 0) presetComboBox.setSelectedId(cur);
    };
    addAndMakeVisible(prevPresetBtn);

    nextPresetBtn.setButtonText(">");
    nextPresetBtn.onClick = [this] {
        int cur = presetComboBox.getSelectedId() + 1;
        if (cur <= processorRef.getNumPrograms()) presetComboBox.setSelectedId(cur);
    };
    addAndMakeVisible(nextPresetBtn);

    // Audition buttons
    strikeTriggerBtn.setButtonText("STRIKE");
    strikeTriggerBtn.onClick = [this] { processorRef.triggerStrike(0.85f, 0.65f); };
    addAndMakeVisible(strikeTriggerBtn);

    frictionTriggerBtn.setButtonText("BOW");
    frictionTriggerBtn.onClick = [this] { processorRef.triggerStrike(0.60f, 0.40f); };
    addAndMakeVisible(frictionTriggerBtn);

    vactrolTriggerBtn.setButtonText("PLUCK");
    vactrolTriggerBtn.onClick = [this] { processorRef.triggerStrike(0.90f, 0.85f); };
    addAndMakeVisible(vactrolTriggerBtn);

    poissonTriggerBtn.setButtonText("POISSON");
    poissonTriggerBtn.setClickingTogglesState(true);
    poissonTriggerBtn.onClick = [this] {
        if (auto* p = processorRef.getAPVTS().getParameter(mr16::ParamIDs::poissonDensity.getParamID()))
            p->setValueNotifyingHost(poissonTriggerBtn.getToggleState() ? 0.35f : 0.0f);
    };
    addAndMakeVisible(poissonTriggerBtn);

    diracTriggerBtn.setButtonText("DIRAC");
    diracTriggerBtn.onClick = [this] { processorRef.triggerStrike(1.0f, 1.0f); };
    addAndMakeVisible(diracTriggerBtn);

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
            addAndMakeVisible(slot->label);
            addAndMakeVisible(slot->comboBox);
            comboSlots.push_back(std::move(slot));
        }
        else if (meta.isBool)
        {
            auto slot = std::make_unique<ButtonSlot>();
            slot->paramId = meta.apvtsId;
            slot->button.setButtonText(meta.name);
            slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, meta.apvtsId, slot->button);
            addAndMakeVisible(slot->button);
            buttonSlots.push_back(std::move(slot));
        }
        else
        {
            auto slot = std::make_unique<KnobSlot>();
            slot->paramId = meta.apvtsId;
            slot->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slot->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 14);
            slot->nameLabel.setText(meta.name, juce::dontSendNotification);
            slot->nameLabel.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
            slot->nameLabel.setJustificationType(juce::Justification::centred);

            slot->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, meta.apvtsId, slot->slider);
            addAndMakeVisible(slot->nameLabel);
            addAndMakeVisible(slot->slider);
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
    }
    for (auto& s : buttonSlots)
    {
        s->button.setVisible(show);
    }
    for (auto& s : comboSlots)
    {
        s->comboBox.setVisible(show);
        s->label.setVisible(show);
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
}

void BRAUN_MR16AudioProcessorEditor::drawBraunChassis(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.fillAll(findColour(mr16::BraunColours::bgAppColourId));

    // Top Header Banner
    auto headerRect = bounds.removeFromTop(48);
    g.setColour(findColour(mr16::BraunColours::borderLineColourId));
    g.fillRect(headerRect.removeFromBottom(1));

    g.setColour(findColour(mr16::BraunColours::textPrimaryColourId));
    g.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    g.drawText("BRAUN MR-16", 18, 10, 140, 18, juce::Justification::centredLeft);

    g.setColour(findColour(mr16::BraunColours::textMutedColourId));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("MODAL RESONATOR & KINETIC SYNTHESIZER", 18, 28, 240, 14, juce::Justification::centredLeft);

    // Vector CRT display
    auto crtArea = juce::Rectangle<int>(bounds.getRight() - 320, 56, 300, 160);
    drawCrtDisplay(g, crtArea);
}

void BRAUN_MR16AudioProcessorEditor::drawCrtDisplay(juce::Graphics& g, juce::Rectangle<int> crtBounds)
{
    // CRT outer bezel
    g.setColour(findColour(mr16::BraunColours::bgBezelColourId));
    g.fillRoundedRectangle(crtBounds.toFloat(), 6.0f);
    g.setColour(findColour(mr16::BraunColours::borderLineColourId));
    g.drawRoundedRectangle(crtBounds.toFloat(), 6.0f, 1.0f);

    auto screen = crtBounds.reduced(8);
    g.setColour(juce::Colour(0xFF070A08));
    g.fillRoundedRectangle(screen.toFloat(), 3.0f);

    // Subtle CRT raster scanlines
    g.setColour(juce::Colours::black.withAlpha(0.20f));
    for (int y = screen.getY(); y < screen.getBottom(); y += 3)
        g.drawHorizontalLine(y, (float)screen.getX(), (float)screen.getRight());

    // Center graticule crosshair
    g.setColour(juce::Colour(0xFF142218));
    g.drawHorizontalLine(screen.getCentreY(), (float)screen.getX(), (float)screen.getRight());
    g.drawVerticalLine(screen.getCentreX(), (float)screen.getY(), (float)screen.getBottom());

    // Determine current display mode
    int mode = 0;
    if (auto* dispParam = processorRef.getAPVTS().getRawParameterValue(mr16::ParamIDs::displayMode.getParamID()))
    {
        mode = static_cast<int>(std::round(dispParam->load(std::memory_order_relaxed)));
    }

    if (mode == 0)
    {
        // --------------------------------------------------------------------
        // Mode 0: 2D Chladni Nodal Geometry Simulation
        // --------------------------------------------------------------------
        g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.85f));

        const float cx = (float)screen.getCentreX();
        const float cy = (float)screen.getCentreY();
        const float maxR = (float)juce::jmin(screen.getWidth(), screen.getHeight()) * 0.42f;

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
        // --------------------------------------------------------------------
        // Mode 1: 3D Lorenz Attractor Orbit
        // --------------------------------------------------------------------
        g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.85f));
        const float cx = (float)screen.getCentreX();
        const float cy = (float)screen.getCentreY();

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
        // --------------------------------------------------------------------
        // Mode 2: 16-Pole Modal FFT Bars
        // --------------------------------------------------------------------
        const int nBars = 16;
        float barWidth = ((float)screen.getWidth() - 4.0f) / (float)nBars;
        for (int i = 0; i < nBars; ++i)
        {
            float energy = std::clamp(latestTelemetryFrame.modalEnergies[i] * 120.0f, 0.0f, 1.0f);
            float barH = energy * ((float)screen.getHeight() - 8.0f);
            float bx = (float)screen.getX() + 2.0f + (float)i * barWidth;
            float by = (float)screen.getBottom() - 4.0f - barH;

            g.setColour(findColour(mr16::BraunColours::phosphorColourId));
            g.fillRect(bx + 1.0f, by, barWidth - 2.0f, barH);
        }
    }

    // Overlay real-time oscilloscope waveform trace
    float scopeL[256];
    processorRef.getScopeSamples(scopeL, nullptr, 256);

    juce::Path wave;
    for (int i = 0; i < screen.getWidth(); ++i)
    {
        int idx = (i * 256) / screen.getWidth();
        float y = (float)screen.getCentreY() - (scopeL[idx] * (float)screen.getHeight() * 0.35f);
        if (i == 0) wave.startNewSubPath((float)screen.getX() + (float)i, y);
        else wave.lineTo((float)screen.getX() + (float)i, y);
    }
    g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.50f));
    g.strokePath(wave, juce::PathStrokeType(1.0f));

    // Corner CRT legend
    g.setColour(findColour(mr16::BraunColours::phosphorColourId).withAlpha(0.65f));
    g.setFont(juce::Font(juce::FontOptions(8.0f, juce::Font::bold)));
    juce::String modeName = (mode == 0) ? "CHLADNI 2D" : (mode == 1) ? "ATTRACTOR 3D" : "MODAL FFT";
    g.drawText(modeName, screen.reduced(4), juce::Justification::topRight);
}

void BRAUN_MR16AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

#if JUCE_WEB_BROWSER
    if (webComponent && !useNativeUI)
    {
        webComponent->setBounds(bounds);
        return;
    }
#endif

    layoutNativeControls();
}

void BRAUN_MR16AudioProcessorEditor::layoutNativeControls()
{
    auto bounds = getLocalBounds();

    // 1. Top Header Bar (48px)
    auto header = bounds.removeFromTop(48);
    powerButton.setBounds(header.removeFromRight(88).reduced(6, 10));
    recordButton.setBounds(header.removeFromRight(88).reduced(6, 10));
    themeButton.setBounds(header.removeFromRight(76).reduced(6, 10));

#if JUCE_WEB_BROWSER
    viewModeButton.setBounds(header.removeFromRight(88).reduced(6, 10));
#endif

    // Preset cluster
    nextPresetBtn.setBounds(header.removeFromRight(32).reduced(2, 12));
    presetComboBox.setBounds(header.removeFromRight(150).reduced(2, 12));
    prevPresetBtn.setBounds(header.removeFromRight(32).reduced(2, 12));
    presetLabel.setBounds(header.removeFromRight(60).reduced(2, 12));

    // 2. Audition Exciter Bar (36px)
    auto audition = bounds.removeFromTop(36).reduced(12, 4);
    strikeTriggerBtn.setBounds(audition.removeFromLeft(80).reduced(4, 2));
    frictionTriggerBtn.setBounds(audition.removeFromLeft(80).reduced(4, 2));
    vactrolTriggerBtn.setBounds(audition.removeFromLeft(80).reduced(4, 2));
    poissonTriggerBtn.setBounds(audition.removeFromLeft(90).reduced(4, 2));
    diracTriggerBtn.setBounds(audition.removeFromLeft(80).reduced(4, 2));

    // 3. Grid for 5 Main Decks
    bounds.reduce(12, 8);

    const int colW = bounds.getWidth() / 5;
    auto deck1Area = bounds.removeFromLeft(colW).reduced(4);
    auto deck2Area = bounds.removeFromLeft(colW).reduced(4);
    auto deck3Area = bounds.removeFromLeft(colW).reduced(4);
    auto deck4Area = bounds.removeFromLeft(colW).reduced(4);
    auto deck5Area = bounds.reduced(4);

    auto layoutSlotInArea = [](juce::Rectangle<int>& area, KnobSlot* k) {
        if (!k) return;
        auto row = area.removeFromTop(58);
        k->nameLabel.setBounds(row.removeFromTop(12));
        k->slider.setBounds(row);
    };

    // Deck 01 (Exciter)
    if (auto* c = findCombo(mr16::ParamIDs::exciterType)) {
        auto row = deck1Area.removeFromTop(36);
        c->label.setBounds(row.removeFromTop(12));
        c->comboBox.setBounds(row);
    }
    layoutSlotInArea(deck1Area, findKnob(mr16::ParamIDs::strikeHardness));
    layoutSlotInArea(deck1Area, findKnob(mr16::ParamIDs::strikeVelocity));
    layoutSlotInArea(deck1Area, findKnob(mr16::ParamIDs::frictionForce));
    layoutSlotInArea(deck1Area, findKnob(mr16::ParamIDs::frictionSpeed));
    layoutSlotInArea(deck1Area, findKnob(mr16::ParamIDs::vactrolSag));
    layoutSlotInArea(deck1Area, findKnob(mr16::ParamIDs::extInputGain));
    layoutSlotInArea(deck1Area, findKnob(mr16::ParamIDs::poissonDensity));

    // Deck 02 (Modal Matrix)
    if (auto* c = findCombo(mr16::ParamIDs::manifoldType)) {
        auto row = deck2Area.removeFromTop(36);
        c->label.setBounds(row.removeFromTop(12));
        c->comboBox.setBounds(row);
    }
    if (auto* c = findCombo(mr16::ParamIDs::materialProfile)) {
        auto row = deck2Area.removeFromTop(36);
        c->label.setBounds(row.removeFromTop(12));
        c->comboBox.setBounds(row);
    }
    layoutSlotInArea(deck2Area, findKnob(mr16::ParamIDs::modalFrequency));
    layoutSlotInArea(deck2Area, findKnob(mr16::ParamIDs::modalDamping));
    layoutSlotInArea(deck2Area, findKnob(mr16::ParamIDs::modalCoupling));
    layoutSlotInArea(deck2Area, findKnob(mr16::ParamIDs::modalSpread));

    // Deck 03 (Lorenz Attractor)
    layoutSlotInArea(deck3Area, findKnob(mr16::ParamIDs::lorenzRate));
    layoutSlotInArea(deck3Area, findKnob(mr16::ParamIDs::lorenzChaos));
    layoutSlotInArea(deck3Area, findKnob(mr16::ParamIDs::lorenzFreqMod));
    layoutSlotInArea(deck3Area, findKnob(mr16::ParamIDs::lorenzQMod));

    // Deck 04 (Chorus)
    if (auto* b = findButton(mr16::ParamIDs::chorusEnable)) {
        b->button.setBounds(deck4Area.removeFromTop(28).reduced(4, 2));
    }
    layoutSlotInArea(deck4Area, findKnob(mr16::ParamIDs::chorusRateHz));
    layoutSlotInArea(deck4Area, findKnob(mr16::ParamIDs::chorusDepthMs));
    layoutSlotInArea(deck4Area, findKnob(mr16::ParamIDs::chorusDimension));
    layoutSlotInArea(deck4Area, findKnob(mr16::ParamIDs::chorusMix));

    // Deck 05 (Spatial & Dynamics)
    layoutSlotInArea(deck5Area, findKnob(mr16::ParamIDs::goldenPanSpread));
    layoutSlotInArea(deck5Area, findKnob(mr16::ParamIDs::vactrolLpgCutoff));
    layoutSlotInArea(deck5Area, findKnob(mr16::ParamIDs::driveSaturation));
    layoutSlotInArea(deck5Area, findKnob(mr16::ParamIDs::masterTrimDb));
    layoutSlotInArea(deck5Area, findKnob(mr16::ParamIDs::dryWetMix));

    // Deck 06 Display Mode Combo
    if (auto* c = findCombo(mr16::ParamIDs::displayMode)) {
        auto row = deck5Area.removeFromTop(36);
        c->label.setBounds(row.removeFromTop(12));
        c->comboBox.setBounds(row);
    }
}

void BRAUN_MR16AudioProcessorEditor::parentHierarchyChanged()
{
}

void BRAUN_MR16AudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
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
