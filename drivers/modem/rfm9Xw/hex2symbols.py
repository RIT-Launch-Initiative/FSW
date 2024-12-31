hexStr = '7a2ab'
if len(hexStr) % 2 == 1:
    print('invalid length input')
    raise Exception("bad input")

hs = [hexStr[i:i+2] for i in range(0, len(hexStr), 2)]
eyes = [int(h, 16) for h in hs]
print(eyes)
