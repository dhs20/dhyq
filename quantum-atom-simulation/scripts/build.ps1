param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$Clean,
    [switch]$Reconfigure,
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

if (-not (Get-Command xmake -ErrorAction SilentlyContinue)) {
    throw "xmake was not found in PATH"
}

Write-Host "[build] project root: $root"
Write-Host "[build] configuration: $Configuration"
Write-Host "[build] xmake global dir: $env:XMAKE_GLOBALDIR"
Write-Host "[build] xmake package cache: $env:XMAKE_PKG_CACHEDIR"
Write-Host "[build] xmake package install: $env:XMAKE_PKG_INSTALLDIR"

Push-Location $root
try {
    if ($Clean) {
        Write-Host "[build] cleaning xmake state"
        & xmake f -c -y
        if ($LASTEXITCODE -ne 0) {
            throw "xmake clean failed with exit code $LASTEXITCODE"
        }
    }

    Write-Host "[build] configuring"
    & xmake f -m $Configuration.ToLowerInvariant() -y
    if ($LASTEXITCODE -ne 0) {
        throw "xmake configure failed with exit code $LASTEXITCODE"
    }

    Write-Host "[build] compiling"
    & xmake -y
    if ($LASTEXITCODE -ne 0) {
        throw "xmake build failed with exit code $LASTEXITCODE"
    }

    $outputDir = Join-Path $root ("build/windows/x64/" + $Configuration.ToLowerInvariant())
    Write-Host "[build] completed: $outputDir"
}
finally {
    Pop-Location
}
