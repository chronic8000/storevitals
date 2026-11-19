<#
.SYNOPSIS
  Package StoreVitals for Microsoft Store (.msix / .msixbundle / .msixupload).

.DESCRIPTION
  1. Expects Release binaries already built for x64 and/or ARM64.
  2. Runs windeployqt into a staging folder per architecture.
  3. Copies Package.appxmanifest + Assets.
  4. Invokes MakeAppx to produce .msix packages, then optional bundle/upload.

.PARAMETER X64Dir
  Path to the x64 Release output containing StoreVitals.exe (e.g. build\x64).

.PARAMETER Arm64Dir
  Path to the ARM64 Release output containing StoreVitals.exe.

.PARAMETER QtPrefix
  Qt kit prefix (default: C:\Qt\6.8.1\msvc2022_64). For ARM64 use the ARM64 kit.

.PARAMETER OutDir
  Output directory for packages (default: packaging\out).

.PARAMETER SkipBundle
  Only produce per-arch .msix files.

.EXAMPLE
  # Local unpackaged builds use -DDEV_BYPASS_STORE_LICENSE=ON (default).
  # Store packages MUST be built with -DDEV_BYPASS_STORE_LICENSE=OFF.

  cmake -S . -B build\x64 -DDEV_BYPASS_STORE_LICENSE=OFF
  cmake --build build\x64 --config Release
  .\packaging\package_msix.ps1 -X64Dir .\build\x64 -QtPrefix C:\Qt\6.8.1\msvc2022_64
#>

[CmdletBinding()]
param(
    [string]$X64Dir = "",
    [string]$Arm64Dir = "",
    [string]$QtPrefix = "C:\Qt\6.8.1\msvc2022_64",
    [string]$QtArm64Prefix = "C:\Qt_arm\6.12.0\msvc2022_arm64",
    [string]$OutDir = "",
    [switch]$SkipBundle
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) { $OutDir = Join-Path $PSScriptRoot "out" }

function Require-File([string]$Path, [string]$Hint) {
    if (-not (Test-Path $Path)) { throw "Missing $Hint : $Path" }
}

function Find-MakeAppx {
    $hostArch = if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") { "arm64" } else { "x64" }
    $candidates = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.26100.0\$hostArch\makeappx.exe",
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.26100.0\x64\makeappx.exe",
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.22621.0\$hostArch\makeappx.exe",
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\10.0.22621.0\x64\makeappx.exe",
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\$hostArch\makeappx.exe",
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\x64\makeappx.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }
    $found = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin" -Recurse -Filter makeappx.exe -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "\\$hostArch\\makeappx.exe$" -or $_.FullName -match '\\x64\\makeappx.exe$' } |
        Select-Object -First 1
    if ($found) { return $found.FullName }
    throw "makeappx.exe not found. Install the Windows 10/11 SDK."
}

function Stage-Arch([string]$Arch, [string]$BuildDir, [string]$QtDir) {
    Require-File (Join-Path $BuildDir "StoreVitals.exe") "StoreVitals.exe for $Arch"
    $windeploy = Join-Path $QtDir "bin\windeployqt.exe"
    Require-File $windeploy "windeployqt"

    # Short stage path — MakeAppx fails with MAX_PATH on deep Qt QML trees under long repos.
    $stageRoot = Join-Path $env:TEMP "svmsix"
    $stage = Join-Path $stageRoot "stage-$Arch"
    if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
    [void](New-Item -ItemType Directory -Path $stage -Force)
    # Keep a junction under packaging\out for inspection.
    $link = Join-Path $OutDir "stage-$Arch"
    if (Test-Path $link) { Remove-Item $link -Recurse -Force -ErrorAction SilentlyContinue }
    [void](New-Item -ItemType Junction -Path $link -Target $stage)

    Copy-Item (Join-Path $BuildDir "StoreVitals.exe") $stage
    # QML module folder next to the exe (URI remains StorePulse in CMake)
    $qmlDir = Join-Path $BuildDir "StorePulse"
    if (Test-Path $qmlDir) {
        Copy-Item $qmlDir (Join-Path $stage "StorePulse") -Recurse -Force
    }

    # IMPORTANT: do not let windeployqt stdout become the function return value
    # (that made MakeAppx see a multi-MB "path" and fail with filename-too-long).
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $windeploy `
            --release `
            --qmldir (Join-Path $Root "src\ui\qml") `
            --compiler-runtime `
            (Join-Path $stage "StoreVitals.exe") *> $null
        $wdExit = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prevEap
    }
    if ($wdExit -ne 0) { throw "windeployqt failed for $Arch (exit $wdExit)" }

    # Manifest + assets (ProcessorArchitecture must match this stage)
    $manifestSrc = Join-Path $PSScriptRoot "Package.appxmanifest"
    Require-File $manifestSrc "Package.appxmanifest"
    $manifestDst = Join-Path $stage "AppxManifest.xml"
    $manifestXml = Get-Content -Raw -Path $manifestSrc
    $archToken = if ($Arch -eq "arm64") { "arm64" } else { "x64" }
    $manifestXml = [regex]::Replace(
        $manifestXml,
        'ProcessorArchitecture="[^"]*"',
        "ProcessorArchitecture=`"$archToken`"")
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($manifestDst, $manifestXml, $utf8NoBom)

    $assetsSrc = Join-Path $PSScriptRoot "Assets"
    Require-File (Join-Path $assetsSrc "StoreLogo.png") "Store assets (run generate_store_assets.py)"
    Copy-Item $assetsSrc (Join-Path $stage "Assets") -Recurse -Force

    # Comma unary forces a single return object even if something else leaked.
    return ,$stage
}

function New-Msix([string]$Stage, [string]$Arch, [string]$MakeAppx) {
    $shortDir = "C:\svpack"
    [void](New-Item -ItemType Directory -Force -Path $shortDir)
    $shortMsix = Join-Path $shortDir "StoreVitals_$Arch.msix"
    if (Test-Path $shortMsix) { Remove-Item $shortMsix -Force }
    if (-not (Test-Path $Stage)) { throw "Stage folder missing: $Stage" }
    Write-Host "makeappx pack /d $Stage /p $shortMsix"
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $MakeAppx pack /d $Stage /p $shortMsix /o *> $null
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prevEap
    }
    if ($code -ne 0) { throw "makeappx pack failed for $Arch (exit $code)" }
    $msix = Join-Path $OutDir "StoreVitals_$Arch.msix"
    Copy-Item $shortMsix $msix -Force
    Write-Host "Created $msix"
    ,$msix
}

Write-Host "Generating Store assets…"
python (Join-Path $PSScriptRoot "generate_store_assets.py")

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$makeappx = Find-MakeAppx
$packages = @()

if ($X64Dir) {
    $stage = Stage-Arch "x64" (Resolve-Path $X64Dir).Path $QtPrefix
    $packages += New-Msix $stage "x64" $makeappx
}
if ($Arm64Dir) {
    $armQt = if ($env:STOREPULSE_QT_ARM64) { $env:STOREPULSE_QT_ARM64 } else { $QtArm64Prefix }
    Write-Host "ARM64 Qt: $armQt"
    $stage = Stage-Arch "arm64" (Resolve-Path $Arm64Dir).Path $armQt
    $packages += New-Msix $stage "arm64" $makeappx
}

if ($packages.Count -eq 0) {
    throw "Pass -X64Dir and/or -Arm64Dir pointing at Release build folders."
}

if (-not $SkipBundle -and $packages.Count -ge 1) {
    $bundleDir = Join-Path $OutDir "bundle-input"
    if (Test-Path $bundleDir) { Remove-Item $bundleDir -Recurse -Force }
    New-Item -ItemType Directory -Path $bundleDir | Out-Null
    foreach ($p in $packages) { Copy-Item $p $bundleDir }

    if ($packages.Count -ge 2) {
        $bundle = Join-Path $OutDir "StoreVitals.msixbundle"
        & $makeappx bundle /d $bundleDir /p $bundle /o
        if ($LASTEXITCODE -ne 0) { throw "makeappx bundle failed" }
        Write-Host "Created $bundle"
        # Partner Center also accepts .msixupload (zip of bundle + symbols).
        $upload = Join-Path $OutDir "StoreVitals.msixupload"
        if (Test-Path $upload) { Remove-Item $upload -Force }
        Compress-Archive -Path $bundle -DestinationPath ($upload + ".zip") -Force
        Move-Item ($upload + ".zip") $upload -Force
        Write-Host "Created $upload"
    } else {
        Write-Host "Single-arch package ready: $($packages[0])"
        Write-Host "Add the other architecture and re-run for .msixbundle / .msixupload."
    }
}

Write-Host @"

Next steps:
  1. Sign with your Store / trusted certificate (signtool) if sideloading.
  2. Upload .msixupload (or .msixbundle) in Partner Center → Packages.
  3. Ensure Release was built with -DDEV_BYPASS_STORE_LICENSE=OFF.
"@
