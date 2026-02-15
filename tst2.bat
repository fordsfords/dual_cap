rem tst2.bat

rem start msend/mdump


start mdump -o mdump.log 224.9.9.9 12000 127.0.0.1

timeout /t 1 /nobreak >nul

start msend 224.9.9.9 12000 15 127.0.0.1 >msend.log

start dual_cap listener.cfg.tst2
timeout /t 1 /nobreak >nul
start dual_cap initiator.cfg.tst2

timeout /t 10 /nobreak >nul

taskkill /IM mdump.exe /F

taskkill /IM msend.exe /F

rem The dual caps should have exited.
