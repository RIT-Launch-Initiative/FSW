import sys
import re
import itertools
import struct
import base64
import os


if (len(sys.argv)) !=4 :
	print("usage: log_file_in param_file_out data_file_out", file=sys.stderr)
	sys.exit(-1)


path = sys.argv[1]
param_path = sys.argv[2]
data_path = sys.argv[3]

PSTART='----++++//[[( params start )]]\\\\++++----'
PEND='----++++//[[( params end )]]\\\\++++----'

DSTART='----++++//[[( data start )]]\\\\++++----'
DEND='----++++//[[( data end )]]\\\\++++----'



param_fmt = 'IIffffIIIIIIbbbb'
param_fields = ['magic', 'timstamp_ms', 'pressure_kpa','bias_x_dps', 'bias_y_dps', 'bias_z_dps', 'bootcount', 'lockout_ms', 'num_flight_packets', 'num_preboost_packets', 'num_gyro_bias_packets', 'controller_hash', 'up_axis', 'd1', 'd2', 'd3']


up_axis_mapping_l = [
    "PosX",
    "NegX",

    "PosY",
    "NegY",

    "PosZ",
    "NegZ",

    "PosXPosY",
    "PosXNegY",

    "NegXPosY",
    "NegXNegY",
]

def map_up_axis(b):
	if b >= len(up_axis_mapping_l):
		raise 'bad up axis mapping error'
	return up_axis_mapping_l

data_fmt = 'Ifffffffffffffffff'
data_size = struct.calcsize(data_fmt)
data_fields = ['timestamp_ms', 'temp_c', 'pressure_kpa', 'accel_x_m_s2', 'accel_y_m_s2', 'accel_z_m_s2', 'gyro_x_dps', 'gyro_y_dps', 'gyro_z_dps','e_alt_m', 'e_vel_m_s', 'e_acc_m_s2', 'e_bias', 'qa', 'qb', 'qc', 'qd', 'effort']

if len(param_fmt) != len(param_fields):
	print("Param packet and labels mismatch", file=sys.stderr)
	sys.exit(-1)


if len(data_fmt) != len(data_fields):
	print("Data packet and labels mismatch", file=sys.stderr)
	sys.exit(-1)


def gen(start, end, lines):
	do = False
	for line in lines:
		if end in line:
			break
		if do:
			yield line.replace('\n', "")
		if start in line:
			do = True


with open(path, 'r') as fp:
	lines = fp.readlines()
	params = "".join([line for line in gen(PSTART, PEND, lines)])
	data= "".join([line for line in gen(DSTART, DEND, lines)])

if len(params) > 0:
	binary = base64.standard_b64decode(bytes(params, 'utf-8'))
	paramsS = struct.unpack(param_fmt, binary)
	with open(param_path, 'w') as f:
		f.write(", ".join(param_fields))
		f.write(", ".join([str(p) for p in paramsS]))
else:
	print("No Params Found", file=sys.stderr)


if len(data) > 0:
	binary = base64.standard_b64decode(bytes(data, 'utf-8'))
	num_whole_pacs = len(binary)//data_size
	datas= struct.iter_unpack(data_fmt, binary[:num_whole_pacs * data_size])
	with open(data_path, 'w') as f:
		f.write(", ".join(data_fields)+"\n")
		for data in datas:
			f.write(", ".join([str(d) for d in data])+"\n")
else:
	print("No Data Found", file=sys.stderr)
