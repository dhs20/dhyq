param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$IncludeTests,
    [switch]$RunHealthCheck = $true,
    [string]$PackageRoot,
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
if (-not $PackageRoot) {
    $PackageRoot = Join-Path $root ("dist/windows/x64/$configDir/quantum_atom_simulation")
}
$PackageRoot = (Resolve-Path $PackageRoot).Path

$requiredFiles = @(
    "quantum_atom_simulation.exe",
    "run_app.bat",
    "README.md",
    "README_BUILD.md",
    "README_PHYSICS.md",
    "assets/data/elements.json",
    "assets/data/reference_catalog.json",
    "assets/data/isotopes.json",
    "assets/data/nist_reference_lines.csv",
    "assets/scenarios/demo_scenes.json",
    "assets/scenarios/demo_script.json",
    "docs/validation.md",
    "docs/performance.md",
    "docs/reports/production-readiness-audit.md",
    "docs/reports/placeholder-cleanup-ledger.md",
    "docs/reports/defect-register.md",
    "docs/reports/data-flow-verification.md",
    "docs/reports/test-report.md",
    "docs/reports/performance-report.md",
    "docs/reports/deployment-verification.md",
    "docs/reports/release-checklist.md"
)
if ($IncludeTests) {
    $requiredFiles += "quantum_atom_tests.exe"
}

$missing = @()
foreach ($relativePath in $requiredFiles) {
    $path = Join-Path $PackageRoot $relativePath
    if (-not (Test-Path $path)) {
        $missing += $relativePath
    }
}
if (-not (Test-Path (Join-Path $PackageRoot "logs"))) {
    $missing += "logs/"
}

if ($missing.Count -gt 0) {
    throw "package verification failed; missing: $($missing -join ', ')"
}

if ($RunHealthCheck) {
    $appExe = Join-Path $PackageRoot "quantum_atom_simulation.exe"
    Push-Location $PackageRoot
    try {
        Write-Host "[package] running packaged health check"
        & $appExe --health-check
        if ($LASTEXITCODE -ne 0) {
            throw "packaged health check failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

Write-Host "[package] verified: $PackageRoot"
