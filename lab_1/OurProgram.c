#include "L2_2WCache.h"

int main() {

  uint32_t value1, value2, clock;

  resetTime();
  initL1Cache();
  initL2Cache();
  value1 = 1;
  value2 = 2;

  clock = getTime();
  printf("Start Time %d\n", clock);

  write(0, (unsigned char *)(&value1));
  clock = getTime();
  printf("Wrote; Address %d; Value %d; Time %d\n", 0, value1, clock);

  write(16384, (unsigned char *)(&value1));
  clock = getTime();
  printf("Wrote; Address %d; Value %d; Time %d\n", 16384, value1, clock);

  write(32768, (unsigned char *)(&value1));
  clock = getTime();
  printf("Wrote; Address %d; Value %d; Time %d\n", 32768, value1, clock);

  read(0, (unsigned char *)(&value2));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 0, value2, clock);

  read(16384, (unsigned char *)(&value2));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 16384, value2, clock);

  read(32768, (unsigned char *)(&value2));
  clock = getTime();
  printf("Read; Address %d; Value %d; Time %d\n", 32768, value2, clock);

  return 0;
}
