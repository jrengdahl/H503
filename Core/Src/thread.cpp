// Thread.cpp
// Implementation of the Thread class.

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdint.h>
#include "thread.hpp"

// Define the maximum depth of the thread stack -- how deep can "resume" calls be nested.
const unsigned THREAD_DEPTH = 8;

// This is the pending thread stack.
static Thread thread_stack[THREAD_DEPTH];

// Push the current thread onto the pending stack, and
// resume a thread saved in a Thread object.
bool Thread::resume()
    {
    __asm__ __volatile__(
"   mrs     ip, primask                 \n"         // save interrupt state
"   cpsid   i                           \n"         // disable interrupts
"   ldr     r2, [r0]                    \n"         // look at the sp in the new thread
"   cbz     r2, 0f                      \n"         // go return false if sp is zero
"   mov     r3, #0                      \n"         // zero the sp in the Thread buffer
"   str     r3, [r0], #4                \n"         // and increment r0
"   mov     r3, sp                      \n"         // sp cannot be pushed, so move it to ip first
"   stmdb   r9!, {r3-r8, r10-ip, lr}    \n"         // push all the non-volatile registers of the current thread onto the thread stack
"   ldmia   r0,  {r4-r8, r10-ip, lr}    \n"         // load the rest of the non-volatile registers of the new thread into the registers
"   mov     sp, r2                      \n"         // move ip back to sp
"   msr     primask, ip                 \n"         // restore the new thread's interrupt state
"   bx      lr                          \n"         // return to the new thread

"0: mov     r0, #0                      \n"         // return false
"   msr     primask, ip                 \n"         // restore previous interrupt state
"   bx      lr                          \n"         //
    );

    return false;                                   // dummy return to keep the compiler happy
    }

// Suspend the current thread into a Thread object,
// pop the next thread from the pending stack into the
// register set, and resume running it.
void Thread::suspend()
    {
    __asm__ __volatile__(
"   mrs ip, primask                     \n"         // save interrupt state
"   cpsid i                             \n"         // disable interrupts
"   mov r3, sp                          \n"         // save sp in ip so it can be saved using STM
"   stmia r0,   {r3-r8, r10-ip, lr}     \n"         // save the non-volatile registers of the current thread into the Thread instance
"   ldmia r9!,  {r3-r8, r10-ip, lr}     \n"         // pop the non-volatile registers of the most recently active thread on the pending stack
"   mov sp, r3                          \n"         // restore sp
"   msr primask, ip                     \n"         // restore the new thread's interrupt state
"   mov r0, #1                          \n"         // return a "true" from the resume call
    );                                              // return to the un-pending thread right after it's call to resume
    }


// Start a new thread, given it's code and initial stack pointer.
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
// When the new thread terminates by returning from it's subroutine, it returns
// to this code, which will run the next thread from the pending stack. This
// is not necessarily the thread which created the terminated thread.

void Thread::start(THREADFN *fn, char *newsp)
    {
    __asm__ __volatile__(
"   mrs ip, primask                     \n"         // save interrupt state
"   cpsid i                             \n"         // disable interrupts
"   mov r3, sp                          \n"         // save the sp of the calling thread in a register that can be pushed
"   stmdb r9!, {r3-r8, r10-ip, lr}      \n"         // push the calling thread onto the pending thread stack

"   mov sp, %[newsp]                    \n"         // setup the new thread's stack
"   mov r1, #0                          \n"         //
"   str r1, [sp, #0]                    \n"         // clear the return value
"   str r1, [sp, #4]                    \n"         // and the "done" flag

"   msr primask, ip                     \n"         // restore the new thread's interrupt state
"   blx %[fn]                           \n"         // start executing the code of the new thread

// when the new thread terminates, it returns here...
// it is assumed that the sp at this point has its initial value (the thread's stack usage is balanced)

"   cpsid i                             \n"         // disable interrupts
"   mov r1, #1                          \n"         //
"   str r0, [sp, #0]                    \n"         // save the return value
"   str r1, [sp, #4]                    \n"         // set the "done" flag

"   ldmia r9!, {r3-r8, r10-ip, lr}      \n"         // pop the next thread from the pending thread stack, and return to it
"   mov sp, r3                          \n"
"   msr primask, ip                     \n"         // restore the new thread's interrupt state

    :
    : [fn]"r"(fn), [newsp]"r"(newsp)
    : 
    );
    }


// init the threading system
// At present, this consists only of setting the pending thread stack pointer
void Thread::init()
    {
    for(auto &x : thread_stack)
        {
        x.clear();
        }

    __asm__ __volatile__(
"   mov r9, %[tsp]"                                     // init the thread stack pointer
    :
    : [tsp]"r"(&thread_stack[THREAD_DEPTH])
    :
    );
    }


