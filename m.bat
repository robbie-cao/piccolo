::Build script

@echo off
setlocal enabledelayedexpansion

:: Keil path
::Set path to your own
::eg, D:\ProgramFiles\Keil
set KEIL_PATH=C:\Keil
path %PATH%;%KEIL_PATH%\ARM\BIN40\;%KEIL_PATH%\UV4


::Help message
set pr_help=FALSE
if "%1"=="?" set pr_help=TRUE
if "%1"=="h" set pr_help=TRUE
if "%1"=="help" set pr_help=TRUE
if "%pr_help%"=="TRUE" (
        call :PrintHelp
        goto EXIT
        )

::Build command
set command=
if "%1"==""         set command=make
if "%1"=="m"        set command=make
if "%1"=="b"        set command=build
if "%1"=="r"        set command=build
if "%1"=="c"        set command=clean
if "%1"=="make"     set command=make
if "%1"=="build"    set command=build
if "%1"=="rebuild"  set command=build
if "%1"=="clean"    set command=clean

if "%command%"=="" (
        echo Error: Invalid command...
        call :PrintHelp
        goto EXIT
        )

set command2=
if "%1"==""         set command2=b
if "%1"=="m"        set command2=b
if "%1"=="b"        set command2=r
if "%1"=="r"        set command2=r
if "%1"=="c"        set command2=r
if "%1"=="make"     set command2=b
if "%1"=="build"    set command2=r
if "%1"=="rebuild"  set command2=r
if "%1"=="clean"    set command2=r

if "%command2%"=="" (
        echo Error: Invalid command...
        call :PrintHelp
        goto EXIT
        )


::Config to build
set config=
if "%2"==""         set config=Debug
if "%2"=="d"        set config=Debug
if "%2"=="r"        set config=Release
if "%2"=="debug"    set config=Debug
if "%2"=="release"  set config=Release

if "%config%"=="" (
        echo Error: Invalid config...
        call :PrintHelp
        goto EXIT
        )

::Target to build
set target=
if "%3"==""         set target=All
if "%3"=="i"        set target=ISD
if "%3"=="all"      set target=All
if "%3"=="isd"      set target=ISD

if "%target%"=="" (
        echo Error: Invalid target...
        call :PrintHelp
        goto EXIT
        )

set build_isd=FALSE
if "%target%"=="ISD" set build_isd=TRUE
if "%target%"=="All" (
        set build_isd=TRUE
        )

::Remove previous built image
del /Q *.hex
del /Q *.bin
del /Q *.map

if "%build_isd%"=="TRUE" (
        echo ***************************************************************************
        echo Build Magic Audio ISD9160 %config%
        echo ***************************************************************************
        ::Del previous built image if any and copy the new built image out after built
        del /Q Projects\Magic\Aud%config%\Obj\*.hex
        del /Q Projects\Magic\Aud%config%\Exe\*.bin
        del /Q Projects\Magic\Aud%config%\List\*.map

        uv4 -%command2% Projects\Magic\ISD9160.uvproj -t "%config%" -j0 -o build_res_aud.log
        if %ERRORLEVEL% NEQ 0 (
            type Projects\Magic\build_res_aud.log
            goto ERROR
            )
        type Projects\Magic\build_res_aud.log

        copy /Y Projects\Magic\Aud%config%\Obj\*.hex .
        copy /Y Projects\Magic\Aud%config%\Exe\*.bin .
        copy /Y Projects\Magic\Aud%config%\List\*.map .

        echo ***************************************************************************
        echo Build Magic Audio ISD9160 %config% done!
)



:DISP_MEMORY
echo ***************************************************************************
::Display memory size
findstr "memory" *.map
echo ***************************************************************************

echo Done
goto EXIT

:ERROR
echo There are errors. Please check!
goto EXIT

:PrintHelp
setlocal
echo Usage:
echo m.bat [command] [config] [target]
echo * command:
echo     - m / make [default]
echo     - b / build
echo     - r / rebuild
echo     - c / clean
echo * config:
echo     - d / debug [default]
echo     - r / release
echo * target:
echo     - all [default]
echo     - i / isd
endlocal

:EXIT

