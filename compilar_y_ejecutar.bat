@echo off
setlocal EnableDelayedExpansion
cd /d "%~dp0Codigo"
set "LOG=%~dp0build_log.txt"
echo === BRAWLINK build %date% %time% === > "%LOG%"

rem --- 0. Releer el PATH (por si CMake/g++ se instalaron hace poco) ---
for /f "usebackq delims=" %%p in (`powershell -NoProfile -Command "[Environment]::GetEnvironmentVariable('Path','Machine')+';'+[Environment]::GetEnvironmentVariable('Path','User')"`) do set "PATH=%%p"

rem --- 1. Buscar CMake ---
set "CMAKE="
where cmake >nul 2>&1 && set "CMAKE=cmake"
if not defined CMAKE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if exist "!VSWHERE!" for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -property installationPath`) do (
    if exist "%%i\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE=%%i\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  )
)
if not defined CMAKE (
  echo NO SE ENCONTRO CMAKE >> "%LOG%"
  echo No se encontro CMake. Instala con:  winget install Kitware.CMake
  pause & exit /b 1
)
echo CMake: !CMAKE! >> "%LOG%"

rem --- 2. Elegir compilador: Visual Studio si existe, si no MinGW (g++) ---
set "GEN="
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  where g++ >nul 2>&1 && set "GEN=-G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release"
  if not defined GEN (
    echo NO HAY COMPILADOR C++ >> "%LOG%"
    echo No hay compilador C++. Instala con:  winget install BrechtSanders.WinLibs.POSIX.UCRT
    pause & exit /b 1
  )
)
echo Generador: !GEN! >> "%LOG%"

echo Configurando...
"!CMAKE!" -S . -B build !GEN! >> "%LOG%" 2>&1
if errorlevel 1 ( echo ERROR AL CONFIGURAR >> "%LOG%" & echo Error al configurar. Revisa build_log.txt & pause & exit /b 1 )

echo Compilando (la primera vez descarga raylib y tarda unos minutos)...
"!CMAKE!" --build build --config Release --target BRAWLINK >> "%LOG%" 2>&1
if errorlevel 1 ( echo ERROR AL COMPILAR >> "%LOG%" & echo Error al compilar. Revisa build_log.txt & pause & exit /b 1 )

set "EXE=%~dp0Codigo\build\Release\BRAWLINK.exe"
if not exist "!EXE!" set "EXE=%~dp0Codigo\build\BRAWLINK.exe"
echo OK: !EXE! >> "%LOG%"
echo Ejecutando BRAWLINK...
for %%F in ("!EXE!") do set "EXEDIR=%%~dpF"
start "" /d "!EXEDIR!" "!EXE!"
