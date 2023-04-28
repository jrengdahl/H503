// gaskets which adapt the serial I/O system (i.e. USB VCP) to printf and getline

// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file

 
#include <stdint.h>
#include "thread.hpp"
#include "ThreadFIFO.hpp"
#include "Fifo.hpp"
#include "usbd_cdc_if.h"
#include "CriticalRegion.hpp"

extern "C" const char *strnchr(const char *s, int n, int c);

Thread txPort;
Thread rxPort;

FIFO<char, 64> ConsoleFifo;

bool ControlC = false;


extern "C"
int __io_getchar()                              // link the CMSIS syslib to the HAL's UART input
    {
    char ch;

    do
        {
        CRITICAL_REGION(InterruptLock)          // close the window between test and wait, where a callback might occur
            {
            if(!ConsoleFifo)
                {
                rxPort.suspend();
                yield();
                }
            }
        }
    while(!ConsoleFifo.take(ch));

    return ch;
    }

// refactored code: atomically wrap the test and wait for txready, then output the buffer
static void vcp_writeblock(uint8_t* Buf, uint16_t Len)
    {
    do
        {
        CRITICAL_REGION(InterruptLock)                          // test for busy and wait atomically
            {
            if(!vcp_txready())
                {
                txPort.suspend();                                  // so the callback cannot occur in the window between the test and wait
                yield();
                }
            }
        }
    while(CDC_Transmit_FS(Buf, Len) == USBD_BUSY);
    }

extern "C"
int _write(int file, const char *ptr, int len)
    {
    int retlen = len;                                           // success return = buffer len

    while(len)                                                  // while there is some of the buffer left
        {
        const char *pnl;                                        // pointer to newline

        pnl = strnchr(ptr, len, '\n');                          // see if there is a newline in the buffer
        if(pnl )
            {
            vcp_writeblock((uint8_t *)ptr, pnl-ptr);            // if so, output the buffer up to before the newline
            vcp_writeblock((uint8_t *)"\r\n", 2);               // output return and newline

            len -= pnl-ptr+1;                                   // skip over the newline in the buffer
            ptr = pnl+1;
            }                                                   // we will repeat until the buffer is empty
        else                                                    // if there is no newline
            {
            vcp_writeblock((uint8_t *)ptr, len);                // just output the whole buffer

            len = 0;                                            // and set the len to zero so there is no repeat
            }
        }

    return retlen;
    }


extern "C"
void vcp_tx_callback()
    {
    txPort.resume();
    }

extern "C"
void vcp_rx_callback(uint8_t *Buf, uint32_t Len)
    {
    for(unsigned i=0; i<Len; i++)
        {
        char ch = Buf[i];

        if(ch == 'C'-'@')
            {
            ControlC = true;
            }
        else
            {
            ConsoleFifo.add(ch);
            rxPort.resume();
            }
        }

    }

