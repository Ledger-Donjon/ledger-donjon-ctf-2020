#!/usr/bin/env python3
"""Implement a SMPC node that participates in a protocol that signs data using ECDSA

Dependencies:

    pip install fastecdsa==2.1.5 noiseprotocol==0.3.1
"""
import argparse
import asyncio
import base64
import json
import hashlib
import logging
from pathlib import Path
import secrets
from typing import Any, Dict, Iterable, List, Optional, Tuple

from cryptography.hazmat.primitives.asymmetric.x25519 import X25519PrivateKey
from cryptography.hazmat.primitives.serialization import (
    Encoding,
    PublicFormat,
    PrivateFormat,
    NoEncryption,
)
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature

import fastecdsa.curve
import fastecdsa.point

from noise.connection import Keypair, NoiseConnection


logger = logging.getLogger(__name__)


SECP256K1_CURVE = fastecdsa.curve.secp256k1
SECP256K1_ORDER = SECP256K1_CURVE.q
assert SECP256K1_ORDER == 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
RANDOM_BITS = 64


def sss_recover(shares: Iterable[Tuple[int, int]]) -> int:
    # Reconstruct the secret using naive implementation of Shamir's Secret Sharing
    value = 0
    for x_i, y_i in shares:
        term_i = y_i
        for x_j, _ in shares:
            if x_i == x_j:
                continue
            inv_ij = pow(x_j - x_i, -1, SECP256K1_ORDER)
            term_i = (term_i * x_j * inv_ij) % SECP256K1_ORDER
        value = (value + term_i) % SECP256K1_ORDER
    return value


class EncryptedChannel:
    """Handle a channel encrypted using Noise pattern XK:
        <- s
        ...
        -> e, es
        <- e, ee
        -> s, se
    """

    def __init__(self, private_key: X25519PrivateKey, my_secret_x: int):
        self.private_key = private_key
        self.private_key_bytes = private_key.private_bytes(Encoding.Raw, PrivateFormat.Raw, NoEncryption())
        self.my_secret_x = my_secret_x
        self.peer_secret_x = None
        self.noise = None
        self.reader = None
        self.writer = None
        self.recv_queue: List[bytes] = []

    async def connect_peer(self, peer_public_info):
        """Initiator side"""
        self.reader, self.writer = await asyncio.open_connection(
            host=peer_public_info["addr"] or "127.0.0.1", port=peer_public_info["port"]
        )
        peer_public_key = base64.b64decode(peer_public_info["public_key"])

        self.noise = NoiseConnection.from_name(b"Noise_XK_25519_ChaChaPoly_BLAKE2s")
        self.noise.set_as_initiator()
        self.noise.set_keypair_from_private_bytes(Keypair.STATIC, self.private_key_bytes)
        self.noise.set_keypair_from_public_bytes(Keypair.REMOTE_STATIC, peer_public_key)
        self.noise.start_handshake()

        # Send a SECU command to the peer
        first_message = b"SECU" + self.noise.write_message()
        logger.debug("NOISE: Init -> e, es (%#x bytes)", len(first_message) - 4)
        assert len(first_message) == 4 + 0x30
        self.writer.write(first_message)

        second_message = await self.reader.readexactly(0x30)
        self.noise.read_message(second_message)
        logger.debug("NOISE: Init <- e, ee (%#x bytes)", len(second_message))
        third_message = self.noise.write_message()
        logger.debug("NOISE: Init -> s, se (%#x bytes)", len(third_message))
        self.writer.write(third_message)
        assert self.noise.handshake_finished

        # Exchange the "x" coordinate of the secret shares
        self.send_encrypted(self.my_secret_x.to_bytes(32, "big"))
        peer_secret = await self.recv_exactly_encrypted(32)
        self.peer_secret_x = int.from_bytes(peer_secret, "big")
        logger.debug("NOISE: Initiated encrypted connection %d -> %d", self.my_secret_x, self.peer_secret_x)

    async def accept_connection(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        """Responder side"""
        self.reader = reader
        self.writer = writer
        self.noise = NoiseConnection.from_name(b"Noise_XK_25519_ChaChaPoly_BLAKE2s")
        self.noise.set_as_responder()
        self.noise.set_keypair_from_private_bytes(Keypair.STATIC, self.private_key_bytes)
        self.noise.start_handshake()

        first_message = await self.reader.readexactly(0x30)
        logger.debug("NOISE: Resp -> e, es (%#x bytes)", len(first_message))
        self.noise.read_message(first_message)
        second_message = self.noise.write_message()
        logger.debug("NOISE: Resp <- e, ee (%#x bytes)", len(second_message))
        assert len(second_message) == 0x30
        self.writer.write(second_message)

        third_message = await self.reader.readexactly(0x40)
        logger.debug("NOISE: Resp -> s, se (%#x bytes)", len(third_message))
        self.noise.read_message(third_message)
        assert self.noise.handshake_finished

        # Exchange the "x" coordinate of the secret shares
        self.send_encrypted(self.my_secret_x.to_bytes(32, "big"))
        peer_secret = await self.recv_exactly_encrypted(32)
        self.peer_secret_x = int.from_bytes(peer_secret, "big")
        logger.debug("NOISE: Responded to encrypted connection %d <- %d", self.my_secret_x, self.peer_secret_x)

    def send_packet(self, data: bytes):
        assert len(data) < 0xFFFF
        if self.peer_secret_x:
            logger.info(
                "Sending %d bytes from %d to %d: %s", len(data), self.my_secret_x, self.peer_secret_x, data.hex()
            )
        self.writer.write(len(data).to_bytes(2, "big") + data)

    def send_encrypted(self, data: bytes):
        self.send_packet(self.noise.encrypt(data))

    async def recv_packet(self):
        pkt_size = await self.reader.readexactly(2)
        return await self.reader.readexactly(int.from_bytes(pkt_size, "big"))

    async def recv_encrypted(self):
        encrypted_data = await self.recv_packet()
        return self.noise.decrypt(encrypted_data)

    async def recv_exactly_clear(self, size: int):
        data = await self.recv_packet()
        if len(data) != size:
            raise ValueError(f"Unexpected received size: {len(data)} != size")
        return data

    async def recv_exactly_encrypted(self, size: int):
        data = await self.recv_encrypted()
        if len(data) != size:
            raise ValueError(f"Unexpected received size: {len(data)} != size")
        return data


class SMPCNode:
    def __init__(self, secret_share: Path, addr: str, port: int):
        self.addr = addr
        self.port = port
        with secret_share.open("r") as f_secret_share:
            secret_share_data = json.load(f_secret_share)
        self.smpc_k = secret_share_data["k"]
        self.smpc_n = secret_share_data["n"]
        self.secret_x = secret_share_data["x"]
        self.secret_y = secret_share_data["y"]
        self.active_channels: Dict[int, EncryptedChannel] = {}

        logging.basicConfig(format=f"[{self.secret_x:2d} %(levelname)-5s] %(message)s", level=logging.INFO)

        # Private key for node-node communications
        self.comm_private_key = X25519PrivateKey.generate()
        self.comm_public_key = base64.b64encode(
            self.comm_private_key.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw)
        ).decode("ascii")
        self.server: Optional[asyncio.AbstractServer] = None

    async def start_server(self, write_info: Path):
        self.server = await asyncio.start_server(
            self.client_connected, host=self.addr, port=self.port, reuse_address=True, start_serving=True,
        )
        logger.info(
            "Started server on %s:%d with public key %s", self.addr or "*", self.port, self.comm_public_key,
        )

        if write_info:
            public_info = {
                "addr": self.addr,
                "port": self.port,
                "public_key": self.comm_public_key,
            }
            with write_info.open("w") as f_out:
                json.dump(public_info, f_out)
        try:
            await self.server.serve_forever()
        except asyncio.CancelledError:
            pass

    async def client_connected(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        assert self.server is not None
        cmd = await reader.readexactly(4)
        if cmd == b"HALT":
            logger.info("Received HALT command, stopping NOW!")
            self.server.close()
            return

        if cmd == b"SIGN":
            payload_size_bytes = await reader.readexactly(4)
            payload_bytes = await reader.readexactly(int.from_bytes(payload_size_bytes, "big"))
            payload = json.loads(payload_bytes)
            logger.info("Received SIGN command %r", payload)

            # Create a SMPC network between all selected nodes and sign the data
            self.active_channels = {}
            signature = await self.smpc_sign(payload)

            # Send the data back to the caller
            writer.write(len(signature).to_bytes(4, "big"))
            writer.write(signature)
            writer.close()
            return

        if cmd == b"SECU":
            # Initiate a Noise protocol with the peer
            channel = EncryptedChannel(self.comm_private_key, self.secret_x)
            await channel.accept_connection(reader, writer)
            logger.debug("Received SECU command from %d", channel.peer_secret_x)

            if channel.peer_secret_x in self.active_channels:
                logger.error("An encrypted channel alerady exists with peer %d!", channel.peer_secret_x)
                writer.close()
                return

            # Read the tree level: 0 to perform the signature, 1 to read data and relay it to the main async task
            tree_level = await channel.recv_exactly_encrypted(1)
            if tree_level == b"\x00":
                self.active_channels = {}
            self.active_channels[channel.peer_secret_x] = channel
            try:
                if tree_level == b"\x01":
                    # Child of child: Maintain the connexion active, and receive data
                    while not channel.reader.at_eof():
                        await asyncio.sleep(0.042)
                elif tree_level == b"\x00":
                    payload_bytes = await channel.recv_encrypted()
                    payload = json.loads(payload_bytes)
                    await self.smpc_sign(payload, from_channel=channel)
                else:
                    logger.error("Unexpected tree level %r", tree_level)
            finally:
                del self.active_channels[channel.peer_secret_x]

            logger.debug("Closing secure channel with %d", channel.peer_secret_x)
            writer.close()
            return

        logger.error("Received unknown command %r", cmd)
        writer.close()

    async def smpc_sign(self, payload: Any, from_channel: Optional[EncryptedChannel] = None) -> bytes:
        peer_nodes_info = []
        peer_nodes_secret_x = []
        for node_info in payload["nodes"]:
            if node_info["public_key"] == self.comm_public_key:
                continue
            channel = EncryptedChannel(self.comm_private_key, self.secret_x)
            await channel.connect_peer(node_info)
            assert channel.peer_secret_x
            assert channel.peer_secret_x not in self.active_channels
            channel.send_encrypted(b"\x01" if from_channel is not None else b"\x00")
            self.active_channels[channel.peer_secret_x] = channel
            peer_nodes_info.append(node_info)
            peer_nodes_secret_x.append(channel.peer_secret_x)

        peer_indexes = sorted(self.active_channels.keys())
        logger.info("Established channels with %d nodes: %r", len(self.active_channels), peer_indexes)

        if from_channel is not None:
            # Send an acknoledgment that the connections were established
            from_channel.send_encrypted(b"connected")
        else:
            # Make peer nodes create channels between themselves
            for peer_idx, peer_node_info in enumerate(peer_nodes_info):
                peer_payload_json = {
                    "data": payload["data"],
                    "nodes": peer_nodes_info[peer_idx + 1 :],
                }
                peer_payload = json.dumps(peer_payload_json).encode("ascii")
                peer_channel = self.active_channels[peer_nodes_secret_x[peer_idx]]
                peer_channel.send_encrypted(peer_payload)
                ack = await peer_channel.recv_encrypted()
                if ack != b"connected":
                    raise RuntimeError(f"Failed to receive connection ack message: {ack!r}")

        # Run the SMPC algorithm
        z = int.from_bytes(hashlib.sha256(base64.b64decode(payload["data"])).digest(), "big")
        k, pb = await self.smpc_random_keypair()
        r = pb.x % SECP256K1_ORDER
        invk = await self.smpc_invert(k)
        t = (r * self.secret_y + z) % SECP256K1_ORDER
        s = await self.smpc_multiply_open(t, invk)
        signature = encode_dss_signature(r, s)

        await asyncio.sleep(0.1)  # Make other nodes receive their pending data

        # Close channels with peers
        for peer_secret_x in peer_nodes_secret_x:
            self.active_channels[peer_secret_x].writer.close()
            del self.active_channels[peer_secret_x]

        await asyncio.sleep(0.1)
        return signature

    async def smpc_biased_rng(self) -> int:
        """Implement Biased Random Number Generation algorithm"""
        # Generate random shares for other parties
        coefs = [secrets.randbits(RANDOM_BITS) for _ in range(self.smpc_k)]
        for peer_channel in self.active_channels.values():
            value = 0
            for power, coef in enumerate(coefs):
                value = (value + coef * pow(peer_channel.peer_secret_x, power, SECP256K1_ORDER)) % SECP256K1_ORDER
            peer_channel.send_encrypted(value.to_bytes(32, "big"))

        # Compute my own share
        brng_share = 0
        for power, coef in enumerate(coefs):
            brng_share = (brng_share + coef * pow(self.secret_x, power, SECP256K1_ORDER)) % SECP256K1_ORDER

        # Receive shares from other parties
        for peer_channel in self.active_channels.values():
            share_from_other = int.from_bytes(await peer_channel.recv_exactly_encrypted(32), "big")
            brng_share = (brng_share + share_from_other) % SECP256K1_ORDER
        return brng_share

    async def smpc_unbiased_rng(self) -> int:
        """Implement Unbiased Random Number Generation algorithm"""
        coefficients: List[int] = []
        for _ in range(self.smpc_k):
            coefficients.append(await self.smpc_biased_rng())

        for peer_channel in self.active_channels.values():
            value = 0
            for power, c_share in enumerate(coefficients):
                value = (value + c_share * pow(peer_channel.peer_secret_x, power, SECP256K1_ORDER)) % SECP256K1_ORDER
            peer_channel.send_encrypted(value.to_bytes(32, "big"))

        value = 0
        for power, c_share in enumerate(coefficients):
            value = (value + c_share * pow(self.secret_x, power, SECP256K1_ORDER)) % SECP256K1_ORDER

        shares: List[Tuple[int, int]] = [(self.secret_x, value)]
        for peer_channel in self.active_channels.values():
            share_from_other = int.from_bytes(await peer_channel.recv_exactly_encrypted(32), "big")
            shares.append((peer_channel.peer_secret_x, share_from_other))
        return sss_recover(shares)

    async def smpc_random_zero(self) -> int:
        """Implement Random Zero Generation algorithm"""
        coefficients: List[int] = [0]
        for _ in range(1, self.smpc_k):
            coefficients.append(await self.smpc_biased_rng())

        for peer_channel in self.active_channels.values():
            value = 0
            for power, c_share in enumerate(coefficients):
                value = (value + c_share * pow(peer_channel.peer_secret_x, power, SECP256K1_ORDER)) % SECP256K1_ORDER
            peer_channel.send_encrypted(value.to_bytes(32, "big"))

        value = 0
        for power, c_share in enumerate(coefficients):
            value = (value + c_share * pow(self.secret_x, power, SECP256K1_ORDER)) % SECP256K1_ORDER

        shares: List[Tuple[int, int]] = [(self.secret_x, value)]
        for peer_channel in self.active_channels.values():
            share_from_other = int.from_bytes(await peer_channel.recv_exactly_encrypted(32), "big")
            shares.append((peer_channel.peer_secret_x, share_from_other))
        return sss_recover(shares)

    async def smpc_share_revealing_open(self, share: int) -> int:
        """Implement Share Revealing Open algorithm"""
        for peer_channel in self.active_channels.values():
            peer_channel.send_packet(share.to_bytes(32, "big"))

        shares: List[Tuple[int, int]] = [(self.secret_x, share)]
        for peer_channel in self.active_channels.values():
            share_from_other = int.from_bytes(await peer_channel.recv_exactly_clear(32), "big")
            shares.append((peer_channel.peer_secret_x, share_from_other))

        return sss_recover(shares)

    async def smpc_share_hiding_open(self, share: int) -> int:
        """Implement Share Hiding Open algorithm"""
        rzg_share = await self.smpc_random_zero()
        return await self.smpc_share_revealing_open((share + rzg_share) % SECP256K1_ORDER)

    async def smpc_multiply_open(self, share_a: int, share_b: int) -> int:
        """Implement Multiply and Open algorithm"""
        share_ab = (share_a * share_b) % SECP256K1_ORDER
        return await self.smpc_share_hiding_open(share_ab)

    async def smpc_invert(self, share) -> int:
        """Implement Inversion algorithm"""
        r = await self.smpc_biased_rng()
        t = await self.smpc_multiply_open(r, share)
        return (pow(t, -1, SECP256K1_ORDER) * r) % SECP256K1_ORDER

    async def smpc_random_keypair(self) -> Tuple[int, fastecdsa.point.Point]:
        """Implement Random Key-pair Generation algorithm"""
        rng_share = await self.smpc_unbiased_rng()
        point_share = rng_share * SECP256K1_CURVE.G
        for peer_channel in self.active_channels.values():
            pt_x = point_share.x.to_bytes(32, "big")
            pt_y = point_share.y.to_bytes(32, "big")
            peer_channel.send_packet(pt_x + pt_y)

        shares: List[Tuple[int, fastecdsa.point.Point]] = [(self.secret_x, point_share)]
        for peer_channel in self.active_channels.values():
            share_from_other = await peer_channel.recv_exactly_clear(64)
            pt_x = int.from_bytes(share_from_other[:32], "big")
            pt_y = int.from_bytes(share_from_other[32:], "big")
            pt_from_other = fastecdsa.point.Point(pt_x, pt_y, SECP256K1_CURVE)
            shares.append((peer_channel.peer_secret_x, pt_from_other))

        final_point = fastecdsa.point.Point.IDENTITY_ELEMENT
        for x_i, pt_i in shares:
            term_i = 1
            for x_j, _ in shares:
                if x_i == x_j:
                    continue
                inv_ij = pow(x_j - x_i, -1, SECP256K1_ORDER)
                term_i = (term_i * x_j * inv_ij) % SECP256K1_ORDER
            final_point += term_i * pt_i
        return rng_share, final_point


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="SMPC node that signs data using ECDSA")
    parser.add_argument("secret_share", type=Path, help="share of the secret key")
    parser.add_argument(
        "-a", "--addr", type=str, help="host address of the server",
    )
    parser.add_argument(
        "-p", "--port", type=int, default=1337, help="TCP port to listen for incoming connections",
    )
    parser.add_argument(
        "-w", "--write-info", type=Path, help="write the public information of the node to a JSON file",
    )
    args = parser.parse_args()
    node = SMPCNode(args.secret_share, args.addr, args.port)
    asyncio.run(node.start_server(args.write_info))
