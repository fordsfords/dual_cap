rem tst.bat

call bld.bat

del /Q capdir1\*.*
del /Q capdir2\*.*
del /Q *.log

start mdump -o mdump.log 224.9.9.9 12000 127.0.0.1

timeout /t 1 /nobreak >nul

start msend 224.9.9.9 12000 15 127.0.0.1 >msend.log

start dual_cap listener.cfg.win
timeout /t 1 /nobreak >nul
start dual_cap initiator.cfg.win

timeout /t 13 /nobreak >nul

taskkill /IM mdump.exe /F

taskkill /IM msend.exe /F

rem The dual caps should have exited.

