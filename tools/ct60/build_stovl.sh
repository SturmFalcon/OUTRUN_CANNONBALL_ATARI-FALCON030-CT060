#!/bin/bash
# Builds STOVL.TOS (ST write overlap micro-benchmark), 68060 code: run on a CT60 only.
set -e
cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
export PATH=/opt/cross-mint/bin:$PATH
m68k-atari-mint-gcc -m68060 -c src/main/atari/stpump_asm.S -o /tmp/stpump_asm.o
m68k-atari-mint-gcc -m68060 -c src/main/atari/truecolor_asm.S -o /tmp/truecolor_asm.o
m68k-atari-mint-g++ -O2 -m68060 -msoft-float -Isrc/main tools/ct60/stovl.cpp /tmp/stpump_asm.o /tmp/truecolor_asm.o -o STOVL.TOS
echo BUILD_OK STOVL.TOS
