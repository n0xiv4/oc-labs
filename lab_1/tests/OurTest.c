// Place in same dir as L2_2WCache.h
#include "L2_2WCache.h"

int main() {

  uint32_t value1, value2, value3, value4, clock;

  resetTime();
  initL1Cache();
  initL2Cache();
  value1 = 16;
  value2 = 32;
  value3 = 64;
  value4 = 0;

  clock = getTime();
  printf("Start Time %d\n", clock);

  // Writes to address 0x0000 (tag 0, index 0, offset 0)
  write(0, (unsigned char *)(&value1));
  clock = getTime();
  printf("Wrote; Address %d; Value %d; Time %d\n", 0, value1, clock);

  // Writes to address 0x4000 to introduce a second line in the L2 set
  // Replaces, naturally, the address 0 in L1 (same index)
  write(16384, (unsigned char *)(&value2));
  clock = getTime();
  printf("Wrote; Address %d; Value %d; Time %d\n", 16384, value2, clock);

  // Writes to address 0x8000 to replace one of the lines in the L2 set
  write(32768, (unsigned char *)(&value3));
  clock = getTime();
  printf("Wrote; Address %d; Value %d; Time %d\n", 32768, value3, clock);

  // Reads from 0x0000
  read(0, (unsigned char *)(&value4));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 0, value4, clock);

  // Reads from 0x4000
  read(16384, (unsigned char *)(&value4));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 16384, value4, clock);

  // Reads from 0x8000
  read(32768, (unsigned char *)(&value4));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 32768, value4, clock);

  // Reads from 0x8000
  read(32768, (unsigned char *)(&value4));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 32768, value4, clock);

  return 0;
}
