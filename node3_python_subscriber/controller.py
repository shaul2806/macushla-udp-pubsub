import socket
import select
import time

UDP_PORT = 1111

estop_active = False
current_velocity = 0.0
boot_time = time.monotonic()


def ms_since_boot():
    return int((time.monotonic() - boot_time) * 1000)


def parse_message(msg: str):
    msg = msg.strip()
    if "," not in msg:
        return None
    topic, data = msg.split(",", 1)
    return topic.lower(), data.strip()


def main():
    global estop_active, current_velocity

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", UDP_PORT))
    sock.setblocking(False)

    print("[Node3] Listening on UDP port 1111")

    try:
        while True:
            readable, _, _ = select.select([sock], [], [], 1.0)

            if not readable:
                continue

            data, _ = sock.recvfrom(1024)
            msg = data.decode("utf-8", errors="replace")

            parsed = parse_message(msg)
            if not parsed:
                continue

            topic, payload = parsed

            if topic == "estop":
                if payload == "1":
                    if not estop_active:
                        estop_active = True
                        current_velocity = 0.0
                        print(f"[Node3 {ms_since_boot()}ms] ESTOP ACTIVE -> velocity forced to 0")
                elif payload == "0":
                    if estop_active:
                        estop_active = False
                        print(f"[Node3 {ms_since_boot()}ms] ESTOP CLEARED -> motor allowed")
                continue

            if topic == "mot_vel":
                if estop_active:
                    continue
                try:
                    current_velocity = float(payload)
                except ValueError:
                    continue
                print(f"[Node3 {ms_since_boot()}ms] Motor velocity: {current_velocity:.1f}")

    except KeyboardInterrupt:
        print("\n[Node3] Stopped.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
