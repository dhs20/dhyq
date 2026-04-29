param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$BuildIfMissing,
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
$appExe = Join-Path $root ("build/windows/x64/$configDir/quantum_atom_simulation.exe")

if ($BuildIfMissing -and -not (Test-Path $appExe)) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build.ps1") -Configuration $Configuration -ProjectRoot $root
    if ($LASTEXITCODE -ne 0) {
        throw "build step failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path $appExe)) {
    throw "application executable not found: $appExe"
}

Write-Host "[health] executing data/resource health check"
& $appExe --health-check
if ($LASTEXITCODE -ne 0) {
    throw "health check failed with exit code $LASTEXITCODE"
}
