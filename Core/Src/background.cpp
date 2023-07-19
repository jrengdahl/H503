// background.cpp
// This is the entry point and background loop of the application.
// It is called at powerup from the ST Micro supplied HAL after it initilizes the hardware.

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdint.h>
#include <stdio.h>
#include "context.hpp"
#include "Port.hpp"
#include "ContextFIFO.hpp"
#include "serial.h"
#include "main.h"
#include "usbd_cdc_if.h"
#include "cyccnt.hpp"
#include "libgomp.hpp"
#include "boundaries.h"

// The DeferFIFO used by yield, for rudimentary time-slicing.
// Note that the only form of "time-slicing" occurs when a thread
// voluntarily calls "yield" to temporarily give up the CPU
// to other threads. A thread resumed by an ISR will run at interrupt
// level until it calls yield, so in some cases, that thread may call
// yield shortly after the call to suspend.

ThreadFIFO DeferFIFO;

extern Port txPort;                              // ports for use by the console (serial or USB VCP)
extern Port rxPort;

extern uint32_t interp(uintptr_t);               // the command line interpreter thread
char InterpStack[2048];                          // the stack for the interpreter thread
omp_thread InterpThread;

uint32_t LastTimeStamp = 0;

// This exists as a central place to put a breakpoint when certain conditions are encountered.
// To use, put a call to foo when the error condition occurs. 
int dummy = 0;
void foo()
    {
    ++dummy;
    }


// constants to enable the FPU at powerup
#define CPACR (*(uint32_t volatile *)0xE000ED88)
#define CPACR_VFPEN 0x00F00000


extern "C"
void background()                                       // powerup init and background loop
    {
    ///////////////////////////
    // powerup initialization
    ///////////////////////////

    // at powerup the stack pointer points to the end of RAM
    // the first thing that must be done is to switch to the stack defined by the linker script

    // clear the background stack
    for(uint32_t *p = &_stack_start; p<&_stack_end; p++)*p = 0;

    // switch to the background stack
    __asm__ __volatile__(
"   ldr sp, =_stack_end"
    );

    Context::init();                                    // init the bare metal threading system
    InitCyccnt();                                       // enable the cycle counter

    CPACR |= CPACR_VFPEN;                               // enable the floating point coprocessor

    libgomp_init();                                     // init the OpenMP threading system

    libgomp_start_thread(InterpThread, interp, InterpStack); // spawn the command line interpreter on core 0

    ////////////////////
    // background loop
    ////////////////////

    while(1)
        {
        undefer();                                      // wake any threads that may have called yield
        }
    }

