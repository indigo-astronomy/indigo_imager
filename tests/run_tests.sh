#!/bin/bash
# Runs the AIN solver agent tests, each case in its own network namespace and HOME.
#
# usage: run_tests.sh [-l] [-k] [case...]
#   -l  list the cases
#   -k  keep the work directory, also when everything passed
#   case  name or number of a case, e.g. menu_switch or 20; all of them when omitted
#
# environment:
#   INDIGO_ROOT          INDIGO checkout with a build (default: ../indigo)
#   AIN_BIN              ain_imager binary (default: ../ain_imager_src/ain_imager)
#   AIN_TEST_IMAGE       FITS image with a WCS header, for the real solve cases
#   AIN_TEST_INDEX_HOME  directory with astrometry/ and solver/ indexes (default: ~/.indigo)
#
# exit status: 0 nothing failed, 1 a case failed, 2 the tests could not run

TESTS_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT_DIR=$(dirname "$TESTS_DIR")
export TESTS_DIR ROOT_DIR
export INDIGO_ROOT=${INDIGO_ROOT:-$ROOT_DIR/indigo}
export AIN_BIN=${AIN_BIN:-$ROOT_DIR/ain_imager_src/ain_imager}
export BUILD_DIR=$TESTS_DIR/build
export AIN_TEST_REAL_HOME=$HOME
export AIN_TEST_INDEX_HOME=${AIN_TEST_INDEX_HOME:-$HOME/.indigo}
export AIN_TEST_IMAGE

KEEP=0
while getopts "lk" option; do
	case $option in
		l) for file in "$TESTS_DIR"/cases/*.sh; do basename "$file" .sh; done; exit 0 ;;
		k) KEEP=1 ;;
		*) sed -n '4,8p' "$0"; exit 2 ;;
	esac
done
shift $((OPTIND - 1))

die() {
	echo "$*" >&2
	exit 2
}

[ -x "$AIN_BIN" ] || die "no ain_imager at $AIN_BIN, build it first"
[ -x "$INDIGO_ROOT/build/bin/indigo_server" ] || die "no INDIGO build in $INDIGO_ROOT, set INDIGO_ROOT"
[ -f "$BUILD_DIR/ain_harness.so" ] && [ -x "$BUILD_DIR/conf_offset" ] || die "test helpers are not built, run make in $TESTS_DIR"
# without an isolated network AIN would discover and change real servers
unshare -rn true 2>/dev/null || die "can not create a network namespace: unshare -rn is required"

CASES=()
if [ $# -eq 0 ]; then
	CASES=("$TESTS_DIR"/cases/*.sh)
else
	for name in "$@"; do
		match=("$TESTS_DIR"/cases/*"$name"*.sh)
		[ -e "${match[0]}" ] || die "no case $name"
		CASES+=("${match[@]}")
	done
fi

index_fingerprint() {
	find "$AIN_TEST_INDEX_HOME/astrometry" "$AIN_TEST_INDEX_HOME/solver" -type f -printf '%p %s %T@\n' 2>/dev/null | sort
}

INDEXES_BEFORE=$(index_fingerprint)
WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/ain_tests.XXXXXX")
export WORK_DIR
PASSED=0 FAILED=0 SKIPPED=0
FAILED_NAMES=()

for file in "${CASES[@]}"; do
	name=$(basename "$file" .sh)
	CASE_DIR=$WORK_DIR/$name
	mkdir -p "$CASE_DIR/home"
	echo "=== $name"
	SECONDS=0
	CASE_DIR=$CASE_DIR unshare -rn bash -c '. "$TESTS_DIR/lib/common.sh"; . "$1"' case "$file" 2>&1 | tee "$CASE_DIR/result.txt"
	status=${PIPESTATUS[0]}
	case $status in
		0) PASSED=$((PASSED + 1)); echo "    PASS ($SECONDS s)" ;;
		77) SKIPPED=$((SKIPPED + 1)); echo "    SKIP" ;;
		*) FAILED=$((FAILED + 1)); FAILED_NAMES+=("$name"); echo "    FAIL ($SECONDS s)" ;;
	esac
done

INDEXES_CHANGED=0
if [ "$INDEXES_BEFORE" != "$(index_fingerprint)" ]; then
	INDEXES_CHANGED=1
	echo
	echo "ERROR: files in $AIN_TEST_INDEX_HOME changed during the run, the tests must only read them"
	echo "(a solve in a real session running at the same time also causes this)"
fi

echo
echo "cases: $PASSED passed, $FAILED failed, $SKIPPED skipped"
if [ $FAILED -gt 0 ] || [ $INDEXES_CHANGED -eq 1 ]; then
	[ $FAILED -gt 0 ] && echo "failed: ${FAILED_NAMES[*]}"
	echo "logs: $WORK_DIR"
	exit 1
fi
if [ $KEEP -eq 1 ]; then
	echo "logs: $WORK_DIR"
else
	rm -rf "$WORK_DIR"
fi
exit 0
