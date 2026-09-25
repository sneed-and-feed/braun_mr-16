# Compiling from Source

The **BRAUN MR-16** is engineered in **ISO C++20** using **JUCE 8** and CMake 3.22+. The plugin compiles into native **VST3**, **CLAP**, and **Standalone** binaries across Windows, macOS, and Linux, with zero external runtime dependencies.

---

## Toolchain & System Prerequisites

| Platform | Recommended Toolchain | Required Dependencies |
| :--- | :--- | :--- |
| **Windows 10 / 11** | Visual Studio 2022 (MSVC v143) or Clang-CL | CMake $\ge 3.22$, Windows 10/11 SDK, WebView2 Runtime |
| **macOS (12–15)** | Xcode 15+ or Apple Clang 15+ | CMake $\ge 3.22$, Universal SDK (`arm64` + `x86_64`) |
| **Linux (x86_64)** | GCC 11+ or Clang 14+ | CMake $\ge 3.22$, ALSA (`libasound2-dev`), JACK (`libjack-jackd2-dev`), WebKitGTK (`libwebkit2gtk-4.1-dev` optional) |
| **Web Environment** | Node.js $\ge 18.0$ (LTS) | `npm` for testing and static asset bundling |

---

## CMake Configuration Options

The following flags can be passed to CMake during the configuration phase:

| CMake Flag | Default | Description |
| :--- | :---: | :--- |
| `-DMR16_BUILD_VST3=ON/OFF` | `ON` | Builds the native VST3 plugin target |
| `-DMR16_BUILD_CLAP=ON/OFF` | `ON` | Builds the native CLAP plugin target |
| `-DMR16_BUILD_STANDALONE=ON/OFF` | `ON` | Builds the standalone desktop executable |
| `-DMR16_BUILD_TESTS=ON/OFF` | `ON` | Compiles the automated CTest DSP verification suite |
| `-DMR16_USE_WEBVIEW=ON/OFF` | `ON`* | Enables embedded Web UI (*`OFF` by default on Linux) |

---

## Step-by-Step Compilation Guides

### 1. Windows (Visual Studio 2022 or Ninja)

Clone the repository and open a PowerShell terminal:

```powershell
# Clone the repository recursively with submodules
git clone https://github.com/sneed-and-feed/braun_mr-16.git
cd braun_mr-16

# Configure the build directory for Visual Studio 2022 x64
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

# Build all targets in parallel
cmake --build build --config Release --parallel
```

Compiled binaries will be generated in:
- Standalone: `build/source/plugin/BRAUN_MR16_artefacts/Release/Standalone/BRAUN_MR16.exe`
- VST3: `build/source/plugin/BRAUN_MR16_artefacts/Release/VST3/BRAUN_MR16.vst3`
- CLAP: `build/source/plugin/BRAUN_MR16_artefacts/Release/CLAP/BRAUN_MR16.clap`

---

### 2. macOS (Universal Apple Silicon & Intel)

```bash
# Clone the repository
git clone https://github.com/sneed-and-feed/braun_mr-16.git
cd braun_mr-16

# Configure Universal binary build (arm64 + x86_64)
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"

# Compile targets
cmake --build build --config Release --parallel
```

Compiled bundles:
- Standalone: `build/source/plugin/BRAUN_MR16_artefacts/Release/Standalone/BRAUN_MR16.app`
- VST3: `build/source/plugin/BRAUN_MR16_artefacts/Release/VST3/BRAUN_MR16.vst3`
- AUv2: `build/source/plugin/BRAUN_MR16_artefacts/Release/AU/BRAUN_MR16.component`
- CLAP: `build/source/plugin/BRAUN_MR16_artefacts/Release/CLAP/BRAUN_MR16.clap`

---

### 3. Linux (Ubuntu, Debian, Fedora, Arch)

#### Headless / Native Build (No WebKitGTK Dependency)
On servers, minimal distributions, or environments without WebKitGTK, compile with `MR16_USE_WEBVIEW=OFF`:

```bash
# Install audio build dependencies (Debian/Ubuntu)
sudo apt-get update
sudo apt-get install -y build-essential cmake libasound2-dev libjack-jackd2-dev

# Configure and compile
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMR16_USE_WEBVIEW=OFF
cmake --build build --config Release --parallel
```

#### Full GUI Build (with WebKitGTK)
To build with the embedded vector CRT and HTML5 frontend on Linux:

```bash
# Install WebKitGTK and audio dependencies
sudo apt-get install -y libwebkit2gtk-4.1-dev libasound2-dev libjack-jackd2-dev

# Configure with WebView enabled
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMR16_USE_WEBVIEW=ON
cmake --build build --config Release --parallel
```

---

## Running the Automated Verification Suites

The MR-16 codebase maintains continuous 100% verification across two test runners:

### 1. C++ DSP Test Suite (CTest)
Validates real-time safety, 0 heap allocations, Householder energy conservation, Zavalishin filter stability, and soft limiting:

```bash
ctest --test-dir build -C Release --output-on-failure
```

Expected result:
```text
100% tests passed, 0 tests failed out of 207
Total Test time (real) = 0.85 sec
```

### 2. Web Audio & UI Test Suite (Node.js)
Validates the sample extractor, Radix-2 FFT, sub-bin parabolic peak interpolation, Lorenz attractor morpher, APVTS synchronization, and UI ergonomics:

```bash
npm test
```

Expected result:
```text
Test Suites: 44 passed, 44 total
Tests:       213 passed, 213 total
Snapshots:   0 total
Time:        3.42 s
```

---

## Automated Release Packaging

To build clean production release archives with SHA-256 manifests on Windows:

```powershell
pwsh -ExecutionPolicy Bypass -File scripts/package-release.ps1
```

This creates:
- `releases/BRAUN_MR16-v1.0.12-Windows-x64.zip` (Standalone + VST3 + CLAP)
- `releases/BRAUN_MR16-v1.0.12-VST3-Windows-x64.zip` (VST3 plugin only)
- `releases/SHA256SUMS.txt` (Cryptographic verification checksums)
