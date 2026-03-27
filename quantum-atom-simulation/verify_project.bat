@echo off
setlocal

set "ROOT=%~dp0"
set "TEST_EXE=%ROOT%build\windows\x64\release\quantum_atom_tests.exe"
set "APP_EXE=%ROOT%build\windows\x64\release\quantum_atom_simulation.exe"

cd /d "%ROOT%"

if not exist "%TEST_EXE%" (
    echo [error] Test executable not found:
    echo         %TEST_EXE%
    echo [hint]  Build the project first with xmake -y.
    exit /b 1
)

if not exist "%APP_EXE%" (
    echo [error] Application executable not found:
    echo         %APP_EXE%
    echo [hint]  Build the project first with xmake -y.
    exit /b 1
)

echo [1/2] Running tests...
"%TEST_EXE%"
if errorlevel 1 (
    echo [error] Tests failed.
    exit /b 1
)

echo [2/2] Launching application...
"%APP_EXE%"
exit /b %errorlevel%
