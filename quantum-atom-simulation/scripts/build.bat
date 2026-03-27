@echo off
setlocal

set "ROOT=%~dp0.."
powershell -ExecutionPolicy Bypass -File "%~dp0build.ps1" -Configuration Release -ProjectRoot "%ROOT%"
exit /b %errorlevel%
