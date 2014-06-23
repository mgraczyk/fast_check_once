#include "timit.h"
#include "fast_check.h"

#include <stdio.h>
#include <inttypes.h>
#include <time.h>

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#define NUM_TESTS 2

static volatile int xRef[NUM_TESTS] = {};
static volatile int xTest[NUM_TESTS] = {};

bool
test_predicate() {
    time_t timer = time(NULL);
    struct tm *tm_time = gmtime(&timer);
    bool oddDay = tm_time->tm_yday % 2;

    return oddDay;
}

bool
test_predicate_inv() {
    return !test_predicate();
}

static inline void bmark_normal_check()
{
    if (test_predicate()) {
        ++xRef[0];
    }
}

static inline void bmark_normal_check_inv()
{
    if (test_predicate_inv()) {
        ++xRef[1];
    }
}

static inline void bmark_fast_check()
{
    if (fast_check(test_predicate())) {
        ++xTest[0];
    }
}

static inline void bmark_fast_check_inv()
{
    if (fast_check(test_predicate_inv())) {
        ++xTest[1];
    }
}


/// Returns true on failure, false on pass.
static bool
assert_equal(uintmax_t actual, uintmax_t expected)
{
    bool neq = actual != expected;
    if (neq) {
        fprintf(stderr, "ERROR: Actual != Expected ->  %ju != %ju\n", actual, expected);
    }
    return neq;
}

static int
self_check()
{
    const unsigned long long iterations = 10000000L;

    fputs("Benchmark Results:\n", stdout);

    fputs("control:\t", stdout);
    timespec_print(run_benchmark(bmark_normal_check, iterations));
    fputs("\n", stdout);

    fputs("control_inv:\t", stdout);
    timespec_print(run_benchmark(bmark_normal_check_inv, iterations));
    fputs("\n", stdout);

    fputs("fast_check:\t", stdout);
    timespec_print(run_benchmark(bmark_fast_check, iterations));
    fputs("\n", stdout);

    fputs("fast_check_inv:\t", stdout);
    timespec_print(run_benchmark(bmark_fast_check_inv, iterations));
    fputs("\n", stdout);

    int result = 0;
    for (size_t i = 0; i < NUM_TESTS; ++i) {
        result += assert_equal(xTest[i], xRef[i]);
    }

    return result;
}

int main() {
    return self_check();
}


