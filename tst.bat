@echo off
rem tst.bat

call bld.bat
if ERRORLEVEL 1 (
  echo ERROR: build failed
  exit /b 1
)

del /Q capdir1\*.* 2>nul
del /Q capdir2\*.* 2>nul
del /Q *.log 2>nul

start mdump -o mdump.log 224.9.9.9 12000 127.0.0.1

timeout /t 1 /nobreak >nul

start msend 224.9.9.9 12000 15 127.0.0.1 >msend.log

start dual_cap listener_cfg.win
timeout /t 1 /nobreak >nul
start dual_cap initiator_cfg.win

timeout /t 13 /nobreak >nul

taskkill /IM mdump.exe /F

taskkill /IM msend.exe /F

rem The dual caps should have exited.
