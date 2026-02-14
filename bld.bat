rem bld.bat

cl /std:c11 /W4 /O2 /MT /nologo dual_cap.c re.c plat_win.c ws2_32.lib /Fe:dual_cap.exe
