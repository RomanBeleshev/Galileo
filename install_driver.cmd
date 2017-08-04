@echo off

if "%1"=="-u" goto :uninstall

if defined ProgramFiles(x86) (set FCTARCH=x64) else set FCTARCH=x86
ver | findstr /i /l " 6.1." 
if %ERRORLEVEL% EQU 0 goto win61
ver | findstr /i /l " 6.2." > nul
if %ERRORLEVEL% EQU 0 goto win62
ver | findstr /i /l " 6.3." > nul
if %ERRORLEVEL% EQU 0 goto win63
ver | findstr /i /l " 10.0." > nul
if %ERRORLEVEL% EQU 0 goto win10
echo The current windows version is not supported.
exit /b
:win61
set FCTVER=win6.1
goto install
:win62
set FCTVER=win6.2
goto install
:win63
set FCTVER=win6.3
goto install
:win10
rem use win8.1 driver for win10
set FCTVER=win10
goto install
:install
call :uninstall

copy /y "%~dp0%FCTARCH%\%FCTVER%\changetracker.sys" %SystemRoot%\system32\drivers
if errorlevel 1 goto :exit
sc create ChangeTracker binPath= \SystemRoot\System32\drivers\ChangeTracker.sys type= filesys start= demand error= normal group= "FSFilter Activity Monitor" tag= yes depend= FltMgr
if errorlevel 1 goto :exit
reg import "%~dp0\ChangeTracker.reg"
goto :exit

:uninstall
sc stop changetracker
sc delete changetracker
del /f %SystemRoot%\system32\drivers\changetracker.sys
exit /b

:exit