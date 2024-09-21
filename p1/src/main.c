#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define WORD_SIZE 4						// 4 Bytes = 32 bits

#define BLOCK_SIZE (16 * WORD_SIZE)		// 16 words = 16 * 4B = 64B
										// 2**4 words = (2**4+2)B
										// 4 bits for word offset (within the block)
										// 2 bits for byte offset (within the word)
										// => 6 bits for offset
										// or, log_2_(block_size) = log_2_(64) = 6

#define L1_SIZE (256 * BLOCK_SIZE)		// 256 blocks = (256 * 64)B = 16KiB
										// 2**8 blocks => 8 bits for index
										// or, log_2_(numer_of_lines_L1) = log_2_(256) = 8

#define L2_SIZE (512 * BLOCK_SIZE)		// 512 blocks = (512 * 64)B = 32KiB
										// 2**9 blocks => 9 bits for index

#define DRAM_SIZE (1024 * BLOCK_SIZE)	// 1024 blocks = (1024 * 64)B = 64KiB

#define READ_MODE 1
#define WRITE_MODE 2

#define DRAM_READ_TIME 100
#define DRAM_WRITE_TIME 50

#define L1_READ_TIME 1
#define L1_WRITE_TIME 1

#define NUM_OF_LINES_L1 256
#define ADDRESS_WIDTH 32

typedef struct cache_line {
	uint8_t valid;
	uint8_t dirty;
	uint32_t tag;
	uint8_t data[BLOCK_SIZE];			// data from one line: 16 words = 64B
} CacheLine;

typedef struct cache {
	uint32_t offset_width;
	uint32_t index_width;
	uint32_t tag_width;
	uint32_t hits;
	uint32_t misses;
	uint64_t mem_access_count;
	CacheLine lines[NUM_OF_LINES_L1];	// cache_L1: 256 lines
} Cache;


Cache cache_L1;
uint8_t main_memory[DRAM_SIZE];
uint32_t time;


uint32_t binary_log(uint32_t n) {
    uint32_t log = 0;

	if (n == 0) { return 1; }
    while (n > 1) {
        n >>= 1;	// divide by 2
        log++;
    }
    return log;
}



void reset_time() { time = 0; }
uint32_t get_time() { return time; }

void init_cache() {
	int i;

	cache_L1.offset_width = binary_log(BLOCK_SIZE);
	cache_L1.index_width = binary_log(NUM_OF_LINES_L1);
	cache_L1.tag_width = ADDRESS_WIDTH - cache_L1.offset_width - cache_L1.index_width; 
	cache_L1.hits = 0;
	cache_L1.misses = 0;
	cache_L1.mem_access_count = 0;

	for (i = 0; i < NUM_OF_LINES_L1; i++) {
		cache_L1.lines[i].valid = 0;
		cache_L1.lines[i].dirty = 0;
		cache_L1.lines[i].tag = 0;
		memset(cache_L1.lines[i].data, 0, BLOCK_SIZE);
	}
}

void access_main_memory(uint32_t address, uint8_t *data, int mode) {
	if (mode == READ_MODE) {
		*data = main_memory[address];
		time += DRAM_READ_TIME;
	}
	else {
		main_memory[address] = *data;
		time += DRAM_WRITE_TIME;
	}
}

/// @brief simulates an access to the L1 cache
/// @param address address to read/write data from
/// @param data data read/written at address
/// @param mode memory access mode (read or write)
void access_cache_L1(uint32_t address, uint8_t *data, int mode) {
	uint32_t tag, index, offset;

	if (data != NULL && mode == 0) { return; }

	offset = address << (cache_L1.tag_width + cache_L1.index_width);
	offset = offset >> (cache_L1.tag_width + cache_L1.index_width);
	index = address << cache_L1.tag_width;
	index = index >> (cache_L1.tag_width + cache_L1.offset_width);
	tag = address >> (cache_L1.offset_width + cache_L1.index_width);

	cache_L1.mem_access_count++;

	// HIT
	if (cache_L1.lines[index].valid && cache_L1.lines[index].tag == tag) {
		if (mode == READ_MODE) {
			*data = cache_L1.lines[index].data[offset];
			time += L1_READ_TIME;
			// printf("R - cache hit.\taddress: %u\tdata read: %u\ttime: %u\n", address, *data, time);
		}
		else {
			// check if is dirty !
			if (cache_L1.lines[index].dirty) {
				access_main_memory(address, data, mode);
			}
			cache_L1.lines[index].data[offset] = *data;
			cache_L1.lines[index].dirty = 1;
			time += L1_WRITE_TIME;
			//printf("W - cache hit.\taddress: %u\tdata written: %u\ttime: %u\n", address, *data, time);
		}
		cache_L1.hits++;
	}

	// MISS
	else {
		cache_L1.lines[index].valid = 1;
		cache_L1.lines[index].tag = tag;

		if (mode == READ_MODE) {
			uint8_t temp;

			access_main_memory(address, &temp, mode);		// read data from main memory
			cache_L1.lines[index].data[offset] = temp;		// update cache
			*data = temp;
			time += L1_READ_TIME;
			//printf("R - cache miss.\taddress: %u\tdata read: %u\ttime: %u\n", address, *data, time);
		}
		else {
			cache_L1.lines[index].data[offset] = *data;		// update cache
			cache_L1.lines[index].dirty = 1;
			time += L1_WRITE_TIME;
			//printf("W - cache miss.\taddress: %u\tdata written: %u\ttime: %u\n", address, *data, time);
		}
		cache_L1.misses--;
	}
}


int main() {
	return 0;
}
