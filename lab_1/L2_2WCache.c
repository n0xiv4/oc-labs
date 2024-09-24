#include "L2_2WCache.h"

// Explanation below
#define L1_OFFSET_BITS 6
#define L1_INDEX_BITS 8

#define L2_2W_OFFSET_BITS 6
#define L2_2W_INDEX_BITS 8

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

  uint8_t set_line; // used to identify hit line of set
  uint32_t set_index, line_index, offset, Tag, MemAddress;
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

  Tag = address >> (L2_2W_OFFSET_BITS + L2_2W_INDEX_BITS);
  offset = address & ((1 << L2_2W_OFFSET_BITS) - 1);
  set_index = (address >> L2_2W_OFFSET_BITS) & ((1 << L2_2W_INDEX_BITS) - 1);

  /*
  We remove the offset bits so we get the exact place in memory
  where our block is located.
  */
  MemAddress = address >> L2_2W_OFFSET_BITS;
  MemAddress = MemAddress << L2_2W_OFFSET_BITS;

  /*
  Our set has two lines. We can create a simple formula to organize
  the lines in terms of indexes - 2*i if it's the first line of a set,
  and 2*i+1 if it's the second. This ensures the sets are encapsulated
  together.
  */
  CacheLine *FirstLine = &l2_cache.lines[2 * set_index];
  CacheLine *SecondLine = &l2_cache.lines[2 * set_index + 1];

  printf("actual tag -> %d\n", Tag);
  /*
  If we get a hit on any of the lines, we don't need to get
  anything from the DRAM, we can just read or write immediately.
  */
  if (FirstLine->Valid && FirstLine->Tag == Tag) {
    // Hit - first line
    set_line = 0;
  }
  else if (SecondLine->Valid && SecondLine->Tag == Tag) {
    // Hit - second line
    set_line = 1;
  }
  /*
  If we don't get a hit on any of the lines of the set,
  the miss is confirmed. We'll now find out in which line
  we can place our block.
  */
  else { 
    // Get block from the DRAM
    accessDRAM(MemAddress, TempBlock, MODE_READ);

    if (!FirstLine->Valid) {
      // Miss - first line is invalid, use it
      set_line = 0;
    } 
    else if (!SecondLine->Valid) {
      // Miss - second line is invalid, use it
      set_line = 1;
    }
    else {
      /*
      As both lines are valid, we can use the LRU policy to
      determine which of the lines is the one where we're
      replacing.
      */
      set_line = (FirstLine->Time < SecondLine->Time) ? 0:1;
    }

    /*
    We use the same formula I mentioned above to get the exact
    index of the line we're working with.
    */
    line_index = 2 * set_index + set_line;
    printf("line_index -> %d\n", line_index);
    CacheLine *Line = &l2_cache.lines[line_index];

  	if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      /*
      To reconstruct the memory address of our block, we use the tag of our line.
      First we shift left all the offset and index bits, and then we use the OR
      operator so the tag bits are kept intact and the index bits are introduced
      in the address.
      */
      MemAddress = Line->Tag << (L2_2W_OFFSET_BITS + L2_2W_INDEX_BITS);
      MemAddress = MemAddress | (line_index << L2_2W_OFFSET_BITS); 

      accessDRAM(MemAddress, &(L2Cache[line_index * BLOCK_SIZE]), MODE_WRITE);
    }

    memcpy(&(L2Cache[line_index * BLOCK_SIZE]), TempBlock, BLOCK_SIZE);
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  }
  
  // Define the pointer again as in the hit cases it wasn't defined yet
  CacheLine *Line = &l2_cache.lines[2 * set_index + set_line];
  line_index = 2 * set_index + set_line;
  printf("%d\n", line_index);

  if (mode == MODE_READ) {
    memcpy(data, &(L2Cache[line_index * BLOCK_SIZE + offset]), WORD_SIZE);
    Line->Time = getTime();
    time += L2_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(L2Cache[line_index * BLOCK_SIZE + offset]), data, WORD_SIZE);
    Line->Time = getTime();
    Line->Dirty = 1;
    time += L2_WRITE_TIME;
  }

}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);

  printf("L2[0] = %d\n", l2_cache.lines[0].Tag);
  printf("L2[0].valid = %d\n", l2_cache.lines[0].Valid);
  printf("L2[0].time = %d\n", l2_cache.lines[0].Time);
  printf("L2[1] = %d\n", l2_cache.lines[1].Tag);
  printf("L2[1].valid = %d\n", l2_cache.lines[1].Valid);
  printf("L2[1].time = %d\n", l2_cache.lines[1].Time);
  printf("\n");
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);

  printf("L2[0] = %d\n", l2_cache.lines[0].Tag);
  printf("L2[0].valid = %d\n", l2_cache.lines[0].Valid);
  printf("L2[0].time = %d\n", l2_cache.lines[0].Time);
  printf("L2[1] = %d\n", l2_cache.lines[1].Tag);
  printf("L2[1].valid = %d\n", l2_cache.lines[1].Valid);
  printf("L2[1].time = %d\n", l2_cache.lines[1].Time);
  printf("\n");
}
