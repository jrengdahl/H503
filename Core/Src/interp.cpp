// a command line interpreter for debugging the target

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdio.h>
#include <string.h>
#include "local.h"
#include "main.h"
#include "serial.h"
#include "ContextFIFO.hpp"
#include "libgomp.hpp"


extern void bear();

char buf[INBUFLEN];
bool waiting_for_command = false;

// print help string, see usage below
#define HELP(s) else if(buf[0]=='?' && puts(s) && 0){}

void interp()
    {
    bear();
    printf("hello, world!\n");
    printf("build: %s %s\n", __DATE__, __TIME__);

    while(1)
        {
        char *p;

        yield();
        libgomp_reinit();                                                       // reset things in OPenMP such as the default number of threads
        ControlC = false;
        SerialRaw = false;
        putchar('>');                                                           // output the command line prompt
        fflush(stdout);
        waiting_for_command = true;
        getline(buf, INBUFLEN);                                                 // get a command line
        waiting_for_command = false;
        p = buf;
        skip(&p);                                                               // skip command and following whitespace

        if(buf[0]==0 || buf[0]=='\r' || buf[0]=='\n' || buf[0]==' '){}          // ignore blank lines

        HELP(  "Addresses, data, and sizes are generally in hex.")

        HELP(  "d <addr> <size>}                dump memory")
        else if(buf[0]=='d' && buf[1]==' ')                                                    // memory dump d <hex address> <dec size> {<dump width>}
            {
            extern void DumpCommand(char *p);
            DumpCommand(p);
            }

        HELP(  "m <addr> <data>...              modify memory")
        else if(buf[0]=='m' && buf[1]==' ')                                                    // memory modify <address> <data> ...
            {
            extern void ModifyCommand(char *p);
            ModifyCommand(p);
            }

        HELP(  "f <addr> <size> <data>          fill memory")
        else if(buf[0]=='f' && buf[1]==' ')
            {
            extern void FillCommand(char *p);
            FillCommand(p);
            }

        HELP(  "rt <addr> <size>...             ram test")
        else if(buf[0]=='r' && buf[1]=='t')
            {
            extern void RamTestCommand(char *p);
            RamTestCommand(p);
            }

        HELP(  "i <pin> [<repeat>]              report inputs")
        else if(buf[0]=='i' && buf[1] == ' ')
            {
            extern void InputCommand(char *p);
            InputCommand(p);
            }

        HELP(  "o <pin> <value>                 set an output pin")
        else if(buf[0]=='o' && buf[1] == ' ')
            {
            extern void OutputCommand(char *p);
            OutputCommand(p);
            }

        HELP(  "g <count> <delay> <dest>        toggle a gpio")
        else if(buf[0]=='g' && buf[1]==' ')
            {
            extern void GpioCommand(char *p);
            GpioCommand(p);
            }

        HELP(  "clk <freq in MHz>               set CPU clock")
        else if(buf[0]=='c' && buf[1]=='l' && buf[2]=='k')
            {
            extern void ClkCommand(char *p);
            ClkCommand(p);
            }

        HELP(  "c <count>                       calibrate")
        else if(buf[0]=='c' && buf[1]==' ')
            {
            extern void CalibrateCommand(char *p);
            CalibrateCommand(p);
            }

        HELP(  "ldr <addr>                      load a word from address using ldr")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]==' ')
            {
            extern void LdrCommand(char *p);
            LdrCommand(p);
            }

        HELP(  "ldrh <addr> {size {o}}          load a halfword from address using ldrh")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]=='h')
            {
            extern void LdrhCommand(char *p);
            LdrhCommand(p);
            }

        HELP(  "ldrb <addr>                     load a byte from address using ldrb")
        else if(buf[0]=='l' && buf[1]=='d' && buf[2]=='r' && buf[3]=='b')
            {
            extern void LdrbCommand(char *p);
            LdrbCommand(p);
            }

        HELP(  "str <value> <addr>              store word to address using str")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]==' ')
            {
            extern void StrCommand(char *p);
            StrCommand(p);
            }

        HELP(  "strh <value> <addr>             store a halfword to address using strh")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]=='h')
            {
            extern void StrhCommand(char *p);
            StrhCommand(p);
            }

        HELP(  "strb <value> <addr>             store a byte to address using strb")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='r' && buf[3]=='b')
            {
            extern void StrbCommand(char *p);
            StrbCommand(p);
            }

        HELP(   "sum {<addr> <size> <block>}     summarize memory")
        else if(buf[0]=='s' && buf[1]=='u' && buf[2]=='m')
            {
            extern void SumCommand(char *p);
            SumCommand(p);
            }

        HELP(  "t <num>                         measure thread switch time")
        else if(buf[0]=='t' && buf[1]==' ')
            {
            extern void ThreadTestCommand(char *p);
            ThreadTestCommand(p);
            }

        HELP(  "tmp                             read the temperature sensor")
        else if(buf[0]=='t' && buf[1]=='m' && buf[2]=='p')
            {
            extern void TemperatureCommand(char *p);
            TemperatureCommand(p);
            }

        HELP(  "omp <num>                       run an OMP test")
        else if(buf[0]=='o' && buf[1]=='m' && buf[2]=='p')
            {
            extern void OmpTestCommand(char *p);
            OmpTestCommand(p);
            }

        HELP(  "v <type> <num>                  set verbosity level")
        else if(buf[0]=='v' && (buf[1]==' ' || buf[1]==0))
            {
            extern void VerboseCommand(char *p);
            VerboseCommand(p);
            }

        HELP(  "stk                             dump stacks")
        else if(buf[0]=='s' && buf[1]=='t' && buf[2]=='k')
            {
            extern void StackCommand(char *p);
            StackCommand(p);
            }

        HELP(  "mem                             display free memory")
        else if(buf[0]=='m' && buf[1]=='e' && buf[2]=='m')
            {
            extern void mem();
            mem();
            }

        // print the help screen
        else if(buf[0]=='?')
            {
            }

        // else I dunno what you want
        else printf("illegal command\n");
        }

    }


