#!/usr/bin/env python3
"""Client for the port's debug console (platform/port_console.cpp, MP2_CONSOLE=<port>).

    tools/mp2con.py [command ...]          run one command per argument, print every reply
    tools/mp2con.py status warp 4          (the arrows: it is one connection, one line each)
    echo status | tools/mp2con.py          read commands from stdin, one per line

The port listens on 127.0.0.1:4780 by default; pass -p/--port for MP2_CONSOLE=<n>. Every reply ends
with a line `=> ok` or `=> err: ...`, which is how a caller knows where one command's output stops.
"""
import argparse
import socket
import sys


def replies(data: bytes) -> int:
    """How many replies have arrived: each one ends in a line `=> ok` or `=> err: ...`."""
    return sum(1 for line in data.split(b"\n") if line.startswith((b"=> ok", b"=> err:")))


def run(port: int, commands: list[str], quiet: bool) -> int:
    with socket.create_connection(("127.0.0.1", port), timeout=10) as sock:
        payload = "".join(command.rstrip("\r\n") + "\n" for command in commands)
        sock.sendall(payload.encode())
        data = b""
        # Read until every command has answered, not until the connection closes: the port keeps it
        # open until the client hangs up (its listener serves one client at a time), so waiting for
        # EOF would always end in a timeout.
        try:
            while replies(data) < len(commands):
                chunk = sock.recv(4096)
                if not chunk:
                    break
                data += chunk
        except TimeoutError:
            print(f"no reply from 127.0.0.1:{port} (is the port running with MP2_CONSOLE={port}?)",
                  file=sys.stderr)
            return 1
    text = data.decode(errors="replace")
    if not quiet:
        sys.stdout.write(text)
    if "=> err:" in text:
        for line in text.splitlines():
            if line.startswith("=> err:"):
                print(line, file=sys.stderr)
        return 1
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Send commands to the port's debug console.")
    parser.add_argument("commands", nargs="*", help="commands to run (default: read stdin, one per line)")
    parser.add_argument("-p", "--port", type=int, default=4780, help="MP2_CONSOLE port (default 4780)")
    parser.add_argument("-q", "--quiet", action="store_true", help="print nothing on success")
    args = parser.parse_args()
    commands = args.commands if args.commands else [line for line in sys.stdin.read().splitlines() if line.strip()]
    if not commands:
        parser.error("no commands (pass them as arguments or on stdin)")
    return run(args.port, commands, args.quiet)


if __name__ == "__main__":
    raise SystemExit(main())
