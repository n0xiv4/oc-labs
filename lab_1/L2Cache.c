#include "L2Cache.h"

// Explanation below
#define L1_OFFSET_BITS 6
#define L1_INDEX_BITS 8

#define L2_OFFSET_BITS 6
#define L2_INDEX_BITS 9

uint8_t L1Cache[L1_SIZE];
uint8_t L2Cache[L2_SIZE];
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
CacheL1 l1_cache;
CacheL2 l2_cache;

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

void initL1Cache() { l1_cache.init = 0; }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, offset, Tag, MemAddress;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (l1_cache.init == 0) {
    l1_cache.init = 1;
    /*
    Now that our cache has several lines, we have to init each line.
    We set all lines as invalid and as clean.
    */
    for (int i = 0; i < (L1_SIZE / BLOCK_SIZE); i++) {
      l1_cache.lines[i].Valid = 0;
      l1_cache.lines[i].Dirty = 0;
      l1_cache.lines[i].Tag = 0;
    }
  }

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

  /*
  printf("memory address -> %p\n", address);
  printf("tag -> %p\n", Tag);
  printf("offset -> %p\n", offset);
  printf("index -> %p\n", index);
  */

  /*
  We remove the offset bits so we get the exact place in memory
  where our block is located.
  */
  MemAddress = address >> L1_OFFSET_BITS;
  MemAddress = MemAddress << L1_OFFSET_BITS;

  /*
  Now we have several lines, so we must locate the matching line,
  using the index.
  */
  CacheLine *Line = &l1_cache.lines[index];

  /* access Cache */

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessL2(MemAddress, TempBlock, MODE_READ); // get new block from L2

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      /*
      To reconstruct the memory address of our block, we use the tag of our line.
      First we shift left all the offset and index bits, and then we use the OR
      operator so the tag bits are kept intact and the index bits are introduced
      in the address.
      */
      MemAddress = Line->Tag << (L1_OFFSET_BITS + L1_INDEX_BITS);
      MemAddress = MemAddress | (index << L1_OFFSET_BITS); 

      accessL2(MemAddress, &(L1Cache[index * BLOCK_SIZE]), MODE_WRITE); // then write back old block
    }

    memcpy(&(L1Cache[index * BLOCK_SIZE]), TempBlock, BLOCK_SIZE);
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block


  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(L1Cache[index * BLOCK_SIZE + offset]), WORD_SIZE);
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(L1Cache[index * BLOCK_SIZE + offset]), data, WORD_SIZE);
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

/*********************** L2 cache *************************/

void initL2Cache() { l2_cache.init = 0; }

void accessL2(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, offset, Tag, MemAddress;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (l2_cache.init == 0) {
    l2_cache.init = 1;
    for (int i = 0; i < (L2_SIZE / BLOCK_SIZE); i++) {
      l2_cache.lines[i].Valid = 0;
      l2_cache.lines[i].Dirty = 0;
      l2_cache.lines[i].Tag = 0;
    }
  }

  /* 
  Our cache L2 has cache lines with 16 * WORD_SIZE size, so our offset
  shall be able to have enough bits to address 16 words. 2^6 = 64 bytes
  (16 words * 4 bytes), so we'll use 6 bits for the offset.
  
  As for the index, 512 lines in L2 mean that we need 2^9 = 512 indexes,
  so, that means we'll use 9 bits for the index.
  */

  Tag = address >> (L2_OFFSET_BITS + L2_INDEX_BITS);
  offset = address & ((1 << L2_OFFSET_BITS) - 1);
  index = (address >> L2_OFFSET_BITS) & ((1 << L2_INDEX_BITS) - 1);

  /*
  We remove the offset bits so we get the exact place in memory
  where our block is located.
  */
  MemAddress = address >> L2_OFFSET_BITS;
  MemAddress = MemAddress << L2_OFFSET_BITS;

  /*
  Now we have several lines, so we must locate the matching line,
  using the index.
  */
  CacheLine *Line = &l2_cache.lines[index];

  /* access Cache */

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      /*
      To reconstruct the memory address of our block, we use the tag of our line.
      First we shift left all the offset and index bits, and then we use the OR
      operator so the tag bits are kept intact and the index bits are introduced
      in the address.
      */
      MemAddress = Line->Tag << (L2_OFFSET_BITS + L2_INDEX_BITS);
      MemAddress = MemAddress | (index << L2_OFFSET_BITS); 

      accessDRAM(MemAddress, &(L2Cache[index * BLOCK_SIZE]), MODE_WRITE); // then write back old block
    }

    memcpy(&(L2Cache[index * BLOCK_SIZE]), TempBlock, BLOCK_SIZE);
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block


  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(L2Cache[index * BLOCK_SIZE + offset]), WORD_SIZE);
    time += L2_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(L2Cache[index * BLOCK_SIZE + offset]), data, WORD_SIZE);
    time += L2_WRITE_TIME;
    Line->Dirty = 1;
  }
}


void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
