#!/usr/bin/env bash
# Retry only the known transient Winit/Xvfb XOpenDisplay failure.
# All game assertions, guest filesystem errors, missing windows, timeouts
# and other exit statuses remain immediately fatal. Separate invocations
# retain the same user-provided NEXIUM_DATA_ROOT and unique OUTPUT_DIR.
set -euo pipefail
: "${OUTPUT_DIR:?Evidence directory required}"
mkdir -p "$OUTPUT_DIR"
for attempt in 1 2; do
  log="$OUTPUT_DIR/display-attempt-$attempt.log"
  set +e
  timeout --kill-after=5s 110s xvfb-run -a -s '-screen 0 1280x720x24' \
    bash tests/run_nexium_graphical.sh >"$log" 2>&1
  status=$?
  set -e
  cat "$log"
  if [ "$status" -eq 0 ]; then
    exit 0
  fi
  if [ "$attempt" -eq 2 ] || ! grep -Fq 'XOpenDisplayFailed' "$log"; then
    echo "Guest failed with status $status; not retrying game or unrelated emulator errors" >&2
    exit "$status"
  fi
  echo "Transient Xvfb/Winit XOpenDisplay failure; retrying the same independent guest launch exactly once"
  sleep 2
done
exit 1
