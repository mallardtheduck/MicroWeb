@echo off
set PATH=c:\watcom\binnt;%PATH%
set INCLUDE=c:\watcom\h
set WATCOM=c:\watcom
set EDPATH=c:\watcom\eddat
set WIPFC=c:\watcom\wipfc
set LIBDOS=c:\watcom\lib286\dos;c:\watcom\lib286
wmake.exe -h -e
if "%ERRORLEVEL%"=="0" copy ..\..\*.dat
if not "%ERRORLEVEL%"=="0" cmd /c exit 1
