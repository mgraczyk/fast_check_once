#pragma once

#ifndef _POSIX_C_SOURCE 
#define _POSIX_C_SOURCE 200112L 
#endif

#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#if defined(__GNUC__)

#define FORCE_INLINE __attribute__((always_inline))

#else

#error "GCC is currently the only supported compiler."

#endif

#if  !(defined(__linux__) && defined(__x86_64__))
#error "Linux 64bit is currently the only supported platform."
#endif

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#define PAGE_ALIGN(addr, pagesize) \
    ((void *)(((uintptr_t)addr) & ~(((uintptr_t)pagesize)-1)))

typedef bool (*predicate_func_t)();

static void
try_set_page_attr(unsigned char * const addr, const long pagesize, const int attr)
{
    if(mprotect(addr, pagesize, attr)) {
        const int error = errno;

        fflush(stdout);

        fputs("\nError: mprotect set failed\n", stderr);
        if (error == EACCES) {
            fputs("Access Violation\n", stderr);
        } else if (error == EINVAL) {
            fputs("Invalid Pointer\n", stderr);
        } else if (error == ENOMEM) {
            fputs("Out of kernel memory or out of range\n", stderr);
        }
        exit(EXIT_FAILURE);
    }
}

bool
convert_check(bool pred, unsigned char * target)
{
    const unsigned char truePatch[] = {
        // mov $0x1,%eax
        0xb8, 0x01, 0x00, 0x00, 0x00,
    };

    const unsigned char falsePatch[] = {
        // mov $0x0,%eax
        0xb8, 0x00, 0x00, 0x00, 0x00,
    };


    const long pagesize = sysconf(_SC_PAGESIZE);

    unsigned char * const alignedSite = PAGE_ALIGN(target, pagesize);
    const unsigned char * const patch = pred ? truePatch : falsePatch;

    const int writableAttr = PROT_READ|PROT_EXEC|PROT_WRITE;
    try_set_page_attr(alignedSite, pagesize, writableAttr);

    const void * const result = memcpy(target, patch, ARRAY_SIZE(truePatch));

   // TODO: I don't think this is necessary
    __asm__ volatile(
        "clflush %[target] \n"
        "mfence \n"
        ::
        [target]"m"(target),
        [dep]"r"(result) // Make this a dependency so the compiler puts this asm after memcpy
        :
        "memory");

    // TODO: For some reason the test segfaults if PROT_WRITE is removed from normalAttr
    //       We do not want to leave the page writable.
    //       Maybe we have to??
    const int normalAttr = PROT_READ | PROT_EXEC | PROT_WRITE;
    try_set_page_attr(alignedSite, pagesize, normalAttr);

    return pred;
}

void
convert_check_thunk()
    // pred is in rax
    // target is in rsi
{
    // TODO: We could get creative here and derive the
    __asm__ volatile(
        "push %rbx \n"
        "push %rcx \n"
        "push %rdx \n"
        "push %rdi \n"
        "push %rsi \n"
        "push %rbp \n"
        "push %r8 \n"
        "push %r9 \n"
        "push %r10 \n"
        "push %r11 \n"
        "push %r12 \n"
        "push %r13 \n"
        "push %r14 \n"
        "push %r15 \n"

        "mov %rax, %rdi \n"
        "call convert_check \n"

        "pop %r15 \n"
        "pop %r14 \n"
        "pop %r13 \n"
        "pop %r12 \n"
        "pop %r11 \n"
        "pop %r10 \n"
        "pop %r9 \n"
        "pop %r8 \n"
        "pop %rbp \n"
        "pop %rsi \n"
        "pop %rdi \n"
        "pop %rdx \n"
        "pop %rcx \n"
        "pop %rbx \n"
        );
    // Returns pred in RAX
}

#define fast_check(predExpr) \
({ \
     /* TODO: Make sure this first block isn't moved past predExpr evaluation */ \
     __asm__ volatile( \
            "1:\n" \
            "jmp 3f \n"  /* jmp is two bytes */ \
            "nop \n"      /* So we add 3 nops to fit 5 byte mov instruction */ \
            "nop \n" \
            "nop \n" \
            "jmp 1f \n" \
            "3: \n" \
            ); \
    bool pred = (predExpr); \
    __asm__ volatile( \
            "\n" \
            "push %%rsi \n" \
            "push %%rdx \n" \
            "mov $1b, %%rsi \n" \
            "mov $convert_check_thunk, %%rdx \n" \
            "call *%%rdx \n" \
            "pop %%rdx \n" \
            "pop %%rsi \n" \
            "1:\n" \
            : [pred]"+A"(pred) \
            );\
    pred; \
})

#undef FORCE_INLINE
#undef ARRAY_SIZE
