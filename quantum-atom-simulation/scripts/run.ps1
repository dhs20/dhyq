param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$RunTests,
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
$testExe = Join-Path $root ("build/windows/x64/$configDir/quantum_atom_tests.exe")

if ($BuildIfMissing -and -not (Test-Path $appExe)) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build.ps1") -Configuration $Configuration -ProjectRoot $root
    if ($LASTEXITCODE -ne 0) {
        throw "build step failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path $appExe)) {
    throw "application executable not found: $appExe"
}

$exitCode = 0
Push-Location $root
try {
    if ($RunTests) {
        if (-not (Test-Path $testExe)) {
            throw "test executable not found: $testExe"
        }
        Write-Host "[run] executing tests"
        & $testExe
        if ($LASTEXITCODE -ne 0) {
            throw "tests failed with exit code $LASTEXITCODE"
        }
    }

    Write-Host "[run] launching application: $appExe"
    & $appExe
    $exitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}

exit $exitCode
