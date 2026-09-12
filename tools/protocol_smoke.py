#!/usr/bin/env python3

import socket
import sys

HOST = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.101"
PORT = 8000


def recv_line(sock, buffer):
    while b"\n" not in buffer:
        data = sock.recv(1024)
        if not data:
            raise RuntimeError("server closed connection")
        buffer += data

    line, _, buffer = buffer.partition(b"\n")
    return line.decode(errors="replace").strip(), buffer


def request(sock, buffer, request_id, command):
    message = f"REQ {request_id} {command}\n".encode()
    sock.sendall(message)

    while True:
        line, buffer = recv_line(sock, buffer)

        if line.startswith(f"RES {request_id} "):
            print(line)
            return buffer

        if line.startswith("EVENT "):
            print("event:", line)


def main():
    buffer = b""

    with socket.create_connection((HOST, PORT), timeout=5) as sock:
        buffer = request(sock, buffer, 1, "PING")
        buffer = request(sock, buffer, 2, "STATUS GET")
        buffer = request(sock, buffer, 3, "KEY GET")
        buffer = request(sock, buffer, 4, "CLIENTS GET")
        buffer = request(sock, buffer, 5, "LED SET ON")
        buffer = request(sock, buffer, 6, "STATUS GET")
        buffer = request(sock, buffer, 7, "LED SET OFF")
        buffer = request(sock, buffer, 8, "STATUS GET")


if __name__ == "__main__":
    main()