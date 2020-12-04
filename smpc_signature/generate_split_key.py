#!/usr/bin/env python3
"""Generate an EC key pair on secp256k1, splitting the secret part into shares"""
import json
import os
import subprocess

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization


SECP256K1_ORDER = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141


# Generate a key pair
subprocess.run(
    ("openssl", "ecparam", "-name", "secp256k1", "-genkey", "-out", "secret_key.pem"), check=True,
)
subprocess.run(
    ("openssl", "ec", "-in", "secret_key.pem", "-pubout", "-out", "public_key.pem"), check=True,
)

# Load the secret key
with open("secret_key.pem", "rb") as f_secretkey:
    secret_key = serialization.load_pem_private_key(f_secretkey.read(), password=None, backend=default_backend())

with open("public_key.pem", "rb") as f_publickey:
    public_key = serialization.load_pem_public_key(f_publickey.read(), backend=default_backend())

# Ensure the public key matches the secret key
serialized_pubkey = public_key.public_bytes(serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint)
serialized_pubkey_from_secret = secret_key.public_key().public_bytes(
    serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint
)
assert serialized_pubkey == serialized_pubkey_from_secret

secret_value = secret_key.private_numbers().private_value

# Split the secret value into a (5, 16) share
SSS_THRESHOLD = 5
SSS_PARTIES = 16
entropy = os.urandom(256 * (SSS_THRESHOLD - 1))
coefs = [int.from_bytes(entropy[i : i + 256], "big") for i in range(0, len(entropy), 256)]
for party_index in range(1, SSS_PARTIES + 1):
    value = secret_value
    for power, coef in enumerate(coefs, start=1):
        value = (value + coef * pow(party_index, power, SECP256K1_ORDER)) % SECP256K1_ORDER
    share_data = {
        "k": SSS_THRESHOLD,
        "n": SSS_PARTIES,
        "x": party_index,
        "y": value,
    }
    with open(f"secret_share_{party_index:02d}.json", "w") as f_share:
        json.dump(share_data, f_share)
