#!/bin/sh
# Integration tests for jpegrip.
# Runs against the compiled binary; Unix-only (macOS / Linux).
# Run from the project root: sh test/integration.sh

set -e

BINARY="${1:-./jpegrip}"
SAMPLE="sample.bin"
PASS=0
FAIL=0
WORKDIR=/tmp/jpegrip_test_$$

pass() { echo "  PASS  $1"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL  $1"; FAIL=$((FAIL + 1)); }

cleanup() { rm -rf "$WORKDIR"; }
trap cleanup EXIT

mkdir -p "$WORKDIR"

# ---- helper: check a file starts with FF D8 (JPEG SOI) ----
is_jpeg() {
    [ -f "$1" ] || return 1
    magic=$(dd if="$1" bs=1 count=2 2>/dev/null | od -An -tx1 | tr -d ' \n')
    [ "$magic" = "ffd8" ]
}

echo "=== integration: binary exit codes ==="

# No arguments -> exit 2
if "$BINARY" > /dev/null 2>&1; then
    fail "no-args: expected exit code 2, got 0"
elif [ $? -eq 2 ]; then
    pass "no-args: exits with code 2"
else
    fail "no-args: expected exit code 2, got $?"
fi

# Non-existent file -> exit non-zero
if "$BINARY" /tmp/does_not_exist_jpegrip_test 2>/dev/null; then
    fail "missing-file: expected non-zero exit, got 0"
else
    pass "missing-file: exits non-zero for missing input"
fi

echo ""
echo "=== integration: sample.bin ==="

if [ ! -f "$SAMPLE" ]; then
    echo "  SKIP  sample.bin not found — run mksample.sh first"
else
    cd "$WORKDIR"
    "$OLDPWD/$BINARY" "$OLDPWD/$SAMPLE" > /dev/null
    count=$(ls jpg*.jpg 2>/dev/null | wc -l | tr -d ' ')
    cd "$OLDPWD"

    if [ "$count" -gt 0 ]; then
        pass "sample.bin: extracted $count file(s)"
    else
        fail "sample.bin: no files extracted"
    fi

    all_valid=1
    for f in "$WORKDIR"/jpg*.jpg; do
        [ -f "$f" ] || continue
        if ! is_jpeg "$f"; then
            fail "extracted $f does not start with FF D8"
            all_valid=0
        fi
    done
    if [ "$all_valid" -eq 1 ] && [ "$count" -gt 0 ]; then
        pass "sample.bin: all extracted files have valid JPEG SOI"
    fi
fi

echo ""
echo "=== integration: file with no JPEGs ==="

NO_JPEG="$WORKDIR/nojpeg.bin"
# write 512 bytes of 0x00
dd if=/dev/zero bs=512 count=1 of="$NO_JPEG" 2>/dev/null

NOJPEG_OUT="$WORKDIR/nojpeg_out"
mkdir -p "$NOJPEG_OUT"
cd "$NOJPEG_OUT"
"$OLDPWD/$BINARY" "$NO_JPEG" > /dev/null
count=$(ls jpg*.jpg 2>/dev/null | wc -l | tr -d ' ')
cd "$OLDPWD"

if [ "$count" -eq 0 ]; then
    pass "no-jpeg file: 0 files extracted"
else
    fail "no-jpeg file: expected 0 extracted, got $count"
fi

echo ""
echo "integration: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
