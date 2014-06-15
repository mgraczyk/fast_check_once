#pragma once

// TODO: For some reason including inttypes before time.h makes
//       CLOCK_PROCESS_CPUTIME_ID undefined
#ifndef _POSIX_C_SOURCE 
#define _POSIX_C_SOURCE 200112L
#endif

#include <time.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef __GNUC__

#define FORCE_INLINE __attribute__((always_inline))

#else

#define FORCE_INLINE inline
#error "GCC is currently the only supported compiler."

#endif

#ifdef __cplusplus 
extern "C" {
#endif

typedef void (*benchmark_func_t)();
typedef struct timespec timespec_t;

static FORCE_INLINE uint64_t get_ticks()
{
    uint64_t ticks;

    __asm__ volatile (
        "rdtsc \n"
        "shl $32, %%rdx \n"
        "or %%rdx, %[ticks] \n"
        :
        [ticks]"=a"(ticks)
        ::
        "rdx");

    return ticks;
}

static timespec_t
timespec_diff(timespec_t start, timespec_t end)
{
    timespec_t temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

static FORCE_INLINE void timespec_print(timespec_t ts)
{
    printf("%lld.%.9ld", (long long)ts.tv_sec, ts.tv_nsec);
}

static FORCE_INLINE void
finish_pending()
{
    __asm__ volatile(
        "mfence \n"
        "wait \n"
        );
}

static FORCE_INLINE timespec_t
run_benchmark(benchmark_func_t func, size_t iterations)
{
    // TODO: Make sure this loop can't be unrolled
    int rounds = 2; 
    timespec_t start;
    timespec_t end;

    const pid_t pid = getpid();
    const int beforeNice = getpriority(PRIO_PROCESS, pid);

    // Ignore errors in case we didn't run as root
    setpriority(PRIO_PROCESS, pid, -20);

    do {
        finish_pending();

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (size_t i=0; i < iterations; ++i) {
            func();
        }
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
    } while(rounds--); 

    setpriority(PRIO_PROCESS, pid, beforeNice);
    return timespec_diff(start, end);
}

#ifdef __cplusplus
}
#endif

#undef FORCE_INLINE


