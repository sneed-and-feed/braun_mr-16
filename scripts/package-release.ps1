<#
    .SYNOPSIS
        BRAUN MR-16 Release Packaging Pipeline.
        Packages Windows Standalone, VST3, CLAP binaries and Web Showcase into standardized distribution archives.
        Calculates SHA-256 cryptographic verification checksums.
    .DESCRIPTION
        Industrial distribution packaging tool adhering to DIN 1451 technical specification standards.
        Outputs distribution artifacts to dist/windows and dist/web, generating SHA256SUMS.txt.
    .PARAMETER ForceBuild
        Forces CMake reconfiguration and compilation even if existing Release binaries are detected.
    .PARAMETER SkipTests
        Bypasses pre-packaging headless DSP unit verification.
#>
[CmdletBinding()]
param(
    [switch]$ForceBuild,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = (Resolve-Path (Join-Path $ScriptDir "..")).Path
$BuildDir = Join-Path $RootDir "build"
$ArtefactsDir = Join-Path $BuildDir "BRAUN_MR16_artefacts\Release"
$DistDir = Join-Path $RootDir "dist"
$DistWinDir = Join-Path $DistDir "windows"
$DistWebDir = Join-Path $DistDir "web"
$StagingBase = Join-Path $DistDir "_staging"

Write-Host "================================================================================"
Write-Host "BRAUN MR-16 -- RELEASE PACKAGING PIPELINE"
Write-Host "Standard: DIN 1451 Technical Specification"
Write-Host "Root Directory: $RootDir"
Write-Host "================================================================================"

# 1. Version Resolution
$PackageJsonPath = Join-Path $RootDir "package.json"
$Version = "1.0.2"
if (Test-Path $PackageJsonPath) {
    try {
        $Pkg = Get-Content $PackageJsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($Pkg.version) {
            $Version = $Pkg.version
        }
    } catch {
        Write-Warning "Failed to parse package.json. Defaulting to version $Version."
    }
}
Write-Host "[INFO] Target Release Version: $Version"

# 2. Binary Verification / Compilation Check
$StandaloneBin = Join-Path $ArtefactsDir "Standalone\BRAUN_MR16.exe"
$Vst3Dir = Join-Path $ArtefactsDir "VST3\BRAUN_MR16.vst3"
$ClapBin = Join-Path $ArtefactsDir "CLAP\BRAUN_MR16.clap"

# Inspect CMakeCache for stale WebView settings or force rebuild
$CacheFile = Join-Path $BuildDir "CMakeCache.txt"
$StaleCacheDetected = $false
if (Test-Path $CacheFile) {
    $CacheContent = Get-Content $CacheFile -Raw -ErrorAction SilentlyContinue
    if ($ForceBuild -or $CacheContent -match "MR16_USE_WEBVIEW:BOOL=OFF" -or $CacheContent -match "JUCE_WEBVIEW2_PACKAGE_LOCATION:BOOL=OFF") {
        Write-Host "[INFO] Clearing CMakeCache.txt, CMakeFiles, and FetchContent subbuilds to enforce clean configuration..."
        Remove-Item -Path $CacheFile -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $BuildDir "CMakeFiles") -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -Path (Join-Path $BuildDir "_deps\*-subbuild") -Recurse -Force -ErrorAction SilentlyContinue
        $StaleCacheDetected = $true
    }
}

$NeedBuild = $ForceBuild -or $StaleCacheDetected -or (-not (Test-Path $StandaloneBin)) -or (-not (Test-Path $Vst3Dir))

# Resolve local WebView2 package repository if available
$LocalWebview2Parent = $null
$CandidateWebview2Dirs = @(
    (Join-Path $RootDir "build\packages"),
    (Join-Path $RootDir "..\braun_rb-26\build\packages"),
    (Join-Path $RootDir "..\braun_as-42\build\packages"),
    (Join-Path $RootDir "..\build\packages")
)
foreach ($Dir in $CandidateWebview2Dirs) {
    if (Test-Path $Dir) {
        $Resolved = (Resolve-Path $Dir).Path
        if (Get-ChildItem -Path $Resolved -Filter "Microsoft.Web.WebView2.*" -Directory -ErrorAction SilentlyContinue) {
            $LocalWebview2Parent = $Resolved
            Write-Host "[INFO] Detected local WebView2 package repository: $LocalWebview2Parent"
            break
        }
    }
}

if ($NeedBuild) {
    Write-Host "[INFO] Compiling Release binaries via CMake (MR16_USE_WEBVIEW=ON)..."
    $CmakeArgs = @(
        "-B", "build",
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DMR16_USE_WEBVIEW=ON",
        "-DMR16_BUILD_TESTS=ON"
    )
    if ($LocalWebview2Parent) {
        $CmakeArgs += "-DJUCE_WEBVIEW2_PACKAGE_LOCATION=$LocalWebview2Parent"
    }

    Write-Host "[EXEC] cmake $($CmakeArgs -join ' ')"
    & cmake @CmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE."
    }

    $BuildArgs = @("--build", "build", "--config", "Release", "--target", "BRAUN_MR16_All", "mr16_headless_dsp_tests")
    Write-Host "[EXEC] cmake $($BuildArgs -join ' ')"
    & cmake @BuildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake compilation failed with exit code $LASTEXITCODE."
    }
} else {
    Write-Host "[INFO] Verified pre-existing Release binaries in $ArtefactsDir"
}

# 3. DSP Verification Test Execution
if (-not $SkipTests) {
    Write-Host "[INFO] Executing headless DSP verification suite..."
    $CandidateTestPaths = @(
        (Join-Path $BuildDir "source\tests\Release\mr16_headless_dsp_tests.exe"),
        (Join-Path $RootDir "build_tests\Release\mr16_headless_dsp_tests.exe"),
        (Join-Path $BuildDir "Release\mr16_headless_dsp_tests.exe"),
        (Join-Path $BuildDir "mr16_headless_dsp_tests.exe")
    )

    $TestExe = $null
    foreach ($Path in $CandidateTestPaths) {
        if (Test-Path $Path) {
            $TestExe = $Path
            break
        }
    }

    if ($TestExe) {
        Write-Host "[EXEC] $TestExe"
        & $TestExe
        if ($LASTEXITCODE -ne 0) {
            throw "Headless DSP verification failed with exit code $LASTEXITCODE. Packaging aborted."
        }
        Write-Host "[INFO] All headless DSP tests passed successfully."
    } else {
        Write-Warning "Headless test executable not found in candidate paths. Skipping test step."
    }
} else {
    Write-Host "[WARN] SkipTests parameter specified. Bypassing DSP verification."
}

# 4. Preparation of Distribution Directories
if (Test-Path $StagingBase) {
    Remove-Item -Path $StagingBase -Recurse -Force
}
New-Item -ItemType Directory -Path $DistWinDir -Force | Out-Null
New-Item -ItemType Directory -Path $DistWebDir -Force | Out-Null
New-Item -ItemType Directory -Path $StagingBase -Force | Out-Null

Add-Type -AssemblyName System.IO.Compression.FileSystem

function Create-ZipArchive {
    param(
        [string]$SourceDirectory,
        [string]$DestinationZipPath
    )
    if (Test-Path $DestinationZipPath) {
        Remove-Item -Path $DestinationZipPath -Force
    }
    [System.IO.Compression.ZipFile]::CreateFromDirectory(
        $SourceDirectory,
        $DestinationZipPath,
        [System.IO.Compression.CompressionLevel]::Optimal,
        $false
    )
    $Size = (Get-Item $DestinationZipPath).Length
    Write-Host "[PACKAGE] Generated $(Split-Path -Leaf $DestinationZipPath) ($([math]::Round($Size / 1MB, 2)) MB / $Size bytes)"
}

$LicenseFile = Join-Path $RootDir "LICENSE"
$ReadmeFile = Join-Path $RootDir "README.md"

# ------------------------------------------------------------------------------
# Package 1: Standalone Application (Windows x64)
# ------------------------------------------------------------------------------
$StageStandalone = Join-Path $StagingBase "standalone"
New-Item -ItemType Directory -Path $StageStandalone -Force | Out-Null
Copy-Item -Path $StandaloneBin -Destination (Join-Path $StageStandalone "BRAUN_MR16.exe")
Copy-Item -Path $LicenseFile -Destination (Join-Path $StageStandalone "LICENSE")
Copy-Item -Path $ReadmeFile -Destination (Join-Path $StageStandalone "README.md")

$StandaloneInstallGuide = @"
BRAUN MR-16 MODAL RESONATOR & KINETIC SYNTHESIZER
STANDALONE APPLICATION (WIN64) INSTALLATION & EXECUTION GUIDE
Standard: DIN 1451 Technical Specification

1. EXECUTION:
   Launch BRAUN_MR16.exe directly. No administrative elevation required.

2. AUDIO CONFIGURATION:
   Navigate to Audio Settings inside the top title bar to select ASIO or WASAPI drivers.
   Recommended buffer size: 128 or 256 samples @ 44.1 kHz, 48.0 kHz, or 96.0 kHz.

3. PERSISTENCE & USER PRESETS:
   Preset and state configurations are stored in RFC 8259 JSON format.
"@
Set-Content -Path (Join-Path $StageStandalone "INSTALL.txt") -Value $StandaloneInstallGuide -Encoding UTF8

$StandaloneZipPath = Join-Path $DistWinDir "BRAUN_MR16_v${Version}_Standalone_Win64.zip"
Create-ZipArchive -SourceDirectory $StageStandalone -DestinationZipPath $StandaloneZipPath

# ------------------------------------------------------------------------------
# Package 2: VST3 Plugin Bundle (Windows x64)
# ------------------------------------------------------------------------------
$StageVst3 = Join-Path $StagingBase "vst3"
New-Item -ItemType Directory -Path $StageVst3 -Force | Out-Null
Copy-Item -Path $Vst3Dir -Destination (Join-Path $StageVst3 "BRAUN_MR16.vst3") -Recurse
Copy-Item -Path $LicenseFile -Destination (Join-Path $StageVst3 "LICENSE")
Copy-Item -Path $ReadmeFile -Destination (Join-Path $StageVst3 "README.md")

$Vst3InstallGuide = @"
BRAUN MR-16 MODAL RESONATOR & KINETIC SYNTHESIZER
VST3 PLUGIN BUNDLE (WIN64) INSTALLATION GUIDE
Standard: DIN 1451 Technical Specification

1. INSTALLATION DIRECTORY:
   Copy the directory 'BRAUN_MR16.vst3' into your system VST3 directory:
   %CommonProgramFiles%\VST3\
   (Default path: C:\Program Files\Common Files\VST3\BRAUN_MR16.vst3)

2. DAW RESCAN:
   Perform a plugin rescan in your Digital Audio Workstation (Cubase, Ableton Live, FL Studio, Reaper, Bitwig).

3. INSTRUMENT CLASSIFICATION:
   Category: Synthesizer / Physical Modelling / Resonator / Spatial FX.
   Manufacturer: Braun
"@
Set-Content -Path (Join-Path $StageVst3 "INSTALL.txt") -Value $Vst3InstallGuide -Encoding UTF8

$Vst3ZipPath = Join-Path $DistWinDir "BRAUN_MR16_v${Version}_VST3_Win64.zip"
Create-ZipArchive -SourceDirectory $StageVst3 -DestinationZipPath $Vst3ZipPath

# ------------------------------------------------------------------------------
# Package 3: CLAP Plugin (Windows x64)
# ------------------------------------------------------------------------------
if (Test-Path $ClapBin) {
    $StageClap = Join-Path $StagingBase "clap"
    New-Item -ItemType Directory -Path $StageClap -Force | Out-Null
    Copy-Item -Path $ClapBin -Destination (Join-Path $StageClap "BRAUN_MR16.clap")
    Copy-Item -Path $LicenseFile -Destination (Join-Path $StageClap "LICENSE")
    Copy-Item -Path $ReadmeFile -Destination (Join-Path $StageClap "README.md")

    $ClapInstallGuide = @"
BRAUN MR-16 MODAL RESONATOR & KINETIC SYNTHESIZER
CLAP PLUGIN (WIN64) INSTALLATION GUIDE
Standard: DIN 1451 Technical Specification / CLAP 1.0+ Standard

1. INSTALLATION DIRECTORY:
   Copy 'BRAUN_MR16.clap' into your system CLAP directory:
   %CommonProgramFiles%\CLAP\
   (Default path: C:\Program Files\Common Files\CLAP\BRAUN_MR16.clap)

2. DAW RESCAN:
   Rescan plugins in your CLAP-compatible host (Bitwig Studio, Reaper, FL Studio).

3. FEATURES:
   Non-destructive polyphonic parameter modulation, sample-accurate automation.
"@
    Set-Content -Path (Join-Path $StageClap "INSTALL.txt") -Value $ClapInstallGuide -Encoding UTF8

    $ClapZipPath = Join-Path $DistWinDir "BRAUN_MR16_v${Version}_CLAP_Win64.zip"
    Create-ZipArchive -SourceDirectory $StageClap -DestinationZipPath $ClapZipPath
} else {
    Write-Host "[INFO] CLAP plugin binary not found; skipping CLAP package generation."
}

# ------------------------------------------------------------------------------
# Package 4: Web Showcase Bundle (Cross-Platform Zero-Install)
# ------------------------------------------------------------------------------
$StageWeb = Join-Path $StagingBase "web"
New-Item -ItemType Directory -Path $StageWeb -Force | Out-Null
Copy-Item -Path (Join-Path $RootDir "web") -Destination (Join-Path $StageWeb "web") -Recurse
Copy-Item -Path (Join-Path $RootDir "server.js") -Destination (Join-Path $StageWeb "server.js")
Copy-Item -Path (Join-Path $RootDir "start.bat") -Destination (Join-Path $StageWeb "start.bat")
Copy-Item -Path (Join-Path $RootDir "package.json") -Destination (Join-Path $StageWeb "package.json")
Copy-Item -Path $LicenseFile -Destination (Join-Path $StageWeb "LICENSE")
Copy-Item -Path $ReadmeFile -Destination (Join-Path $StageWeb "README.md")

$WebInstallGuide = @"
BRAUN MR-16 MODAL RESONATOR & KINETIC SYNTHESIZER
WEB SHOWCASE BUNDLE (CROSS-PLATFORM ZERO-INSTALL)
Standard: DIN 1451 Technical Specification

1. EXECUTION:
   Windows: Double-click 'start.bat'
   macOS/Linux: Run 'node server.js' or 'npm start'

2. BROWSER ACCESS:
   Navigate to http://localhost:3816/
   Chrome, Edge, Firefox, or Safari with Web Audio API support.

3. NO EXTERNAL DEPENDENCIES:
   100% client-side DSP, zero remote network calls, offline capable.
"@
Set-Content -Path (Join-Path $StageWeb "INSTALL.txt") -Value $WebInstallGuide -Encoding UTF8

$WebZipPath = Join-Path $DistWebDir "BRAUN_MR16_v${Version}_Web_Showcase.zip"
Create-ZipArchive -SourceDirectory $StageWeb -DestinationZipPath $WebZipPath

# ------------------------------------------------------------------------------
# Package 5: Root Releases (Standalone + VST3 bundles)
# ------------------------------------------------------------------------------
$ReleasePy = Join-Path $RootDir "scripts\package_release.py"
if (Test-Path $ReleasePy) {
    Write-Host "[INFO] Generating root releases/ bundles via package_release.py..."
    python $ReleasePy
    $ReleaseZips = Get-ChildItem -Path (Join-Path $RootDir "releases") -Filter "BRAUN_MR16-v${Version}-*.zip"
    foreach ($RZip in $ReleaseZips) {
        Copy-Item $RZip.FullName -Destination $DistWinDir -Force
    }
}

# ------------------------------------------------------------------------------
# 5. Cryptographic Verification Checksum Generation (SHA-256)
# ------------------------------------------------------------------------------
Write-Host "[INFO] Computing SHA-256 cryptographic verification checksums..."
$AllZipFiles = Get-ChildItem -Path $DistDir -Filter "*.zip" -Recurse | Sort-Object FullName

$ChecksumLines = [System.Collections.Generic.List[string]]::new()
foreach ($Zip in $AllZipFiles) {
    $Hash = (Get-FileHash -Path $Zip.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $RelativePath = ($Zip.FullName.Substring($DistDir.Length)).TrimStart("\", "/").Replace("\", "/")
    $ChecksumLines.Add("$Hash  $RelativePath")
    Write-Host "  SHA256: $Hash  $RelativePath"
}

$ChecksumFile = Join-Path $DistDir "SHA256SUMS.txt"
[System.IO.File]::WriteAllLines($ChecksumFile, $ChecksumLines, [System.Text.Encoding]::UTF8)
Write-Host "[INFO] Wrote checksum manifest to $ChecksumFile"

# ------------------------------------------------------------------------------
# 6. Cleanup Staging Directory
# ------------------------------------------------------------------------------
Remove-Item -Path $StagingBase -Recurse -Force

Write-Host "================================================================================"
Write-Host "RELEASE PACKAGING COMPLETED SUCCESSFULLY"
Write-Host "Artifacts location: $DistDir"
Write-Host "================================================================================"
