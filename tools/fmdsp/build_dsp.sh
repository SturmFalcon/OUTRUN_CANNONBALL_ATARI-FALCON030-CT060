#!/bin/bash
# Assembles fm_dsp.asm with a56 (Debian/Ubuntu package "a56") and regenerates
# src/main/atari/fm_dsp_p56.h. Stops on any a56 error or warning (pipeline hazards included).
set -e
cd "$(dirname "$0")"
OUT=$(a56 -o /tmp/fm_dsp.lod fm_dsp.asm 2>&1)
echo "$OUT" | grep -E "^errors=|^warnings="
if ! echo "$OUT" | grep -q "^errors=0" || ! echo "$OUT" | grep -q "^warnings=0"; then
  echo "$OUT" | grep -B1 -A1 -E "line [0-9]+:"; exit 1
fi
python3 lod2h.py /tmp/fm_dsp.lod ../../src/main/atari/fm_dsp_p56.h fm_dsp_p56
# [EXPÉRIMENTAL] -DSND24 variant: same program, second output buffer moved to X:$2800 so each
# holds 1024 frames (24585 Hz: 820 frames per game step). Memory still below $3DFF.
sed 's/^BUF1\tequ\t\$2400/BUF1\tequ\t$2800/' fm_dsp.asm > /tmp/fm_dsp24.asm
OUT=$(a56 -o /tmp/fm_dsp24.lod /tmp/fm_dsp24.asm 2>&1)
echo "$OUT" | grep -q "^errors=0" || { echo "$OUT"; exit 1; }
python3 lod2h.py /tmp/fm_dsp24.lod ../../src/main/atari/fm_dsp24_p56.h fm_dsp_p56
