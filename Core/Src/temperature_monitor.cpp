#include <stdint.h>
#include <stdio.h>
#include <omp.h>
#include "main.h"
#include "cmsis.h"
#include "context.hpp"
#include "Port.hpp"
#include "stm32h5xx_hal.h"
#include "adc.h"
#include "tim.h"

extern int getline_nchar;

#define TS_CAL1 0x02fa
#define TS_CAL2 0x03f6
#define TS_CAL1_TEMP 30000
#define TS_CAL2_TEMP 130000

int temp_verbose = 10;      // minimum time (in seconds) between temperature reports, 0 disables reporting

Port tempPort;

#define INTERCEPT 1150
#define SLOPE 2

int read_temperature_raw()
    {
    // Start a conversion.
    HAL_ADC_Start(&hadc1);

    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

    int TS_DATA = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return TS_DATA;
    }


// This function reads the temperature sensor and returns the temperature in degrees Celsius times 1000.
int read_temperature()
    {
    int TS_DATA = read_temperature_raw();

    // Convert the raw temperature value to degrees Celsius.
    int temperature = (TS_CAL2_TEMP-TS_CAL1_TEMP) * (TS_DATA-TS_CAL1) / (TS_CAL2-TS_CAL1) + TS_CAL1_TEMP;

    return temperature;
    }

int temp;

void temperature_monitor()
    {
    uint32_t last_temp_report;
    int avg_temp;
    int last_avg_temp = 0;
    const int NAVG = 50;
    uint32_t now;

    last_temp_report = __HAL_TIM_GET_COUNTER(&htim2);
    avg_temp = read_temperature();

    while(true)
        {
        now = (uint32_t)tempPort.suspend();

        temp = read_temperature();
        avg_temp = (avg_temp*(NAVG-1) + temp)/NAVG;

        if(temp_verbose > 0
        && now - last_temp_report > (uint32_t)temp_verbose*1000000
        && getline_nchar == 0)
            {
            if(avg_temp-last_avg_temp > 1000 || last_avg_temp-avg_temp > 1000)
                {
                printf("chip temperature = %d C\n", (avg_temp+500)/1000);
                putchar('>');
                fflush(stdout);
                last_avg_temp = avg_temp;
                last_temp_report = now;
                }
            }
        }
    }
