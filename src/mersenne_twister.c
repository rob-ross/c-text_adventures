// mersenne_twister.c
// see https://en.wikipedia.org/wiki/Mersenne_Twister

// Created 2026/05/11 17:42:20 PDT

// todo (rob) we need to run a statistical analysis on this code to verify it produces uniform distributions

#include "mersenne_twister.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/fcntl.h>

#define m 397
#define w 32
#define r 31
#define UMASK (0xffffffffUL << r)
#define LMASK (0xffffffffUL >> (w-r))
#define a 0x9908b0dfUL
#define u 11
#define s 7
#define t 15
#define l 18
#define b 0x9d2c5680UL
#define c 0xefc60000UL
#define f 1812433253UL



static uint32_t get_random_uint_from_urandom() ;

// Must call to initialize and seed generator before calling mt_rand()
// if `seed` is 0, tries to get seed from "/dev/urandom"
void mt_initialize_state(MTState* state, uint32_t seed)
{
    if (!seed) {
        seed = get_random_uint_from_urandom();
    }

    uint32_t* state_array = &(state->state_array[0]);

    state_array[0] = seed;                          // suggested initial seed = 19650218UL

    for (int i=1; i<MT_NUM_STATES; i++)
    {
        seed = f * (seed ^ (seed >> (w-2))) + i;    // Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
        state_array[i] = seed;
    }

    state->state_index = 0;
}


uint32_t mt_rand_range(MTState* state, uint32_t min_inclusive, uint32_t max_exclusive) {
    if (min_inclusive > max_exclusive) {
        // Handle invalid range by swapping them
        const uint32_t temp = min_inclusive;
        min_inclusive = max_exclusive;
        max_exclusive = temp;
    }

    // If min_inclusive == max_exclusive, the range is empty. Return min_inclusive.
    if (min_inclusive == max_exclusive) {
        return min_inclusive;
    }

    uint32_t range_size = max_exclusive - min_inclusive;

    // Calculate the largest multiple of 'range_size' that is less than or equal to UINT32_MAX.
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range_size);
    uint32_t rand_val;

    // Rejection sampling: keep generating numbers until one falls within the unbiased range.
    do {
        rand_val = mt_random_uint32(state);
    } while (rand_val >= limit);

    // Map the unbiased random number to the desired range.
    return min_inclusive + (rand_val % range_size);
}

// Convenience function that calls mt_rand_range() and adds 1 to the `max` argument so that
// the bound is inclusive here
uint32_t mt_rand_range_inclusive(MTState* state, uint32_t min_inclusive, uint32_t max_inclusive) {
    if (max_inclusive == UINT32_MAX) {
        if (min_inclusive == 0) {
            // Full range [0, UINT32_MAX]
            return mt_random_uint32(state);
        }
        // Range is [min_inclusive, UINT32_MAX]
        // This is equivalent to generating a random number in [0, UINT32_MAX - min_inclusive]
        // and then adding min_inclusive to the result.
        // The exclusive upper bound for this sub-range would be (UINT32_MAX - min_inclusive + 1).
        uint32_t adjusted_max_exclusive = UINT32_MAX - min_inclusive + 1;
        return min_inclusive + mt_rand_range(state, 0, adjusted_max_exclusive);
    }
    // Normal case: max_inclusive < UINT32_MAX
    // The exclusive upper bound for mt_rand_range is max_inclusive + 1
    return mt_rand_range(state, min_inclusive, max_inclusive + 1);
}

// Function to generate a random float between 0.0f (inclusive) and 1.0f (exclusive)
float mt_random_float(MTState* state) {
    // Generate a random 32-bit unsigned integer
    uint32_t random_uint = mt_random_uint32(state);

    // Perform the division using double precision to avoid loss of precision,
    // then cast the final result to float.
    return (float)((double)random_uint / (UINT32_MAX + 1.0));
}

// Function to generate a random double between 0.0 (inclusive) and 1.0 (exclusive)
double mt_random_double(MTState* state) {
    // Generate a random 32-bit unsigned integer
    uint32_t random_uint = mt_random_uint32(state);

    // Divide by (UINT32_MAX + 1.0) to get a double in [0.0, 1.0)
    // Using 1.0 ensures double division.
    // Adding 1.0 to UINT32_MAX ensures that UINT32_MAX itself maps to a value < 1.0.
    return (double)random_uint / (UINT32_MAX + 1.0);
}

uint32_t mt_random_uint32(MTState* state)
{
    uint32_t* state_array = &(state->state_array[0]);

    int k = state->state_index;      // point to current state location
                                     // 0 <= state_index <= n-1   always

//  int k = k - MT_NUM_STATES;                   // point to state MT_NUM_STATES iterations before
//  if (k < 0) k += MT_NUM_STATES;               // modulo MT_NUM_STATES circular indexing
                                     // the previous 2 lines actually do nothing
                                     //  for illustration only

    int j = k - (MT_NUM_STATES-1);               // point to state MT_NUM_STATES-1 iterations before
    if (j < 0) j += MT_NUM_STATES;               // modulo MT_NUM_STATES circular indexing

    uint32_t x = (state_array[k] & UMASK) | (state_array[j] & LMASK);

    uint32_t xA = x >> 1;
    if (x & 0x00000001UL) xA ^= a;

    j = k - (MT_NUM_STATES-m);                   // point to state MT_NUM_STATES-m iterations before
    if (j < 0) j += MT_NUM_STATES;               // modulo MT_NUM_STATES circular indexing

    x = state_array[j] ^ xA;         // compute next value in the state
    state_array[k++] = x;            // update new state value

    if (k >= MT_NUM_STATES) k = 0;               // modulo MT_NUM_STATES circular indexing
    state->state_index = k;

    uint32_t y = x ^ (x >> u);       // tempering
             y = y ^ ((y << s) & b);
             y = y ^ ((y << t) & c);
    uint32_t z = y ^ (y >> l);

    return z;
}

// Function to get a random unsigned int from /dev/urandom
static uint32_t get_random_uint_from_urandom() {
    uint32_t random_value;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        perror("Error opening /dev/urandom");
        // Fallback: If /dev/urandom fails, you might fall back to a less ideal method
        // or terminate if strong randomness is critical.
        // For this example, we'll just return a simple rand() result as a last resort.
        srand(time(nullptr)); // NOLINT(*-msc51-cpp)
        return rand(); // NOLINT(*-msc50-cpp)
    }

    ssize_t bytes_read = read(fd, &random_value, sizeof(random_value));
    if (bytes_read == -1) {
        perror("Error reading from /dev/urandom");
        close(fd);
        srand(time(nullptr)); // NOLINT(*-msc51-cpp)
        return rand(); // NOLINT(*-msc50-cpp)
    } else if (bytes_read != sizeof(random_value)) {
        fprintf(stderr, "Warning: Read fewer bytes (%zd) than expected (%zu) from /dev/urandom\n", bytes_read, sizeof(random_value));
        close(fd);
        srand(time(nullptr)); // NOLINT(*-msc51-cpp)
        return rand(); // NOLINT(*-msc50-cpp)
    }

    close(fd);
    return random_value;
}