# A real solve of AIN_TEST_IMAGE with Platesolver Agent, compared with the WCS in the image header.

need_image
need_platesolver

read -r EXPECTED_RA EXPECTED_DEC < <(python3 -I - "$AIN_TEST_IMAGE" <<'PY'
import re, sys
header = open(sys.argv[1], "rb").read(2880 * 8).decode("ascii", "replace")
def value(key):
	match = re.search(r"%-8s=\s*([-+0-9.Ee]+)" % key, header)
	return float(match.group(1)) if match else None
ra, dec = value("CRVAL1"), value("CRVAL2")
print("" if ra is None else ra / 15, "" if dec is None else dec)
PY
)
[ -n "$EXPECTED_DEC" ] || skip_case "AIN_TEST_IMAGE has no CRVAL1/CRVAL2 in its header"

close_to() {	# value, expected, tolerance
	python3 -I -c "import sys; a, b, t = map(float, sys.argv[1:4]); sys.exit(abs(a - b) > t)" "$1" "$2" "$3"
}

not_busy() {
	[ "$(state_of "Platesolver Agent.AGENT_PLATESOLVER_WCS")" != BUSY ]
}

A="Platesolver Agent"
services local:$PORT
make_config
link_indexes
set_saved_agent 1
start_server
start_ain
wait_for 30 agent_present "$A"
prop_set "$A.AGENT_PLATESOLVER_USE_INDEX.INDEX_00_5DEG_GAIA=ON;INDEX_01DEG_TYC2=ON"
ain_do tab Solver
ain_do source Upload File
sleep 1
ain_do upload "$AIN_TEST_IMAGE"
check "upload is logged" wait_for 15 log_has "File uploaded for solving"
check "solving starts in Platesolver Agent" wait_for 15 log_has "Platesolver Agent @ local: Solving started"
check "image is solved" wait_for 120 log_has "Platesolver Agent @ local: Solved"
wait_for 5 not_busy
RA=$(item "$A.AGENT_PLATESOLVER_WCS" RA)
DEC=$(item "$A.AGENT_PLATESOLVER_WCS" DEC)
echo "  solved RA $RA Dec $DEC, header RA $EXPECTED_RA Dec $EXPECTED_DEC"
check "RA matches the header" close_to "$RA" "$EXPECTED_RA" 0.01
check "Dec matches the header" close_to "$DEC" "$EXPECTED_DEC" 0.1

stop_ain
stop_server
finish_case
