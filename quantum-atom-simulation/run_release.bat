@echo off
setlocal

set "ROOT=%~dp0"
powershell -ExecutionPolicy Bypass -File "%ROOT%scripts\run.ps1" -Configuration Release -BuildIfMissing -ProjectRoot "%ROOT%"
exit /b %errorlevel%
