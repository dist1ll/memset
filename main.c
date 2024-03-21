/* Testing speed of memory setting on x86_64 hardware */
#include "bits/time.h"
#include "time.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <x86intrin.h>

/* memset iterations per method */
const int ITER = 10;

/* All following functions assume len is page-aligned.  */
void memset_loop(char *addr, char value, size_t len);
void memset_fsrm(char *addr, char value, size_t len);
void memset_avx2(char *addr, char value, size_t len);

unsigned long long diff_timespec(struct timespec *start, struct timespec *end) {
  return ((unsigned long long)(end->tv_sec - start->tv_sec)) * 1000000000 +
         (unsigned long long)(end->tv_nsec - start->tv_nsec);
}

void bench(void (*f)(char *, char, size_t), char *addr, char value,
           size_t len) {
  unsigned long long min = -1, max = 0, avg = 0;
  struct timespec start, end;
  for (int i = 0; i < ITER; i++) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    f(addr, value, len);
    clock_gettime(CLOCK_MONOTONIC, &end);

    unsigned long long duration = diff_timespec(&start, &end);
    max = max > duration ? max : duration;
    min = min < duration ? min : duration;
    avg += duration / ITER;
  }

  unsigned long long ns_to_ms = 1000000;
  printf(":   %5llums   %5llums   %5llums   %7.2f GB/s\n", min / ns_to_ms,
         avg / ns_to_ms, max / ns_to_ms, ((double)len) / ((double)avg));
}

int main(int argc, char *argv[]) {
  /* allocate and pre-fault 4 gigs of memory */
  size_t map_size = ((size_t)1) << 32;
  char *addr = (char *)mmap(NULL, map_size, PROT_WRITE | PROT_READ,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  madvise(addr, map_size, MADV_POPULATE_WRITE);
  printf("[*] pre-faulted 4GiB of memory.\n");
  printf("[*] running %d iterations per method...\n", ITER);

  printf("                       min       avg       max       avg GB/s\n");
  printf("    memset_loop");
  bench(memset_loop, addr, 0, map_size);
  printf("    memset_fsrm");
  bench(memset_fsrm, addr, 0, map_size);
  printf("    memset_avx2");
  bench(memset_avx2, addr, 0, map_size);

  munmap((void *)addr, map_size);
}
