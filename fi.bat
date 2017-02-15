::Flash program script

@echo off
setlocal enabledelayedexpansion

:: Keil path
::Set path to your own
::eg, D:\ProgramFiles\Keil
set KEIL_PATH=C:\Keil
path %PATH%;%KEIL_PATH%\ARM\BIN40\;%KEIL_PATH%\UV4

echo ***************************************************************************
echo Flash image to Nuvoton ISD9160
echo ***************************************************************************
uv4 -f Projects\Magic\ISD9160.uvproj
@echo ...
@echo Flash done...


:EXIT

