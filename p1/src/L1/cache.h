#ifndef CACHE_H
#define CACHE_H




/* - - - - - - - - - - - - - - - - - sizes - - - - - - - - - - - - - - - - - */

#define WORD_SIZE 4                     // 4B = 32b
#define BLOCK_SIZE (16 * WORD_SIZE)     // 16 words = 16 x 4B = 64B

#define DRAM_SIZE (1024 * BLOCK_SIZE)   // 1024 blocks = 1024 x 64B = 64KiB

#define L1_SIZE (256 * BLOCK_SIZE)      // 256 blocks = 256 x 64B = 16KiB
#define L2_SIZE (512 * BLOCK_SIZE)      // 512 blocks = 512 x 64B = 32Kib




/* - - - - - - - - - - - - - - - - access mode - - - - - - - - - - - - - - - - */

#define MODE_READ 1
#define MODE_WRITE 0

#define DRAM_READ_TIME 100
#define DRAM_WRITE_TIME 50
#define L2_READ_TIME 10
#define L2_WRITE_TIME 5
#define L1_READ_TIME 1
#define L1_WRITE_TIME 1

#endif