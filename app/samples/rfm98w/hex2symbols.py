import sys
if len(sys.argv) < 2:
    raise Exception("usage: ./script [hex string]")
hexStr = sys.argv[1]
if len(hexStr) % 2 == 1:
    raise Exception("bad input")

hs = [hexStr[i:i+2] for i in range(0, len(hexStr), 2)]
eyes = [int(h, 16) for h in hs]

syms = ([[((byte >> bi) & 0x3) for bi in range(6, -1, -2)] for byte in eyes])


print("hex: ", '   '.join([f"{e:02x}" for e in eyes]))
print("sym: ", ' '.join([''.join([str(s) for s in hsyms]) for hsyms in syms]))
print(syms)
