#!/usr/bin/env bash
# tests/run_tests.sh — Integration tests for vcdgen
#
# Builds blink.elf, runs vcdgen, validates the resulting VCD file.

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
VCDGEN="$ROOT_DIR/vcdgen"
BLINK_HEX="$SCRIPT_DIR/blink/blink.hex"
OUT_VCD="/tmp/vcdgen_test_blink.vcd"

PASS=0
FAIL=0

check() {
    local desc="$1"
    local result="$2"
    if [ "$result" = "0" ]; then
        echo "[PASS] $desc"
        PASS=$((PASS + 1))
    else
        echo "[FAIL] $desc"
        FAIL=$((FAIL + 1))
    fi
}

echo "=== vcdgen integration tests ==="
echo

# Build blink firmware
echo "--- Building blink firmware ---"
make -C "$SCRIPT_DIR/blink" --no-print-directory
echo

# Run vcdgen (indefinite mode so it stops when firmware halts)
echo "--- Running vcdgen ---"
"$VCDGEN" --indefinite -m atmega328p -f 16000000 -o "$OUT_VCD" -v "$BLINK_HEX"
echo

# Test 1: VCD file created
[ -f "$OUT_VCD" ]
check "VCD file created" $?

# Test 2: Non-empty
[ -s "$OUT_VCD" ]
check "VCD file is non-empty" $?

# Test 3: VCD header present
grep -q '^\$version' "$OUT_VCD" 2>/dev/null
check "VCD has \$version header" $?

# Test 4: Port B 8-bit bus signal declared
grep -q 'PB ' "$OUT_VCD" 2>/dev/null || grep -q '"PB"' "$OUT_VCD" 2>/dev/null
check "Port B 8-bit bus signal (PB) in VCD" $?

# Test 5: Pin-level signal declared (PB0)
grep -q 'PB0' "$OUT_VCD" 2>/dev/null
check "Port B bit 0 signal (PB0) in VCD" $?

# Test 6: Value changes present (lines starting with b or 0/1 for bit signals)
# VCD value change lines look like: b00000001 ! or 1" (for 1-bit)
CHANGES=$(grep -c '^[b01]' "$OUT_VCD" 2>/dev/null || echo 0)
[ "$CHANGES" -gt 0 ]
check "VCD contains value changes ($CHANGES lines)" $?

# Test 7: At least 6 transitions expected (BLINK_COUNT=6 in blink.c)
[ "$CHANGES" -ge 6 ]
check "At least 6 value-change lines (matches BLINK_COUNT=6)" $?

echo
echo "=== Results: $PASS passed, $FAIL failed ==="
[ "$FAIL" -eq 0 ]
