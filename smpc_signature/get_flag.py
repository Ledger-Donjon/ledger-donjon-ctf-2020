#!/usr/bin/env python3
import argparse
import sys

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric.ec import derive_private_key, SECP256K1

import fastecdsa.curve
import fastecdsa.point


FLAG_MESSAGE_HASH = 0xDBF4D4498D233D6179FAE13CBABDEFA3C098BF700F50E6F87D0D397BFF04DF2E
FLAG_ECDSA_NONCE = 0x004543445341206E6F6E636520666F7220444F4E4A4F4E2043544620666C6167

parser = argparse.ArgumentParser(description="Get the flag")
parser.add_argument("secret_key", type=int, help="ECDSA secret key, in decimal format")
args = parser.parse_args()

PUBLIC_KEY_PEM = b"""
-----BEGIN PUBLIC KEY-----
MFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEvKsN5HwvH71x0hB2omeCdiQRBy9PqSKi
k8b4gRRkyHnYQYKnBHwzErzm2zuvLY07bsY1eZ9CILDvbMwgct1ATA==
-----END PUBLIC KEY-----
"""

public_key = serialization.load_pem_public_key(PUBLIC_KEY_PEM, backend=default_backend())

secret_key_value = args.secret_key
secret_key = derive_private_key(secret_key_value, curve=SECP256K1(), backend=default_backend())

# Compare the public key with the secret key
if secret_key.public_key().public_numbers() != public_key.public_numbers():
    print("Invalid secret key, try again!", file=sys.stderr)
    sys.exit(1)

# Generate an ECDSA signature
SECP256K1_CURVE = fastecdsa.curve.secp256k1
SECP256K1_ORDER = SECP256K1_CURVE.q
r = (FLAG_ECDSA_NONCE * SECP256K1_CURVE.G).x % SECP256K1_ORDER
inv_nonce = pow(FLAG_ECDSA_NONCE, -1, SECP256K1_ORDER)
s = ((FLAG_MESSAGE_HASH + r * secret_key_value) * inv_nonce) % SECP256K1_ORDER
print(s.to_bytes(32, "big").decode())
