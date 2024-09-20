#include "L1Cache.h"

int main() {

  uint32_t value1, value2, value3, clock;

  resetTime();
  initCache();
  value1 = -1;
  value2 = 9342;
  value3 = 43;

  write(1, (uint8_t *)(&value1));

  clock = getTime();
  printf("Time: %d\n", clock);

  read(1, (uint8_t *)(&value2));
  clock = getTime();
  printf("Time: %d\n", clock);

  write(512, (uint8_t *)(&value1));
  clock = getTime();
  printf("Time: %d\n", clock);

  read(512, (uint8_t *)(&value2));
  clock = getTime();
  printf("Time: %d\n", clock);

  read(5923, (uint8_t *)(&value3));
  clock = getTime();
  printf("Time: %d\n", clock);

  return 0;
}
