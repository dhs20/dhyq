param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$Build,
    [switch]$IncludeTests,
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"

function Resolve-ProjectRoot {
    param([string]$RootOverride)
    if ($RootOverride) {
        return (Resolve-Path $RootOverride).Path
    }
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

$root = Resolve-ProjectRoot -RootOverride $ProjectRoot
$configDir = $Configuration.ToLowerInvariant()

if ($Build) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build.ps1") -Configuration $Configuration -ProjectRoot $root
    if ($LASTEXITCODE -ne 0) {
        throw "build step failed with exit code $LASTEXITCODE"
    }
}

$appExe = Join-Path $root ("build/windows/x64/$configDir/quantum_atom_simulation.exe")
$testExe = Join-Path $root ("build/windows/x64/$configDir/quantum_atom_tests.exe")
if (-not (Test-Path $appExe)) {
    throw "application executable not found: $appExe"
}

$packageRoot = Join-Path $root ("dist/windows/x64/$configDir/quantum_atom_simulation")
if (Test-Path $packageRoot) {
    Remove-Item -Recurse -Force $packageRoot
}
New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null

$directoriesToCopy = @(
    "assets",
    "docs"
)
foreach ($dir in $directoriesToCopy) {
    Copy-Item -Recurse -Force (Join-Path $root $dir) (Join-Path $packageRoot $dir)
}

$filesToCopy = @(
    "README.md",
    "README_BUILD.md",
    "README_PHYSICS.md"
)
foreach ($file in $filesToCopy) {
    Copy-Item -Force (Join-Path $root $file) (Join-Path $packageRoot $file)
}

Copy-Item -Force $appExe (Join-Path $packageRoot "quantum_atom_simulation.exe")
if ($IncludeTests -and (Test-Path $testExe)) {
    Copy-Item -Force $testExe (Join-Path $packageRoot "quantum_atom_tests.exe")
}

$launcher = @"
@echo off
setlocal
cd /d "%~dp0"
"%~dp0quantum_atom_simulation.exe"
exit /b %errorlevel%
"@
$launcherPath = Join-Path $packageRoot "run_app.bat"
[System.IO.File]::WriteAllText($launcherPath, $launcher, [System.Text.Encoding]::ASCII)

New-Item -ItemType Directory -Force -Path (Join-Path $packageRoot "logs") | Out-Null

Write-Host "[package] created: $packageRoot"
