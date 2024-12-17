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
    DEBUG_PRINT("Read from DRAM at address %d. (+100t)\n", address);
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    DEBUG_PRINT("Wrote to DRAM at address %d. (+50t)\n", address);
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

  DEBUG_PRINT("Started trying to access tag %d in L1...\n", Tag);
  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    DEBUG_PRINT("Miss in L1!\n");
    accessL2(MemAddress, TempBlock, MODE_READ); // get new block from L2
    DEBUG_PRINT("Received TempBlock in L1 from L2. Checking if current block is dirty...\n");
    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      /*
      To reconstruct the memory address of our block, we use the tag of our line.
      First we shift left all the offset and index bits, and then we use the OR
      operator so the tag bits are kept intact and the index bits are introduced
      in the address.
      */
      DEBUG_PRINT("Started L1 Dirty process for tag %d...\n", Line->Tag);
      MemAddress = Line->Tag << (L1_OFFSET_BITS + L1_INDEX_BITS);
      MemAddress = MemAddress | (index << L1_OFFSET_BITS); 

      accessL2(MemAddress, &(L1Cache[index * BLOCK_SIZE]), MODE_WRITE); // then write back old block
      DEBUG_PRINT("L1 Dirty process end for tag %d.\n", Line->Tag);
    }
    memcpy(&(L1Cache[index * BLOCK_SIZE]), TempBlock, BLOCK_SIZE);
    DEBUG_PRINT("Replaced L1 block with L2 block for tag %d.\n", Tag);
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block
  else {
    DEBUG_PRINT("Hit in L1!\n");
  }

  if (mode == MODE_READ) {    // read data from cache line
    DEBUG_PRINT("Read from L1 with tag %d. (+1t)\n", Tag);
    memcpy(data, &(L1Cache[index * BLOCK_SIZE + offset]), WORD_SIZE);
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    DEBUG_PRINT("Wrote to L1 with tag %d. Flagged as dirty. (+1t)\n", Tag);
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

  DEBUG_PRINT("Started trying to access tag %d in L2...\n", Tag);
  /*
  If we get a hit on any of the lines, we don't need to get
  anything from the DRAM, we can just read or write immediately.
  */
  if (FirstLine->Valid && FirstLine->Tag == Tag) {
    // Hit - first line
    DEBUG_PRINT("Hit in L2! (First line of the set)\n");
    set_line = 0;
  }
  else if (SecondLine->Valid && SecondLine->Tag == Tag) {
    // Hit - second line
    DEBUG_PRINT("Hit in L2! (Second line of the set)\n");
    set_line = 1;
  }
  /*
  If we don't get a hit on any of the lines of the set,
  the miss is confirmed. We'll now find out in which line
  we can place our block.
  */
  else { 
    // Get block from the DRAM
    DEBUG_PRINT("Miss in L2!\n");
    accessDRAM(MemAddress, TempBlock, MODE_READ);
    DEBUG_PRINT("Received TempBlock in L2 from DRAM. Checking if current block is dirty...\n");

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
    CacheLine *Line = &l2_cache.lines[line_index];

  	if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      /*
      To reconstruct the memory address of our block, we use the tag of our line.
      First we shift left all the offset and index bits, and then we use the OR
      operator so the tag bits are kept intact and the index bits are introduced
      in the address.
      */
      DEBUG_PRINT("Started L2 Dirty process for tag %d...\n", Line->Tag);
      MemAddress = Line->Tag << (L2_2W_OFFSET_BITS + L2_2W_INDEX_BITS);
      MemAddress = MemAddress | (line_index << L2_2W_OFFSET_BITS); 

      accessDRAM(MemAddress, &(L2Cache[line_index * BLOCK_SIZE]), MODE_WRITE);
      DEBUG_PRINT("L2 Dirty process ended for tag %d.\n", Line->Tag);
    }

    memcpy(&(L2Cache[line_index * BLOCK_SIZE]), TempBlock, BLOCK_SIZE);
    DEBUG_PRINT("Replaced L2 block with memory block for tag %d.\n", Tag);
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Dirty = 0;
  }
  
  // Define the pointer again as in the hit cases it wasn't defined yet
  line_index = 2 * set_index + set_line;
  CacheLine *Line = &l2_cache.lines[line_index];

  if (mode == MODE_READ) {
    DEBUG_PRINT("Read from L2 with tag %d. (+10t)\n", Tag);
    Line->Time = getTime();
    memcpy(data, &(L2Cache[line_index * BLOCK_SIZE + offset]), WORD_SIZE);
    time += L2_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    DEBUG_PRINT("Wrote to L2 with tag %d. Flagged as dirty. (+5t)\n", Tag);
    Line->Time = getTime();
    memcpy(&(L2Cache[line_index * BLOCK_SIZE + offset]), data, WORD_SIZE);
    Line->Dirty = 1;
    time += L2_WRITE_TIME;
  }

}

void read(uint32_t address, uint8_t *data) {
  DEBUG_PRINT("\nReading process started for address %d...\n", address);
  accessL1(address, data, MODE_READ);
  DEBUG_PRINT("Ended reading process for address %d at time %d.\n\n", address, getTime());
}

void write(uint32_t address, uint8_t *data) {
  DEBUG_PRINT("\nWriting process started for address %d...\n", address);
  accessL1(address, data, MODE_WRITE);
  DEBUG_PRINT("Ended writing process for address %d at time %d.\n\n", address, getTime());
}
