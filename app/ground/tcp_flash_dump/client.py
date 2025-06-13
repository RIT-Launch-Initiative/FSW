#!/usr/bin/env python3
"""
Flash Reader Client - Connects to the TCP server on the embedded device
and receives the flash dump data.

Usage: python client.py [ip_address] [port] [output_file]
"""

import socket
import struct
import sys
import time
import os

DEFAULT_IP = "10.0.0.2"
DEFAULT_PORT = 9000
DEFAULT_OUTPUT = "flash_dump.bin"

def receive_flash_data(ip_addr, port, output_file):
    """Connect to server and receive flash data"""
    print(f"Connecting to {ip_addr}:{port}...")

    # Create socket and connect
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((ip_addr, port))
        print("Connected successfully!")

        # First receive the total size (4 bytes)
        size_data = s.recv(4)
        if not size_data or len(size_data) != 4:
            print("Error: Failed to receive size data")
            return False

        total_size = struct.unpack("<I", size_data)[0]
        print(f"Expected flash size: {total_size} bytes")

        # Prepare to receive data
        bytes_received = 0
        with open(output_file, 'wb') as f:
            print("Receiving...")
            while bytes_received < total_size:
                # Receive data in small chunks
                chunk = s.recv(4096)  # Receive up to 4KB at a time
                if not chunk:
                    # Connection closed prematurely
                    print(f"\nWarning: Connection closed after receiving "
                          f"{bytes_received}/{total_size} bytes")
                    break

                # Write received data to file
                f.write(chunk)
                chunk_size = len(chunk)
                bytes_received += chunk_size
                print(f"Received {bytes_received}/{total_size} bytes", end='\r')

        print()  # Newline after progress output

        # Verify received size
        if bytes_received == total_size:
            print(f"Successfully received {bytes_received} bytes. "
                  f"Data saved to {output_file}")
            return True
        else:
            print(f"Warning: Received {bytes_received}/{total_size} bytes")
            return False

    except ConnectionRefusedError:
        print("Error: Connection refused. Make sure the server is running.")
    except Exception as e:
        print(f"Error: {str(e)}")
    finally:
        s.close()

    return False

def main():
    ip_addr = DEFAULT_IP
    port = DEFAULT_PORT
    output_file = DEFAULT_OUTPUT

    # Parse command line arguments
    if len(sys.argv) > 1:
        ip_addr = sys.argv[1]
    if len(sys.argv) > 2:
        try:
            port = int(sys.argv[2])
        except ValueError:
            print(f"Error: Invalid port number '{sys.argv[2]}'")
            return 1
    if len(sys.argv) > 3:
        output_file = sys.argv[3]

    # Create output directory if it doesn't exist
    output_dir = os.path.dirname(output_file)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Receive the flash data
    start_time = time.time()
    success = receive_flash_data(ip_addr, port, output_file)
    end_time = time.time()

    if success:
        elapsed = end_time - start_time
        size_mb = os.path.getsize(output_file) / (1024 * 1024)
        print(f"Transfer completed in {elapsed:.2f} seconds ({size_mb:.2f} MB)")

    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
