// libgomp.cpp
//
// A subset of libgomp for bare metal.
// This is a simplistic implementation for a single-core processor for learning, development, and testing.
//
// Target is an ARM CPU with very simple bare metal threading support.
//
// These implementations only work on a single-core, non-preemtive system,
// until they is made thread-safe.

// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file


#include <context.hpp>
#include <ContextFIFO.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>
#include "FIFO.hpp"
#include "libgomp.hpp"


// The threads' stacks
// The thread number can be determined by looking at certain bits of the sp.

char gomp_stacks[GOMP_MAX_NUM_THREADS][GOMP_STACK_SIZE] __ALIGNED(GOMP_STACK_SIZE);



// the code that constitutes a task
typedef void TASKFN(void *);


// a task is defined by code and data
struct task
    {
    TASKFN *fn;             // the thread function generated by OMP
    char *data;             // the thread's local data pointer
    };

// An array of tasks.
static task tasks[GOMP_NUM_TASKS];

// A pool of idle tasks
static FIFO<task *, GOMP_NUM_TASKS> task_pool;

// tasks ready to be run
static FIFO<task *, GOMP_NUM_TASKS> ready_tasks;



// an omp_thread

struct omp_thread
    {
    Context context;        // the thread's registers
    int id;                 // a unique ID for each thread, also the index into the thread table
    Context *next;          // link to the next thread, when chained
    struct task *task = 0;  // the thread's implicit task, nonzero if running
    int team = 0;           // the current team number
    unsigned single = 0;    // counts the number of "#pragma omp single" seen
    bool arrived = false;   // arrived at a barrier, waiting for other threads to arrive
    bool mwaiting = false;  // waiting on a mutex
    char stack_low;         // low address of the stack (for debug)
    char *stack_high;       // high address of the stack pointer
    };

// an array of omp_threads
static omp_thread omp_threads[GOMP_MAX_NUM_THREADS];


struct team
    {
    bool mutex = false;
    unsigned single = 0;
    int sections_count = 0;
    int sections = 0;
    int section = 0;
    int tasks = 0;
    void *copyprivate = 0;
    };

static team teams[GOMP_NUM_TEAMS];

static int team = 0;


int dyn_var = 0;

static int gomp_num_threads = GOMP_DEFAULT_NUM_THREADS;

int omp_verbose = OMP_VERBOSE_DEFAULT;



// this is the code for every member of the thread pool

static uint32_t gomp_worker()
    {
    int id = omp_get_thread_num();
    struct task *task;
    TASKFN *fn;
    char *data;

    while(true)
        {
        if((task=omp_threads[id].task) != 0)
            {
            if(omp_verbose>=2)printf("start implicit task %8p, id = %3d\n", task, id);
            fn = task->fn;                              // run the assigned implicit task
            data = task->data;
            fn(data);
            if(omp_verbose>=2)printf("end   implicit task %8p, id = %3d\n", task, id);
            task_pool.add(task);                        // return the task to the pool
            --teams[team].tasks;
            omp_threads[id].task = 0;                   // forget the completed task
            }
        else if(ready_tasks.take(task))                 // if there are any explicit tasks waiting for a context
            {
            if(omp_verbose>=2)printf("start explicit task %8p, id = %3d\n", task, id);
            fn = task->fn;                              // run the explicit task
            data = task->data;
            fn(data);
            if(omp_verbose>=2)printf("end   explicit task %8p, id = %3d\n", task, id);
            data = data - data[-1];                     // undo the arg alignment to recover the address returned from malloc
            if(omp_verbose>=2)printf("free data %8p\n", data);
            free(data);                                 // free the data
            task_pool.add(task);                        // free the task
            --teams[team].tasks;
            }

        yield();
        }

    return 0;
    }



// Powerup initialization of libgomp.
// Must be called after thread and threadFIFO are setup.

void libgomp_init()
    {
    for(int i=0; i<GOMP_NUM_TASKS; i++)                           // put all tasks into the idle task pool
        {
        task_pool.add(&tasks[i]);
        }

    for(int i=0; i<GOMP_MAX_NUM_THREADS; i++)       // start all the omp_threads
        {
        omp_threads[i].id = i;
        omp_threads[i].context.spawn(gomp_worker, gomp_stacks[i]);
        }
    }



extern "C"
void GOMP_parallel(
    TASKFN *fn,                                     // the context code
    char *data,                                     // the context local data
    unsigned num_threads,                           // the requested number of threads
    unsigned flags __attribute__((__unused__)))     // flags (ignored for now)
    {
    if(num_threads == 0)
        {
        num_threads = GOMP_DEFAULT_NUM_THREADS;
        }

    if(num_threads > GOMP_MAX_NUM_THREADS)
        {
        num_threads = GOMP_MAX_NUM_THREADS;
        }

    gomp_num_threads = num_threads;

    ++team;

    teams[team].mutex = false;
    teams[team].single = 0;
    teams[team].sections_count = 0;
    teams[team].sections = 0;
    teams[team].section= 0;
    teams[team].copyprivate = 0;

    // start each member of the team
    for(unsigned i=0; i<num_threads; i++)
        {
        struct task *task = 0;

        bool ok = task_pool.take(task);
        if(!ok)printf("task_pool.take failed\n");
        ++teams[team].tasks;
        task->fn = fn;
        task->data = data;

        if(omp_verbose>=2)printf("create implicit task %8p, id = %3d\n", task, i);


        omp_threads[i].arrived = false;
        omp_threads[i].mwaiting = false;
        omp_threads[i].single = 0;
        omp_threads[i].team = team;
        omp_threads[i].task = task;                     // this field becoming non-zero kicks off the implicit task
        }

    // wait for each team member and any explicit tasks to complete
    while(teams[team].tasks)
        {
        yield();
        }

    --team;
    }


extern "C"
void GOMP_barrier()
    {
    int id = omp_get_thread_num();              // the id if this context
    int num = omp_get_num_threads();            // the number of threads in the current team

    omp_threads[id].arrived = true;             // signal that this thread has reached the barrier

    for(int i=0; i<num; i++)                    // see if any other threads in the team have not yet reached the barrier
        {
        if(omp_threads[i].arrived == false)
            {                                   // get here if any team member has not yet arrived
            omp_threads[id].context.suspend();  // suspend this thread until all other threads have arrived
            return;                             // when resumed, some other thread has done all the barrier cleanup work, so just keep going
            }
        }

    // get here if all threads in the team have arrived at the barrier

    for(int i=0; i<num; i++)                    // clear the arrived flag for all team members
        {
        omp_threads[i].arrived = false;         // clear the arrived-at-barrier flag
        }

    for(int i=0; i<num; i++)                    // resume all other threads in the team
        {
        if(i != id)                             // don't try to resume self
            {
            omp_threads[i].context.resume();    // resume the team member
            }
        }
    }


extern "C"
void GOMP_critical_start()
    {
    int id = omp_get_thread_num();

    while(teams[team].mutex == true)
        {
        omp_threads[id].mwaiting = true;
        omp_threads[id].context.suspend();      // suspend this thread until it can grab the mutex
        omp_threads[id].mwaiting = false;
        }

    teams[team].mutex = true;
    }

extern "C"
void GOMP_atomic_start()
    {
    GOMP_critical_start();
    }


static int mindex = 0;                          // a commutator

extern "C"
void GOMP_critical_end()
    {
    int num = omp_get_num_threads();

    teams[team].mutex = false;

    for(int i=0; i<num; i++)                    // resume all other threads in the team
        {
        int m = mindex;
        mindex = (mindex+1)%num;

        if(omp_threads[m].mwaiting == true)
            {
            omp_threads[m].context.resume();    // resume the next context in the rotation
            return;
            }
        }
    }

extern "C"
void GOMP_atomic_end()
    {
    GOMP_critical_end();
    }



extern "C"
bool GOMP_single_start()
    {
    int id = omp_get_thread_num();

    if(omp_threads[id].single++ == teams[team].single)
        {
        teams[team].single++;
        return true;
        }
    else
        {
        return false;
        }
    }



extern "C"
void *GOMP_single_copy_start()
    {
    int id = omp_get_thread_num();

    if(omp_threads[id].single++ == teams[team].single)
        {
        teams[team].single++;
        return 0;
        }
    else
        {
        return teams[team].copyprivate;
        }
    }


extern "C"
void GOMP_single_copy_end(void *data)
    {
    teams[team].copyprivate = data;
    }




extern "C"
int GOMP_sections_next()                // for each thread that iterates the "sections"
    {
    if(teams[team].section > teams[team].sections)  // if all sections have been executed
        {
        teams[team].section = 0;        // clear the index, it latches at zero
        }

    if(teams[team].section == 0)        // if all sections have been run
        {
        return 0;                       // return 0, select the "end" action and stop iterating
        }

    return teams[team].section++;       // otherwise return the current index and increment it
    }

extern "C"
int GOMP_sections_start(int num)        // each team member calls this once at the beginning of sections
    {
    if(teams[team].sections_count == 0) // when the first context gets here
        {
        teams[team].sections = num;     // capture the number of sections
        teams[team].section = 1;        // init to the first section
        }

    ++teams[team].sections_count;       // count the number of threads that have started the sections

    return GOMP_sections_next();        // for the rest, start is the same as next
    }

extern "C"
void GOMP_sections_end()                // each thread runs this once when all the sections hae been executed
    {
    int num = omp_get_num_threads();

    if(teams[team].sections_count == num) // if all team members have encountered the "start"
        {
        teams[team].sections_count = 0; // re-arm the sections start, though note that some may still be in a section
        }

    GOMP_barrier();                     // hold everyone here until all have arrived
    }


#if 0

/////////////
// LOCKING //
/////////////

extern "C"
void omp_init_lock(omp_lock_t *lock)
    {
    *lock = 0;
    }

extern "C"
void omp_set_lock(omp_lock_t *lock)
    {
    while(*lock)
        {
        yield();
        }

    *lock = 1;
    }

extern "C"
void omp_unset_lock(omp_lock_t *lock)
    {
    *lock = 0;
    }

extern "C"
int omp_test_lock(omp_lock_t *lock)
    {
    if(*lock)
        {
        return 0;
        }

    *lock = 1;
    return 1;
    }

extern "C"
void omp_destroy_lock(omp_lock_t *lock)
    {
    *lock = 0;
    }



////////////////////
// NESTED LOCKING //
////////////////////

extern "C"
void omp_init_nest_lock(omp_nest_lock_t *lock)
    {
    lock->lock = 0;
    lock->count = 0;
    lock->owner = 0;
    }

extern "C"
void omp_set_nest_lock(omp_nest_lock_t *lock)
    {
    int id = omp_get_thread_num();

    if(lock->lock && lock->owner == id)
        {
        ++lock->count;
        return;
        }
 
    while(lock->lock==1)
        {
        yield();
        }

    lock->lock = 1;
    lock->count = 1;
    lock->owner = id;
    }

extern "C"
void omp_unset_nest_lock(omp_nest_lock_t *lock)
    {
    // It is assumed that the caller matches the owner. This is not checked.
    // It is assumed that count>0. This is not checked.

    --lock->count;
    if(lock->count == 0)
        {
        lock->owner = 0;
        lock->lock = 0;
        }
    }

extern "C"
int omp_test_nest_lock(omp_nest_lock_t *lock)
    {
    int id = omp_get_thread_num();

    if(lock->lock && lock->owner == id)
        {
        ++lock->count;
        return lock->count;
        }
 
    if(lock->lock == 0)
        {
        lock->lock = 1;
        lock->count = 1;
        lock->owner = id;
        return 1;
        }

    return 0;
    }

extern "C"
void omp_destroy_nest_lock(omp_nest_lock_t *lock)
    {
    lock->lock = 0;
    lock->count = 0;
    lock->owner = 0;
    }

#endif


///////////
// Tasks //
///////////

extern "C"
void GOMP_task (    void (*fn) (void *),
                    void *data,
                    void (*cpyfn) (void *, void *),
                    long arg_size,
                    long arg_align,
                    bool if_clause,
                    unsigned flags,
                    void **depend,
                    int priority_arg,
                    void *detach)
    {
    int id = omp_get_thread_num();
    if(!if_clause                               // if if_clause if false
    || !task_pool)                              // or the task pool is empty, we have to run the task right now
        {
        if(cpyfn)                               // if a copy function is defined, copy the data to a private buffer first
            {
            char buf[arg_size + arg_align - 1];
            char *dst = &buf[arg_align-1];
            dst = (char *)((uintptr_t)dst & ~(arg_align-1));
            cpyfn(dst, data);
            if(omp_verbose>=2)printf("call explicit task, id = %3d, code = %8p, data = %8p\n", id, fn, data);
            fn(dst);            
            }
        else
            {
            if(omp_verbose>=2)printf("call explicit task, id = %3d, code = %8p, data = %8p\n", id, fn, data);
            fn(data);
            }
        }
    else                                        // else queue the task to be executed by another context later
        {
        char *argmem = (char *)malloc(arg_size + arg_align);                                    // allocate memory for data
        if(omp_verbose>=2)printf("malloc data %8p, id = %3d\n", argmem, id);
        if(argmem == 0)printf("malloc returned 0\n");
        char *arg = (char *)((uintptr_t)(argmem + arg_align) & ~(uintptr_t)(arg_align - 1));    // align the data memory
        arg[-1] = arg-argmem;                                                                   // save the alignment offset at arg-1 so we can calc the addr for free later

        if(cpyfn)
            {
            cpyfn(arg, data);
            }
        else
            {
            memcpy(arg, data, arg_size);
            } 
        
        struct task *task = 0;

        bool ok = task_pool.take(task);        // create a new task
        if(!ok)printf("task_pool.take failed\n");
        ++teams[team].tasks;

        task->fn = fn;                          // give it code
        task->data = arg;                       // and data

        if(omp_verbose>=2)printf("create explicit task %8p, id = %3d\n", task, id);
        ready_tasks.add(task);                  // add it to the list of explicit tasks
        }

    }



/////////////////////////////////
// Explicitly called functions //
/////////////////////////////////

// return the number of threads available
extern "C"
int omp_get_num_threads()
    {
    return gomp_num_threads;
    }


extern "C"
int omp_get_thread_num()
    {
    omp_thread *thrd;

    __asm__ __volatile__("    mov %[thrd], r9" : [thrd]"=r"(thrd));

    return thrd->id;
    }


// extern "C" void omp_set_num_threads (int);
// extern "C" int omp_get_max_threads (void);



extern "C"
void omp_set_dynamic(int dyn)
    {
    dyn_var = dyn;
    }

extern "C"
int omp_get_dynamic(void)
    {
    return dyn_var;
    }



// extern "C" int omp_get_num_teams (void);
// extern "C" int omp_get_team_num (void);
// extern "C" int omp_get_team_size (int);


// extern "C" int omp_get_num_procs (void);
// extern "C" int omp_in_parallel (void);
// extern "C" void omp_set_nested (int);
// extern "C" int omp_get_nested (void);
// extern "C" void omp_init_lock_with_hint (omp_lock_t *, omp_sync_hint_t);
// extern "C" void omp_init_nest_lock_with_hint (omp_nest_lock_t *, omp_sync_hint_t);
// extern "C" double omp_get_wtime (void);
// extern "C" double omp_get_wtick (void);
// extern "C" void omp_set_schedule (omp_sched_t, int);
// extern "C" void omp_get_schedule (omp_sched_t *, int *);
// extern "C" int omp_get_thread_limit (void);
// extern "C" void omp_set_max_active_levels (int);
// extern "C" int omp_get_max_active_levels (void);
// extern "C" int omp_get_level (void);
// extern "C" int omp_get_ancestor_thread_num (int);
// extern "C" int omp_get_active_level (void);
// extern "C" int omp_in_final (void);
// extern "C" int omp_get_cancellation (void);
// extern "C" omp_proc_bind_t omp_get_proc_bind (void);
// extern "C" int omp_get_num_places (void);
// extern "C" int omp_get_place_num_procs (int);
// extern "C" void omp_get_place_proc_ids (int, int *);
// extern "C" int omp_get_place_num (void);
// extern "C" int omp_get_partition_num_places (void);
// extern "C" void omp_get_partition_place_nums (int *);
// extern "C" void omp_set_default_device (int);
// extern "C" int omp_get_default_device (void);
// extern "C" int omp_get_num_devices (void);
// extern "C" int omp_is_initial_device (void);
// extern "C" int omp_get_initial_device (void);
// extern "C" int omp_get_max_task_priority (void);
// extern "C" void *omp_target_alloc (__SIZE_TYPE__, int);
// extern "C" void omp_target_free (void *, int);
// extern "C" int omp_target_is_present (const void *, int);
// extern "C" int omp_target_memcpy (void *, const void *, __SIZE_TYPE__, __SIZE_TYPE__, __SIZE_TYPE__, int, int);
// extern "C" int omp_target_memcpy_rect (void *, const void *, __SIZE_TYPE__, int, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, const __SIZE_TYPE__ *, int, int);
// extern "C" int omp_target_associate_ptr (const void *, const void *, __SIZE_TYPE__, __SIZE_TYPE__, int);
// extern "C" int omp_target_disassociate_ptr (const void *, int);
// extern "C" void omp_set_affinity_format (const char *);
// extern "C" __SIZE_TYPE__ omp_get_affinity_format (char *, __SIZE_TYPE__);
// extern "C" void omp_display_affinity (const char *);
// extern "C" __SIZE_TYPE__ omp_capture_affinity (char *, __SIZE_TYPE__, const char *);
// extern "C" int omp_pause_resource (omp_pause_resource_t, int);
// extern "C" int omp_pause_resource_all (omp_pause_resource_t);
