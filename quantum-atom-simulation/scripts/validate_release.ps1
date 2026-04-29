param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
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

function Initialize-XmakeEnvironment {
    param([string]$Root)

    if ($env:XMAKE_GLOBALDIR -and $env:XMAKE_PKG_CACHEDIR -and $env:XMAKE_PKG_INSTALLDIR) {
        return
    }

    $parentRoot = Split-Path $Root -Parent
    $sharedGlobal = Join-Path $parentRoot ".xmake-global"
    $sharedPkgCache = Join-Path $parentRoot ".xmake-pkg-cache"
    $sharedPkgInstall = Join-Path $parentRoot ".xmake-pkg-install"
    if ((Test-Path $sharedGlobal) -and (Test-Path $sharedPkgCache) -and (Test-Path $sharedPkgInstall)) {
        $env:XMAKE_GLOBALDIR = $sharedGlobal
        $env:XMAKE_PKG_CACHEDIR = $sharedPkgCache
        $env:XMAKE_PKG_INSTALLDIR = $sharedPkgInstall
        return
    }

    $cacheRoot = Join-Path $Root ".cache"
    $env:XMAKE_GLOBALDIR = Join-Path $cacheRoot "xmake-global"
    $env:XMAKE_PKG_CACHEDIR = Join-Path $cacheRoot "xmake-pkg-cache"
    $env:XMAKE_PKG_INSTALLDIR = Join-Path $cacheRoot "xmake-pkg-install"
    New-Item -ItemType Directory -Force -Path $env:XMAKE_GLOBALDIR, $env:XMAKE_PKG_CACHEDIR, $env:XMAKE_PKG_INSTALLDIR | Out-Null
}

$root = Resolve-ProjectRoot -RootOverride $ProjectRoot
Initialize-XmakeEnvironment -Root $root
$reportsDir = Join-Path $root "docs/reports"
New-Item -ItemType Directory -Force -Path $reportsDir | Out-Null

$startedAt = Get-Date
$steps = New-Object System.Collections.Generic.List[string]

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Action
    )
    Write-Host "[validate] $Name"
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & $Action
    if ($LASTEXITCODE -ne 0) {
        throw "$Name failed with exit code $LASTEXITCODE"
    }
    $sw.Stop()
    $steps.Add("| $Name | Passed | $([math]::Round($sw.Elapsed.TotalSeconds, 2)) s |")
}

Push-Location $root
try {
    Invoke-Step "Configure release build" { powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build.ps1") -Configuration $Configuration -ProjectRoot $root }
    Invoke-Step "Build tests" { xmake build quantum_atom_tests }
    Invoke-Step "Run tests" { xmake run quantum_atom_tests }
    Invoke-Step "Build application" { xmake build quantum_atom_simulation }
    Invoke-Step "Health check" { powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "health_check.ps1") -Configuration $Configuration -ProjectRoot $root }
    Invoke-Step "Smoke test" { powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "smoke_test.ps1") -Configuration $Configuration -ProjectRoot $root }
    Invoke-Step "Package and verify" { powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "package.ps1") -Configuration $Configuration -ProjectRoot $root -Build -IncludeTests -Verify }
    Invoke-Step "Coverage report" { powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "coverage_report.ps1") -Configuration $Configuration -ProjectRoot $root }
}
finally {
    Pop-Location
}

$completedAt = Get-Date
$packagePath = Join-Path $root ("dist/windows/x64/" + $Configuration.ToLowerInvariant() + "/quantum_atom_simulation")
$summaryPath = Join-Path $reportsDir "release-validation-summary.md"
@(
    "# Release Validation Summary",
    "",
    "- Started: $($startedAt.ToString('s'))",
    "- Completed: $($completedAt.ToString('s'))",
    "- Configuration: $Configuration",
    "- Package: $packagePath",
    "",
    "| Step | Result | Duration |",
    "| --- | --- | --- |"
) + $steps + @(
    "",
    "Result: Windows x64 release validation passed for the teaching/scientific desktop deployment target."
) | Set-Content -Encoding UTF8 $summaryPath

if (Test-Path $packagePath) {
    $packageReportsDir = Join-Path $packagePath "docs/reports"
    New-Item -ItemType Directory -Force -Path $packageReportsDir | Out-Null
    Copy-Item -Force $summaryPath (Join-Path $packageReportsDir "release-validation-summary.md")
}

Write-Host "[validate] release validation complete: $summaryPath"
