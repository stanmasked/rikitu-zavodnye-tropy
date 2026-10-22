#!/usr/bin/env bash
set -euo pipefail
test -n "$NEXIUM_BIN" || { echo "Missing emulator binary"; exit 2; }
test -n "$NRO_PATH" || { echo "Missing NRO path"; exit 2; }
test -n "$OUTPUT_DIR" || { echo "Missing evidence path"; exit 2; }
if [[ "${RIKITU_QA_MODE:-0}" == "3" || "${RIKITU_QA_MODE:-0}" == "4" || "${RIKITU_QA_MODE:-0}" == "5" || "${RIKITU_QA_MODE:-0}" == "6" || "${RIKITU_QA_MODE:-0}" == "7" ]]; then
  test -n "${NEXIUM_DATA_ROOT:-}" || { echo "Missing persistent NeXium SD root"; exit 2; }
  mkdir -p "$NEXIUM_DATA_ROOT/sdmc"
fi
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
if [[ "${RIKITU_QA_WRAP:-0}" == "1" ]]; then
  # Stage99 performs its own generation/CRC audit, since the ordinary
  # save QA checkers intentionally expect only generations 1 and 2.
  [[ "${RIKITU_QA_MODE:-0}" == "3" || "${RIKITU_QA_MODE:-0}" == "4" ]] || exit 2
  python3 tests/check_wrap_guest_slots.py frame "${RIKITU_QA_PHASE:?phase required}" \
    "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png"
elif [[ "${RIKITU_QA_MODE:-0}" == "7" ]]; then
  python3 tests/check_staged_faults_guest.py "${RIKITU_QA_PHASE:?phase required}" \
    "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png" \
    "${NEXIUM_DATA_ROOT:?root required}"
elif [[ "${RIKITU_QA_MODE:-0}" == "6" ]]; then
  python3 tests/check_staged_codecs_guest.py "${RIKITU_QA_PHASE:?phase required}" \
    "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png" \
    "${NEXIUM_DATA_ROOT:?root required}"
elif [[ "${RIKITU_QA_MODE:-0}" == "5" ]]; then
  python3 tests/check_staged_rename_guest.py "${RIKITU_QA_PHASE:?phase required}" \
    "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png" \
    "${NEXIUM_DATA_ROOT:?root required}"
elif [[ "${RIKITU_QA_MODE:-0}" == "4" ]]; then
  python3 tests/check_campaign_guest.py "${RIKITU_QA_PHASE:?phase required}" \
    "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png" \
    "${NEXIUM_DATA_ROOT:?root required}"
elif [[ "${RIKITU_QA_MODE:-0}" == "3" ]]; then
  python3 tests/check_save_guest.py "${RIKITU_QA_PHASE:?phase required}" \
    "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png" \
    "${NEXIUM_DATA_ROOT:?root required}"
elif [[ "${RIKITU_QA_MODE:-0}" == "1" ]]; then
  python3 tests/check_combat_guest.py \
    "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png"
else
  python3 tests/check_emulator_scene.py \
  "$OUTPUT_DIR/scene-before.png" "$OUTPUT_DIR/scene-after-right.png"
fi

# Explicit opt-in: explore the real graphical NRO's Nintendo Y stage selector.
# NeXium's default keyboard config maps Nintendo Y to X11 key Z (not Y).
# Source: nexium-emu/nexium/nexium-gui/src/controller_config.rs
# The normal smoke test above remains unchanged; failed map switches are
# reported with the screenshots, not treated as a successful host simulation.
if [[ "${RIKITU_MAP_CYCLE_QA:-0}" == "1" && "${RIKITU_QA_MODE:-0}" != "1" ]]; then
  out="$OUTPUT_DIR/map-cycle"
  mkdir -p "$out"
  cp "$OUTPUT_DIR/scene-before.png" "$out/map-01.png"
  for stage in $(seq 2 16); do
    printf -v id "%02d" "$stage"
    xdotool windowfocus --sync "$window" || true
    timeout 3s xdotool keydown --window "$window" z
    sleep 0.16
    timeout 3s xdotool keyup --window "$window" z
    sleep 0.42
    timeout 12s import -window "$window" "$out/map-$id.png"
    echo "NeXium guest Y map cycle: captured chapter $stage"
  done
  timeout 3s xdotool keydown --window "$window" z
  sleep 0.16
  timeout 3s xdotool keyup --window "$window" z
  sleep 0.42
  timeout 12s import -window "$window" "$out/map-wrap-01.png"
  python3 tests/check_emulator_map_cycle.py "$out"
fi
