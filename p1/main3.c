#include "Cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define L1_LINES (L1_SIZE / BLOCK_SIZE)
#define L2_LINES (L2_SIZE / BLOCK_SIZE)
#define SET_NUMBER 2

// Structure representing a cache line
typedef struct CacheLine { 
    uint8_t valid;
    uint8_t dirty;
    uint32_t tag;
    uint8_t block[BLOCK_SIZE]; // data block
} CacheLine;

typedef struct CacheSet { 
    CacheLine lines[SET_NUMBER];
    uint8_t lru; // 0 for line 0 is LRU, 1 for line 1 is LRU
} CacheSet;

// Structure representing L1 cache
typedef struct Cache {
    uint32_t init; // initialization flag
    CacheSet sets[L1_LINES / SET_NUMBER];
} Cache;

// Structure representing L2 cache
typedef struct CacheL2 {
    uint32_t init; // initialization flag
    CacheSet sets[L2_LINES / SET_NUMBER];
} CacheL2;

// Global variables
uint32_t time;
uint8_t DRAM[DRAM_SIZE];
Cache cache_L1;
CacheL2 cache_L2;

/**
 * @brief Initialize the L2 cache by setting all lines to invalid and clean.
 */
void initCacheL2() {
	memset(&cache_L2, 0, sizeof(CacheL2)); // set all bytes to zero
	cache_L2.init = 1;
}

/**
 * @brief Initialize the L1 cache by setting all lines to invalid and clean.
 */
void initCacheL1() {
	memset(&cache_L1, 0, sizeof(Cache)); // set all bytes to zero
	cache_L1.init = 1;
}

/**
 * @brief Initialize both L1 and L2 caches.
 */
void initCache() {
	initCacheL1();
	initCacheL2();
}

/**
 * @brief Reset the global time variable to zero.
 */
void resetTime() {
	time = 0;
}

/**
 * @brief Get the current value of the global time variable.
 * @return The current time.
 */
uint32_t getTime() {
	return time;
}

/**
 * @brief Reset the memory address to the nearest block boundary.
 * @param memAddress A pointer to the memory address to reset.
 */
void resetOffset(uint32_t* memAddress) {
	(*memAddress) &= ~(BLOCK_SIZE - 1); // clear the lower bits
}

/**
 * @brief Calculate the tag, index, and offset from a given memory address.
 * @param address The memory address to analyze.
 * @param numLines The number of lines in the cache.
 * @param tag Pointer to store the calculated tag.
 * @param index Pointer to store the calculated index.
 * @param offset Pointer to store the calculated offset.
 */
void calculateTagIndexOffset(uint32_t address, int numLines, uint32_t* tag, uint32_t* index, uint32_t* offset) {
	*offset = address % BLOCK_SIZE;
	*index = (address / BLOCK_SIZE) % numLines;
	*tag = address / (BLOCK_SIZE * numLines);
}

/**
 * @brief Access the DRAM to read or write data.
 */
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {
    if (address >= DRAM_SIZE) {
        exit(EXIT_FAILURE);
    }

    if (mode == MODE_READ) {
        memcpy(data, &DRAM[address], BLOCK_SIZE); 
        time += DRAM_READ_TIME;
    } else if (mode == MODE_WRITE) {
        memcpy(&DRAM[address], data, BLOCK_SIZE); 
        time += DRAM_WRITE_TIME;
    }
}

/**
 * @brief Access the L2 cache to read or write data.
 */
void accessL2(uint32_t address, uint8_t *data, uint32_t mode) {
    uint32_t index, tag, offset, memAddress = address;
    CacheLine *refLine;
    CacheSet *set;

    if (!cache_L2.init) {
        initCache(); // initialize if not done
    }

    calculateTagIndexOffset(address, L2_LINES, &tag, &index, &offset);

    set = &cache_L2.sets[index];
    refLine = &set->lines[0]; // Start with the first line in the set
    int hitLine = -1;

    // Check both lines in the set
    for (int i = 0; i < SET_NUMBER; i++) {
        refLine = &set->lines[i];
        if (refLine->valid && refLine->tag == tag) { // cache hit
            hitLine = i;
            break;
        }
    }

    // If we missed in the cache
    if (hitLine == -1) {
        uint8_t tempBlock[BLOCK_SIZE];
        resetOffset(&memAddress);
        accessDRAM(memAddress, tempBlock, MODE_READ);

        // Determine which line to replace based on LRU
        int lruLine = set->lru != 0;

        if (set->lines[lruLine].valid && set->lines[lruLine].dirty) {
            accessDRAM(memAddress, set->lines[lruLine].block, MODE_WRITE);
        }

        memcpy(set->lines[lruLine].block, tempBlock, BLOCK_SIZE);
        set->lines[lruLine].valid = 1;
        set->lines[lruLine].dirty = 0;
        set->lines[lruLine].tag = tag;

        hitLine = lruLine; // Update hit line to newly loaded line
    }

    // Update LRU status
    set->lru = (hitLine == 0) ? 1 : 0; // Set the LRU bit accordingly

    if (mode == MODE_READ) {
        memcpy(data, &(set->lines[hitLine].block[offset]), WORD_SIZE);
        time += L2_READ_TIME;
    } else if (mode == MODE_WRITE) {
        memcpy(&(set->lines[hitLine].block[offset]), data, WORD_SIZE);
        time += L2_WRITE_TIME;
        set->lines[hitLine].dirty = 1;
    }
}

/**
 * @brief Access the L1 cache to read or write data.
 */
void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {
    uint32_t index, tag, offset, memAddress = address;
    CacheLine *refLine;
    CacheSet *set;

    if (!cache_L1.init) {
        initCache(); // initialize if not done
    }

    calculateTagIndexOffset(address, L1_LINES, &tag, &index, &offset);

    set = &cache_L1.sets[index];
    refLine = &set->lines[0]; // Start with the first line in the set
    int hitLine = -1;

    // Check both lines in the set
    for (int i = 0; i < SET_NUMBER; i++) {
        refLine = &set->lines[i];
        if (refLine->valid && refLine->tag == tag) { // cache hit
            hitLine = i;
            break;
        }
    }

    // If we missed in the cache
    if (hitLine == -1) {
        uint8_t tempBlock[BLOCK_SIZE];
        accessL2(address, tempBlock, MODE_READ);

        // Determine which line to replace based on LRU
        int lruLine = set->lru != 0;

        if (set->lines[lruLine].valid && set->lines[lruLine].dirty) {
            accessL2(address, set->lines[lruLine].block, MODE_WRITE);
        }

        memcpy(set->lines[lruLine].block, tempBlock, BLOCK_SIZE);
        set->lines[lruLine].valid = 1;
        set->lines[lruLine].dirty = 0;
        set->lines[lruLine].tag = tag;

        hitLine = lruLine; // Update hit line to newly loaded line
    }

    // Update LRU status
    set->lru = (hitLine == 0) ? 1 : 0; // Set the LRU bit accordingly

    if (mode == MODE_READ) {
        memcpy(data, &(set->lines[hitLine].block[offset]), WORD_SIZE);
        time += L1_READ_TIME;
    } else if (mode == MODE_WRITE) {
        memcpy(&(set->lines[hitLine].block[offset]), data, WORD_SIZE);
        time += L1_WRITE_TIME;
        set->lines[hitLine].dirty = 1;
    }
}

/**
 * @brief Read data from a specified address starting with the L1 cache.
 */
void read(uint32_t address, uint8_t *data) {
    accessL1(address, data, MODE_READ);
}

/**
 * @brief Write data to a specified address starting with the L1 cache.
 */
void write(uint32_t address, uint8_t *data) {
    accessL1(address, data, MODE_WRITE);
}

int main() {

  // set seed for random number generator
  srand(0);

  int clock1, value;

  for(int n = 1; n <= DRAM_SIZE/4; n*=WORD_SIZE) {

    resetTime();
    initCache();

    printf("\nNumber of words: %d\n", (n-1)/WORD_SIZE + 1);
    
    for(int i = 0; i < n; i+=WORD_SIZE) {
      write(i, (unsigned char *)(&i));
      clock1 = getTime();
      printf("Write; Address %d; Value %d; Time %d\n", i, i, clock1);
    }

    for(int i = 0; i < n; i+=WORD_SIZE) {
      read(i, (unsigned char *)(&value));
      clock1 = getTime();
      printf("Read; Address %d; Value %d; Time %d\n", i, value, clock1);
    }  

  }

  printf("\nRandom accesses\n");

  // Do random accesses to the cache
  for(int i = 0; i < 100; i++) {
    int address = rand() % (DRAM_SIZE/4);
    address = address - address % WORD_SIZE;
    int mode = rand() % 2;
    if (mode == MODE_READ) {
      read(address, (unsigned char *)(&value));
      clock1 = getTime();
      printf("Read; Address %d; Value %d; Time %d\n", address, value, clock1);
    }
    else {
      write(address, (unsigned char *)(&address));
      clock1 = getTime();
      printf("Write; Address %d; Value %d; Time %d\n", address, address, clock1);
    }
  }
  
  return 0;
}