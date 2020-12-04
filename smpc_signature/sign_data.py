#!/usr/bin/env python3
"""Sign some data by spawning some nodes"""
import base64
import json
import random
import socket
import subprocess
from pathlib import Path
import time
from typing import Any, Dict, Set


DATA_TO_SIGN = "👋 Hello, world 🌍 🎉".encode("utf-8")


def recv_exact(sock: socket.socket, size: int):
    data = b""
    while len(data) < size:
        new_data = sock.recv(size - len(data))
        if not new_data:
            raise RuntimeError("Connection closed")
        data += new_data
    assert len(data) == size
    return data


# Choose which nodes to start (at least 10 out of 16)
chosen_nodes: Set[int] = set()
while len(chosen_nodes) < 10:
    for node_id in range(1, 17):
        if random.randint(0, 1):
            chosen_nodes.add(node_id)

print(f"Spawning {len(chosen_nodes)} nodes: {chosen_nodes}")

node_processes: Dict[int, subprocess.Popen] = {}
all_nodes_info = []
first_node_info: Any = None
try:
    for node_id in sorted(chosen_nodes):
        secret_share_file = f"secret_share_{node_id:02d}.json"
        info_file = Path(f"tmp_info_node_{node_id:02d}.json")
        if info_file.exists():
            info_file.unlink()
        node_processes[node_id] = subprocess.Popen(
            [
                "./smpc_signer.py",
                secret_share_file,
                "--addr",
                "127.0.0.1",
                "--port",
                str(4200 + node_id),
                "--write-info",
                str(info_file),
            ]
        )
        while not info_file.exists():
            time.sleep(0.001)
        time.sleep(0.001)
        with info_file.open("r") as f_info:
            node_info = json.load(f_info)
        all_nodes_info.append(node_info)
        if not first_node_info:
            first_node_info = node_info

    # Ask the first node to sign the data
    command_json = {"data": base64.b64encode(DATA_TO_SIGN).decode("ascii"), "nodes": all_nodes_info}
    command_payload = json.dumps(command_json).encode("ascii")
    command = b"SIGN" + len(command_payload).to_bytes(4, "big") + command_payload

    sock = socket.create_connection((first_node_info["addr"], first_node_info["port"]))
    sock.sendall(command)
    recv_size = int.from_bytes(recv_exact(sock, 4), "big")
    signature = recv_exact(sock, recv_size)
    print(f"Signature: {signature.hex()}")

    # Verify the signature
    with open("signature.der", "wb") as f_signature:
        f_signature.write(signature)
    subprocess.run(
        ["openssl", "dgst", "-sha256", "-verify", "public_key.pem", "-signature", "signature.der"],
        input=DATA_TO_SIGN,
        check=True,
    )

finally:
    for child in node_processes.values():
        child.terminate()
