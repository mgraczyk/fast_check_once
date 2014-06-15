#include "timit.h"
#include "fast_check.h"

#include <stdio.h>
#include <time.h>

static volatile int x = 0;
static volatile int y = 0;

bool
test_predicate() {
    time_t timer = time(NULL);
    struct tm *tm_time = gmtime(&timer);
    bool oddDay = tm_time->tm_yday % 2;

    return oddDay;
}

static inline void bmark_normal_check()
{
    if (test_predicate()) {
        ++x;
    }
}

static inline void bmark_fast_check_function()
{
    if (fast_check_function(test_predicate)) {
        ++x;
    }
}

static inline void bmark_fast_check()
{
    if (fast_check(test_predicate())) {
        ++x;
    }
}

int main() {
    const unsigned long long iterations = 10000000L;

    fputs("Benchmark Results:\n", stdout);

    fputs("Normal:     ", stdout);
    timespec_print(run_benchmark(bmark_normal_check, iterations));
    fputs("\n", stdout);

    fputs("fast_check: ", stdout);
    timespec_print(run_benchmark(bmark_fast_check, iterations));
    fputs("\n", stdout);

    fputs("fast_check_function: ", stdout);
    timespec_print(run_benchmark(bmark_fast_check_function, iterations));
    fputs("\n", stdout);
}


