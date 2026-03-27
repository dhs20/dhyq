@echo off
setlocal

set "ROOT=%~dp0"
set "EXE=%ROOT%build\windows\x64\release\quantum_atom_simulation.exe"

if not exist "%EXE%" (
    echo [error] Release executable not found:
    echo         %EXE%
    echo [hint]  Run verify_project.bat first, or build with xmake -y.
    exit /b 1
)

cd /d "%ROOT%"
"%EXE%"
exit /b %errorlevel%
