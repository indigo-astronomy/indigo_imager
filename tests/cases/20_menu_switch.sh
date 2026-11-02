# Settings > Plate Solver switches the agent, saves the choice and AIN restores it.

equals_saved() { equals "$(saved_agent)" "$1"; }

services local:$PORT
make_config
set_saved_agent 0
start_server
start_ain
check "Astrometry is loaded" wait_for 20 driver_is astrometry ON

section "initial state"
check "menu shows Astrometry" equals "$(menu_agent)" "Astrometry Agent"
check "Solver tab lists Astrometry" wait_for 10 combo_has SOLVER_AGENT "Astrometry Agent @ local"
ain_do tooltip Platesolver Agent
check "Platesolver item explains the indexes" grep -q "TOOLTIP Needs its indexes" ain.out

section "selecting the current agent does nothing"
ain_do click Astrometry Agent
sleep 2
check "Astrometry is still loaded" driver_is astrometry ON
check "nothing was logged" log_lacks "Solver agent changed"

section "to Platesolver"
ain_do click Platesolver Agent
check "Platesolver is loaded" wait_for 15 driver_is platesolver ON
check "Astrometry is unloaded" driver_is astrometry OFF
check "choice is saved" wait_for 5 equals_saved 1
check "menu shows Platesolver" equals "$(menu_agent)" "Platesolver Agent"
check "Solver tab lists Platesolver" wait_for 10 combo_has SOLVER_AGENT "Platesolver Agent @ local"
check "change is logged" log_has "Solver agent changed to Platesolver Agent"
check "index note is logged" log_has "Platesolver Agent needs its indexes"

section "back to Astrometry"
ain_do click Astrometry Agent
check "Astrometry is loaded" wait_for 15 driver_is astrometry ON
check "Platesolver is unloaded" driver_is platesolver OFF
check "choice is saved" wait_for 5 equals_saved 0
check "Solver tab lists Astrometry" wait_for 10 combo_has SOLVER_AGENT "Astrometry Agent @ local"

section "restart keeps the choice"
ain_do click Platesolver Agent
check "Platesolver is loaded" wait_for 15 driver_is platesolver ON
wait_for 5 equals_saved 1
stop_ain
start_ain
sleep 3
check "Platesolver is still loaded" driver_is platesolver ON
check "menu shows Platesolver" equals "$(menu_agent)" "Platesolver Agent"
check "choice is still saved" equals "$(saved_agent)" 1

stop_ain
stop_server
finish_case
