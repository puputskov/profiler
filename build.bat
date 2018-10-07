@echo off
WHERE cl.exe
IF %ERRORLEVEL% NEQ 0 goto setup_devenv
goto skip_setup

:setup_devenv
set VSCMD_START_DIR=%CD%
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
IF %ERRORLEVEL% NEQ 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:skip_setup
set PROJECT_NAME=profiler
set DEFINES=/D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN
set LIB_DIRS=
set LIBS=ws2_32.lib
set INCLUDE_DIRS=/Iinclude
set FORCE_INCLUDE=


cl /c /Zi source\profiler\_BUILD_.c %DEFINES% %INCLUDE_DIRS% %FORCE_INCLUDE% /W3 /MT
lib /OUT:builds\%PROJECT_NAME%.lib _BUILD_.obj %LIBS%


cl /Zi source\tool\_BUILD_.c %DEFINES% %INCLUDE_DIRS% %FORCE_INCLUDE% kernel32.lib user32.lib gdi32.lib builds\profiler.lib opengl32.lib /Febuilds\%PROJECT_NAME% /W3 /MT 
cl /Zi source\test\_BUILD_.c %DEFINES% %INCLUDE_DIRS% %FORCE_INCLUDE% kernel32.lib user32.lib gdi32.lib builds\profiler.lib /Febuilds\test.exe /W3 /MT 
@echo on

