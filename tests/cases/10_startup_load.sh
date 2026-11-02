# AIN loads the saved solver agent when a server connects, unloading the other one first.

startup() {	# title, saved choice, expected, other, server drivers, driver loaded before AIN
	section "$1"
	rm -rf "$HOME"
	services local:$PORT
	make_config
	set_saved_agent "$2"
	start_server $5
	[ -n "$6" ] && prop_set "Server.DRIVERS.$6=ON" && sleep 1
	start_ain
	check "$3 is loaded" wait_for 20 driver_is "$3" ON
	check "$4 is not loaded" driver_is "$4" OFF
	check "saved choice is unchanged" equals "$(saved_agent)" "$2"
	stop_ain
	stop_server
}

startup "listed drivers, saved Astrometry, nothing loaded" 0 astrometry platesolver "" ""
startup "listed drivers, saved Platesolver, nothing loaded" 1 platesolver astrometry "" ""
startup "listed drivers, saved Platesolver, Astrometry loaded" 1 platesolver astrometry "" indigo_agent_astrometry
startup "listed drivers, saved Astrometry, Platesolver loaded" 0 astrometry platesolver "" indigo_agent_platesolver
startup "command line drivers, saved Platesolver, load by name" 1 platesolver astrometry indigo_agent_astrometry ""

finish_case
