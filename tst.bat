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

(
  echo listen_port=9877
  echo mon_file=msend.log
  echo mon_pattern=ERROR
  echo cap_cmd=tshark -i \Device\NPF_Loopback -w capdir1\cap.pcapng -b filesize:100 -b files:3 -q
  echo cap_linger_ms=500
) > listener.cfg

(
  echo init_ip=127.0.0.1
  echo init_port=9877
  echo mon_file=mdump.log
  echo mon_pattern=Message a\s*$
  echo cap_cmd=tshark -i \Device\NPF_Loopback -w capdir2\cap.pcapng -b filesize:100 -b files:3 -q
  echo cap_linger_ms=500
) > initiator.cfg

start mdump -o mdump.log 224.9.9.9 12000 127.0.0.1

timeout /t 1 /nobreak >nul

start msend 224.9.9.9 12000 15 127.0.0.1 >msend.log

start dual_cap listener.cfg
timeout /t 1 /nobreak >nul
start dual_cap initiator.cfg

timeout /t 13 /nobreak >nul

taskkill /IM mdump.exe /F

taskkill /IM msend.exe /F

rem The dual caps should have exited.
