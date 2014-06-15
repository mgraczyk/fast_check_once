# Fast One Time Predicate Checker
===============


This micro-optimization module converts a one time predicate check into a constant always taken branch.

For example, the code
    
```
bool long_computation() { .... }


if (fast_check_function(long_computation)) {
    do_something();
}
```

will call run long_computation() once, then convert the call site to

```
if (1) { // or 0 if long_computation returned false
    do_something();
}
```

Currently only GCC on Linux x86-64 is supported.


# Benchmark from test.c

```
$> make
$> ./test
Benchmark Results:
Normal:     0.332003022
fast_check: 0.336111127
fast_check_function: 0.000001452
$>
```

# TODO

The optimization is currently only a performance improvement when the caller uses a predicate function.  The implemented needs to be tweaked to exhibit performance improvements for the no-function cases (fast_check instead of fast_check_function).
