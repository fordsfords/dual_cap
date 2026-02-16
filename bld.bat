@echo off
rem bld.bat

cl /std:c11 /W4 /O2 /MT /nologo /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE dual_cap.c re.c plat_win.c ws2_32.lib /Fe:dual_cap.exe
exit /b %ERRORLEVEL%
