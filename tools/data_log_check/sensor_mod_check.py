import struct
import socket
import sys
import matplotlib.pyplot as plt

# Define the binary structure
BAROMETER_DATA_FORMAT = "ff"        # Pressure, Temperature
ACCELEROMETER_DATA_FORMAT = "fff"   # X, Y, Z
GYROSCOPE_DATA_FORMAT = "fff"       # X, Y, Z
MAGNETOMETER_DATA_FORMAT = "fff"    # X, Y, Z
TEMPERATURE_DATA_FORMAT = "f"       # Temperature (float)

SENSOR_DATA_FORMAT = (
    BAROMETER_DATA_FORMAT * 2 +       # Primary and Secondary Barometers
    ACCELEROMETER_DATA_FORMAT +       # Acceleration
    ACCELEROMETER_DATA_FORMAT +       # IMU Acceleration
    GYROSCOPE_DATA_FORMAT +           # IMU Gyroscope
    MAGNETOMETER_DATA_FORMAT +        # Magnetometer
    TEMPERATURE_DATA_FORMAT           # Temperature
)

SENSOR_DATA_SIZE = struct.calcsize(SENSOR_DATA_FORMAT)
UDP_PORT = 12100  # UDP Broadcast Port

def parse_and_broadcast_sensor_data(binary_file, udp_ip="255.255.255.255"):
    # Setup UDP broadcast socket
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    with open(binary_file, "rb") as file:
        data = file.read()

    num_records = len(data) // SENSOR_DATA_SIZE
    print(f"Number of SensorData records: {num_records}")

    # Data storage for plotting
    records = {
        "PrimaryBarometer_Pressure": [],
        "PrimaryBarometer_Temperature": [],
        "SecondaryBarometer_Pressure": [],
        "SecondaryBarometer_Temperature": [],
        "Acceleration_X": [],
        "Acceleration_Y": [],
        "Acceleration_Z": [],
        "IMU_Acceleration_X": [],
        "IMU_Acceleration_Y": [],
        "IMU_Acceleration_Z": [],
        "IMU_Gyroscope_X": [],
        "IMU_Gyroscope_Y": [],
        "IMU_Gyroscope_Z": [],
        "Magnetometer_X": [],
        "Magnetometer_Y": [],
        "Magnetometer_Z": [],
        "Temperature": [],
    }

    for i in range(num_records):
        offset = i * SENSOR_DATA_SIZE
        record = data[offset:offset + SENSOR_DATA_SIZE]
        unpacked = struct.unpack(SENSOR_DATA_FORMAT, record)

        # Extract individual structures
        primary_barometer = unpacked[:2]
        secondary_barometer = unpacked[2:4]
        acceleration = unpacked[4:7]
        imu_acceleration = unpacked[7:10]
        imu_gyroscope = unpacked[10:13]
        magnetometer = unpacked[13:16]
        temperature = unpacked[16]

        # Store data for plotting
        records["PrimaryBarometer_Pressure"].append(primary_barometer[0])
        records["PrimaryBarometer_Temperature"].append(primary_barometer[1])
        records["SecondaryBarometer_Pressure"].append(secondary_barometer[0])
        records["SecondaryBarometer_Temperature"].append(secondary_barometer[1])
        records["Acceleration_X"].append(acceleration[0])
        records["Acceleration_Y"].append(acceleration[1])
        records["Acceleration_Z"].append(acceleration[2])
        records["IMU_Acceleration_X"].append(imu_acceleration[0])
        records["IMU_Acceleration_Y"].append(imu_acceleration[1])
        records["IMU_Acceleration_Z"].append(imu_acceleration[2])
        records["IMU_Gyroscope_X"].append(imu_gyroscope[0])
        records["IMU_Gyroscope_Y"].append(imu_gyroscope[1])
        records["IMU_Gyroscope_Z"].append(imu_gyroscope[2])
        records["Magnetometer_X"].append(magnetometer[0])
        records["Magnetometer_Y"].append(magnetometer[1])
        records["Magnetometer_Z"].append(magnetometer[2])
        records["Temperature"].append(temperature)

        # Print the unpacked data
        print(f"Record {i + 1}:")
        print(f"  PrimaryBarometer - Pressure: {primary_barometer[0]:.2f}, Temperature: {primary_barometer[1]:.2f}")
        print(f"  SecondaryBarometer - Pressure: {secondary_barometer[0]:.2f}, Temperature: {secondary_barometer[1]:.2f}")
        print(f"  Acceleration - X: {acceleration[0]:.2f}, Y: {acceleration[1]:.2f}, Z: {acceleration[2]:.2f}")
        print(f"  IMU Acceleration - X: {imu_acceleration[0]:.2f}, Y: {imu_acceleration[1]:.2f}, Z: {imu_acceleration[2]:.2f}")
        print(f"  IMU Gyroscope - X: {imu_gyroscope[0]:.2f}, Y: {imu_gyroscope[1]:.2f}, Z: {imu_gyroscope[2]:.2f}")
        print(f"  Magnetometer - X: {magnetometer[0]:.2f}, Y: {magnetometer[1]:.2f}, Z: {magnetometer[2]:.2f}")
        print(f"  Temperature: {temperature:.2f}")
        print()

        # Broadcast the binary data over UDP
        udp_socket.sendto(record, (udp_ip, UDP_PORT))

    udp_socket.close()
    print("Finished broadcasting all SensorData records over UDP.")

    # Generate graphs
    plot_data(records)

def plot_data(records):
    plt.figure(figsize=(15, 10))

    # Plot each dataset
    plt.subplot(2, 2, 1)
    plt.plot(records["PrimaryBarometer_Pressure"], label="Primary Barometer Pressure")
    plt.plot(records["SecondaryBarometer_Pressure"], label="Secondary Barometer Pressure")
    plt.xlabel("Record Index")
    plt.ylabel("Pressure (hPa)")
    plt.legend()
    plt.title("Barometer Pressure")

    plt.subplot(2, 2, 2)
    plt.plot(records["Temperature"], label="Temperature", color="orange")
    plt.xlabel("Record Index")
    plt.ylabel("Temperature (°C)")
    plt.legend()
    plt.title("Temperature Data")

    plt.subplot(2, 2, 3)
    plt.plot(records["Acceleration_X"], label="Acceleration X")
    plt.plot(records["Acceleration_Y"], label="Acceleration Y")
    plt.plot(records["Acceleration_Z"], label="Acceleration Z")
    plt.xlabel("Record Index")
    plt.ylabel("Acceleration (m/s²)")
    plt.legend()
    plt.title("Acceleration Data")

    plt.subplot(2, 2, 4)
    plt.plot(records["Magnetometer_X"], label="Magnetometer X")
    plt.plot(records["Magnetometer_Y"], label="Magnetometer Y")
    plt.plot(records["Magnetometer_Z"], label="Magnetometer Z")
    plt.xlabel("Record Index")
    plt.ylabel("Magnetic Field (µT)")
    plt.legend()
    plt.title("Magnetometer Data")

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <binary_file>")
        sys.exit(1)

    binary_file = sys.argv[1]
    parse_and_broadcast_sensor_data(binary_file)

