import struct
import sys
import matplotlib.pyplot as plt
import argparse
from collections import defaultdict
import os
import numpy as np


STRUCT_FORMAT = '<I' + 'f'*17
STRUCT_SIZE = struct.calcsize(STRUCT_FORMAT)

# Flight detection parameters
TIMESTAMP_GAP_THRESHOLD = 60000

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

def identify_flights(records):
    if not records:
        return {}

    # Sort records by timestamp to ensure proper sequencing
    sorted_records = sorted(records, key=lambda r: r['timestamp'])

    flights = defaultdict(list)
    current_flight = 0
    last_timestamp = sorted_records[0]['timestamp']

    # Add the first record to the first flight
    flights[current_flight].append(sorted_records[0])

    for record in sorted_records[1:]:
        time_gap = record['timestamp'] - last_timestamp

        if time_gap > TIMESTAMP_GAP_THRESHOLD:
            current_flight += 1
            print(f"Detected new flight at timestamp {record['timestamp']} (gap: {time_gap} ms)")

        flights[current_flight].append(record)
        last_timestamp = record['timestamp']

    print(f"Identified {len(flights)} potential flight(s)")
    for flight_num, flight_records in flights.items():
        start_time = flight_records[0]['timestamp']
        end_time = flight_records[-1]['timestamp']
        duration = (end_time - start_time) / 1000  # Convert to seconds
        print(f"  Flight {flight_num}: {len(flight_records)} records, duration: {duration:.2f} seconds")

    return flights

def plot_graphs(records, flight_num=None, output_dir=None):
    if not records:
        print("No records to plot.")
        return
    fields = list(records[0].keys())
    fields.remove('timestamp')
    timestamps = [r['timestamp'] for r in records]

    if output_dir:
        flight_dir = os.path.join(output_dir, f"flight_{flight_num}" if flight_num is not None else "all_flights")
        os.makedirs(flight_dir, exist_ok=True)
        print(f"Saving plots to: {flight_dir}")

    for field in fields:
        values = [r[field] for r in records]
        plt.figure()
        plt.plot(timestamps, values)
        title = field
        if flight_num is not None:
            title += f" (Flight {flight_num})"
        plt.title(title)
        plt.xlabel('Timestamp')
        plt.ylabel(field)
        plt.tight_layout()

        if output_dir:
            filename = f"{field.replace('_', '-')}.png"
            filepath = os.path.join(flight_dir, filename)
            plt.savefig(filepath)
            print(f"Saved: {filepath}")
            plt.close()

    if not output_dir:
        plt.show()

def plot_all_flights(flights, output_dir=None):
    if not flights:
        print("No flights to plot.")
        return

    first_flight_records = next(iter(flights.values()))
    if not first_flight_records:
        print("No records in the first flight.")
        return

    fields = list(first_flight_records[0].keys())
    fields.remove('timestamp')

    colors = plt.cm.tab10.colors

    if output_dir:
        combined_dir = os.path.join(output_dir, "combined_flights")
        os.makedirs(combined_dir, exist_ok=True)
        print(f"Saving combined plots to: {combined_dir}")

    for field in fields:
        plt.figure(figsize=(12, 6))

        for flight_num, records in flights.items():
            if not records:
                continue

            timestamps = [r['timestamp'] for r in records]
            values = [r[field] for r in records]

            t0 = timestamps[0]
            norm_timestamps = [(t - t0) / 1000 for t in timestamps]

            color_idx = flight_num % len(colors)
            plt.plot(norm_timestamps, values, color=colors[color_idx],
                     label=f"Flight {flight_num}")

        plt.title(f"{field} - All Flights")
        plt.xlabel('Time (seconds from flight start)')
        plt.ylabel(field)
        plt.legend()
        plt.tight_layout()

        if output_dir:
            filename = f"{field.replace('_', '-')}_combined.png"
            filepath = os.path.join(combined_dir, filename)
            plt.savefig(filepath)
            print(f"Saved: {filepath}")
            plt.close()

    if not output_dir:
        plt.show()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Process sensor data from flash dump')
    parser.add_argument('file', help='Flash dump binary file')
    parser.add_argument('--flight', type=int, help='Specific flight number to plot (default: all flights)')
    parser.add_argument('--combined', action='store_true', help='Plot all flights on the same graphs')
    parser.add_argument('--gap', type=int, default=TIMESTAMP_GAP_THRESHOLD,
                        help=f'Timestamp gap threshold in ms (default: {TIMESTAMP_GAP_THRESHOLD})')
    parser.add_argument('--output', help='Directory to save plots instead of displaying them')

    args = parser.parse_args()

    TIMESTAMP_GAP_THRESHOLD = args.gap

    records = scan_file(args.file)
    if not records:
        print("No valid records found.")
        sys.exit(1)

    # Identify separate flights
    flights = identify_flights(records)

    output_dir = args.output
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
        print(f"Output directory: {output_dir}")

    if args.flight is not None:
        if args.flight in flights:
            print(f"Plotting flight {args.flight} with {len(flights[args.flight])} records")
            plot_graphs(flights[args.flight], args.flight, output_dir)
        else:
            print(f"Flight {args.flight} not found. Available flights: {sorted(flights.keys())}")
    elif args.combined:
        plot_all_flights(flights, output_dir)
    else:
        for flight_num, flight_records in flights.items():
            print(f"Plotting flight {flight_num} with {len(flight_records)} records")
            plot_graphs(flight_records, flight_num, output_dir)
