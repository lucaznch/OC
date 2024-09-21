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
		memcpy(data, &(main_memory[address]), BLOCK_SIZE);
		time += DRAM_READ_TIME;
	}
	else {
		memcpy(&(main_memory[address]), data, BLOCK_SIZE);		// only when the block in L1 is getting replaced
		time += DRAM_WRITE_TIME;
	}
}

/// @brief simulates an access to the L1 cache with write-back policy
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

	if (offset % 4 != 0) { while (offset % 4 != 0) { offset--; } }

	cache_L1.mem_access_count++;

	// HIT
	if (cache_L1.lines[index].valid && cache_L1.lines[index].tag == tag) {
		if (mode == READ_MODE) {
			memcpy(data, &(cache_L1.lines[index].data[offset]), WORD_SIZE);
			time += L1_READ_TIME;
			//printf("R - cache hit.\taddress: %u\tdata read: %u\ttime: %u\n", address, *data, time);
		}
		else {
			if (cache_L1.lines[index].dirty) {
				// if this line is dirty, i.e. it was written in L1 and not in RAM
				// we need to write in RAM since this block is getting replaced
			}		
			memcpy(&(cache_L1.lines[index].data[offset]), data, WORD_SIZE);
			cache_L1.lines[index].dirty = 1;
			time += L1_WRITE_TIME;
			//printf("W - cache hit.\taddress: %u\tdata written: %u\ttime: %u\n", address, *data, time);
		}
		cache_L1.hits++;
	}

	// MISS
	// the data block is necessarily copied from main memory to L1
	// note that in write mode it doesn't write in the RAM and only in L1
	// only when that block is getting replaced do we write to RAM
	else {
		uint8_t temp[BLOCK_SIZE];

		cache_L1.lines[index].valid = 1;
		cache_L1.lines[index].dirty = 0;
		cache_L1.lines[index].tag = tag;

		access_main_memory(address, temp, READ_MODE);
		memcpy(&(cache_L1.lines[index].data[0]), temp, BLOCK_SIZE);		// copy the whole block


		if (mode == READ_MODE) {
			memcpy(data, &(cache_L1.lines[index].data[offset]), WORD_SIZE);
			time += L1_READ_TIME;
			//printf("R - cache miss.\taddress: %u\tdata read: %u\ttime: %u\n", address, *data, time);
		}
		else {
			memcpy(&(cache_L1.lines[index].data[offset]), data, WORD_SIZE);
			cache_L1.lines[index].dirty = 1;
			time += L1_WRITE_TIME;
			//printf("W - cache miss.\taddress: %u\tdata written: %u\ttime: %u\n", address, *data, time);
		}
		cache_L1.misses--;
	}
}


int main() {

/*
int32_t addr1;
uint8_t dt1;

memset(main_memory, 0, DRAM_SIZE);
init_cache();
reset_time();

addr1 = 0;
dt1 = 0;
access_cache_L1(addr1, &dt1, WRITE_MODE);

addr1 = 4;
dt1 = 4;
access_cache_L1(addr1, &dt1, WRITE_MODE);

addr1 = 8;
dt1 = 8;
access_cache_L1(addr1, &dt1, WRITE_MODE);

addr1 = 12;
dt1 = 12;
access_cache_L1(addr1, &dt1, WRITE_MODE);

addr1 = 0;
dt1 = 0;
access_cache_L1(addr1, &dt1, READ_MODE);

addr1 = 4;
dt1 = 4;
access_cache_L1(addr1, &dt1, READ_MODE);

addr1 = 8;
dt1 = 8;
access_cache_L1(addr1, &dt1, READ_MODE);

addr1 = 12;
dt1 = 12;
access_cache_L1(addr1, &dt1, READ_MODE);
*/


	int clock1, value;

	memset(main_memory, 0, DRAM_SIZE);

	for(int n = 1; n <= DRAM_SIZE/4; n*=WORD_SIZE) {

		reset_time();
		init_cache();

		printf("\nNumber of words: %d\n", (n-1)/WORD_SIZE + 1);

		for(int i = 0; i < n; i+=WORD_SIZE) {
			access_cache_L1(i, (unsigned char *)(&i), WRITE_MODE);
			clock1 = get_time();
			printf("Write; Address %d; Value %d; Time %d\n", i, i, clock1);
		}

		for(int i = 0; i < n; i+=WORD_SIZE) {
			access_cache_L1(i, (unsigned char *)(&value), READ_MODE);
			clock1 = get_time();
			printf("Read; Address %d; Value %d; Time %d\n", i, value, clock1);
		}  

	}


	return 0;

}