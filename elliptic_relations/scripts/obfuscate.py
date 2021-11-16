import hashlib
import itertools
import random

from ecdsa.curves import SECP256k1

# Grid has been taken from
# https://www.telegraph.co.uk/news/science/science-news/9359579/Worlds-hardest-sudoku-can-you-crack-it.html
# It is supposed to be the hardest Sudoku ever made. I wanted a grid that had few numbers filled.

INITIAL_ARRAY = [
    8, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 6, 0, 0, 0, 0, 0,
    0, 7, 0, 0, 9, 0, 2, 0, 0,
    0, 5, 0, 0, 0, 7, 0, 0, 0,
    0, 0, 0, 0, 4, 5, 7, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 3, 0,
    0, 0, 1, 0, 0, 0, 0, 6, 8,
    0, 0, 8, 5, 0, 0, 0, 1, 0,
    0, 9, 0, 0, 0, 0, 4, 0, 0
]

solution = [
    8, 1, 2, 7, 5, 3, 6, 4, 9,
    9, 4, 3, 6, 8, 2, 1, 7, 5,
    6, 7, 5, 4, 9, 1, 2, 8, 3,
    1, 5, 4, 2, 3, 7, 8, 9, 6,
    3, 6, 9, 8, 4, 5, 7, 2, 1,
    2, 8, 7, 1, 6, 9, 5, 3, 4,
    5, 2, 1, 9, 7, 4, 3, 6, 8,
    4, 3, 8, 5, 2, 6, 9, 1, 7,
    7, 9, 6, 3, 1, 8, 4, 5, 2
]


def permute_sudoku():
    """
    Generate a random list of positions such as:
    - All the first entries contain a positions whose value on the Sudoku grid is already set
    - All the remaining entries are not set

    This produces a list like [p1, p2, p3, ... p11, 0, 0, 0, 0..., 0]
    """
    already_set = []
    unknown = []
    for i, value in enumerate(INITIAL_ARRAY):
        if value == 0:
            unknown.append(i)
        else:
            already_set.append(i)
    random.shuffle(unknown)
    random.shuffle(already_set)
    return already_set + unknown


def get_random_primes():
    """
    Return a set of distinct random primes < 64
    """
    out = set()
    primes_list = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61]
    while len(out) < 9:
        out.add(random.choice(primes_list))
    return out


FIXED_PERM = [52, 12, 78, 61, 42, 73, 19, 24, 40, 41, 28, 32, 0, 62, 56, 70, 48, 66, 22, 65, 11, 53, 71, 37, 33, 75,
              26, 44, 67, 80, 68, 72, 76, 57, 45, 4, 64, 10, 43, 46, 54, 34, 35, 2, 58, 25, 8, 39, 55, 13, 9, 14,
              30, 6, 16, 15, 63, 27, 47, 51, 23, 1, 38, 5, 69, 60, 29, 21, 74, 7, 31, 36, 3, 18, 17, 77, 49, 79, 50,
              20, 59]

INV_FIXED_PERM = [0] * 81
for i in range(len(FIXED_PERM)):
    INV_FIXED_PERM[FIXED_PERM[i]] = i


def create_lines_relations():
    coefs = []
    for i in range(0, 81, 9):
        coefs.append(INV_FIXED_PERM[i:i + 9])
    return coefs


def create_columns_relations():
    coefs = []
    for i in range(9):
        coefs.append(INV_FIXED_PERM[i::9])
    return coefs


"""
Squares:

{0, 1, 2, 9, 10, 11, 18, 19, 20},
{3, 4, 5, 12, 13, 14, 21, 22, 23},
{6, 7, 8, 15, 16, 17, 24, 25, 26},

{27, 28, 29, 36, 37, 38, 45, 46, 47},
{30, 31, 32, 39, 40, 41, 48, 49, 50},
{33, 34, 35, 42, 43, 44, 51, 52, 53},

{54, 55, 56, 63, 64, 65, 72, 73, 74},
{57, 58, 59, 66, 67, 68, 75, 76, 77},
{60, 61, 62, 69, 70, 71, 78, 79, 80}
"""


def create_inner_square_relations():
    coefs = []
    for i in range(3):
        l = []
        for j in range(9):
            l += INV_FIXED_PERM[9 * j + 3 * i:9 * j + 3 * i + 3]
        coefs.append(l[:9])
        coefs.append(l[9:18])
        coefs.append(l[18:27])
    return coefs


# random substitution table. each digit is replaced with a random prime
GRID_NUMBERS = {0: 0,
                1: 37, 2: 5, 3: 11, 4: 43, 5: 13, 6: 19, 7: 53, 8: 23, 9: 31
                }


def encrypt_flag(flag: str) -> bytes:
    """
    Flag is simply xored with the digest of the mixed content of the Sudoku grid.
    """
    # apply substitution and permutation on the original solution
    final_grid = bytes([GRID_NUMBERS[solution[c]] for c in FIXED_PERM])
    md = hashlib.sha256(final_grid).digest()

    flag_bytes = flag.encode()
    encoded_flag = bytes([md[i] ^ flag_bytes[i] for i in range(len(flag_bytes))])
    return encoded_flag


def compute_expected_point():
    """
    Each line / column / inner square is checked by multiplying each value it contains with the curve generator of
    # secp256k1. Each value being a prime number, there is a unique solution. This function retrieves the coordinates
    of the expected point.
    """
    g1 = SECP256k1.generator
    for k in GRID_NUMBERS.values():
        if k != 0:
            g1 = g1 * k
    serialized_point = b"\x04"  # uncompressed point
    serialized_point += int.to_bytes(g1.x(), 32, "big")
    serialized_point += int.to_bytes(g1.y(), 32, "big")
    return serialized_point


def main():
    print(encrypt_flag("CTF{r4nD0m1Z3d_sUD0ku_FTW}").hex())

    # Create list of relations to be checked by the crackme.
    # Sudoku grid is mixed using the FIXED_PERM table, computed with permute_sudoku().
    # Relations take this permutation into account.
    rels = create_lines_relations()
    rels += create_columns_relations()
    rels += create_inner_square_relations()
    random.shuffle(rels)
    print([c for c in itertools.chain(*rels)])

    # Sudoku values are substituted: 1..9 are replaced with the small random primes of PRIME_NUMBERS.
    # The get_random_primes() function has been used to create this dictionary.
    print(get_random_primes())

    print(compute_expected_point().hex())


if __name__ == "__main__":
    main()
