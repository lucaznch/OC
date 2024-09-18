#ifndef SIMPLECACHE_H
#define SIMPLECACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cache.h"




/* - - - - - - - - time manipulation - - - - - - - - */

/// @brief reset the time
void reset_time();

/// @brief returns the time
/// @return time
uint32_t get_time();

/* - - - - - - - - - - - - - - - - - - - - - - - - - */




/* - - - - - - - - - memory RAM - - - - - - - - - */

/// @brief access main memory DRAM (byte addressable)
/// @param address address to access
/// @param data data from address
/// @param mode mode of access (read or write)
void access_DRAM(uint32_t, uint8_t *, uint32_t);

/* - - - - - - - - - - - - - - - - - - - - - - - - */




/* - - - - - - - - - - cache - - - - - - - - - - */

/// @brief initialize the L1 cache
void init_cache();

/// @brief access the L1 cache
/// @param  address address to access
/// @param  data data from address
/// @param  mode mode of access (read or write)
void access_L1(uint32_t, uint8_t *, uint32_t);

typedef struct CacheLine {
    uint8_t valid;
    uint8_t dirty;
    uint32_t tag;
} CacheLine;

typedef struct Cache {
    uint32_t init;
    CacheLine line;
} Cache;

/* - - - - - - - - - - - - - - - - - - - - - - - */




/* - - - - - - - - - - interfaces - - - - - - - - - - */

/// @brief access the L1 cache in read mode
/// @param address address to access
/// @param data data from the address
void read(uint32_t, uint8_t *);

/// @brief access the L1 cache in read mode
/// @param address to access
/// @param data data from the address
void write(uint32_t, uint8_t *);

/* - - - - - - - - - - - - - - - - - - - - - - - - - */


#endif
