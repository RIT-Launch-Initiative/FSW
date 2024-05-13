import socket
import sys

def broadcast_message(ip_address, port, message):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    try:
        sock.sendto(message.encode(), (ip_address, port))
        print(f"Message '{message}' broadcasted successfully to {ip_address} on port {port}")
    except Exception as e:
        print("Error broadcasting message:", e)
    finally:
        sock.close()

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python script.py <ip_address> <port> <message>")
        sys.exit(1)

    ip_address = sys.argv[1]
    port = int(sys.argv[2])
    message = " ".join(sys.argv[3:])
    broadcast_message(ip_address, port, message)