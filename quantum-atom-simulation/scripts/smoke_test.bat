@echo off
setlocal

set "ROOT=%~dp0.."
powershell -ExecutionPolicy Bypass -File "%~dp0smoke_test.ps1" -Configuration Release -ProjectRoot "%ROOT%"
exit /b %errorlevel%
