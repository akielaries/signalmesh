#!/usr/bin/env python3
"""
FPGA Programming "Client"
"""

import socket
import json
import argparse
import sys
import time
from pathlib import Path


def program_fpga(bitstream_path, board_type, host, port):

    bitstream_path = Path(bitstream_path)
    if not bitstream_path.exists():
        print(f"Error: Bitstream file not found: {bitstream_path}")
        return 1

    # read bitstream file
    with open(bitstream_path, "rb") as f:
        bitstream_data = f.read()

    file_size = len(bitstream_data)
    print(f"Sending {file_size:,} bytes ({bitstream_path.name}) to {host}:{port}")
    print(f"Board type: {board_type}")

    try:
        # connect to server
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(60)  # 60 second timeout
        sock.connect((host, port))

        # send header (4 bytes file size)
        sock.sendall(file_size.to_bytes(4, "big"))

        # send board type (1 byte length + string)
        board_bytes = board_type.encode("utf-8")
        sock.sendall(len(board_bytes).to_bytes(1, "big"))
        sock.sendall(board_bytes)

        # send bitstream data
        total_sent = 0
        chunk_size = 4096

        for i in range(0, file_size, chunk_size):
            chunk = bitstream_data[i : i + chunk_size]
            sent = sock.sendall(chunk)
            total_sent += len(chunk)
            # simple progress indicator
            if i % (chunk_size * 100) == 0:
                percent = (total_sent / file_size) * 100
                print(f"Progress: {percent:.1f}%", end="\r")

        print(f"\nTransfer complete. Waiting for programming...")

        # receive response
        response_data = b""
        while True:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response_data += chunk
            except socket.timeout:
                break

        # parse response
        response = json.loads(response_data.decode("utf-8"))

        print("\n" + "=" * 60)
        print("Programming Results:")
        print("=" * 60)

        if response["stdout"]:
            print("STDOUT:")
            print(response["stdout"])

        if response["stderr"]:
            print("\nSTDERR:")
            print(response["stderr"])

        print(f"\nReturn code: {response['returncode']}")
        print("=" * 60)

        return response["returncode"]

    except socket.timeout:
        print("Error: Connection timeout")
        return 1
    except ConnectionRefusedError:
        print(f"Error: Could not connect to {host}:{port}")
        print("Make sure the server is running on the Jetson Nano")
        return 1
    except Exception as e:
        print(f"Error: {e}")
        return 1
    finally:
        sock.close()


def main():
    parser = argparse.ArgumentParser(description="OpenFPGALoader client")
    parser.add_argument(
        "-f",
        "--file",
        help="Path to bitstream file (.fs)"
    )
    parser.add_argument(
        "-b",
        "--board",
        help="Board type (e.g., tangnano9k)"
    )
    parser.add_argument(
        "-H",
        "--host",
        default="192.168.86.56",
        help="Remote server's IP/hostname"
    )
    parser.add_argument(
        "-p",
        "--port",
        type=int,
        default=65432,
        help="Remote server's port"
    )

    args = parser.parse_args()

    retcode = program_fpga(args.file, args.board, args.host, args.port)
    sys.exit(retcode)

if __name__ == "__main__":
    main()
