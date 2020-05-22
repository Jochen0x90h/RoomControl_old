#!/usr/bin/env python3

CIRCLE = 0.5
ROUNDRECT = 0.25
RECT = 0

userWidth = 0.1
userLayer = "Dwgs.User"

def line(x1, y1, x2, y2, width, layer):
	print(f"  (fp_line (start {x1:.5g} {y1:.5g}) (end {x2:.5g} {y2:.5g}) (width {width}) (layer {layer}))")

def fabRect(x1, y1, x2, y2):
	d = min(abs(x2 - x1), abs(y2 - y1)) * 0.2
	x = x1 + (d if x2 > x1 else -d)
	y = y1 + (d if y2 > y1 else -d)
	line(x1, y, x, y1, 0.1, "F.Fab")
	line(x, y1, x2, y1, 0.1, "F.Fab")
	line(x2, y1, x2, y2, 0.1, "F.Fab")
	line(x2, y2, x1, y2, 0.1, "F.Fab")
	line(x1, y2, x1, y, 0.1, "F.Fab")

def courtyardRect(x1, y1, x2, y2):
	line(x1, y1, x2, y1, 0.05, "F.CrtYd")
	line(x2, y1, x2, y2, 0.05, "F.CrtYd")
	line(x2, y2, x1, y2, 0.05, "F.CrtYd")
	line(x1, y2, x1, y1, 0.05, "F.CrtYd")

def silkScreenRect(x1, y1, x2, y2):
	d = min(abs(x2 - x1), abs(y2 - y1)) * 0.2
	x = x1 + (d if x2 > x1 else -d)
	y = y1 + (d if y2 > y1 else -d)
	line(x1, y1, x, y1, 0.2, "F.SilkS")
	line(x1, y1, x1, y, 0.2, "F.SilkS")
	line(x+0.4, y1, x2, y1, 0.1, "F.SilkS")
	line(x2, y1, x2, y2, 0.1, "F.SilkS")
	line(x2, y2, x1, y2, 0.1, "F.SilkS")
	line(x1, y2, x1, y+0.4, 0.1, "F.SilkS")	

def userRect(x1, y1, x2, y2):
	line(x1, y1, x2, y1, userWidth, userLayer)
	line(x2, y1, x2, y2, userWidth, userLayer)
	line(x2, y2, x1, y2, userWidth, userLayer)
	line(x1, y2, x1, y1, userWidth, userLayer)

def pad(index, type, shape, x, y, width, height, layers, drill = 0, offsetX = 0, offsetY = 0):
	print(f"  (pad {index} {type}", end = '')
	if shape <= RECT:
		print(f" rect", end = '')
	elif shape >= CIRCLE:
		if width == height:
			print(f" circle", end = '')
		else:
			print(f" oval", end = '')
	elif shape == ROUNDRECT:
		print(f" roundrect", end = '')
	else:
		print(f" roundrect (roundrect_rratio {shape})", end = '')
	print(f" (at {x:.5g} {y:.5g}) (size {width:.5g} {height:.5g})", end = '')
	if drill > 0:
		print(f" (drill {drill:.5g}", end = '')
		if offsetX != 0 or offsetY != 0:	
			print(f" (offset {offsetX:.5g} {offsetY:.5g})", end = '')
		print(f")", end = '')
	print(f" (layers {layers}))")

def thruHolePad(index, shape, x, y, width, height, drill, offsetX = 0, offsetY = 0, layers = "*.Cu *.Mask"):
	pad(index, "thru_hole", shape, x, y, width, height, layers, drill, offsetX, offsetY)

def smdPad(index, shape, x, y, width, height, layers = "F.Cu F.Mask F.Paste"):
	pad(index, "smd", shape, x, y, width, height, layers)

def exposedPad(index, shape, x, y, width, height):
	pad(index, "smd", shape, x, y, width, height, "F.Cu F.Mask")
	nx = int(round(width / 1.25))
	ny = int(round(height / 1.25))
	w = width/nx
	h = height/ny
	for j in range(0, ny):
		for i in range(0, nx):
			pad('""', "smd", 0.25, x+(i-(nx-1)*0.5)*w, y+(j-(ny-1)*0.5)*h, w-0.3, h-0.3, "F.Paste")

def npthPad(x, y, width, height, clearance = 0):
	print('  (pad "" np_thru_hole ', end = '')
	if width == height:
		print(f" circle (at {x:.5g} {y:.5g}) (size {width:.5g} {height:.5g}) (drill {width:.5g})", end = '')
	else:
		print(f" oval (at {x:.5g} {y:.5g}) (size {width:.5g} {height:.5g}) (drill oval {width:.5g} {height:.5g})", end = '')
	if clearance > 0:
		print(f" (clearance {clearance})", end = '')
	print(f" (layers *.Cu *.Mask))")


print("E73-2G4M08S1C-52840")

# pad
size = 0.8
outerSize = 1.4
outerOffset = -1.0 / 2 + (outerSize - 1) / 2
bottomSize = 0.6
shape = 0.05 / size
pitch = 1.27
drill = 0.3

# bunding box
right = 13.0 / 2.0
left = -right
top = -(1.27 * 4.5 + 2.6)
bottom = 1.27 * 4.5 + 3.97
fabRect(right, bottom, left, top)
courtyardRect((left - 0.1), (top - 0.1), (right + 0.1), (bottom + 0.1))

# right
for i in range(0, 10):
	smdPad(1 + i, shape, (right + outerOffset), (4.5 - i) * pitch, outerSize, size)

# top
for i in range(0, 8):
	smdPad(11 + i * 2, shape, (3.5 - i) * pitch, (top - outerOffset), size, outerSize)
for i in range(0, 7):
	smdPad(12 + i * 2, shape, (3 - i) * pitch, (top + 2.1), size, size)
	thruHolePad(12 + i * 2, ROUNDRECT, (3 - i) * pitch, (top + 2.1), bottomSize, size, drill)

# left
for i in range(0, 10):
	smdPad(26 if i == 0 else 25 + i * 2, shape, (left - outerOffset), (-4.5 + i) * pitch, outerSize, size)
for i in range(0, 8):
	smdPad(28 + i * 2, shape, (left + 2.1), (-3 + i) * pitch, size, size)
	thruHolePad(28 + i * 2, ROUNDRECT, (left + 2.1), (-3 + i) * pitch, size, bottomSize, drill)


for pinCount in [7, 10]:
	print(f"Wago_236-4{pinCount:02d}")
	
	# pad
	size = 2
	drill = 1.1
	pitch = 5
	
	# bounding box
	right = 6.5
	left = -7.5
	top = -(2.5 + 1.3)
	bottom = top + pinCount * pitch + 2.3
	fabRect(left, top, right-1.5, bottom)
	courtyardRect(left, top, right, bottom)
	silkScreenRect(left, top, right, bottom)
	
	# pads
	for i in range(0, pinCount):
		thruHolePad(1 + i, ROUNDRECT, -2.5, i * pitch, size, size, drill)
		thruHolePad(1 + i, ROUNDRECT, 2.5, i * pitch, size, size, drill)


print("On_NCV7726B")

# pad
width = 1.15
height = 0.40
pitch = 0.65

# exposed pad
epWidth = 2.8
epHeight = 5.6
epShape = 0.05 / epWidth

# bounding box
right = 6.40 / 2
left = -right
bottom = (11 * pitch + height) / 2
top = -bottom
E1 = 3.90
D = 8.64
fabRect(-E1/2, -D/2, E1/2, D/2)
courtyardRect(left, -D/2, right, D/2)
silkScreenRect(-E1/2, -D/2, E1/2, D/2)

# pads
for i in range(0, 12):
	smdPad(1 + i, ROUNDRECT, left + width / 2, top + height / 2 + i * pitch, width, height)
	smdPad(24 - i, ROUNDRECT, right - width / 2, top + height / 2 + i * pitch, width, height)
exposedPad(25, epShape, 0, 0, epWidth, epHeight)


print("Texas_TPS82130")

# pad
margin = 0.025 # copper larger than masks
width = 0.5 + 2 * margin
height = 0.4 + 2 * margin
shape = 0.05 / height
pitch = 0.65

# exposed pad
epWidth = 1.1 + 2 * margin
epHeight = 1.9 + 2 * margin
epShape = 0.05 / epWidth
epDrill = 0.3

# bounding box
right = 2.8 / 2
left = -right
bottom = 3.0 / 2
top = -bottom
fabRect(left, top, right, bottom)
courtyardRect(left, top, right, bottom)
silkScreenRect(left, top, right, bottom)

# pads
for i in range(0, 4):
	smdPad(1 + i, shape, -2.1 / 2, (-1.5 + i) * pitch, width, height)
	smdPad(8 - i, shape, 2.1 / 2, (-1.5 + i) * pitch, width, height)
exposedPad(9, epShape, 0, 0, epWidth, epHeight)
for i in range(0, 3):
	thruHolePad(9, CIRCLE, 0, (-1 + i) * 0.75, epDrill + 0.1, epDrill + 0.1, epDrill, layers="*.Cu")


print("Omron_G6SU-2")

# pad
offset = 0.15
width = 1.4 + offset * 2
height = 1.4
drill = 1

# bounding box
right = 2.54 + 1.11
left = -2.54 - 1.11
top = -1.05
bottom = top + 14.8
fabRect(left, top, right, bottom)
courtyardRect(left, top, right, bottom)
silkScreenRect(left, top, right, bottom)

# pads
for i in range(0, 5):
	if i != 1:
		thruHolePad(1 + i, ROUNDRECT, -2.54, i * 2.54, width, height, drill, offsetX=-offset)
		thruHolePad(12 - i, ROUNDRECT, 2.54, i * 2.54, width, height, drill, offsetX=offset)


print("Omron_D2F-01")

# pad
width = 2
height = 1.5
drill = 1.1
pitch = 5.08

# bounding box
right = 5.8 / 2
left = -right
bottom = 12.8 / 2
top = -bottom
fabRect(left, top, right, bottom)
courtyardRect(left, top, right, bottom)
silkScreenRect(left, top, right, bottom)

thruHolePad(3, ROUNDRECT, 0, 0, width, height, drill)
thruHolePad(2, ROUNDRECT, 0, -pitch, width, height, drill)
thruHolePad(1, ROUNDRECT, 0, pitch, width, height, drill)


print("Switch_Mount")

def switchMountDrawing(coords):
	# mirror at x/y
	i = len(coords)
	while i > 0:
		i -= 1
		(x, y) = coords[i]
		coords += [(y, x)]
	
	# mirror at y
	i = len(coords)
	while i > 0:
		i -= 1
		(x, y) = coords[i]
		coords += [(-x, y)]
	
	# mirror at x
	i = len(coords)
	while i > 0:
		i -= 1
		(x, y) = coords[i]
		coords += [(x, -y)]
	
	for i in range(0, len(coords)):
		line(*coords[i], *coords[(i+1) % len(coords)], userWidth, userLayer)
	
# mounting hole
width = 3
height = 7

npthPad(30, 0, width, height, 2)
npthPad(0, 30, height, width, 2)
npthPad(-30, 0, width, height, 2)
npthPad(0, -30, height, width, 2)
npthPad(20.5, -8.5, 3, 3, 1.5)
npthPad(-20.5, -8.5, 3, 3, 1.5)

switchMountDrawing([(25.5, 7), (22.8, 7), (22.8, 13.2), (24, 13.2), (24, 18.2), (23, 18.2)])
switchMountDrawing([(25.5, 7), (22.8, 7), (22.8, 13.2), (22, 13.2), (22, 17.8)])

userRect(24, 4, -24, 7)
userRect(24, -4, -24, -7)
userRect(5.6/2, 34.4/2, -5.6/2, 40/2)
userRect(5.6/2, -34.4/2, -5.6/2, -40/2)
