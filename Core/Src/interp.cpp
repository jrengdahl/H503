// a command line interpreter for debugging the target

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <context.hpp>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ContextFIFO.hpp>
#include <math.h>
#include "cmsis.h"
#include "local.h"
#include "bogodelay.hpp"
#include "serial.h"
#include "random.hpp"
#include "cyccnt.hpp"
#include "main.h"
#include "boundaries.h"
#include "libgomp.hpp"
#include "omp.h"

extern void bear();
extern "C" char *strchrnul(const char *s, int c);   // POSIX function but not included in newlib, see https://linux.die.net/man/3/strchr
extern void summary(unsigned char *, unsigned, unsigned, int);
extern void RamTest(uint8_t *addr, unsigned size, unsigned repeat, unsigned limit);
extern "C" void SystemClock_PLL_Config(unsigned);
extern "C" void SystemClock_HSI_Config(void);
extern char InterpStack[2048];
extern omp_thread omp_threads[GOMP_MAX_NUM_THREADS];


// print a large number with commas
void commas(uint32_t x)
    {
    if(x>=1000000000)
        {
        printf("%lu,%03lu,%03lu,%03lu", x/1000000000, x/1000000%1000, x/1000%1000, x%1000);
        } // those are "L"s after the threes, not ones
    else if(x>=1000000)
        {
        printf("%lu,%03lu,%03lu", x/1000000, x/1000%1000, x%1000);
        }
    else if(x>=1000)
        {
        printf("%lu,%03lu", x/1000%1000, x%1000);
        }
    else printf("%lu", x);
    }

// print help string, see usage below
#define HELP(s) else if(buf[0]=='?' && puts(s) && 0){}

char buf[INBUFLEN];

// a simple thread used to benchmark the thread switching calls
Context TestCtx;
char TestStack[512];
unsigned TestCount = 0;

uint32_t TestThread(uintptr_t arg)
    {
    (void)arg;

    while(TestCount--)
        {
        Context::suspend();
        Context::suspend();
        Context::suspend();
        Context::suspend();
        Context::suspend();
        Context::suspend();
        Context::suspend();
        Context::suspend();
        Context::suspend();
        Context::suspend();
        }
    return 0;
    }


uint32_t interp(uintptr_t arg)
    {
    (void)arg;

    bear();
    printf("hello, world!\n");

    while(1)
        {
        const char *p;

        ControlC = false;
        putchar('>');                                                           // output the command line prompt
        fflush(stdout);
        getline(buf, INBUFLEN);                                                 // get a command line
        p = buf;
        skip(&p);                                                               // skip command and following whitespace

        if(buf[0]==0 || buf[0]=='\r' || buf[0]=='\n' || buf[0]==' '){}          // ignore blank lines

        HELP(  "Addresses, data, and sizes are generally in hex.")

//              //                              //
        HELP(  "d <addr> <size>}                dump memory")
        else if(buf[0]=='d' && buf[1]==' ')                                                    // memory dump d <hex address> <dec size> {<dump width>}
            {
            unsigned *a;
            int s;

            a = (unsigned *)gethex(&p);                                         // get the first arg
            skip(&p);
            if(isxdigit(*p))s = gethex(&p);                                      // the second arg is the size
            else s = 4;

            dump(a, s);                                                         // do the dump
            }

//              //                              //
        HELP(  "m <addr> <data>...              modify memory")
        else if(buf[0]=='m' && buf[1]==' ')                                                    // memory modify <address> <data> ...
            {
            unsigned *a;
            unsigned v;

            a = (unsigned *)gethex(&p);                                         // get the first arg
            skip(&p);
            while(isxdigit(*p))                                                 // while there is any data left on the command line
                {
                v = gethex(&p);                                                 // get the data
                skip(&p);                                                       // skip that arg and folowing whitespace
                *a++ = v;                                                       // store the data
                }
            }


//              //                              //
        HELP(  "f <addr> <size> <data>          fill memory")
        else if(buf[0]=='f' && buf[1]==' ')
            {
            uint32_t *a;
            uintptr_t s;
            uint32_t v;

            a = (uint32_t *)gethex(&p);                     // get the first arg
            skip(&p);
            s = gethex(&p);                                 // the second arg is the size
            skip(&p);
            v = gethex(&p);                                 // value is the third arg

            while(s>0)                                      // fill memory with the value
                {
                *a++ = v;
                s -= sizeof(*a);
                }
            }


//              //                              //
        HELP(  "rt <addr> <size>...             ram test")
        else if(buf[0]=='r' && buf[1]=='t')
            {
            uint8_t *addr;
            unsigned size;
            int repeat = 1;
            unsigned limit = 10;

            addr = (uint8_t *)gethex(&p);                   // get the first arg (address)
            skip(&p);
            size = gethex(&p);                              // get second arg (size of area to test)
            skip(&p);
            if(isdigit(*p))                                 // get optional third arg (repeat count)
                {
                repeat = getdec(&p);
                skip(&p);
                if(isdigit(*p))                             // get optional fourth arg (max errors to report)
                    {
                    limit = getdec(&p);
                    }
                }

            RamTest(addr, size, repeat, limit);
            }


//              //                              //
        HELP(  "i <pin> [<repeat>]              report inputs")
        else if(buf[0]=='i' && buf[1] == ' ')
            {
            unsigned i = 0;
            unsigned value = 0;
            unsigned repeat = 1;
            unsigned last = 99;

            if(isdigit(*p))
                {
                i = getdec(&p);
                skip(&p);
                if(isdigit(*p))
                    {
                    repeat = getdec(&p);
                     }
                }

            while(repeat && !ControlC)
                {
                switch(i)
                    {
                case 0:  value = HAL_GPIO_ReadPin(GPIONAME( SW1)); break;
                default: value = HAL_GPIO_ReadPin(GPIONAME( SW1)); break;
                    }

                if(value != last)
                    {
                    printf("%u\n", value);
                    last = value;
                    --repeat;
                    }

                yield();
                }
            }


//              //                              //
        HELP(  "o <pin> <value>                 set an output pin")
        else if(buf[0]=='o' && buf[1] == ' ')
            {
            unsigned o = 0;                     // output number
            unsigned value = 0;                 // output value

            if(isdigit(*p))
                {
                o = getdec(&p);
                skip(&p);
                if(isdigit(*p))
                    {
                    value = getdec(&p);
                    }
                }

            switch(o)
                {
                case 0:     HAL_GPIO_WritePin(GPIONAME(LED), (GPIO_PinState)(value));  break;
                case 1:     HAL_GPIO_WritePin(GPIONAME(PA8), (GPIO_PinState)(value));  break;
                default:    HAL_GPIO_WritePin(GPIONAME(LED), (GPIO_PinState)(value));  break;
                }
            }

//              //                              //
        HELP(  "g <count> <delay> <dest>        toggle a gpio")
        else if(buf[0]=='g' && buf[1]==' ')
            {
            unsigned count = 1;                 // number of blinks
            unsigned delay = 1;                 // delay (in CPU clocks) between blinks
            unsigned dest = 0;                  // which LED to blink

            if(isdigit(*p))
                {
                count = getdec(&p);
                skip(&p);
                if(isdigit(*p))
                    {
                    delay = getdec(&p);
                    skip(&p);
                    if(isdigit(*p))
                        {
                        dest = getdec(&p);
                        }
                    }
                }

            for(unsigned i=0; i<count && !ControlC; i++)
                {
                switch(dest)
                    {
                    case  0: HAL_GPIO_WritePin(GPIONAME( LED), (GPIO_PinState)(i&1)); break;
                    case  1: HAL_GPIO_WritePin(GPIONAME( PA8), (GPIO_PinState)(i&1)); break;
                    default: HAL_GPIO_WritePin(GPIONAME( LED), (GPIO_PinState)(i&1)); break;
                    }

                bogodelay(delay);
                }
            }


//              //                              //
        HELP(  "clk <freq in MHz>               set CPU clock")
        else if(buf[0]=='c' && buf[1]=='l' && buf[2]=='k')
            {
            if(isdigit(*p))                     // if an arg is given
                {
                unsigned clk = getdec(&p);      // set the clock freuqnecy to the new value
                SystemClock_HSI_Config();       // have to deselect the PLL before reprogramming it
                SystemClock_PLL_Config(clk);    // set the PLL to the new frequency
                }

            printf("CPU clock is %u MHz\n", CPU_CLOCK_FREQUENCY);
            if(CPU_CLOCK_FREQUENCY > 100)
                {
                printf("trace may not be stable at frequencies over 100 MHz\n");
                }
            }


//              //                              //
        HELP(  "c <count>                       calibrate")
        else if(buf[0]=='c' && buf[1]==' ')
            {
            int size = 32;
            uint32_t ticks = 0;

            uint64_t count = 1;

            if(isdigit(*p))                                                 // while there is any data left on the command line
                {
                count = getlong(&p);                                        // get the count
                skip(&p);
                if(isdigit(*p))                                             // while there is any data left on the command line
                    {
                    size = getdec(&p);                                      // get the size
                    }
                }

            if(size==64)
                {
                __disable_irq();
                Elapsed();
                bogodelay(count);
                ticks = Elapsed();
                __enable_irq();
                }
            else
                {
                __disable_irq();
                Elapsed();
                bogodelay((uint32_t)count);
                ticks = Elapsed();
                __enable_irq();
                }
            commas(ticks);
            printf(" microseconds\n");
            }


        //              //                              //
        HELP(  "ldr <addr>                      load a word from address using ldr")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r')
            {
            uintptr_t addr = gethex(&p);            // get the address
            uint32_t value;

            __asm__ __volatile__("ldr %0, [%1]"     // load a word from the given address
             : "=r"(value)
             : "r"(addr)
             : );

            printf("%lx\n", value);                 // print the result
            }

//              //                              //
        HELP(  "str <value> <addr>              store value to address using str")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r')
            {
            uint32_t value = gethex(&p);
            skip(&p);
            uintptr_t addr = gethex(&p);            // get the address

            __asm__ __volatile__("str %0, [%1]"
            :
            : "r"(value), "r"(addr)
            : );
            }


//               //                              //
        HELP(   "sum {<addr> <size> <block>}     summarize memory")
        else if(buf[0]=='s' && buf[1]=='u' && buf[2]=='m')
            {
            unsigned char *addr = (unsigned char *)&_memory_start;
            unsigned size = (unsigned char *)&_memory_end - (unsigned char *)&_memory_start;
            unsigned inc = 256;
            int values = 0;
            bool defaults = false;

            if(*p == 'v')                                       // flag, if specified print value of RAM if all bytes are the same
                {
                values = 1;
                skip(&p);
                }

            if(*p == 0)                                         // if no other args then assume the default memory ranges from the linker script
                {
                defaults = true;
                }

            else if(isxdigit(*p))
                {
                addr = (unsigned char *)gethex(&p);             // the first arg is the base address, in hex
                skip(&p);
                if(isxdigit(*p))                                // the second arg is the memory size, in hex
                    {
                    size = gethex(&p);
                    skip(&p);
                    if(isxdigit(*p))                            // the third arg is the block size, in hex
                        {
                        inc = gethex(&p);
                        }
                    }
                }

            summary(addr, size, inc, values);

            if(defaults && &_memory2_start != &_memory2_end)
                {
                printf("\n");
                summary((unsigned char *)&_memory2_start, (unsigned char *)&_memory2_end - (unsigned char *)&_memory2_start, inc, values);
                }
            }


//              //                              //
        HELP(  "t <num>                         run the thread test")
        else if(buf[0]=='t' && buf[1]==' ')
            {
            unsigned ticks;
            unsigned count;

            count = getdec(&p);
            TestCount = count;
            TestCtx.spawn(TestThread, TestStack);
            Elapsed();

            while(!Context::done(TestStack))
                    {
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    TestCtx.resume();
                    }

            ticks = Elapsed();
            commas(ticks);
            printf(" microseconds\n");
            printf("%u ns\n", ticks/(count/50));
             }


//              //                              //
        HELP(  "tmp                             read the temperature sensor")
        else if(buf[0]=='t' && buf[1]=='m' && buf[2]=='p')
            {
            extern int read_temperature();
            int last_avg = -1;
            int repeat = 50;
            int avg = 30;
            int count = 0;
            int last_count = 0;

            if(isdigit(*p))                                                 // while there is any data left on the command line
                {
                repeat = getdec(&p);                                        // get the count
                }

            while(repeat && !ControlC)
                {
                int tmp = read_temperature();

                avg = (avg*9 + tmp)/10;

                if(avg-last_avg > 1 || last_avg-avg > 1)
                    {
                    printf("temperature: %3d C, %3d F, avg = %3d C, delta = %d\n", tmp, (tmp*18 + 325)/10, avg, count - last_count);
                    last_avg = avg;
                    last_count = count;
                    -- repeat;
                    }

                ++count;

                yield();
                }
            }

//              //                              //
        HELP(  "omp <num>                       run an OMP test")
        else if(buf[0]=='o' && buf[1]=='m' && buf[2]=='p')
            {
            extern void omp_hello(int);
            extern void omp_for(int);
            extern void omp_single(int);
            extern void permute(int colors_arg, int balls, int plevel_arg, int verbose_arg);

            int test = 0;

            if(isdigit(*p))
                {
                test = getdec(&p);                              // get the count
                skip(&p);
                }

            switch(test)
                {
            case 0: omp_hello(getdec(&p));      break;
            case 1: omp_for(getdec(&p));        break;
            case 2: omp_single(getdec(&p));     break;
            case 3:
                int colors = getdec(&p);
                skip(&p);
                int balls = getdec(&p);
                skip(&p);
                int plevel = *p?getdec(&p):32;
                skip(&p);
                int v = *p?getdec(&p):0;
                permute(colors, balls, plevel, v);
                break;
                }
            }

//              //                              //
        HELP(  "v <num>                         set verbosity level")
        else if(buf[0]=='v' && buf[1]==' ')
            {
            extern int omp_verbose;

            if(isdigit(*p))
                {
                omp_verbose = getdec(&p);                              // get the count
                }
            }

//              //                              //
        HELP(  "stk                             dump stacks")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='k')
            {
            printf("\ninterp stack:\n");
            dump(&InterpStack, sizeof(InterpStack));

            printf("\nTestStack:\n");
            dump(&TestStack, sizeof(TestStack));

            for(int i=0; i<GOMP_MAX_NUM_THREADS; i++)
                {
                printf("\nomp thread %d stack:\n", i);
                dump(omp_threads[i].stack_low, omp_threads[i].stack_high - omp_threads[i].stack_low);
                }
            }

        else if(buf[0]=='?')
            {
            }

        // else I dunno what you want
        else printf("illegal command\n");

        yield();
        }

    return 0;
    }


