#!/usr/bin/env bash
set -euo pipefail
test -n "$NEXIUM_BIN" || { echo "Missing emulator binary"; exit 2; }
test -n "$NRO_PATH" || { echo "Missing NRO path"; exit 2; }
test -n "$OUTPUT_DIR" || { echo "Missing evidence path"; exit 2; }
mkdir -p "$OUTPUT_DIR"
"$NEXIUM_BIN" "$NRO_PATH" >"$OUTPUT_DIR/nexium.log" 2>&1 &
pid=$!
cleanup() {
  kill -INT "$pid" 2>/dev/null || true
  sleep 1
  kill -KILL "$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true
}
trap cleanup EXIT
window=""
for i in $(seq 1 34); do
  if ! kill -0 "$pid" 2>/dev/null; then
    cat "$OUTPUT_DIR/nexium.log"
    echo "NeXium process exited before graphical window appeared" >&2
    exit 1
  fi
  window="$(xdotool search --onlyvisible --name 'NeXium' 2>/dev/null | tail -n 1 || true)"
  if [[ -n "$window" ]]; then
    sleep 3
    break
  fi
  sleep 1
done
if [[ -z "$window" ]]; then
  cat "$OUTPUT_DIR/nexium.log"
  echo "No NeXium graphical window visible" >&2
  exit 1
fi
timeout 12s import -window "$window" "$OUTPUT_DIR/scene-before.png"
xdotool windowfocus --sync "$window" || true
timeout 3s xdotool keydown --window "$window" Right
sleep 0.8
timeout 3s xdotool keyup --window "$window" Right
sleep 0.4
timeout 12s import -window "$window" "$OUTPUT_DIR/scene-after-right.png"
python3 tests/check_emulator_scene.py \
  "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png"
