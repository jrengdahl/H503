#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include "libgomp.hpp"

static const int COLORS = 2;               // max number of colors
static const int MAX = 16;                 // max number of colors times steps
static const int THREADS = 4;              // max number of threads in the team


// return a color count from the input supply
static inline unsigned cget(int i, unsigned input)
    {
    return (input>>(i*8)) & 255;
    }

// decrement a color in the input supply
static inline unsigned cdec(int i, unsigned input)
    {
    return input - (1<<(i*8));
    }


static int colors = 2;                     // the number of colors for this test
static int plevel = MAX;                   // max level to make parallel
static int permutations = 0;               // total number of permutations
static bool verbose = false;               // whether to print each permutation
static int maxthread = 0;                  // the highest thread number that was spawned
static int ids[THREADS+10];                // an array to record what threads ran



// compute permutations of colored balls.
// There may be multiple balls of each color.
// input: the number of balls left of each color. Maximum of four colors. Each byte of the word represents one color.
// output: a reference to an array of the balls that have been chosen so far.
// step: the depth of recursion, starts at zero and increments for each level descended.
// left: the number of balls remaining to be chosen. Starts at colors*n and decrements for each recursion.

static void permuter(unsigned input, unsigned char (&output)[MAX], int step, int left)
    {
    int id = gomp_get_thread_id();                                          // record the thread number
    ids[id] = id;                                                           // for analysis only, it's not used in the permutation calculation

    if(left == 0)                                                           // if all the balls have been chosen
        {
        if(verbose)                                                         // if verbose, print out the permutation
            {
            const int SIZE = 32;
            int left = SIZE;
            char buf[SIZE];
            char *p = &buf[0];
            for(int i=0; i<step; i++)
                {
                int n = snprintf(p, left, "%u ", output[i]);
                p += n;
                left -= n;
                }
            printf("%s (%d(%d))\n", buf, omp_get_thread_num(), id);
            }

        #pragma omp atomic
            ++permutations;
        }
    else                                                                    // if not done yet
        {
        for(int i=0; i<colors; i++)                                         // for each possible color
            {
            if(cget(i, input) > 0)                                          // if there are any left of that color
                {
                output[step] = i;                                           // pick a ball of that color
                #pragma omp task firstprivate(output) if(step < plevel)     // note that firstprivate make a copy of the output for the new task
                    {
                    permuter(cdec(i, input), output, step+1, left-1);       // and recurse, deleting one ball of the chosen color from the input
                    }
                }
            }
        }
    }




void permute(int colors_arg, int balls, int plevel_arg, int verbose_arg)
    {
    unsigned input;
    unsigned char output[MAX];

    colors = colors_arg;
    plevel = plevel_arg;
    verbose = verbose_arg;

    if(colors > COLORS || colors*balls > MAX)           // check validity of inputs
        {
        printf("too many colors and/or balls\n");
        }

    input = 0;                                          // init the input
    for(int i=0; i<colors; i++)
        {
        input |= balls<<(i*8);
        }

    for(auto &x : output) x = 0;                        // init the output
    for(auto &x : ids) x = 0;                           // init the ID array (used for curiosity only)

    #pragma omp parallel num_threads(THREADS)           // kick off the permuter
    #pragma omp single
    permuter(input, output, 0, colors*balls);

    for(auto id: ids)maxthread = id>maxthread?id:maxthread; // figure out max thread used

    printf("permutations = %d\n", permutations);        // print results
    printf("maxthread = %d\n", maxthread);
    }
