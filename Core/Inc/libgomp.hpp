// Copyright (c) 2009, 2023 Jonathan Engdahl
// BSD license -- see the accompanying README.txt

#ifndef LIBGOMP_H
#define LIBGOMP_H

#include "context.hpp"

#define GOMP_STACK_SIZE 3072

#define GOMP_MAX_NUM_THREADS 6
#define GOMP_NUM_TEAMS 4
#define GOMP_NUM_TASKS 16

#define OMP_NUM_THREADS 4

extern void libgomp_init();

extern "C" int gomp_get_thread_id();


extern int omp_verbose;
#define OMP_VERBOSE_DEFAULT 0

// the code that constitutes a task
typedef void TASKFN(void *);


// a task is defined by code and data
struct task
    {
    TASKFN *fn;             // the thread function generated by OMP
    char *data;             // the thread's local data pointer
    };

// an omp_thread
struct omp_thread
    {
    // the thread's context
    Context context;        // the thread's registers

    int id = 0;             // a unique ID for each thread, also the index into the thread table
    int team_id = 0;        // the number of a thread within the team
    const char *name = 0;   // optional thread name, for debug

    struct task *task = 0;  // the thread's implicit task, nonzero if running

    omp_thread *team = 0;   // pointer to the master thread of the team this thread is a member of
    omp_thread *next = 0;   // link to the next team member

    unsigned single = 0;    // used to detect the first thread to arrive at a "single"
    bool arrived = false;   // arrived at a barrier, waiting for other threads to arrive
    bool mwaiting = false;  // waiting on a mutex

    // stuff pertaining to this thread as a team master
    int team_count = 0;
    omp_thread *members = 0; // list of the other members of the team this thread is the master of, which are linked by their "next" pointer.
    bool mutex = false;
    unsigned tsingle = 0;    // used to detect the first thread to arrive at a "single"
    int sections_count = 0;
    int sections = 0;
    int section = 0;
    int tasks = 0;
    void *copyprivate = 0;

    // debug data
    char *stack_low;        // low address of the stack (for debug)
    char *stack_high;       // high address of the stack pointer
    };

extern omp_thread omp_threads[GOMP_MAX_NUM_THREADS];

extern "C"
inline omp_thread *omp_this_thread()
    {
    omp_thread *thread;

    __asm__ __volatile__("    mov %[thread], r9" : [thread]"=r"(thread));

    return thread;
    }


extern "C"
inline omp_thread *omp_this_team()
    {
    omp_thread &thread = *omp_this_thread();
    omp_thread *team = thread.team_id == 0 ? &thread : thread.team;
    return team;
    }

template<unsigned N>
void libgomp_start_thread(omp_thread &thread, THREADFN *code, char (&stack)[N], uintptr_t arg = 0)
    {
    thread.stack_low = &stack[0];
    thread.stack_high = &stack[N];
    thread.context.spawn(code, stack, arg);
    }

#endif // LIBGOMP_H
