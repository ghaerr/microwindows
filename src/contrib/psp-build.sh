#!/bin/bash
export BIN=mine
cd bin
psp-fixup-imports $BIN
mksfo 'nano demo' PARAM.SFO
psp-strip $BIN -o $BIN.elf
pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL $BIN.elf NULL
cp EBOOT.PBP ../
cp PARAM.SFO ../
cd ..
