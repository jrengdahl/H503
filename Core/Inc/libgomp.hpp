// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying README.txt

#ifndef LIBGOMP_H
#define LIBGOMP_H

#define GOMP_STACK_SIZE 512

#define GOMP_MAX_NUM_THREADS 4
#define GOMP_DEFAULT_NUM_THREADS 4

#define GOMP_NUM_TEAMS 4

#define GOMP_NUM_TASKS 16

extern void libgomp_init();

extern int omp_verbose;

#define OMP_VERBOSE_DEFAULT 0

#endif // LIBGOMP_H
