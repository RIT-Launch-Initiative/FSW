import struct
import sys
import matplotlib.pyplot as plt

STRUCT_FORMAT = '<I' + 'f'*17
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)

def parse_record(data):
    fields = struct.unpack(STRUCT_FORMAT, data)
    record = {
        'timestamp': fields[0],
        'PrimaryBarometer_Pressure': fields[1],
        'PrimaryBarometer_Temperature': fields[2],
        'SecondaryBarometer_Pressure': fields[3],
        'SecondaryBarometer_Temperature': fields[4],
        'Acceleration_X': fields[5],
        'Acceleration_Y': fields[6],
        'Acceleration_Z': fields[7],
        'ImuAcceleration_X': fields[8],
        'ImuAcceleration_Y': fields[9],
        'ImuAcceleration_Z': fields[10],
        'ImuGyroscope_X': fields[11],
        'ImuGyroscope_Y': fields[12],
        'ImuGyroscope_Z': fields[13],
        'Magnetometer_X': fields[14],
        'Magnetometer_Y': fields[15],
        'Magnetometer_Z': fields[16],
        'Temperature': fields[17],
    }
    return record

def scan_file(filename):
    records = []
    with open(filename, 'rb') as f:
        offset = 0
        while True:
            chunk = f.read(STRUCT_SIZE)
            if len(chunk) < STRUCT_SIZE:
                break
            record = parse_record(chunk)
            if (
                1_000_000 < record['timestamp'] < 4_294_967_295 and
                62.28 - 10 <= record['PrimaryBarometer_Pressure'] <= 100 and
                (2.109 - 5) <= record['PrimaryBarometer_Temperature'] <= (29.281 + 10) and
                62.28 - 10 <= record['SecondaryBarometer_Pressure'] <= 100 and
                (2.109 - 5) <= record['SecondaryBarometer_Temperature'] <= (29.281 + 10) and
                -19.668 <= abs(record['Acceleration_X']) <= 218.881 and
                -19.668 <= abs(record['ImuAcceleration_X']) <= 218.881 and
                -19.668 <= abs(record['Acceleration_Y']) <= 218.881 and
                -19.668 <= abs(record['ImuAcceleration_Y']) <= 218.881 and
                -19.668 <= abs(record['Acceleration_Z']) <= 218.881 and
                -19.668 <= abs(record['ImuAcceleration_Z']) <= 218.881
            ):
                records.append(record)
            offset += STRUCT_SIZE
    return records

def plot_graphs(records):
    if not records:
        print("No records to plot.")
        return
    import numpy as np
    fields = list(records[0].keys())
    fields.remove('timestamp')
    timestamps = [r['timestamp'] for r in records]
    for field in fields:
        values = [r[field] for r in records]
        plt.figure()
        plt.plot(timestamps, values)
        plt.title(field)
        plt.xlabel('Timestamp')
        plt.ylabel(field)
        plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: python data_finder.py <flash_dump.bin>")
        sys.exit(1)
    records = scan_file(sys.argv[1])
    plot_graphs(records)
