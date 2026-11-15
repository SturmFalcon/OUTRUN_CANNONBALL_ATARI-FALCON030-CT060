#!/bin/bash
# Rebuilds the immutable reference pjport v0.39-beta (tag v0.39-beta, commit 07b04bd)
# in a separate git worktree, never mixed with the development sources, and checks that
# the binaries are bit-identical to the ones of the release.
# Usage: tools/ct60/build_reference.sh [output_dir]   (default: ../cannonball_ref039)
set -e
REPO=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
OUTDIR=${1:-$REPO/../cannonball_ref039}
export PATH=/opt/cross-mint/bin:$PATH
if [ ! -d "$OUTDIR" ]; then
  git -C "$REPO" worktree add --detach "$OUTDIR" v0.39-beta
fi
cd "$OUTDIR"
test "$(git rev-parse HEAD)" = "07b04bd96848f5869351ae241812a0ef627e2b21" || { echo "not v0.39-beta"; exit 1; }
CPU=68030 EXTRA="-msoft-float -DPLATFORM_FALCON" LINKCPU=68000 OUT=CB030.TOS bash ./build_atari.sh | tail -1
CPU=68060 EXTRA="-msoft-float -DPLATFORM_FALCON" LINKCPU=68000 OUT=CB060.TOS bash ./build_atari.sh | tail -1
md5sum -c - <<'MD5'
9ea0b6869f5e5fa12fb60689a626aa2d  CB030.TOS
4168938aba86a5d6cbce23f1dda30a53  CB060.TOS
MD5
echo "REFERENCE v0.39-beta OK in $OUTDIR"
