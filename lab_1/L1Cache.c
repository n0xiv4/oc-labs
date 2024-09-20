#include "SimpleCache.h"

// Explanation below
#define L1_OFFSET_BITS 6
#define L1_INDEX_BITS 8

uint8_t L1Cache[L1_SIZE];
uint8_t L2Cache[L2_SIZE];
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache SimpleCache;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}

/*********************** L1 cache *************************/

void initCache() { SimpleCache.init = 0; }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, offset, Tag, MemAddress;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (SimpleCache.init == 0) {
    SimpleCache.line.Valid = 0;
    SimpleCache.init = 1;
  }

  CacheLine *Line = &SimpleCache.line;

  /* 
  Our cache L1 has cache lines with 16 * WORD_SIZE size, so our offset
  shall be able to have enough bits to address 16 words. 2^6 = 64 bytes
  (16 words * 4 bytes), so we'll use 6 bits for the offset.
  
  As for the index, 256 lines in L1 mean that we need 2^8 = 256 indexes,
  so, that means we'll use 8 bits for the index.
  */

  Tag = address >> (L1_OFFSET_BITS + L1_INDEX_BITS);
  offset = address & ((1 << L1_OFFSET_BITS) - 1);
  index = (address >> L1_OFFSET_BITS) & ((1 << L1_INDEX_BITS) - 1);

  printf("memory address -> %p\n", address);
  printf("tag -> %p\n", Tag);
  printf("offset -> %p\n", offset);
  printf("index -> %p\n", index);

  /*
  As we have 16 words in each block, we must remove the last 4 bits
  from our address so we find the adequate place in memory for the
  current address. 2^6=64, 2^4=16
  */

  MemAddress = address >> 4;
  MemAddress = MemAddress << 4;

  /* access Cache*/

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      MemAddress = Line->Tag << 3;        // get address of the block in memory
      accessDRAM(MemAddress, &(L1Cache[0]), MODE_WRITE); // then write back old block
    }

    memcpy(&(L1Cache[0]), TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line
    if (0 == (address % 8)) { // even word on block
      memcpy(data, &(L1Cache[0]), WORD_SIZE);
    } else { // odd word on block
      memcpy(data, &(L1Cache[WORD_SIZE]), WORD_SIZE);
    }
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    if (!(address % 8)) {   // even word on block
      memcpy(&(L1Cache[0]), data, WORD_SIZE);
    } else { // odd word on block
      memcpy(&(L1Cache[WORD_SIZE]), data, WORD_SIZE);
    }
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
