import struct
import sys

# Define the binary structure based on the packed attribute
# Each ShuntData is three floats (Voltage, Current, Power)
SHUNT_DATA_FORMAT = "fff"
SENSOR_DATA_FORMAT = SHUNT_DATA_FORMAT * 3  # 3 ShuntData structures in SensorData
SENSOR_DATA_SIZE = struct.calcsize(SENSOR_DATA_FORMAT)

def parse_sensor_data(binary_file):
    with open(binary_file, "rb") as file:
        data = file.read()

    num_records = len(data) // SENSOR_DATA_SIZE
    print(f"Number of SensorData records: {num_records}")

    for i in range(num_records):
        offset = i * SENSOR_DATA_SIZE
        record = data[offset:offset + SENSOR_DATA_SIZE]
        unpacked = struct.unpack(SENSOR_DATA_FORMAT, record)

        # Separate into individual ShuntData entries
        rail_battery = unpacked[:3]
        rail_3v3 = unpacked[3:6]
        rail_5v0 = unpacked[6:9]

        print(f"Record {i + 1}:")
        print(f"  RailBattery - Voltage: {rail_battery[0]:.2f}, Current: {rail_battery[1]:.2f}, Power: {rail_battery[2]:.2f}")
        print(f"  Rail3v3     - Voltage: {rail_3v3[0]:.2f}, Current: {rail_3v3[1]:.2f}, Power: {rail_3v3[2]:.2f}")
        print(f"  Rail5v0     - Voltage: {rail_5v0[0]:.2f}, Current: {rail_5v0[1]:.2f}, Power: {rail_5v0[2]:.2f}")
        print()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <binary_file>")
        sys.exit(1)

    binary_file = sys.argv[1]
    parse_sensor_data(binary_file)

