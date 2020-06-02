#!/usr/bin/env python3
import re

# https://github.com/DaveBerkeley/kicad/blob/master/drill.py
def readDrill(path):
	with open(path) as f:
		
		# regular expressions
		toolDefinition = re.compile("(T[0-9])C(.*)")
		toolUsage = re.compile("(T[0-9])$")
		hole = re.compile("X(-?[\.0-9]*)Y(-?[\.0-9]*)$")
		
		sizes = {}
		
		# parse header until line containing "%"
		scale = 1.0
		for line in f:
			line = line.strip()
			if line == "%":
				break
			if line.startswith("METRIC"):
				scale = 1.0
				continue
			if line.startswith("INCH"):
				scale = 25.4
				continue
			m = toolDefinition.match(line)
			if not m:
				continue
		
			tool, size = m.groups()
			sizes[tool] = float(size)
		
		# parse commands
		tool = None
		holes = []
		for line in f:
			line = line.strip()
			
			# check if a new tool gets used
			m = toolUsage.match(line)
			if m:
				tool, = m.groups()
				continue
			
			# check if current tool is in list of tools
			if tool in sizes.keys():
				m = hole.match(line)
				if m:
					x, y = m.groups()
					x, y = [ float(a) * scale for a in [ x, y ] ]
					holes.append((sizes[tool], x, y))
		
		return holes


holes = readDrill("main-PTH.drl") + readDrill("main-NPTH.drl")

print("module drills() {")
for size, x, y in holes:
	if size > 0.79:
		print(f"\tdrill({x-100:.5g}, {y+100:.5g}, {size:.5g});")
print("}")
		

holes = readDrill("switch-PTH.drl")

print("module drills() {")
for size, x, y in holes:
	if size > 0.79:
		print(f"\tdrill({x-100:.5g}, {y+100:.5g}, {size:.5g});")
print("}")
