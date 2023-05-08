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


#include "cmsis.h"
#include "thread.hpp"
#include "ThreadFIFO.hpp"


__NOINLINE
__NAKED
bool ThreadFIFO::suspend()
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

    this->suspend_switch();

    __asm__ __volatile__(
    "0: mov     r0, #0                      \n"
    "   msr     primask, ip                 \n"         // restore caller's interrupt state
    "   bx      lr                          \n"
    );

    return false;                                       // fake return to keep compiler from complaining
    }

__NOINLINE
__NAKED
void ThreadFIFO::suspend_switch()
    {
    __asm__ __volatile__(
    "   ldmia   r9!, {r3-r8, r10-ip, lr}    \n"         // get new thread from FIFO
    "   mov     sp, r3                      \n"         // restore the new thead's sp
    "   mov     r0, #1                      \n"         // return true
    "   msr     primask, ip                 \n"         // and interrupt state interrupt state
    "   bx      lr                          \n"         //
    );
    }


__NOINLINE
__NAKED
bool ThreadFIFO::resume()
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

    this->resume_switch();

    __asm__ __volatile__(
    "0: mov     r0, #0                      \n"
    "   msr     primask, ip                 \n"         // restore caller's interrupt state
    "   bx      lr                          \n"
    );

    return false;
    }

__NOINLINE
__NAKED
void ThreadFIFO::resume_switch()
    {
    __asm__ __volatile__(
    "   ldm     r2,  {r3-r8, r10-ip, lr}    \n"         // get new thread from FIFO
    "   mov     sp, r3                      \n"         // restore the new thead's sp
    "   mov     r0, #1                      \n"         // return true
    "   msr     primask, ip                 \n"         // and interrupt state interrupt state
    "   bx      lr                          \n"         //
    );
    }

