#!/usr/bin/env bash
# Automated end-to-end test: starts a sandboxed headless KWin with the plugin,
# injects fake pointer input, and asserts on the plugin's decisions via its log.
# Touches neither the running session nor the user's config.
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR=${BUILD_DIR:-build}
SOCKET=wayland-easecursor-test
GEN=tests/gen

log() { printf '\033[1m== %s\033[0m\n' "$*"; }
pass=0
fail=0
check() { # check <description> <pass-condition (0/1)>
    if [[ $2 -eq 0 ]]; then
        printf '\033[32mPASS\033[0m %s\n' "$1"
        pass=$((pass + 1))
    else
        printf '\033[31mFAIL\033[0m %s\n' "$1"
        fail=$((fail + 1))
    fi
}

log "building plugin"
if [[ ! -d $BUILD_DIR ]]; then
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi
cmake --build "$BUILD_DIR" >/dev/null

log "building fake input client"
mkdir -p "$GEN"
wayland-scanner client-header tests/fake-input.xml "$GEN/fake-input-client-protocol.h"
wayland-scanner private-code tests/fake-input.xml "$GEN/fake-input-protocol.c"
cc -o "$GEN/fakeinput" -I"$GEN" tests/fakeinput.c "$GEN/fake-input-protocol.c" -lwayland-client -lm

KWIN_LOG=$(mktemp /tmp/easecursor-test-log.XXXXXX)
SANDBOX=$(mktemp -d /tmp/easecursor-test-home.XXXXXX)
cleanup() {
    [[ -n ${KWIN_PID:-} ]] && kill "$KWIN_PID" 2>/dev/null || true
    rm -rf "$SANDBOX"
}
trap cleanup EXIT

log "starting sandboxed KWin (two virtual outputs, plugin from $BUILD_DIR)"
XDG_CONFIG_HOME="$SANDBOX/config" XDG_CACHE_HOME="$SANDBOX/cache" XDG_DATA_HOME="$SANDBOX/data" \
QT_PLUGIN_PATH=$PWD/$BUILD_DIR/bin \
QT_FORCE_STDERR_LOGGING=1 \
QT_LOGGING_RULES="kwin_easecursorcrossing.debug=true" \
KWIN_WAYLAND_NO_PERMISSION_CHECKS=1 \
WAYLAND_DISPLAY= DISPLAY= \
kwin_wayland --virtual --width 1280 --height 720 --output-count 2 \
    --socket "$SOCKET" --no-lockscreen >"$KWIN_LOG" 2>&1 &
KWIN_PID=$!

wait_for_log() { # wait_for_log <pattern> [timeout-seconds]
    local deadline=$((SECONDS + ${2:-10}))
    until grep -q "$1" "$KWIN_LOG"; do
        if ((SECONDS >= deadline)) || ! kill -0 "$KWIN_PID" 2>/dev/null; then
            return 1
        fi
        sleep 0.2
    done
}

if ! wait_for_log "config loaded, active: true"; then
    echo "KWin did not start or plugin did not load; log follows:" >&2
    tail -20 "$KWIN_LOG" >&2
    exit 1
fi

log "arranging outputs: Virtual-1 offset to 1280,300 (deadzone on top part of shared edge)"
WAYLAND_DISPLAY=$SOCKET kscreen-doctor output.Virtual-1.position.1280,300 >/dev/null 2>&1
sleep 1

easings() { grep -c "easing cursor across deadzone" "$KWIN_LOG" || true; }
inject() { WAYLAND_DISPLAY=$SOCKET "$GEN/fakeinput" "$@"; }

# layout: Virtual-0 at 0,0 1280x720; Virtual-1 at 1280,300 1280x720.
# deadzone: right edge of Virtual-0 for y < 300.

log "scenario 1: push right through deadzone (y=150) -> expect warp clamped to (1282,302)"
inject abs 1270 150 rel 5 0 30 sleep 200
s1_warp=1
wait_for_log 'easing cursor across deadzone from QPointF(1279,150) to QPointF(1282,302) on "Virtual-1"' 5 && s1_warp=0
check "deadzone push warps to nearest point on Virtual-1" "$s1_warp"

log "scenario 2: push right through aligned edge (y=500) -> expect NO easing (KWin crosses natively)"
before=$(easings)
inject abs 1270 500 rel 5 0 30 sleep 200
after=$(easings)
check "aligned crossing does not trigger the plugin" "$((after != before))"

log "scenario 3: push left where no output exists at all -> expect NO easing"
before=$(easings)
inject abs 10 360 rel -5 0 30 sleep 200
after=$(easings)
check "edge with nothing beyond does not trigger a warp" "$((after != before))"

log "scenario 4: disable via config notify -> expect live reload and NO easing in deadzone"
XDG_CONFIG_HOME="$SANDBOX/config" kwriteconfig6 --file kwinrc \
    --group EaseCursorCrossing --key Active --type bool --notify false
s4_reload=1
wait_for_log "config loaded, active: false" 5 && s4_reload=0
check "plugin reloads config on change notification" "$s4_reload"
before=$(easings)
inject abs 1270 150 rel 5 0 30 sleep 200
after=$(easings)
check "disabled plugin does not ease" "$((after != before))"

echo
log "result: $pass passed, $fail failed (kwin log: $KWIN_LOG)"
[[ $fail -eq 0 ]]
