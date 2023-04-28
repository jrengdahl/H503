// dump a summary of the memory

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <stdio.h>
#include <stdint.h>

#include "boundaries.h"

// print a summary of memory

void summary(   unsigned char *mem,                     // start of memory region to summarize
                unsigned size,                          // size of memory region to summarize
                unsigned inc,                           // size of block in the summary
                int values)                             // if 0 simply print the type for every block
                                                        // if 1 print blocks which have all the same value as that value
    {
    int line=0;                                         // counts elements per line
    for(unsigned i=0; i<size; i += inc)                 // for each block in the size
        {
        if(line==0)                                     // if at beginning of line print address
            {
            printf("%08zx: ", (uintptr_t)(mem+i));
            }

        unsigned char b = mem[i];                       // remember first byte in block
        char c[3];                                      // print buffer

        snprintf(c, 3, "%02x", b);                      // save hex representation of the first byte

        for(unsigned j=0; j<inc; j++)                   // for each byte in the block
            {
            if(mem[i+j] != b || values==0)              // if the byte is different from the first byte
                {
                char sym;
                uint32_t *addr = (uint32_t *)&mem[i+j];

                if(     addr >= &_text_start      && addr < &_text_end)      sym = 'T';
                else if(addr >= &_sdata           && addr < &_edata)         sym = 'D';
                else if(addr >= &_sbss            && addr < &_ebss)          sym = 'B';
                else if(addr >= &_heap_start      && addr < &_heap_end)      sym = 'H';
                else if(addr >= &_stack_start     && addr < &_stack_end)     sym = 'S';
                else                                                         sym = '#';

                c[0] = sym;                             // change representation to hash
                c[1] = sym;
                }
            }

        printf("%s ", c);                               // print result of this block
        line++;                                         // count up to 16 clicks per line
        if(line>=16)                                    // at end of line
            {
            printf("\n");                               // print newline
            line=0;                                     // and start new line
            }
        }

    if(line != 0)
        {
        printf("\n");
        }
    }

