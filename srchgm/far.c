#include <stdio.h>

int table[1024];

void main()
{
  int magic, occupancy;
  for(magic = 1; magic < 10000000; magic++) {
    for(occupancy = 0; occupancy < 1024; occupancy++) {
      int index = magic * occupancy >> 8 & 0x3FF;
      int target = occupancy & ~0xFF;
      if(occupancy & 0xFF) target = 10000;
      if(table[index] >= 0 && table[index] != target) break;
      table[index] = target;
    }
    if(occupancy == 1024) printf("magic = %x\n", magic);
    for(int i = 0; i < occupancy; i++) table[i] = -1;
  }
}