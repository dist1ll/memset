#define _GNU_SOURCE

#include "bits/time.h"
#include "time.h"
#include <assert.h>
#include <fcntl.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <threads.h>
#include <time.h>
#include <x86intrin.h>

/* memset iterations per method */
const int ITER = 5;
const size_t MAP_SIZE = ((size_t)1) << 32;

/* All following functions assume len is page-aligned.  */
void memset_loop(char *addr, char value, size_t len);
void memset_fsrm(char *addr, char value, size_t len);
void memset_avx2(char *addr, char value, size_t len);
void memset_avx2_nt(char *addr, char value, size_t len);
void memset_clzero(char *addr, char value, size_t len);

struct args {
  void (*f)(char *, char, size_t);
  char *addr;
  size_t len;
  int cpu;
};

int thread_memset(void *arg) {
  struct args *a = (struct args *)arg;
  cpu_set_t my_set;
  CPU_ZERO(&my_set);
  CPU_SET(a->cpu, &my_set);
  sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
  (a->f)(a->addr, 0, a->len);
  return 0;
}

unsigned long long diff_timespec(struct timespec *start, struct timespec *end) {
  return ((unsigned long long)(end->tv_sec - start->tv_sec)) * 1000000000 +
         (unsigned long long)(end->tv_nsec - start->tv_nsec);
}

void bench(void (*f)(char *, char, size_t), char *addr, char value,
           int threads) {
  unsigned long long min = -1, max = 0, avg = 0;
  struct timespec start, end;

  struct args as[threads];
  thrd_t ids[threads];

  for (int i = 0; i < ITER; i++) {
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* -- BEGIN MEASURING -- */
    char *tmp = addr;
    for (int j = 0; j < threads; j++) {
      as[j].addr = tmp;
      as[j].len = MAP_SIZE / threads;
      as[j].cpu = j;
      as[j].f = f;
      thrd_create(&ids[j], thread_memset, (void *)&as[j]);
      tmp += MAP_SIZE / threads;
    }
    for (int j = 0; j < threads; j++) {
      thrd_join(ids[j], NULL);
    }
    /* -- END MEASURING -- */

    clock_gettime(CLOCK_MONOTONIC, &end);

    unsigned long long duration = diff_timespec(&start, &end);
    max = max > duration ? max : duration;
    min = min < duration ? min : duration;
    avg += duration / ITER;
  }

  unsigned long long ns_to_ms = 1000000;
  printf("    %5llums   %5llums   %5llums   %7.2f GB/s\n", min / ns_to_ms,
         avg / ns_to_ms, max / ns_to_ms, ((double)MAP_SIZE) / ((double)avg));
}

/**
 * Runs a test suite of all memset versions, for the given number of threads.
 */
void run_suite(char *addr, char value, int threads) {
  printf("  THREADS=%-3d              min       avg       max       avg GB/s\n",
         threads);
  printf("  -----------            -----     -----     -----     ----------\n");
  printf("  memset_loop      ");
  bench(memset_loop, addr, 0, threads);
  printf("  memset_fsrm      ");
  bench(memset_fsrm, addr, 0, threads);
  printf("  memset_avx2      ");
  bench(memset_avx2, addr, 0, threads);
  printf("  memset_avx2_nt   ");
  bench(memset_avx2_nt, addr, 0, threads);
  printf("  memset_clzero    ");
  bench(memset_clzero, addr, 0, threads);
  printf("\n");
}

int main(int argc, char *argv[]) {
  /* allocate and pre-fault 4 gigs of memory */
  char *addr = (char *)mmap(NULL, MAP_SIZE, PROT_WRITE | PROT_READ,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  madvise(addr, MAP_SIZE, MADV_POPULATE_WRITE);
  printf("[*] pre-faulted 4GiB of memory.\n");
  printf("[*] running %d iterations per method...\n", ITER);

  int counts[] = {1, 2, 4, 8, 16};
  for (int i = 0; i < 5; i++) {
    run_suite(addr, 0, counts[i]);
  }

  munmap((void *)addr, MAP_SIZE);
}
