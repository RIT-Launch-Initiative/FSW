import pandas as pd
import os
import sys
import struct
print("Waiting for pipe")
while True:
	try:
		s = input()
	except:
		print("all done")
		break
	parts = s.split(',')
	time = parts[0]
	raw = parts[14]
	rb = bytes.fromhex(raw)
	custom = [str(int(b)) for b in rb[-11:-2]]
	x, y, z, _, press = struct.unpack('bbbBH',rb[-11:-5])
	print(time, x,y,z, press,  ' | ', ' '.join(custom))
