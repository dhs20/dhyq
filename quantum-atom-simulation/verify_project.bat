@echo off
setlocal

set "ROOT=%~dp0"
powershell -ExecutionPolicy Bypass -File "%ROOT%scripts\build.ps1" -Configuration Release -ProjectRoot "%ROOT%"
if errorlevel 1 exit /b %errorlevel%
powershell -ExecutionPolicy Bypass -Command "& '%ROOT%build\windows\x64\release\quantum_atom_tests.exe'"
if errorlevel 1 exit /b %errorlevel%
powershell -ExecutionPolicy Bypass -File "%ROOT%scripts\smoke_test.ps1" -Configuration Release -ProjectRoot "%ROOT%"
exit /b %errorlevel%
