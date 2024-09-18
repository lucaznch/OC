#include "cacheL1.h"

// represents the L1 cache as an array with 256 blocks = 16KiB
uint8_t cacheL1[L1_SIZE];

// represents the main memory (DRAM) as an array with 1024 blocks = 64KiB
uint8_t main_memory[DRAM_SIZE];

// represents the time
uint32_t time;

// represents the L1 cache
Cache simple_cache;



/* - - - - - - - - time manipulation - - - - - - - - */

void reset_time() { time = 0; }

uint32_t get_time() { return time; }



/* - - - - - - - - - memory RAM - - - - - - - - - */

void access_DRAM(uint32_t address, uint8_t *data, uint32_t mode) {

    if (address >= DRAM_SIZE - WORD_SIZE + 1) { exit(-1); }

    if (mode == MODE_READ) {
        memcpy(data, &(main_memory[address]), BLOCK_SIZE);
        time += DRAM_READ_TIME;
    }

    if (mode == MODE_WRITE) {
        memcpy(&(main_memory[address]), data, BLOCK_SIZE);
        time += DRAM_WRITE_TIME;
    }
}




/* - - - - - - - - - - cache - - - - - - - - - - */

void init_cache() { simple_cache.init = 0; }

void access_L1(uint32_t address, uint8_t *data, uint32_t mode) {
    uint32_t address_tag, MemAddress;       // add: index
    uint8_t TempBlock[BLOCK_SIZE];

    // if 
    if (simple_cache.init == 0) {
        simple_cache.line.valid = 0;
        simple_cache.init = 1;
    }

    CacheLine *cache_line = &simple_cache.line;

    address_tag = address >> 3; // Why do I do this?

    MemAddress = address >> 3; // again this....!
    MemAddress = MemAddress << 3; // address of the block in memory

    /* access Cache*/

    if (!(*cache_line).valid || (*cache_line).tag != address_tag) {         // if block not present - miss
        access_DRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

        if (((*cache_line).valid) && ((*cache_line).dirty)) { // line has dirty block
            MemAddress = (*cache_line).tag << 3;        // get address of the block in memory
            access_DRAM(MemAddress, &(cacheL1[0]), MODE_WRITE); // then write back old block
        }

        // copy new block to cache line
        memcpy(&(cacheL1[0]), TempBlock, BLOCK_SIZE);
        (*cache_line).valid = 1;
        (*cache_line).tag = address_tag;
        (*cache_line).dirty = 0;

    } // if miss, then replaced with the correct block

    if (mode == MODE_READ) {    // read data from cache line
        
        // even word on block
        if (0 == (address % 8)) { memcpy(data, &(cacheL1[0]), WORD_SIZE); }

        // odd word on block
        else { memcpy(data, &(cacheL1[WORD_SIZE]), WORD_SIZE); }
        
        time += L1_READ_TIME;
    }

    if (mode == MODE_WRITE) { // write data from cache line
        
        // even word on block
        if (!(address % 8)) { memcpy(&(cacheL1[0]), data, WORD_SIZE); }
        
        // odd word on block
        else { memcpy(&(cacheL1[WORD_SIZE]), data, WORD_SIZE); }
        
        time += L1_WRITE_TIME;
        (*cache_line).dirty = 1;
    }
}




/* - - - - - - - - - - interfaces - - - - - - - - - - */

void read(uint32_t address, uint8_t *data) { access_L1(address, data, MODE_READ); }

void write(uint32_t address, uint8_t *data) { access_L1(address, data, MODE_WRITE); }
