// threadFIFO
//
// A threadFIFO is a subclass of FIFO. The data type stored in the FIFO are Threads.
// The inherited member functions add and take are not much use.
// The subclass member functions, suspend and resume, enable multiple threads to suspend at
// a threadFIFO. When a threadFIFO is resumed the oldest suspended thread is resumed.
// One notable use of a threadFIFO is the DeferFIFO, which is used to support yield and
// timesharing among multiple threads.

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file




#ifndef THREADFIFO_HPP
#define THREADFIFO_HPP

#include "cmsis.h"
#include "thread.hpp"
#include "FIFO.hpp"

static const unsigned THREAD_FIFO_DEPTH = 15;    // must be a power of 2, minus 1, if this is changed, the inline ASM must be updated

class threadFIFO : public FIFO<Thread, THREAD_FIFO_DEPTH>
    {
    public:

    __NOINLINE
    __NAKED
    bool suspend()
        {
        __asm__ __volatile__(
    "   mrs     ip, primask                 \n"         // save interrupt state
    "   cpsid   i                           \n"         // disable interrupts
    "   ldrd    r2, r3, [r0, #0x280]        \n"         // get nextin (r2) and nextout (r3)
    "   add     r1, r2, #1                  \n"         // increment nextin
    "   and     r1, #15                     \n"         // wrap if needed
    "   cmp     r1, r3                      \n"         // if updated nextin == nextout, the FIFO is full
    "   beq     0f                          \n"         // so go return false
    "   str     r1, [r0, #0x280]            \n"         // update nextout
    "   mov     r3, #40                     \n"         // calc address of indexed Thread in the FIFO
    "   mla     r1, r2, r3, r0              \n"         // nextin * 40 + base address
    "   mov     r3, sp                      \n"         // save previous thread on the stack
    "   stm     r1,  {r3-r8, r10-ip, lr}    \n"         //
    );

    suspend_switch();

    __asm__ __volatile__(
    "0: msr     primask, ip                 \n"         // restore caller's interrupt state
    "   mov     r0, #0                      \n"
    "   bx      lr                          \n"
    );

    return false;                                       // fake return to keep compiler from complaining
    }

    __NOINLINE
    __NAKED
    static void suspend_switch()
        {
        __asm__ __volatile__(
        "   ldmia   r9!, {r3-r8, r10-ip, lr}    \n"         // get new thread from FIFO
        "   mov     sp, r3                      \n"         // restore the new thead's sp
        "   msr     primask, ip                 \n"         // and interrupt state interrupt state
        "   mov     r0, #1                      \n"         // return true
        "   bx      lr                          \n"         //
        );
        }


    __NOINLINE
    __NAKED
    bool resume()
        {
        __asm__ __volatile__(
    "   mrs     ip, primask                 \n"         // save interrupt state
    "   cpsid   i                           \n"         // disable interrupts
    "   ldrd    r3, r2, [r0, #0x280]        \n"         // get nextin (r3) and nextout (r2)
    "   cmp     r2, r3                      \n"         // if equal, the FIFO is empty
    "   beq     0f                          \n"         // so go return false
    "   add     r3, r2, #1                  \n"         // increment nextout
    "   and     r3, #15                     \n"         // wrap if needed
    "   str     r3, [r0, #0x284]            \n"         // update nextout
    "   mov     r3, #40                     \n"         // calc address of indexed Thread in the FIFO
    "   mla     r2, r3, r2, r0              \n"         // nextout * 40 + base address
    "   mov     r3, sp                      \n"         // save previous thread on the stack
    "   stmdb   r9!, {r3-r8, r10-ip, lr}    \n"         //
    );

    resume_switch();

    __asm__ __volatile__(
    "0: msr     primask, ip                 \n"         // restore caller's interrupt state
    "   mov     r0, #0                      \n"
    "   bx      lr                          \n"
    );

    return false;
    }

    __NOINLINE
    __NAKED
    static void resume_switch()
        {
        __asm__ __volatile__(
        "   ldm     r2,  {r3-r8, r10-ip, lr}    \n"         // get new thread from FIFO
        "   mov     sp, r3                      \n"         // restore the new thead's sp
        "   msr     primask, ip                 \n"         // and interrupt state interrupt state
        "   mov     r0, #1                      \n"         // return true
        "   bx      lr                          \n"         //
        );
        }

    };


extern threadFIFO DeferFIFO;


// suspend the current thread at the DeferFIFO.
// It will be resumed later by the background thread.
// If the DeferFIFO is full yield will simply return immediately without yielding.

static inline void yield()
    {
    DeferFIFO.suspend();
    }


// Called by the backgroud thread to resume any supended threads in the DeferFIFO.
// If the DeferFIFO is empty this routine does nothing.

static inline void undefer()
    {
    DeferFIFO.resume();
    }


#endif // THREADFIFO_HPP

