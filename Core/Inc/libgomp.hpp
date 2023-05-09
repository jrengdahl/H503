// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying README.txt

#ifndef LIBGOMP_H
#define LIBGOMP_H

#define GOMP_STACK_SIZE 1024

#define GOMP_MAX_NUM_THREADS 8
#define GOMP_DEFAULT_NUM_THREADS 8

#define GOMP_NUM_TEAMS 16

#define GOMP_NUM_TASKS 64

extern void libgomp_init();

extern int omp_verbose;

#endif // LIBGOMP_H
