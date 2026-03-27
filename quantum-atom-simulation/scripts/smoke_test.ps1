param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$Build,
    [int]$TimeoutSeconds = 5,
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

if ($Build) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build.ps1") -Configuration $Configuration -ProjectRoot $root
    if ($LASTEXITCODE -ne 0) {
        throw "build step failed with exit code $LASTEXITCODE"
    }
}

if (-not (Test-Path $appExe)) {
    throw "application executable not found: $appExe"
}

Write-Host "[smoke] launching: $appExe"
$process = Start-Process -FilePath $appExe -WorkingDirectory $root -PassThru
try {
    Start-Sleep -Seconds $TimeoutSeconds
    if ($process.HasExited) {
        throw "application exited early with code $($process.ExitCode)"
    }
    Write-Host "[smoke] application stayed alive for $TimeoutSeconds second(s)"
}
finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
}
