/* Copyright (c) 2016 Joel Sherrill <joel@rtems.org> */
/*
FUNCTION
<<random>>, <<srandom>>---pseudo-random numbers

INDEX
        random
INDEX
        srandom

SYNOPSIS
        #define _XOPEN_SOURCE 500
        #include <stdlib.h>
        long int random(void);
        void srandom(unsigned int <[seed]>);



DESCRIPTION
<<random>> returns a different integer each time it is called; each
integer is chosen by an algorithm designed to be unpredictable, so
that you can use <<random>> when you require a random number.
The algorithm depends on a static variable called the ``random seed'';
starting with a given value of the random seed always produces the
same sequence of numbers in successive calls to <<random>>.

You can set the random seed using <<srandom>>.  You can
exploit this to make the pseudo-random sequence less predictable, if
you wish, by using some other unpredictable value (often the least
significant parts of a time-varying value) as the random seed before
beginning a sequence of calls to <<random>>; or, if you wish to ensure
(for example, while debugging) that successive runs of your program
use the same ``random'' numbers, you can use <<srandom>> to set the same
random seed at the outset.

RETURNS
<<random>> returns the next pseudo-random integer in sequence; it is a
number between <<0>> and <<2**31 - 1>> (inclusive).

<<srandom>> does not return a result.

NOTES
<<random>> and <<srandom>> share one state, protected by a lock.

_XOPEN_SOURCE may be any value >= 500.

PORTABILITY
<<random>> is required by XSI. This implementation uses the additive
feedback generator from BSD, with a sequence independent of <<rand>>.

<<random>> requires no supporting OS subroutines.
*/

#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/lock.h>
#include "local.h"

/*
 * The non-linear additive feedback generator POSIX describes (the BSD
 * one). A state array holds a header word, MAX_TYPES * rear + type,
 * and then 'deg' words of state. The front index is always 'sep'
 * words after the rear one. Each step adds the rear word to the front
 * one and returns the sum without its lowest bit. Arrays of 8 to 31
 * bytes use a linear congruential generator instead. The words are
 * copied in and out, as the application's array may be unaligned.
 *
 * All four functions must be thread-safe, so they share one state,
 * guarded by the libc lock.
 */

#define MAX_TYPES 5

static const struct {
    uint8_t deg, sep;
} __random_types[MAX_TYPES] = {
    { 0,  0 },
    { 7,  3 },
    { 15, 1 },
    { 31, 3 },
    { 63, 1 },
};

/* The default: as if initstate(1, state, 128) had been called */
#define RANDOM_DEFAULT_TYPE 3
static uint32_t __random_builtin[1 + 31] = { RANDOM_DEFAULT_TYPE };
static bool     __random_builtin_seeded;
char           *__random_state = (char *)__random_builtin;

static uint32_t
__random_get(const char *state, unsigned i)
{
    uint32_t v;

    memcpy(&v, state + i * sizeof(v), sizeof(v));
    return v;
}

static void
__random_put(char *state, unsigned i, uint32_t v)
{
    memcpy(state + i * sizeof(v), &v, sizeof(v));
}

/* Check the header of a state array; returns the type or -1 */
int
__random_type(const char *state)
{
    uint32_t header = __random_get(state, 0);
    unsigned type = header % MAX_TYPES;
    unsigned rear = header / MAX_TYPES;

    if (rear >= __random_types[type].deg && rear != 0)
        return -1;
    return (int)type;
}

/* The next value from a state array; the caller holds the lock */
static long
__random_step(char *state)
{
    uint32_t header = __random_get(state, 0);
    unsigned type = header % MAX_TYPES;
    unsigned rear = header / MAX_TYPES;
    unsigned deg = __random_types[type].deg;
    unsigned front;
    uint32_t v;

    if (deg == 0) {
        v = (__random_get(state, 1) * 1103515245U + 12345U) & 0x7fffffff;
        __random_put(state, 1, v);
        return (long)v;
    }

    front = rear + __random_types[type].sep;
    if (front >= deg)
        front -= deg;
    v = __random_get(state, 1 + front) + __random_get(state, 1 + rear);
    __random_put(state, 1 + front, v);
    if (++rear >= deg)
        rear = 0;
    __random_put(state, 0, MAX_TYPES * rear + type);
    return (long)(v >> 1);
}

/* Seed a state array of the given type; the caller holds the lock */
void
__random_seed(char *state, int type, unsigned seed)
{
    unsigned deg = __random_types[type].deg;
    int32_t  word;
    unsigned i;

    if (state == (char *)__random_builtin)
        __random_builtin_seeded = true;

    __random_put(state, 0, (uint32_t)type);

    /* Zero would be a fixed point */
    if (seed == 0)
        seed = 1;
    __random_put(state, 1, seed);
    if (deg == 0)
        return;

    /* Fill the rest with 16807 * previous % (2^31 - 1), by Schrage's method */
    word = (int32_t)seed;
    for (i = 1; i < deg; i++) {
        int32_t hi = word / 127773;
        int32_t lo = word % 127773;

        word = 16807 * lo - 2836 * hi;
        if (word < 0)
            word += 2147483647;
        __random_put(state, 1 + i, (uint32_t)word);
    }

    /* Discard the start of the sequence, where the seed shows through */
    for (i = 0; i < 10 * deg; i++)
        (void)__random_step(state);
}

/* Seed the built-in array on first use; the caller holds the lock */
void
__random_init(void)
{
    if (!__random_builtin_seeded)
        __random_seed((char *)__random_builtin, RANDOM_DEFAULT_TYPE, 1);
}

long int
random(void)
{
    long v;

    __LIBC_LOCK();
    __random_init();
    v = __random_step(__random_state);
    __LIBC_UNLOCK();
    return v;
}
