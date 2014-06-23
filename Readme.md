# Fast One Time Predicate Checker
===============


This micro-optimization module converts a one time predicate check into a constant always taken branch.

For example, the code
    
```
bool long_computation() { .... }


if (fast_check(long_computation())) {
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
control:        0.329808754
control_inv:	0.343990744
fast_check:     0.017974983
fast_check_inv:	0.006938278
$>
```

## TODO (In order)

- More tests

- LLVM Linux x86-64 support
- GCC Linux IA-32 support
- LLVM Linux IA-32 support
- MSVC Windows x86-64 support
- MSVC Windows IA-32 support

- GCC/LLVM ARMv7/8 support
