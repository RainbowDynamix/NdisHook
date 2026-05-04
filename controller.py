import argparse
import socket
import sys

MAGIC = b"MP5"
DELIM = b"|"


def build_packet(command: str, params: str = "") -> bytes:
    parts = [MAGIC, command.encode("ascii")]
    if params:
        parts.append(params.encode("ascii"))
    return DELIM.join(parts)


def send(ip: str, port: int, payload: bytes) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((ip, port))
        sock.sendall(payload)


def handle(line: str, ip: str, port: int) -> bool:
    tokens = line.strip().split()
    if not tokens:
        return True

    cmd = tokens[0].lower()

    if cmd in ("quit", "exit"):
        return False

    if cmd == "kill":
        if len(tokens) != 2:
            print("usage: kill <pid>")
            return True
        try:
            pid = int(tokens[1])
        except ValueError:
            print("pid must be an integer")
            return True
        if pid < 0:
            print("pid must be non-negative")
            return True
        payload = build_packet("KILL", str(pid))
        try:
            send(ip, port, payload)
        except OSError as e:
            print(f"send failed: {e}")
            return True
        #print(f"sent {payload!r} to {ip}:{port}")
        return True

    print(f"unknown command: {cmd}")
    return True


def main() -> None:
    parser = argparse.ArgumentParser(description="Interactive NdisHook controller")
    parser.add_argument("ip", help="Destination IP address")
    parser.add_argument("port", type=int, help="Destination port")
    args = parser.parse_args()

    print(f"connected: {args.ip}:{args.port}")
    while True:
        try:
            line = input("controller > ")
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not handle(line, args.ip, args.port):
            break


if __name__ == "__main__":
    main()