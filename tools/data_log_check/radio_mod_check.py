import struct
import sys

GNSS_DATA_FORMAT = "fffIIBBHB"
GNSS_LOGGING_DATA_FORMAT = "I" + GNSS_DATA_FORMAT
SENSOR_DATA_SIZE = struct.calcsize(GNSS_LOGGING_DATA_FORMAT)

def parse_gnss_data(binary_file):
    with open(binary_file, "rb") as file:
        data = file.read()

    file_size_bytes = len(data)
    # Convert to megabits (Mb). 1 byte = 8 bits. Divide by 1_000_000 for Mb (decimal)
    file_size_mbits = (file_size_bytes * 8) / 1_000_000.0

    num_records = file_size_bytes // SENSOR_DATA_SIZE
    print(f"Number of GnssLoggingData records: {num_records}")
    print(f"File size: {file_size_mbits:.3f} Mbits\n")

    kml_placemarks = []

    for i in range(num_records):
        offset = i * SENSOR_DATA_SIZE
        record_data = data[offset:offset + SENSOR_DATA_SIZE]

        unpacked = struct.unpack(GNSS_LOGGING_DATA_FORMAT, record_data)

        systemTime = unpacked[0]

        # GnssData fields
        latitude = unpacked[1]
        longitude = unpacked[2]
        altitude = unpacked[3]

        gnss_time = unpacked[4]
        gnss_date = unpacked[5]

        satelliteId = unpacked[6]
        elevation = unpacked[7]
        azimuth = unpacked[8]
        snr = unpacked[9]

        print(f"Record {i+1}:")
        print(f"  SystemTime: {systemTime}")
        print(f"  Coordinates - Lat: {latitude:.6f}, Lon: {longitude:.6f}, Alt: {altitude:.2f}")
        print(f"  GNSS Time: {gnss_time}, GNSS Date: {gnss_date}")
        print(f"  GNSS Info - SatID: {satelliteId}, Elevation: {elevation}, Azimuth: {azimuth}, SNR: {snr}")
        print()

        placemark = f"""
        <Placemark>
            <name>Record {i+1}</name>
            <description>SystemTime: {systemTime}, SatID: {satelliteId}, Elev: {elevation}, Az: {azimuth}, SNR: {snr}</description>
            <Point>
                <coordinates>{longitude},{latitude},{altitude}</coordinates>
            </Point>
        </Placemark>
        """
        kml_placemarks.append(placemark.strip())

    generate_kml(kml_placemarks, "output.kml")
    print("KML file 'output.kml' generated successfully.")

def generate_kml(placemarks, kml_file):
    kml_header = """<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
<Document>"""
    kml_footer = """</Document>
</kml>"""

    with open(kml_file, "w") as f:
        f.write(kml_header + "\n")
        for pm in placemarks:
            f.write(pm + "\n")
        f.write(kml_footer)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <binary_file>")
        sys.exit(1)

    binary_file = sys.argv[1]
    parse_gnss_data(binary_file)
