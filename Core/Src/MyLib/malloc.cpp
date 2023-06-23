#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "CriticalRegion.hpp"

extern unsigned _heap_start;
extern unsigned _heap_end;
extern uintptr_t _break;


struct MemBlock
    {
    union
        {
        MemBlock *next;
        unsigned bucket;
        };
    uintptr_t useable;
    };

static const unsigned MINSIZE = 8;
static const unsigned MAXSIZE = 1024;
static const unsigned SIZES = 8;

MemBlock *FreeBlocks[SIZES];



extern "C"
void *malloc(unsigned size)
    {
    MemBlock *blk = 0;
    unsigned bucket;

    size = (size+7) & -8;
    assert(size > 0 && size <= MAXSIZE);
    bucket = 31-__builtin_clz(size>>3);

    CRITICAL_REGION(InterruptLock)
        {
        blk = FreeBlocks[bucket];
        if(blk != 0)
            {
            FreeBlocks[bucket] = blk->next;
            }
        else
            {
            blk = (MemBlock *)_break;
            _break += (1<<(bucket+3))+4;
            assert(_break < (uintptr_t)&_heap_end);                 // assert that the heap has not overflowed
            }
        }

    blk->bucket = bucket;

    return (void *)&(blk->useable);
    }

void free(void *ptr)
    {
    MemBlock *blk = (MemBlock *)((uintptr_t)ptr - 4);
    unsigned bucket = blk->bucket;

    CRITICAL_REGION(InterruptLock)
        {
        blk->next = FreeBlocks[bucket];
        FreeBlocks[bucket] = blk;
        }
    }




