#!/usr/bin/env python3
"""Generate information for the flag checker

With z = hash(message) as integer and k being the ECDSA nonce, in ECDSA, a signature (r, s) consists in:
    r = (k * G).x
    s = ((z + r * secretkey) / k) mod curve_order

s can encode a flag if z is manipulated according to k
"""
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization

import fastecdsa.curve
import fastecdsa.point


FLAG = "CTF{weak_RNG_strikes_back_again}"
assert len(FLAG) == 32

with open("secret_key.pem", "rb") as f_secretkey:
    secret_key = serialization.load_pem_private_key(f_secretkey.read(), password=None, backend=default_backend())

secret_value = secret_key.private_numbers().private_value

with open("public_key.pem", "rb") as f_publickey:
    public_key = serialization.load_pem_public_key(f_publickey.read(), backend=default_backend())

SECP256K1_CURVE = fastecdsa.curve.secp256k1
SECP256K1_ORDER = SECP256K1_CURVE.q

# Check that the public key matches the secret key
public_numbers = public_key.public_numbers()
public_key_point = secret_value * SECP256K1_CURVE.G
assert public_numbers.x == public_key_point.x
assert public_numbers.y == public_key_point.y

print(f"# secret key is {secret_value}")

s = int.from_bytes(FLAG.encode("ascii"), "big")
k = int.from_bytes(b"ECDSA nonce for DONJON CTF flag", "big")
assert k.bit_length() <= 256
r = (k * SECP256K1_CURVE.G).x % SECP256K1_ORDER
z = (s * k - r * secret_value) % SECP256K1_ORDER

print(f"FLAG_MESSAGE_HASH = 0x{z:064X}")
print(f"FLAG_ECDSA_NONCE = 0x{k:064X}")
