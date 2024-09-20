#include "L1Cache.h"

int main() {

  uint32_t value1, value2, clock;

  resetTime();
  initCache();
  value1 = -1;
  value2 = 2;

  // 0x81 -> 129
  write(129, (uint8_t *)(&value1));

  clock = getTime();
  printf("Time: %d\n", clock);

  read(129, (uint8_t *)(&value2));
  clock = getTime();
  printf("Time: %d\n", clock);

  // 0x95C9 -> 38345
  write(38345, (uint8_t *)(&value1));
  clock = getTime();
  printf("Time: %d\n", clock);

  read(38345, (uint8_t *)(&value2));
  clock = getTime();
  printf("Time: %d\n", clock);

  return 0;
}
