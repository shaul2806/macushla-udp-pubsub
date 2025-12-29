import socket
import random
import time

UDP_PORT = 1111
BROADCAST_IP = "255.255.255.255"
TOPIC = "mot_vel"

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # allow UDP broadcast
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    print(f"[Node1] Publishing '{TOPIC}' to {BROADCAST_IP}:{UDP_PORT} (Ctrl+C to stop)")
    try:
        while True:
            value = random.uniform(-100.0, 100.0)
            msg = f"{TOPIC},{value:.1f}"
            sock.sendto(msg.encode("utf-8"), (BROADCAST_IP, UDP_PORT))
            print(f"[Node1] Sent: {msg}")
            time.sleep(0.2)  # 5 Hz
    except KeyboardInterrupt:
        print("\n[Node1] Stopped.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
