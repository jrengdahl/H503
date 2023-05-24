#include <stdio.h>
#include <omp.h>

void omp_hello()
    {
    #pragma omp parallel num_threads(4)
    printf("hello, world! %d\n", omp_get_thread_num());
    }
