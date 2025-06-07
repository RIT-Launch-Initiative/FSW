import pandas as pd
import os
import sys
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
	print(time, ' '.join(custom))
