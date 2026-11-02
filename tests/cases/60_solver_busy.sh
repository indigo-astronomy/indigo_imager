# The agent can not be switched during a real solve. AIN_TEST_IMAGE must be narrower than the 421x
# indexes, so that solve-field keeps working on it instead of finishing at once.

need_image
need_astrometry

A="Astrometry Agent"
wcs_is() {
	[ "$(state_of "$A.AGENT_PLATESOLVER_WCS")" = "$1" ]
}
wcs_not_busy() {
	! wcs_is BUSY
}
equals_saved() {
	equals "$(saved_agent)" "$1"
}

services local:$PORT
make_config
link_indexes
set_saved_agent 0
start_server
start_ain
wait_for 30 agent_present "$A"
prop_set "$A.AGENT_PLATESOLVER_USE_INDEX.$ASTROMETRY_INDEXES"
ain_do tab Solver
ain_do source Upload File
sleep 1
ain_do upload "$AIN_TEST_IMAGE"
wait_for 20 wcs_is BUSY || skip_case "solve-field did not stay busy on this image"

section "while solving"
ain_do click Platesolver Agent
check "switch is refused" wait_for 5 log_has "Can not switch the solver agent, 'Astrometry Agent @ local' is busy"
check "Astrometry stays loaded" driver_is astrometry ON
check "Platesolver is not loaded" driver_is platesolver OFF
check "solve is still running" wcs_is BUSY
check "menu is reset" equals "$(menu_agent)" "Astrometry Agent"
check "choice is not saved" equals "$(saved_agent)" 0

section "after aborting"
ain_do button Solve
check "solve stops" wait_for 20 wcs_not_busy
ain_do click Platesolver Agent
check "Platesolver is loaded" wait_for 15 driver_is platesolver ON
check "choice is saved" wait_for 5 equals_saved 1

stop_ain
stop_server
finish_case
