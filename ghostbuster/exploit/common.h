#define _GNU_SOURCE

#include <err.h>
#include <sched.h>

#define SECRET_SIZE      32

int run_attacker(int argc, char * const *argv);
void init_attacker(void);

static inline void put_on_cpu(unsigned int cpu_num)
{
  cpu_set_t my_set;

  CPU_ZERO(&my_set);
  CPU_SET(cpu_num, &my_set);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set) != 0) {
    err(1, "sched_setaffinity");
  }
}

static inline __attribute__((always_inline)) void clflush(const void* addr)
{
  __asm__ volatile("clflush (%0)\n" ::"r"(addr));
}

static inline __attribute__((always_inline)) uint64_t rdtscp(void) {
	uint64_t lo, hi;

	asm volatile("rdtscp\n" : "=a" (lo), "=d" (hi) :: "rcx");

	return (hi << 32) | lo;
}
