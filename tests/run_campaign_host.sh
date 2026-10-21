#!/bin/sh
# Portable C99 campaign-wide integration test for Linux and Intel macOS.
# Does not need devkitPro, Nintendo SDK, a ROM, or any approved visual assets.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
TMP_ROOT=${TMPDIR:-/tmp}
OUT=$(mktemp -d "$TMP_ROOT/rikitu-host-qa.XXXXXXXX") || exit 1
trap 'rm -rf "$OUT"' EXIT HUP INT TERM
SOURCES="native/rikitu_rules.c native/rikitu_loop.c native/rikitu_surfaces.c native/rikitu_stage.c native/rikitu_draw.c native/rikitu_demo.c native/rikitu_enemies.c native/rikitu_boss.c native/rikitu_flower.c native/rikitu_mill.c native/rikitu_grove.c native/rikitu_bridge.c native/rikitu_workshop.c native/rikitu_delivery.c native/rikitu_clock.c native/rikitu_orders.c native/rikitu_mist.c native/rikitu_cloud.c native/rikitu_crystal.c native/rikitu_mine.c native/rikitu_gardens.c native/rikitu_foundry.c native/rikitu_gallery.c native/rikitu_heart.c native/test_campaign_scene.c"
for cc in gcc clang; do
  if ! command -v "$cc" >/dev/null 2>&1; then
    echo "ERROR: $cc not on PATH" >&2
    exit 2
  fi
  echo "RIKITU 16-map C99 host QA: compiling with $cc"
  # SOURCES is a fixed trusted list, deliberately word-split into filenames.
  # shellcheck disable=SC2086
  "$cc" -std=c99 -O1 -Wall -Wextra -Werror -pedantic -Inative $SOURCES -lm -o "$OUT/campaign-$cc"
  "$OUT/campaign-$cc"
done
if command -v node >/dev/null 2>&1; then
  echo "RIKITU MAP16 browser coordinate parity"
  node tests/test-heart-source.cjs
else
  echo "ERROR: node not on PATH (browser coordinate parity not checked)" >&2
  exit 2
fi
echo "PASS: both C99 compilers, all 16 scene transitions and source parity"
