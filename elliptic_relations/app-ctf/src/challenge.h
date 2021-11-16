#pragma once

#include <stdbool.h>

#define NUM_COLS 9
#define NUM_LINES 9

#define GRID_SIZE (NUM_LINES * NUM_COLS)

extern char array[GRID_SIZE];

void reset_grid();
bool check_grid(const char array[GRID_SIZE]);
int fill_grid_entry(unsigned int pos, int value);
