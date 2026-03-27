@echo off
setlocal

set "ROOT=%~dp0.."
powershell -ExecutionPolicy Bypass -File "%~dp0run.ps1" -Configuration Release -BuildIfMissing -ProjectRoot "%ROOT%"
exit /b %errorlevel%
