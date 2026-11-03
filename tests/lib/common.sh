# Sourced by every test case, which runs in its own network namespace with its own HOME:
# no real server can be discovered or changed, and no user configuration is touched.

REAL_HOME=${AIN_TEST_REAL_HOME:-$HOME}
BIN=$INDIGO_ROOT/build/bin
PORT=17624
PASSED=0
FAILED=0
SKIP=77
SERVER_PID=
AIN_PID=
FAKE_PIDS=

export HOME=$CASE_DIR/home
export XDG_CONFIG_HOME=$HOME/.config
export LD_LIBRARY_PATH=$INDIGO_ROOT/build/lib
export QT_QPA_PLATFORM=offscreen

cd "$CASE_DIR" || exit 1
ip link set lo up

read -r CONF_OFFSET CONF_SIZE < <("$BUILD_DIR/conf_offset")

cleanup() {
	[ -n "$AIN_PID" ] && kill -9 "$AIN_PID" 2>/dev/null
	[ -n "$SERVER_PID" ] && kill -9 "$SERVER_PID" 2>/dev/null
	[ -n "$FAKE_PIDS" ] && kill -9 $FAKE_PIDS 2>/dev/null
	wait 2>/dev/null
}
trap cleanup EXIT

# ---- results

check() {	# description, command...
	local description=$1
	shift
	if "$@" >/dev/null 2>&1; then
		PASSED=$((PASSED + 1))
		echo "  ok    $description"
	else
		FAILED=$((FAILED + 1))
		echo "  FAIL  $description"
	fi
}

equals() {
	[ "$1" = "$2" ]
}

section() {
	echo "--- $*"
}

skip_case() {
	echo "  skip  $*"
	exit $SKIP
}

finish_case() {
	echo "  $PASSED passed, $FAILED failed"
	[ "$FAILED" -eq 0 ] && [ "$PASSED" -gt 0 ]
	exit $?
}

wait_for() {	# seconds, command...
	local deadline=$(($(date +%s) + $1))
	shift
	while ! "$@" >/dev/null 2>&1; do
		[ "$(date +%s)" -ge "$deadline" ] && return 1
		sleep 0.5
	done
}

# ---- INDIGO

prop() {
	timeout 8 "$BIN/indigo_prop_tool" "$@" -p $PORT 2>&1
}

prop_set() {
	prop set -t 1 "$1" >/dev/null
}

item() {	# Device.PROPERTY ITEM - first reported value
	prop list "$1" -t 1 | grep -E "\.$2 = " | head -1 | sed 's/.* = //'
}

state_of() {	# Device.PROPERTY
	prop list_state "$1" -t 1 | head -1 | sed 's/.* = //'
}

driver_is() {	# driver, ON or OFF; an unlisted driver is OFF
	local value
	value=$(item Server.DRIVERS "indigo_agent_$1")
	equals "${value:-OFF}" "$2"
}

agent_present() {
	prop list -t 1 | cut -d. -f1 | grep -qx "$1"
}

start_server() {	# drivers...
	"$BIN/indigo_server" -p $PORT -v "$@" > "$CASE_DIR/server.log" 2>&1 &
	SERVER_PID=$!
	wait_for 15 prop list Server.DRIVERS -t 1
}

# a server does not exit while one of its drivers is in use, e.g. during a batch
stop_process() {
	kill "$1" 2>/dev/null
	local i
	for i in $(seq 1 30); do
		kill -0 "$1" 2>/dev/null || break
		sleep 0.5
	done
	kill -9 "$1" 2>/dev/null
	wait "$1" 2>/dev/null
}

stop_server() {
	stop_process "$SERVER_PID"
	SERVER_PID=
}

start_fake() {	# port, mode
	python3 -I "$TESTS_DIR/fake/fake_indigo_server.py" "$1" "$2" "$CASE_DIR/fake.log" &
	FAKE_PIDS="$FAKE_PIDS $!"
	sleep 1
}

stop_fakes() {
	kill $FAKE_PIDS 2>/dev/null
	wait $FAKE_PIDS 2>/dev/null
	FAKE_PIDS=
}

# ---- AIN

services() {	# name:port...
	mkdir -p "$XDG_CONFIG_HOME"
	: > "$XDG_CONFIG_HOME/indigo_imager.services"
	local entry
	for entry in "$@"; do
		echo "${entry%%:*}@127.0.0.1:${entry##*:}, M, 1" >> "$XDG_CONFIG_HOME/indigo_imager.services"
	done
}

# AIN writes a config with its defaults when it exits, the choice is then set in it
make_config() {
	timeout -s TERM 3 "$AIN_BIN" >/dev/null 2>&1
	[ "$(stat -c %s "$XDG_CONFIG_HOME/indigo_imager.conf")" = "$CONF_SIZE" ] || { echo "unexpected config size"; exit 1; }
}

set_saved_agent() {	# 0 Astrometry, 1 Platesolver
	printf "\\x0$1" | dd of="$XDG_CONFIG_HOME/indigo_imager.conf" bs=1 seek="$CONF_OFFSET" conv=notrunc status=none
}

saved_agent() {
	od -An -tu1 -j "$CONF_OFFSET" -N1 "$XDG_CONFIG_HOME/indigo_imager.conf" | tr -d ' '
}

start_ain() {
	: > "$CASE_DIR/ain.cmd"
	: >> "$CASE_DIR/ain.out"
	AIN_CMD=$CASE_DIR/ain.cmd AIN_OUT=$CASE_DIR/ain.out LD_PRELOAD=$BUILD_DIR/ain_harness.so "$AIN_BIN" > "$CASE_DIR/ain.log" 2>&1 &
	AIN_PID=$!
	ain_do sync
}

stop_ain() {
	kill -TERM "$AIN_PID" 2>/dev/null
	wait "$AIN_PID" 2>/dev/null
	AIN_PID=
}

# a command is finished when the sync after it is reported
ain_do() {
	[ "$1" != sync ] && echo "$*" >> "$CASE_DIR/ain.cmd"
	# not a counter: this also runs in subshells, where an increment would be lost
	local id
	id=$(date +%s%N)
	echo "sync $id" >> "$CASE_DIR/ain.cmd"
	wait_for 20 grep -qx "SYNC $id" "$CASE_DIR/ain.out" || { echo "AIN does not respond to: $*"; return 1; }
}

menu_agent() {	# the checked Plate Solver item
	ain_do dump
	tac "$CASE_DIR/ain.out" | grep -m2 '^MENU' | grep 'checked=1' | sed 's/MENU \(.*\) checked=1/\1/'
}

combo() {	# SOLVER_AGENT, SOURCE or FILTER - the first combo with that content
	ain_do dump
	tac "$CASE_DIR/ain.out" | sed '/^DUMP/q' | tac | grep -m1 "^COMBO $1 "
}

combo_has() {	# tag, text
	combo "$1" | grep -qF "$2"
}

ain_log() {
	ain_do log "$CASE_DIR/ain_panel.txt"
	cat "$CASE_DIR/ain_panel.txt"
}

log_has() {
	ain_log | grep -qF "$1"
}

log_lacks() {
	! log_has "$1"
}

log_order() {	# texts expected to appear in this order
	local line=0 text found
	ain_log > "$CASE_DIR/ain_panel_order.txt"
	for text in "$@"; do
		found=$(tail -n +$((line + 1)) "$CASE_DIR/ain_panel_order.txt" | grep -nF -m1 "$text" | cut -d: -f1)
		[ -n "$found" ] || return 1
		line=$((line + found))
	done
}

# ---- solving: real indexes as links, so that the originals can not be changed

ASTROMETRY_INDEXES="4210=ON;4211=ON;4212=ON;4213=ON"
PLATESOLVER_INDEXES="INDEX_01DEG_TYC2=ON;INDEX_02DEG_TYC2=ON;INDEX_05DEG_TYC2=ON;INDEX_10DEG_TYC2=ON"

need_astrometry() {
	command -v solve-field >/dev/null || skip_case "solve-field is not installed"
	ls "$AIN_TEST_INDEX_HOME"/astrometry/index-421* >/dev/null 2>&1 || skip_case "no 421x indexes in $AIN_TEST_INDEX_HOME/astrometry"
}

need_platesolver() {
	ls "$AIN_TEST_INDEX_HOME"/solver/*.isix >/dev/null 2>&1 || skip_case "no indexes in $AIN_TEST_INDEX_HOME/solver"
}

need_image() {
	[ -f "$AIN_TEST_IMAGE" ] || skip_case "AIN_TEST_IMAGE is not set to a FITS image"
}

link_indexes() {
	mkdir -p "$HOME/.indigo/solver" "$HOME/.indigo/astrometry"
	local file
	for file in "$AIN_TEST_INDEX_HOME"/solver/*.isix; do
		[ -e "$file" ] && ln -sf "$file" "$HOME/.indigo/solver/"
	done
	# only the index files: the agent writes astrometry.cfg, through a link it would reach the original
	for file in "$AIN_TEST_INDEX_HOME"/astrometry/index-*; do
		[ -e "$file" ] && ln -sf "$file" "$HOME/.indigo/astrometry/"
	done
}

# ---- simulators

SIM_CAMERA="CCD Imager Simulator"
SIM_WHEEL="CCD Imager Simulator (wheel)"
SIM_MOUNT="Mount Simulator"

start_rig() {	# 0 Astrometry, 1 Platesolver
	services local:$PORT
	make_config
	link_indexes
	set_saved_agent "$1"
	start_server
	prop_set "Server.DRIVERS.indigo_ccd_simulator=ON;indigo_mount_simulator=ON"
	wait_for 15 agent_present "$SIM_MOUNT"
	start_ain
	wait_for 30 agent_present "Scripting Agent"
	if [ "$1" = 0 ]; then
		SOLVER="Astrometry Agent"
		wait_for 30 agent_present "$SOLVER"
		prop_set "$SOLVER.AGENT_PLATESOLVER_USE_INDEX.$ASTROMETRY_INDEXES"
	else
		SOLVER="Platesolver Agent"
		wait_for 30 agent_present "$SOLVER"
		prop_set "$SOLVER.AGENT_PLATESOLVER_USE_INDEX.$PLATESOLVER_INDEXES"
	fi
	prop_set "Imager Agent.FILTER_CCD_LIST.$SIM_CAMERA=ON"
	prop_set "Imager Agent.FILTER_WHEEL_LIST.$SIM_WHEEL=ON"
	prop_set "Mount Agent.FILTER_MOUNT_LIST.$SIM_MOUNT=ON"
	sleep 2
	ain_do tab Solver
	ain_do source Imager Agent
	sleep 2
}

# The simulator always renders the same star field, the location brings it to transit when needed.
TARGET_RA=6.5268
TARGET_DEC=5.0933

set_location() {	# longitude
	prop_set "Mount Agent.GEOGRAPHIC_COORDINATES.LATITUDE=42;LONGITUDE=$1"
	prop_set "$SIM_MOUNT.GEOGRAPHIC_COORDINATES.LATITUDE=42;LONGITUDE=$1"
	sleep 2
}

# Slews to the field well before its transit and enables the flip. TIME_TO_TRANSIT counts to the
# nearest of both meridian crossings, so the site is found from the hour angle, which tells them
# apart, and the exact timing is left to set_transit_in().
point_at_target() {
	local display="Mount Agent.AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY"
	set_location "$(python3 -I -c "
import sys
ra, ha, longitude, target = map(float, sys.argv[1:5])
print(round((longitude + ((target - 0.1 - (ra + ha) + 12) % 24 - 12) * 15 + 180) % 360 - 180, 5))" "$(item "$display" RA_JNOW)" "$(item "$display" HA)" "$(item "Mount Agent.GEOGRAPHIC_COORDINATES" LONGITUDE)" $TARGET_RA)"
	cat > point.js <<JS
var sequence = new Sequence("point at target");
sequence.select_imager_camera("$SIM_CAMERA");
sequence.select_filter_wheel("$SIM_WHEEL");
sequence.select_mount("$SIM_MOUNT");
sequence.unpark();
sequence.enable_tracking();
sequence.enable_meridian_flip(true, 0);
sequence.slew($TARGET_RA, $TARGET_DEC);
sequence.sync_center(1);
sequence.start();
JS
	run_sequence "$CASE_DIR/point.js" 90
	# just east of the upper meridian crossing
	python3 -I -c "
import sys
sys.exit(not 23.5 < float(sys.argv[1]) < 24)" "$(item "$display" HA)"
}

seconds_to_transit() {
	python3 -I -c "
import sys
print(round(((float(sys.argv[1]) + 12) % 24 - 12) * 3600))" "$(item "Mount Agent.AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY" TIME_TO_TRANSIT)"
}

set_transit_in() {	# seconds, the mount has to be on the target
	set_location "$(python3 -I -c "
import sys
seconds, longitude, wanted = map(float, sys.argv[1:4])
print(round((longitude + (seconds - wanted) / 3600 * 15 + 180) % 360 - 180, 5))" "$(seconds_to_transit)" "$(item "Mount Agent.GEOGRAPHIC_COORDINATES" LONGITUDE)" "$1")"
	local now
	now=$(seconds_to_transit)
	echo "  transit in $now s"
	[ "$now" -gt $(($1 - 20)) ] && [ "$now" -le "$1" ]
}

stop_rig() {
	stop_ain
	stop_server
}

sequence_state() {
	state_of "Scripting Agent.SEQUENCE_STATE"
}

sequence_done() {
	local state
	state=$(sequence_state)
	[ -n "$state" ] && [ "$state" != BUSY ]
}

run_sequence() {	# file, timeout
	ain_do tab Sequence
	ain_do loadseq "$1"
	sleep 2
	ain_do button Run
	wait_for 15 equals_busy
	wait_for "$2" sequence_done
}

equals_busy() {
	[ "$(sequence_state)" = BUSY ]
}
