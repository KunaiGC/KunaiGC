@echo off
for %%I in (build/KunaiLoader.dol) do set size=%%~zI


srec_cat ../KunaiRecovery/build/KunaiRecovery.vgc -bin -offset 0x800 ^
-generate 0x23000 0x23004 -constant-b-e %size% 4 ^
( build/KunaiLoader.dol -bin -offset 0x24000 ^
-crc16-b-e 0x23004 ) ^
-o KunaiGC.bin -bin