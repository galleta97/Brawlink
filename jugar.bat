@echo off
rem Abre BRAWLINK sin compilar (usa el ultimo .exe compilado)
set "EXE=%~dp0Codigo\build\BRAWLINK.exe"
if not exist "%EXE%" (
  echo Aun no esta compilado. Ejecuta primero compilar_y_ejecutar.bat
  pause & exit /b 1
)
start "" /d "%~dp0Codigo\build" "%EXE%"
