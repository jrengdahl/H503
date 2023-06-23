#include <stdio.h>
#include <omp.h>

void omp_hello(int arg)
    {
    if(arg==0)arg=4;
    #pragma omp parallel num_threads(arg)
    printf("hello, world! %d\n", omp_get_thread_num());
    }
