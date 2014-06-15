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

// TODO: Any way to check this at compile time?
#define FAST_CHECK_CODE_SIZE 9

typedef bool (*predicate_func_t)();

static FORCE_INLINE bool
fast_check_function(predicate_func_t fpred)
{
    uintptr_t rtmp;
    __asm__ volatile(
        "mov $convert_check_pfunc_thunk, %[rtmp] \n"
        "call *%[rtmp] \n"
        :
        [rtmp]"=r"(rtmp)
        :
        "D"(fpred)
        );

    // never reached
    return fpred;
}

static FORCE_INLINE bool
fast_check(bool predValue)
{
    uintptr_t rtmp;
    __asm__ volatile(
        "mov $convert_check_thunk, %[rtmp] \n"
        "call *%[rtmp] \n"
        :
        [rtmp]"=r"(rtmp)
        :
        "D"(predValue)
        );

    // never reached
    return predValue;
}

bool
convert_check(bool pred, unsigned char * returnSite)
{
    const unsigned char truePatch[FAST_CHECK_CODE_SIZE] = {
        // mov %0x1,%eax
        0xb8, 0x01, 0x00, 0x00, 0x00,

        // 0x66... 2e 0f 1f is nopw with 66 operand prefix
        //0x66, 0x2e, 0x0f, 0x1f,
        0x0f, 0x1f, 0x40, 0x00
        
    };

    const unsigned char falsePatch[FAST_CHECK_CODE_SIZE] = {
        // xor %eax,%eax
        0x31, 0xc0,

        // 0x66... 2e 0f 1f is nopw with 66 operand prefix
        //0x66, 0x66, 0x66, 0x66, 0x2e, 0x0f, 0x1f,
        0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00,
    };

    unsigned char * restrict const callSite = returnSite - FAST_CHECK_CODE_SIZE;

    const unsigned int pagesize = getpagesize();

    const unsigned char * patch =
        pred ? truePatch : falsePatch;

    unsigned char * const alignedSite = PAGE_ALIGN(callSite, pagesize);
    const int writableAttr = PROT_READ|PROT_EXEC|PROT_WRITE;
    if(mprotect(alignedSite, pagesize, writableAttr))
    {
        const int error = errno;
        printf("mprotect set failed: %p->%p\n", callSite, alignedSite);
        if (error == EACCES) {
            puts("Access Violation");
        } else if (error == EINVAL) {
            puts("Invalid Pointer");
        } else if (error == ENOMEM) {
            puts("Out of kernel memory or out of range");
        }
        exit(EXIT_FAILURE);
    } 

    const void * const result = memcpy(callSite, patch, FAST_CHECK_CODE_SIZE);
   
    __asm__ volatile(
        "clflush %[callSite] \n"
        "mfence \n"
        ::
        [callSite]"m"(callSite),
        [dep]"r"(result) // Make this a dependency so the compiler puts this asm after memcpy
        );

    // TODO: For some reason the test segfaults if PROT_WRITE is removed from normalAttr
    //       We do not want to leave the page writable.
    //       Maybe we have to??
    const int normalAttr = PROT_READ | PROT_EXEC | PROT_WRITE;
    if(mprotect(alignedSite, pagesize, normalAttr))
    {
        const int error = errno;
        printf("mprotect reset failed: %p->%p\n", callSite, alignedSite);
        if (error == EACCES) {
            puts("Access Violation");
        } else if (error == EINVAL) {
            puts("Invalid Pointer");
        } else if (error == ENOMEM) {
            puts("Out of kernel memory or out of range");
        }
        exit(EXIT_FAILURE);
    } 

    return pred;
}

#define PUSH_ALL_REGS   \
        "push %rax \n"  \
        "push %rbx \n"  \
        "push %rcx \n"  \
        "push %rdx \n"  \
        "push %rbp \n"  \
        "push %rdi \n"  \
        "push %rsi \n"  \
        "push %r8 \n"   \
        "push %r9 \n"   \
        "push %r10 \n"  \
        "push %r11 \n"  \
        "push %r12 \n"  \
        "push %r13 \n"  \
        "push %r14 \n"  \
        "push %r15 \n"

#define POP_ALL_REGS   \
        "pop %r15 \n"  \
        "pop %r14 \n"  \
        "pop %r13 \n"  \
        "pop %r12 \n"  \
        "pop %r11 \n"  \
        "pop %r10 \n"  \
        "pop %r9 \n"   \
        "pop %r8 \n"   \
        "pop %rsi \n"  \
        "pop %rdi \n"  \
        "pop %rbp \n"  \
        "pop %rdx \n"  \
        "pop %rcx \n"  \
        "pop %rbx \n"  \
        "pop %rax \n"  \

void
convert_check_thunk(bool pred)
{
    __asm__ volatile(
        PUSH_ALL_REGS

        // Get return pointer in rsi
        // pred is already in rdi
        "mov 0x78(%rsp), %rsi \n"
        "call convert_check \n"

        POP_ALL_REGS
        );
}
void
convert_check_pfunc_thunk(predicate_func_t fpred)
{
    __asm__ volatile(
        PUSH_ALL_REGS 

        // Get predicate value fron func in rdi
        "callq *%rdi \n"
        "mov %rax, %rdi \n"

        // Get return pointer in rsi
        "mov 0x78(%rsp), %rsi \n"
        "call convert_check \n"

        POP_ALL_REGS 
        );
}

#undef FORCE_INLINE
