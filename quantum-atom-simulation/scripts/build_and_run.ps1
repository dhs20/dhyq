param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$RunTests,
    [switch]$Clean,
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"

$root = if ($ProjectRoot) { (Resolve-Path $ProjectRoot).Path } else { (Resolve-Path (Join-Path $PSScriptRoot "..")).Path }

$buildArgs = @(
    "-ExecutionPolicy", "Bypass",
    "-File", (Join-Path $PSScriptRoot "build.ps1"),
    "-Configuration", $Configuration,
    "-ProjectRoot", $root
)
if ($Clean) {
    $buildArgs += "-Clean"
}

& powershell @buildArgs
if ($LASTEXITCODE -ne 0) {
    throw "build step failed with exit code $LASTEXITCODE"
}

$runArgs = @(
    "-ExecutionPolicy", "Bypass",
    "-File", (Join-Path $PSScriptRoot "run.ps1"),
    "-Configuration", $Configuration,
    "-ProjectRoot", $root
)
if ($RunTests) {
    $runArgs += "-RunTests"
}

& powershell @runArgs
exit $LASTEXITCODE
