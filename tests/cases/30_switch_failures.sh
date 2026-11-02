# What a real server can not be made to do: refused unloads, failed loads, lost connections.

SWITCH_TIMEOUT=18

server_saw_not_platesolver() {
	! grep -q "indigo_agent_platesolver" fake.log
}

scenario() {	# title, mode, saved choice
	section "$1"
	: > fake.log
	start_fake $PORT "$2"
	services local:$PORT
	set_saved_agent "$3"
	start_ain
	sleep 2
}

end_scenario() {
	stop_ain
	stop_fakes
	sleep 1
}

server_saw() {
	grep -qF "$PORT $1" fake.log
}

services local:$PORT
make_config

scenario "refused unload" refuse 0
ain_do click Platesolver Agent
sleep $SWITCH_TIMEOUT
check "unload was requested" server_saw "DRIVERS indigo_agent_astrometry=Off"
check "load was not requested" server_saw_not_platesolver
check "server reason is logged" log_has "is in use"
check "failure is logged" log_has "Failed to switch to Platesolver Agent"
check "menu is reset" equals "$(menu_agent)" "Astrometry Agent"
check "choice is not saved" equals "$(saved_agent)" 0
end_scenario

scenario "failed load restores the old agent" noload 0
ain_do click Platesolver Agent
sleep $SWITCH_TIMEOUT
check "old agent is restored" grep -q "DRIVERS indigo_agent_astrometry=On" fake.log
check "failure is logged" log_has "Failed to switch to Platesolver Agent"
check "menu is reset" equals "$(menu_agent)" "Astrometry Agent"
check "choice is not saved" equals "$(saved_agent)" 0
end_scenario

scenario "load by name fails on a server that does not list the driver" unlisted_noload 0
ain_do click Platesolver Agent
sleep $SWITCH_TIMEOUT
check "load by name was requested" server_saw "LOAD indigo_agent_platesolver"
check "old agent is restored" grep -q "DRIVERS indigo_agent_astrometry=On" fake.log
check "menu is reset" equals "$(menu_agent)" "Astrometry Agent"
check "choice is not saved" equals "$(saved_agent)" 0
end_scenario

scenario "load by name succeeds on a server that does not list the driver" unlisted 0
ain_do click Platesolver Agent
sleep 3
check "load by name was requested" server_saw "LOAD indigo_agent_platesolver"
check "menu shows Platesolver" equals "$(menu_agent)" "Platesolver Agent"
check "choice is saved" equals "$(saved_agent)" 1
check "Solver tab lists Platesolver" combo_has SOLVER_AGENT "Platesolver Agent @ local"
end_scenario

scenario "refused unload at startup keeps the saved choice" refuse 1
sleep $SWITCH_TIMEOUT
check "failure is logged" log_has "Failed to switch to Platesolver Agent"
check "choice stays saved" equals "$(saved_agent)" 1
end_scenario

scenario "connection lost during a switch" disconnect 0
ain_do click Platesolver Agent
sleep 3
check "AIN still responds" ain_do sync
check "choice is saved for the next connection" equals "$(saved_agent)" 1
end_scenario

section "two servers, one refuses"
: > fake.log
start_fake $PORT refuse
start_fake $((PORT + 1)) ok
services a:$PORT b:$((PORT + 1))
set_saved_agent 0
start_ain
sleep 3
ain_do click Platesolver Agent
ain_do click Astrometry Agent
check "second click is refused" log_has "already in progress"
sleep $SWITCH_TIMEOUT
check "working server switched" grep -q "$((PORT + 1)) DRIVERS indigo_agent_platesolver=On" fake.log
check "refusing server is reported" log_has "Failed to switch to Platesolver Agent on 'Server @ a'"
check "menu shows Platesolver" equals "$(menu_agent)" "Platesolver Agent"
check "choice is saved" equals "$(saved_agent)" 1
end_scenario

finish_case
