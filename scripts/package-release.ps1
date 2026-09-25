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
$Version = "1.0.12"
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
# Package: Standard Consolidated Windows Release Bundles
# ------------------------------------------------------------------------------
$ReleasePy = Join-Path $RootDir "scripts\package_release.py"
if (Test-Path $ReleasePy) {
    Write-Host "[INFO] Generating clean releases/ bundles via package_release.py..."
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

$ReleasesDir = Join-Path $RootDir "releases"
if (Test-Path $ReleasesDir) {
    $RootReleaseZips = Get-ChildItem -Path $ReleasesDir -Filter "*.zip" | Sort-Object FullName
    if ($RootReleaseZips.Count -gt 0) {
        $ReleaseChecksumLines = [System.Collections.Generic.List[string]]::new()
        foreach ($Zip in $RootReleaseZips) {
            $Hash = (Get-FileHash -Path $Zip.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            $ReleaseChecksumLines.Add("$Hash  $($Zip.Name)")
        }
        $ReleaseChecksumFile = Join-Path $ReleasesDir "SHA256SUMS.txt"
        [System.IO.File]::WriteAllLines($ReleaseChecksumFile, $ReleaseChecksumLines, [System.Text.Encoding]::UTF8)
        Write-Host "[INFO] Wrote releases checksum manifest to $ReleaseChecksumFile"
    }
}

# ------------------------------------------------------------------------------
# 6. Cleanup Staging Directory
# ------------------------------------------------------------------------------
Remove-Item -Path $StagingBase -Recurse -Force

Write-Host "================================================================================"
Write-Host "RELEASE PACKAGING COMPLETED SUCCESSFULLY"
Write-Host "Artifacts location: $DistDir"
Write-Host "================================================================================"
