// thread.hpp

// A simple but very fast threading system for bare metal ARM processors.
//
// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file
//
//
// A thread is represented by its non-volatile registers.
// A thread can be in one of three states;
// -- running: it currently is running on the CPU and its state is contained in the CPU registers.
// -- suspended: the thread is saved in a Thread object, and is waiting to be resumed. A thread in this
//    state is typically waiting for an event.
// -- pending: a thread that has resumed another thread has its state pushed on the pending thread stack.
//    A thread in this state is waiting for a running thread to suspend itself.
//
// The threading system is non-preemptive. That is, a thread never leaves the running state other than
// by its own action (suspend itself, resume another thread, or terminate).
//
// A thread consists of:
// -- a subroutine. Multiple threads can be running the same code.
// -- a context. The context consists of the non-volatile registers (r4-r11, sp, lr, on some systems
//    excluding fixed registers such as r9). The volatile registers r0-r3 and ip are not members of a
//    saved context because this threading system is non-preemptive -- a thread state change is only
//    initiated by a subroutine call, and volatile registers do not need to be saved across a call.
//    Furthermore, the pc is saved in lr by a subroutine call, thus the pc is not explicitly saved.
// -- a stack, pointed to by sp. The Thread functions that receive a stack as an argument are passed a
//    reference to an array of char. In this way the templated functions can determine the stack size,
//    whereas if a pointer were passed the size information would be lost.
//
// Multiple threads can be running virtually concurrently on a single CPU. In actuality, virtually
// concurrent threads run serially. Multiple threads can actually run concurrently on multiple cores.
//
// Memory available to a thread is either local (stack) or globally shared with all other threads.
// This implmentation does not provide thread-private static memory. On single and multi-core systems
// the threads share the same address space. It is assumed that the address space is coherent (all
// threads see the same content for all addresses, regardless of cacheing or which core they are on).
//
// This implementation stores the thread stack pointer (points to a stack of pending threads, not to be
// confused with the normal sp) in r9. This removes one more register from the pool allocatable by the
// compiler, but has the following significant benefit:
// -- the thread switching code becomes much faster and simpler.
// -- the compiler requires fewer registers to implement a context switch, thus the threadFIFO algorithm
//    can be implemented using only the five volatile registers. This greatly simplifies the threadFIFO
//    suspend and resume routines.
// 
// Excluding the subroutine call and return, a thread switch costs only four instructions. On a 500 MHz
// CPU this would be 40 nanoseconds. Compare this to 10 microseonds for a thread switch on a preemptive
// RTOS. Obviously, a fully preemptive, prioritized, time-sliced RTOS has its advantages, but where sheer
// performance outweighs these advantages, a non-preemtive system such as this is necessary. The other
// alternative would be to implement the firmware as a complex state machine, but 45 years experience coding
// real-time firmware has convinced me that a set of cooperating threads are much easier to design, implement,
// and debug than a huge single-threaded state machine.


#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

// the code of a thread
typedef uint32_t THREADFN();

// the context of a thread
class Thread
    {
    private:

    uint32_t r3;        // holds the sp, which can't be accessed by the ldm/stm instructions
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
                        // skip over r9 which holds the pending stack pointer
    uint32_t r10;
    uint32_t r11;
    uint32_t ip;        // holds the interrupt state
    uint32_t lr;        // holds the execution address

    // r9 is reserved by Thread for the thread stack pointer

    public:

    // Constructor
    Thread() : r3(0), r4(0), r5(0), r6(0), r7(0), r8(0), r10(0), r11(0), ip(0), lr(0)
        {
        }

    void clear()
        {
        r3 = 0;
        r4 = 0;
        r5 = 0;
        r6 = 0;
        r7 = 0;
        r8 = 0;
        r10 = 0;
        r11 = 0;
        ip = 0;
        lr = 0;
        }


    void suspend();                                             // suspend self until resumed
    void suspend_switch();                                      // an entry point to resume after the condition is tested (for Ozone RTOS awareness)

    bool resume();                                              // resume a suspended thread
    void resume_switch();                                       // an entry point to resume after the condition is tested (for Ozone RTOS awareness)

    static void start(THREADFN *fn, char *sp);                  // an internal function to start a new thread
    static void start_switch1();                                // an entry point to resume after the condition is tested (for Ozone RTOS awareness)
    static void start_switch2();                                // an entry point to resume after the condition is tested (for Ozone RTOS awareness)


    // spawn a new thread
    template<unsigned N>
    static void spawn(THREADFN *fn,         // code
                      char (&stack)[N])     // reference to the stack. The template can determine the stack size from the reference.
        {
        start(fn, &stack[N-8]);             // reserve two words at stack top, then call the start function, passing the initial sp
        }

    static void init();                     // powerup init of the thread system (inits the thread stack pointer in r9)

    // Thread::done -- test whether a thread is running
    // arg: the thread's stack
    // return: true if the thread is done, false if it is still running.
    // The result is only valid after the thread has been started (and you are waiting for it to complete).
    template<unsigned N>
    static inline bool done(char (&stack)[N])
        {
        return stack[N-4] == 1;
        }

    };




#endif // THREAD_H
