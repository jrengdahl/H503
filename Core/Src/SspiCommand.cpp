#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "local.h"
#include "main.h"
#include "cmsis.h"
#include "serial.h"
#include "SSPI.h"

extern uint32_t qbuf[512/4];
extern void print_status_register(uint8_t regCommand);


void SspiCommand(char *p)
    {
    if(*p == 'r')
        {
        int count = 1;

        skip(&p);
        uint32_t addr = gethex(&p);
        skip(&p);
        if(isxdigit(*p))count = gethex(&p);

        for(int i=0; i<count; i++)
            {
            SPI_ReadPage(&hspi2, addr, (uint8_t *)&qbuf, 256);
            printf("%08x\n", (unsigned)addr);
            dump(qbuf, 256);
            addr += 256;
            }
        }

    else if(p[0] == 's')
        {
        print_status_register(READ_STATUS_REG_1_CMD);
        print_status_register(READ_STATUS_REG_2_CMD);
        print_status_register(READ_STATUS_REG_3_CMD);
        }

    else if(p[0] == 'f')
        {
        skip(&p);
        uint32_t addr = gethex(&p);
        skip(&p);
        uint32_t data = gethex(&p);

        for(int i=0; i<256/4; i++)qbuf[i] = data;

        SPI_WritePage(&hspi2, addr, (uint8_t *)&qbuf, 256);
        }

    else if(p[0] == 'e' && p[1] == 's')
        {
        int count = 1;

        skip(&p);
        uint32_t addr = gethex(&p);
        skip(&p);
        if(isxdigit(*p))count = gethex(&p);

        for(int i=0; i<count; i++)
            {
            SPI_EraseSector(&hspi2, addr);
            addr += 4096;
            }
        }

    else if(p[0] == 'e' && p[1] == 'c')
        {
        HAL_StatusTypeDef status;

        printf("erasing entire SPI-NOR, this may take several minutes\n");
        status = SPI_EraseChip(&hspi2);
        if(status != HAL_OK)
            {
            printf("erase failed, status = %d\n", status);
            }
        else
            {
            printf("erasing complete\n");
            }
        }

    else if(p[0] == 'x')
        {
        extern int xmodem_receive(uint8_t *buf);
        xmodem_receive((uint8_t *)&qbuf);
        }

    else
        {
        printf("unrecognized subcommand\n");
        }
    }
