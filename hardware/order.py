#!/usr/bin/env python3
import sys, re

if (len(sys.argv) < 2):
	print(f"usage: order.py <inventory>")
	quit()

inventoryPath = sys.argv[1]
print(f"inventory: {inventoryPath}")

def readList(path, list):
	with open(path) as f:
			
		ex = re.compile("^\s*([0-9]+)\s+([^\(]+)")
	
		for line in f:
			if line.startswith('['):
				return
			
			m = ex.match(line)
			
			if m:
				count, item = m.groups()
				item = item.strip()
				
				#print(f"count {count}, item {item}!")
				if item in list:
					list[item] += int(count)
				else:
					list[item] = int(count)

# read inventory								
inventory = {}				
readList("inventory.txt", inventory)

#for item in inventory:
#	print(f"{inventory[item]} {item}")

# read boms
bom = {}
readList("main/bom.txt", bom)
readList("switch/bom.txt", bom)

# print items to order
print(f"items to order:")
for item in bom:
	count = bom[item]
	if item in inventory:
		count -= inventory[item]
	if count > 0:
		print(f"{count} {item}")
