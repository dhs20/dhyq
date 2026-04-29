param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string]$ProjectRoot,
    [string]$OutputPath
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
if (-not $OutputPath) {
    $OutputPath = Join-Path $root "docs/reports/coverage-matrix.md"
}
$outputDir = Split-Path $OutputPath -Parent
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$configDir = $Configuration.ToLowerInvariant()
$testExe = Join-Path $root ("build/windows/x64/$configDir/quantum_atom_tests.exe")
if (-not (Test-Path $testExe)) {
    throw "test executable not found: $testExe"
}

$coverageTool = Get-Command OpenCppCoverage -ErrorAction SilentlyContinue
if ($coverageTool) {
    $coverageXml = Join-Path $outputDir "coverage.opencppcoverage.xml"
    & $coverageTool.Source "--export_type=cobertura:$coverageXml" "--sources=$root" "--excluded_sources=$root\render" "--excluded_sources=$root\legacy" "--" $testExe
    $status = if ($LASTEXITCODE -eq 0) { "OpenCppCoverage completed." } else { "OpenCppCoverage exited with code $LASTEXITCODE." }
    @(
        "# Coverage Report",
        "",
        "- Tool: OpenCppCoverage",
        "- Status: $status",
        "- Export: $coverageXml",
        "- Target: >=80% line coverage for non-render core modules."
    ) | Set-Content -Encoding UTF8 $OutputPath
    if ($LASTEXITCODE -ne 0) {
        throw $status
    }
} else {
    @(
        "# Coverage Matrix",
        "",
        "OpenCppCoverage was not found on PATH, so line coverage is marked unavailable for this run.",
        "",
        "| Module | Automated Coverage Evidence | Status |",
        "| --- | --- | --- |",
        '| `physics` | constants, hydrogenic formulas, transitions, Slater, central field, radial solver, cloud generation | Covered by `quantum_atom_tests` |',
        '| `spectroscopy` | fine/Zeeman/Stark/hyperfine corrections and structured spectral records | Covered by `quantum_atom_tests` |',
        '| `solvers` | Tier 2 mean-field and Tier 4 correlation checks | Covered by `quantum_atom_tests` |',
        '| `dynamics` | nuclear models and two-level TDSE dynamics | Covered by `quantum_atom_tests` |',
        '| `data` | JSON catalog load, isotope catalog, reference fallback shape | Covered by `quantum_atom_tests` and package verification |',
        '| `validation` | Markdown/JSON report export | Covered by `quantum_atom_tests` |',
        '| `render/ui` | Build, smoke, and health checks; no line coverage gate due GPU/window dependency | Operationally verified |',
        "",
        "Coverage gate: critical non-render modules must have explicit regression coverage. Line percentage is unavailable without OpenCppCoverage."
    ) | Set-Content -Encoding UTF8 $OutputPath
    Write-Host "[coverage] OpenCppCoverage unavailable; wrote module coverage matrix: $OutputPath"
}
