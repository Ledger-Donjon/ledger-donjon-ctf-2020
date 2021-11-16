import enum

from ledgerwallet.client import LedgerClient
from ledgerwallet.transport import enumerate_devices

initial_array = [
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


class CrackmeIns(enum.IntEnum):
    RESET = 1
    FILL = 2
    CHECK = 3


def invert_permutation_table(table):
    inv_table = [0] * len(table)
    for i in range(len(table)):
        inv_table[table[i]] = i
    return inv_table


FIXED_PERM = [52, 12, 78, 61, 42, 73, 19, 24, 40, 41, 28, 32, 0, 62, 56, 70, 48, 66, 22, 65, 11, 53, 71, 37, 33, 75,
              26, 44, 67, 80, 68, 72, 76, 57, 45, 4, 64, 10, 43, 46, 54, 34, 35, 2, 58, 25, 8, 39, 55, 13, 9, 14,
              30, 6, 16, 15, 63, 27, 47, 51, 23, 1, 38, 5, 69, 60, 29, 21, 74, 7, 31, 36, 3, 18, 17, 77, 49, 79, 50,
              20, 59]
INV_FIXED_PERM = invert_permutation_table(FIXED_PERM)


def create_coefficients_list():
    # Lines
    for i in range(0, 81, 9):
        print(INV_FIXED_PERM[i:i + 9])
    # Columns
    for i in range(9):
        print(INV_FIXED_PERM[i::9])
    # Squares
    print([[INV_FIXED_PERM[9 * i + 3 * j:9 * i + 3 * j + 3] for i in range(9)] for j in range(3)])


grid_numbers = {0: 0,
                1: 37, 2: 5, 3: 11, 4: 43, 5: 13, 6: 19, 7: 53, 8: 23, 9: 31
                }


def main():
    device = enumerate_devices()[0]
    client = LedgerClient(device)
    client.apdu_exchange(CrackmeIns.RESET)

    for i in range(len(initial_array)):
        if initial_array[i] == 0:
            client.apdu_exchange(CrackmeIns.FILL, bytes([INV_FIXED_PERM[i], grid_numbers[solution[i]]]))

    print(client.apdu_exchange(CrackmeIns.CHECK))


if __name__ == "__main__":
    main()
