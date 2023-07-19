// Thread.cpp
// Implementation of the Thread class.

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


// Loading or storing the sp using ldr or str generates the warning:
// "This instruction may be unpredictable if executed on M-profile cores with interrupts enabled."
// Since the instructions below which trigger this warning are executed with interrupts off,
// it is safe to ignore the warning. You can add the option:
// "-Wa,--no-warn", TO THIS FILE ONLY
// to suppress the assembler warning. I recommend that you remove that option if you make any
// changes to this module, until you have tested the changes.

#include <context.hpp>
#include <stdint.h>
#include "cmsis.h"


Context BackgroundContext;


// Suspend the current thread into a Thread object,
// pop the next thread from the pending stack into the
// register set, and resume running it.
__NOINLINE
__NAKED
void Context::suspend()
    {
    __asm__ __volatile__(
    STORE_CONTEXT                                   // save the CPU registers to the current thread struct
"   mov     r3, r9                      \n"         // unlink current context from the ready chain
"   ldr     r9, [r3, #40]               \n"         // point r9 to the "next" pointer of the old thread
    );

    suspend_switch();
    }

__NOINLINE
__NAKED
void Context::suspend_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT                                    // load the new thread into the CPU registers
"   bx      lr                          \n"
    );                                              // return to the un-pending thread right after its call to resume
    }


// Push the current thread onto the pending chain, and
// resume the first thread in a chain
__NOINLINE
__NAKED
void Context::resume()
    {
    __asm__ __volatile__(
    STORE_CONTEXT
"   str     r9, [r0, #40]               \n"         // save r9 to the "next" pointer of the new thread
"   mov     r9, r0                      \n"         // the new thread becomes the head of the ready chain
    );

    this->resume_switch();
    }

__NOINLINE
__NAKED
void Context::resume_switch()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT                                    // load the new thread into the CPU registers
"   bx      lr                          \n"
    );
    }


// Start a new thread, given its code and initial stack pointer.
// args: fn:    a pointer to a subroutine which will be run as a thread
//       newsp: the initial stack pointer of the new thread

// The newsp must point to the second word from the high end of the stack.
// The word pointed to by newsp is reserved to pass the thread's return value
// during the rendezvous at the completion of the thread. The second word
// past newsp is used for the thread's "done" flag.
//
// The thread which calls spawn is pushed to the pending stack as if it
// had called "resume". The calling thread will be unpended the first time
// the new thread suspends itself. The calling thread will continue
// executing after the call to "start". It will most likely continue executing
// before the new thread terminates.
//
// When the new thread terminates by returning from its subroutine, it returns
// to this code, which will run the next thread from the pending stack. This
// is not necessarily the thread which created the terminated thread.

__NOINLINE
__NAKED
void Context::start(THREADFN *fn, char *newsp, uintptr_t arg)
    {
    (void)fn;
    (void)newsp;
    (void)arg;

    __asm__ __volatile__(
    STORE_CONTEXT
"   str     r9, [r0, #40]               \n"
"   mov     r9, r0                      \n"         // link the new context onto the head of the ready chain
    );

    start_switch1();
    }

__NOINLINE
__NAKED
void Context::start_switch1()
    {
    __asm__ __volatile__(
"   mov     sp, r2                      \n"         // setup the new thread's stack
"   mov     r2, #0                      \n"         //
"   str     r2, [sp, #0]                \n"         // clear the return value
"   str     r2, [sp, #4]                \n"         // and the "done" flag
"   mov     r0, r3                      \n"         // pass the arg in r0
"   msr     primask, ip                 \n"         // restore the new thread's interrupt state
"   blx     r1                          \n"         // start executing the code of the new thread

// when the new thread terminates, it returns here...
// it is assumed that the sp at this point has its initial value (the thread's stack usage is balanced)

"   cpsid   i                           \n"         // disable interrupts
"   mov     r1, #1                      \n"         //
"   str     r0, [sp, #0]                \n"         // save the return value
"   str     r1, [sp, #4]                \n"         // set the "done" flag

"   mov     r3, r9                      \n"         // unlink current context from the ready chain
"   ldr     r9, [r3, #40]               \n"
    );

    start_switch2();
    }

__NOINLINE
__NAKED
void Context::start_switch2()
    {
    __asm__ __volatile__(
    LOAD_CONTEXT
"   bx      lr                          \n"
    );
    }


// init the threading system
// At present, this consists only of pointer the current thread pointer to the background thread's struct
void Context::init()
    {
    __asm__ __volatile__(
"   mov r9, %[bgctx]"                                     // init the thread stack pointer
    :
    : [bgctx]"r"(&BackgroundContext)
    :
    );
    }


// results of optimization steps
// >t 5000000
// 13,708,501 microseconds
// 13,308,256 microseconds
// 12,908,006 microseconds
// 12,707,884 microseconds
// 12,507,722 microseconds
// nearly 10% improvemebt
