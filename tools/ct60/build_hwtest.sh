#!/bin/bash
# Builds HWTEST.TOS (hardware detection alone), 68000 code so it runs on every machine.
set -e
cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
export PATH=/opt/cross-mint/bin:$PATH
m68k-atari-mint-g++ -O2 -mcpu=68000 -Isrc/main -DPLATFORM_ATARI -c src/main/atari/hwdetect.cpp -o /tmp/hwdetect.o
m68k-atari-mint-gcc -mcpu=68000 -c src/main/atari/hwdetect_asm.S -o /tmp/hwdetect_asm.o
m68k-atari-mint-g++ -O2 -mcpu=68000 -Isrc/main -DPLATFORM_ATARI tools/ct60/hwtest.cpp /tmp/hwdetect.o /tmp/hwdetect_asm.o -o HWTEST.TOS
echo BUILD_OK HWTEST.TOS
