#!/usr/bin/env python3
import base64
import datetime
import hashlib
import itertools
import re
from typing import Iterable, List, Tuple

import fastecdsa.curve
import fastecdsa.point

SECP256K1_CURVE = fastecdsa.curve.secp256k1
SECP256K1_ORDER = SECP256K1_CURVE.q
assert SECP256K1_ORDER == 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141

count_for_pairs = {}

STATE_NAMES = {
    1: "Tree level (1 byte)",
    2: "JSON payload",
    10: "smpc_random_keypair/smpc_unbiased_rng/smpc_biased_rng[0]",
    11: "smpc_random_keypair/smpc_unbiased_rng/smpc_biased_rng[1]",
    12: "smpc_random_keypair/smpc_unbiased_rng/smpc_biased_rng[2]",
    13: "smpc_random_keypair/smpc_unbiased_rng/smpc_biased_rng[3]",
    14: "smpc_random_keypair/smpc_unbiased_rng/smpc_biased_rng[4]",
    15: "smpc_random_keypair/smpc_unbiased_rng",
    16: "smpc_random_keypair/CLEAR pt share",
    17: "smpc_invert/smpc_biased_rng",
    18: "smpc_invert/smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[1]",
    19: "smpc_invert/smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[2]",
    20: "smpc_invert/smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[3]",
    21: "smpc_invert/smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[4]",
    22: "smpc_invert/smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero",
    23: "smpc_invert/smpc_multiply_open/smpc_share_hiding_open/smpc_share_revealing_open/CLEAR share k*r",
    24: "smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[1]",
    25: "smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[2]",
    26: "smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[3]",
    27: "smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero/smpc_biased_rng[4]",
    28: "smpc_multiply_open/smpc_share_hiding_open/smpc_random_zero",
    29: "smpc_multiply_open/smpc_share_hiding_open/smpc_share_revealing_open/CLEAR share s",
}

main_node_idx = None
current_signature_ctx = {}


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


def sss_recover_pt(shares: Iterable[Tuple[int, fastecdsa.point.Point]]) -> fastecdsa.point.Point:
    final_point = fastecdsa.point.Point.IDENTITY_ELEMENT
    for x_i, pt_i in shares:
        term_i = 1
        for x_j, _ in shares:
            if x_i == x_j:
                continue
            inv_ij = pow(x_j - x_i, -1, SECP256K1_ORDER)
            term_i = (term_i * x_j * inv_ij) % SECP256K1_ORDER
        final_point += term_i * pt_i
    return final_point


def test_factorize(kr: int, sig_point: fastecdsa.point.Point, verbose: bool = True) -> Tuple[int, bool, int]:
    if verbose:
        print(f"k*r[{kr.bit_length()}] = {kr}")

    factors_of_product: List[int] = []
    remaining = kr
    for factor in range(2, remaining):
        if factor * factor > remaining:
            break
        while remaining % factor == 0:
            factors_of_product.append(factor)
            remaining //= factor
            if verbose:
                print(f"   - found factor: {factor}, remaining {remaining} ({remaining.bit_length()} bits)")
        if factor > (1 << 24):
            break
    if verbose:
        print(f"   - last factor: {remaining}")
    factors_of_product.append(remaining)

    if verbose:
        print(f"TOTAL: {len(factors_of_product)} factors")
    for test_idx, factor_choice in enumerate(itertools.product((0, 1), repeat=len(factors_of_product))):
        value = 1
        for c, f in zip(factor_choice, factors_of_product):
            if c:
                value *= f

        if not (64 - 2 <= value.bit_length() <= 64 + 10):
            continue

        # print(f"Testing {test_idx} [{value.bit_length()}]: {value}")
        assert kr % (value) == 0
        if sig_point == value * SECP256K1_CURVE.G:
            return len(factors_of_product), True, value
    return len(factors_of_product), False, 0


num_found = 0
num_tests = 0

found_private_key = None

with open("../smpc_trace.log") as f:
    for line in f:
        if "Signature:" in line:
            # Check signature
            signature_bytes = bytes.fromhex(line.split()[1])
            sign_rlen = signature_bytes[3]
            sign_slen = signature_bytes[5 + sign_rlen]
            assert len(signature_bytes) == 6 + sign_rlen + sign_slen
            sign_r = int.from_bytes(signature_bytes[4 : 4 + sign_rlen], "big")
            sign_s = int.from_bytes(signature_bytes[6 + sign_rlen :], "big")
            # print(signature_bytes.hex())
            sign_pt = sss_recover_pt(
                [
                    (
                        idx,
                        fastecdsa.point.Point(
                            int.from_bytes(pt_bytes[:32], "big"), int.from_bytes(pt_bytes[32:], "big"), SECP256K1_CURVE
                        ),
                    )
                    for idx, pt_bytes in current_signature_ctx["signpt"].items()
                ]
            )
            assert sign_pt.x % SECP256K1_ORDER == sign_r

            s = sss_recover(
                [(idx, int.from_bytes(share_bytes, "big")) for idx, share_bytes in current_signature_ctx["s"].items()]
            )
            assert s == sign_s

            kr = sss_recover(
                [(idx, int.from_bytes(share_bytes, "big")) for idx, share_bytes in current_signature_ctx["kr"].items()]
            )
            # print(f"k*r[{kr.bit_length()}] = {kr}")
            start_time = datetime.datetime.now()
            len_factors, is_ok, found_k = test_factorize(kr, sign_pt, verbose=True)
            end_time = datetime.datetime.now()
            num_seconds = (end_time - start_time).total_seconds()
            num_tests += 1
            if is_ok:
                num_found += 1
                print(
                    f"[{num_tests:5d}] FOUND with {len_factors:3} in {num_seconds:.2f}, {num_found:5d} ok => {100. * num_found / num_tests:.3f}%"  # noqa
                )
                assert found_k
                assert sign_pt == found_k * SECP256K1_CURVE.G
                z = int.from_bytes(
                    hashlib.sha256(base64.b64decode("8J+RiyBIZWxsbywgd29ybGQg8J+MjSDwn46J")).digest(), "big"
                )
                # s = (r * d + z) / k
                # s*k = r*d + z
                # d = (s*k - z) / r
                found_d = ((sign_s * found_k - z) * pow(sign_r, -1, SECP256K1_ORDER)) % SECP256K1_ORDER
                print(f"=> private key: {found_d:#x}")
                if found_private_key:
                    assert found_private_key == found_d
                else:
                    found_private_key = found_d
            else:
                print(
                    f"[{num_tests:5d}] NOT   with {len_factors:3} in {num_seconds:.2f}, {num_found:5d} ok => {100. * num_found / num_tests:.3f}%"  # noqa
                )
            # Reset
            count_for_pairs = {}
            main_node_idx = None
            current_signature_ctx = {}
            continue

        if "Sending" not in line:
            continue
        matches = re.match(
            r"^\[ *([0-9]+) INFO \] Sending ([0-9]+) bytes from ([0-9]+) to ([0-9]+): ([0-9a-f]+)\s*$", line
        )
        assert matches, line
        from1_txt, len_txt, from2_txt, to_txt, data_hex = matches.groups()
        assert from1_txt == from2_txt
        assert len(data_hex) == 2 * int(len_txt)
        from_idx = int(from1_txt)
        to_idx = int(to_txt)
        data = bytes.fromhex(data_hex)

        if main_node_idx is None:
            main_node_idx = from_idx

        tuple_idx = (from_idx, to_idx)
        if tuple_idx not in count_for_pairs:
            count_for_pairs[tuple_idx] = 0
        count_for_pairs[tuple_idx] += 1

        if count_for_pairs[tuple_idx] < 10 and len(data) == 48:
            count_for_pairs[tuple_idx] = 10  # First RNG data

        state_value = count_for_pairs[tuple_idx]
        state_name = STATE_NAMES.get(state_value, "?")
        # print(f"({state_value:3d}) {from_idx:2d} -> {to_idx:2d}: {state_name} [{len(data):3d}] {data.hex()}")

        if state_value in (16, 23, 29):
            clear_name = {16: "signpt", 23: "kr", 29: "s"}[state_value]
            if clear_name not in current_signature_ctx:
                current_signature_ctx[clear_name] = {}
            if from_idx not in current_signature_ctx[clear_name]:
                current_signature_ctx[clear_name][from_idx] = data
            else:
                assert current_signature_ctx[clear_name][from_idx] == data
