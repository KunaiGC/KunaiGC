@echo off

if "%1" equ "" (
	echo please pass file to build.
	pause
	goto :eof
)

srec_cat build/KunaiRecovery_LFS.vgc -bin -offset 0x800 ^
%1 -bin -offset 0x40000 ^
-o KunaiGC_lfs.bin -bin