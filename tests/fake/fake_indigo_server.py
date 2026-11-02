#!/usr/bin/env python3
"""Scripted INDIGO server for the switch failures a real server can not be made to produce.

usage: fake_indigo_server.py port mode logfile

modes:
  ok              both solver drivers listed, requests succeed
  refuse          unloading a driver is refused
  noload          loading indigo_agent_platesolver fails
  unlisted        started with drivers on the command line: Platesolver is not listed, LOAD works
  unlisted_noload as unlisted, LOAD of indigo_agent_platesolver fails
  disconnect      the connection is closed when a driver is unloaded
"""
import re
import socket
import sys
import threading

ASTROMETRY = "indigo_agent_astrometry"
PLATESOLVER = "indigo_agent_platesolver"
AGENTS = {ASTROMETRY: "Astrometry Agent", PLATESOLVER: "Platesolver Agent"}

SWITCH_REQUEST = re.compile(r"<newSwitchVector[^>]*name=['\"]DRIVERS['\"][^>]*>(.*?)</newSwitchVector>", re.S)
LOAD_REQUEST = re.compile(r"<newTextVector[^>]*name=['\"]LOAD['\"][^>]*>.*?<oneText[^>]*>([^<]*)</oneText>\s*</newTextVector>", re.S)
SWITCH_ITEM = re.compile(r"<oneSwitch name=['\"]([^'\"]+)['\"]>\s*(On|Off)")

port, mode, log_path = int(sys.argv[1]), sys.argv[2], sys.argv[3]
drivers = {ASTROMETRY: False}
if not mode.startswith("unlisted"):
	drivers[PLATESOLVER] = False
drivers[ASTROMETRY] = True


def log(text):
	with open(log_path, "a") as file:
		file.write("%d %s\n" % (port, text))


def define_agent(name):
	return "<defTextVector device='%s' name='INFO' group='Main' label='i' state='Ok' perm='ro'><defText name='DEVICE_NAME' label='n'>%s</defText></defTextVector>" % (name, name)


def define_drivers():
	items = "".join("<defSwitch name='%s' label='%s'>%s</defSwitch>" % (k, k, "On" if v else "Off") for k, v in drivers.items())
	return "<defSwitchVector device='Server' name='DRIVERS' group='Main' label='d' state='Ok' perm='rw' rule='AnyOfMany'>%s</defSwitchVector>" % items


def update_drivers(message=""):
	items = "".join("<oneSwitch name='%s'>%s</oneSwitch>" % (k, "On" if v else "Off") for k, v in drivers.items())
	attribute = " message='%s'" % message if message else ""
	return "<setSwitchVector device='Server' name='DRIVERS' state='Ok'%s>%s</setSwitchVector>" % (attribute, items)


def definitions():
	result = define_drivers()
	result += "<defTextVector device='Server' name='LOAD' group='Main' label='l' state='Ok' perm='rw'><defText name='DRIVER' label='d'></defText></defTextVector>"
	for driver, agent in AGENTS.items():
		if drivers.get(driver):
			result += define_agent(agent)
	return result


def handle_switch(connection, name, value):
	log("DRIVERS %s=%s" % (name, value))
	agent = AGENTS.get(name)
	if value == "Off" and mode == "refuse":
		connection.sendall(update_drivers("Driver %s is in use" % name).encode())
		return True
	if value == "Off" and mode == "disconnect":
		connection.close()
		return False
	if value == "On" and name == PLATESOLVER and mode == "noload":
		connection.sendall(update_drivers("Driver %s failed to load" % name).encode())
		return True
	drivers[name] = value == "On"
	if agent:
		connection.sendall((define_agent(agent) if value == "On" else "<delProperty device='%s'/>" % agent).encode())
	connection.sendall(update_drivers().encode())
	return True


def handle_load(connection, name):
	if name not in AGENTS:
		return
	log("LOAD " + name)
	if name == PLATESOLVER and mode == "unlisted_noload":
		connection.sendall(b"<setTextVector device='Server' name='LOAD' state='Alert' message='Driver failed'><oneText name='DRIVER'></oneText></setTextVector>")
	elif name == PLATESOLVER:
		drivers[PLATESOLVER] = True
		connection.sendall(("<delProperty device='Server' name='DRIVERS'/>" + define_drivers() + define_agent(AGENTS[name])).encode())


def handle(connection):
	buffer = ""
	defined = False
	while True:
		try:
			data = connection.recv(65536)
		except OSError:
			return
		if not data:
			return
		buffer += data.decode("utf8", "replace")
		if not defined and "getProperties" in buffer:
			defined = True
			connection.sendall(definitions().encode())
		while True:
			switch = SWITCH_REQUEST.search(buffer)
			load = LOAD_REQUEST.search(buffer)
			if switch and (not load or switch.start() < load.start()):
				buffer = buffer[switch.end():]
				for name, value in SWITCH_ITEM.findall(switch.group(1)):
					if not handle_switch(connection, name, value):
						return
			elif load:
				buffer = buffer[load.end():]
				handle_load(connection, load.group(1).strip())
			else:
				break


listener = socket.socket()
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", port))
listener.listen(5)
while True:
	client, _ = listener.accept()
	threading.Thread(target=handle, args=(client,), daemon=True).start()
