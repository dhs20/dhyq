@echo off
setlocal

set "ROOT=%~dp0.."
powershell -ExecutionPolicy Bypass -File "%~dp0package.ps1" -Configuration Release -Build -IncludeTests -ProjectRoot "%ROOT%"
exit /b %errorlevel%
