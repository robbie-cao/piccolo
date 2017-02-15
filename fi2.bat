::Flash program script
::Flash ISD9160 with NuLink command tool

@echo off
setlocal enabledelayedexpansion

:: NuLink path
::Set path to your own
::eg, D:\ProgramFiles\Keil
set NULINK_PATH=C:\Program Files (x86)\Nuvoton Tools\NuLink Command Tool
path %PATH%;%NULINK_PATH%

echo ***************************************************************************
echo Flash image to Nuvoton ISD9160
echo ***************************************************************************

echo Erasing...
NuLink -e APROM

echo Programming...
NuLink -w APROM Magic.hex

echo Verifying...
NuLink -v APROM Magic.hex

@echo ...
@echo Flash done...

NuLink -reset


:EXIT

