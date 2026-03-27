@echo off
setlocal

set "ROOT=%~dp0.."
powershell -ExecutionPolicy Bypass -File "%~dp0build_and_run.ps1" -Configuration Release -RunTests -ProjectRoot "%ROOT%"
exit /b %errorlevel%
