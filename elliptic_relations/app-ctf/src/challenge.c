#include "challenge.h"

#include "os.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cx.h"

char array[GRID_SIZE];

void reset_grid() {
  static const int8_t initial_array[] = {11, 19, 43, 19, 53, 31, 53,
                                         5,  43, 13, 13, 53, 23, 23,
                                         37, 37, 37, 13, 31, 23, 11};
  for (int i = 0; i < 21; i++) {
    array[i] = initial_array[i];
  }
  for (int i = 21; i < GRID_SIZE; i++) {
    array[i] = 0;
  }
}

int fill_grid_entry(unsigned int pos, int value) {
  uint8_t tmp1[1], tmp2[1];

  if (array[pos] != 0 || pos >= GRID_SIZE) {
    return -1;
  }

  tmp1[0] = value;
  tmp2[0] = 64;
  if (!cx_math_is_prime(tmp1, 1) || cx_math_cmp(tmp1, tmp2, 1) >= 0) {
    return -1;
  }

  array[pos] = value;
  return 0;
}

static const uint8_t expected_point[65] = {
    0x04, 0x64, 0x50, 0x77, 0xFC, 0x8A, 0xA9, 0x31, 0x7B, 0x19, 0xA4,
    0xDC, 0xBD, 0x3D, 0x1F, 0x52, 0xBE, 0x4B, 0x98, 0xA5, 0x69, 0xA4,
    0x46, 0x96, 0x12, 0x75, 0x27, 0x2C, 0xC2, 0xD3, 0xAA, 0x67, 0x4A,
    0x73, 0x15, 0xF0, 0x7C, 0x2E, 0x67, 0xC2, 0xB1, 0xEB, 0xE6, 0xED,
    0x8A, 0x52, 0xB2, 0x50, 0xBD, 0x99, 0xE2, 0xC9, 0xF8, 0xF8, 0xE9,
    0x75, 0xAF, 0x53, 0xD7, 0x95, 0x41, 0x20, 0x10, 0x3D, 0xFA};

bool check_grid(const char array[GRID_SIZE]) {
  cx_ecfp_private_key_t private_key;
  cx_ecfp_public_key_t public_key;

  const int8_t pos[] = {
      56, 36, 19, 17, 28, 30, 64, 15, 22, 53, 69, 46, 55, 54, 74, 7,  45, 26,
      52, 70, 11, 47, 8,  9,  16, 76, 78, 35, 49, 18, 70, 8,  76, 44, 28, 32,
      31, 5,  68, 25, 32, 75, 2,  77, 29, 61, 37, 6,  10, 23, 39, 48, 36, 5,
      40, 48, 14, 33, 44, 80, 65, 3,  13, 12, 61, 43, 50, 37, 20, 73, 6,  79,
      72, 1,  67, 52, 47, 16, 33, 17, 25, 33, 44, 80, 17, 28, 30, 25, 32, 75,
      72, 35, 63, 1,  49, 51, 67, 18, 60, 69, 54, 45, 41, 38, 0,  3,  15, 77,
      12, 61, 43, 72, 35, 63, 53, 69, 46, 71, 23, 62, 47, 8,  9,  4,  38, 27,
      57, 10, 66, 71, 23, 62, 34, 39, 58, 73, 6,  79, 67, 18, 60, 7,  45, 26,
      34, 39, 58, 16, 76, 78, 59, 0,  21, 46, 74, 26, 42, 27, 21, 13, 22, 29,
      65, 3,  13, 64, 15, 22, 2,  77, 29, 43, 20, 79, 66, 62, 58, 14, 19, 68,
      40, 48, 14, 56, 36, 19, 31, 5,  68, 50, 37, 20, 1,  49, 51, 55, 54, 74,
      24, 41, 42, 4,  38, 27, 59, 0,  21, 63, 51, 60, 11, 9,  78, 80, 30, 75,
      53, 55, 7,  24, 4,  59, 65, 64, 2,  57, 10, 66, 52, 70, 11, 24, 41, 42,
      12, 50, 73, 57, 71, 34, 40, 56, 31};

  for (size_t i = 0; i < sizeof(pos) / NUM_COLS; i++) {
    cx_ecfp_init_private_key(CX_CURVE_SECP256K1,
                             (uint8_t *)&array[pos[NUM_COLS * i]], 1,
                             &private_key);
    cx_ecfp_generate_pair(CX_CURVE_SECP256K1, &public_key, &private_key, 1);

    // added for speculos, which does not throw an exception when
    // cx_ecfp_generate_pair fails.
    if (public_key.W_len == 0) {
        return false;
    }

    for (int j = 1; j < 9; j++) {
      if (array[pos[NUM_COLS * i + j]] == 0) {
        return false;
      }
      cx_ecfp_scalar_mult(public_key.curve, public_key.W, public_key.W_len,
                          (uint8_t *)&array[pos[NUM_COLS * i + j]], 1);
    }
    if (public_key.W_len != sizeof(expected_point) ||
        memcmp(public_key.W, expected_point, sizeof(expected_point)) != 0) {
      return false;
    }
  }
  return true;
}
