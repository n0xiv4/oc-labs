#include "L2Cache.h"

int main() {

  uint32_t value1, value2, clock;

  resetTime();
  initL1Cache();
  initL2Cache();
  value1 = 1;
  value2 = 2;

  write(0, (unsigned char *)(&value1));
  write(16384, (unsigned char *)(&value1));
  write(32768, (unsigned char *)(&value1));

  read(0, (unsigned char *)(&value1));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 0, value1, clock);

  read(16384, (unsigned char *)(&value2));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 16384, value2, clock);

  read(32768, (unsigned char *)(&value2));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 32768, value2, clock);

  return 0;

}
