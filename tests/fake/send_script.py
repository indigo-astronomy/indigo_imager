#!/usr/bin/env python3
"""Runs a script in the Scripting Agent: send_script.py port file

Sent directly, because indigo_prop_tool set_script can not read a script of this size."""
import socket
import sys
import time
from xml.sax.saxutils import escape

port, path = int(sys.argv[1]), sys.argv[2]
with open(path) as file:
	script = file.read()
connection = socket.create_connection(("127.0.0.1", port))
connection.sendall(b"<getProperties version='2.0' client='test'/>")
time.sleep(1)
request = "<newTextVector device='Scripting Agent' name='AGENT_SCRIPTING_RUN_SCRIPT'><oneText name='SCRIPT'>%s</oneText></newTextVector>" % escape(script)
connection.sendall(request.encode())
time.sleep(2)
connection.close()
