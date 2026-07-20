#include <stdio.h>

typedef unsigned long long int Bitboard;

Bitboard keyBits;
int keyStart;
int cnt;

int Bishop[] = { 15, 17, -15, -17, 0 };
int Rook[] = { 1, -1, 16, -16, 0 };

int popcnt(Bitboard b) {
  int res = 0;
  while(b) res += b & 1, b >>= 1;
  return res;
}

Bitboard MakeRay(int sqr, int *steps)
{
  Bitboard b;
  sqr += *steps;
  if(sqr + *steps & 0x88) return 0; // terminal squares not in mask
  b = MakeRay(sqr, steps);
  sqr = (sqr & 7) + sqr >> 1; // 0x88 to nr conversion
  return b | 1LL << sqr;
}

Bitboard MakeMask(int sqr, int *steps)
{
  Bitboard b = 0;
  while(*steps) {
    b |= MakeRay(sqr, steps++);
  }
  return b;
}

// verify that all occupation patterns produce a unique index
int Verify(Bitboard mask, Bitboard magic, int epoch)
{
  static char table[4096];
  int i;
  Bitboard occupied = 0;
  do {
    int index = magic*occupied >> keyStart;
    if(table[index] == epoch) return 1;
    table[index] = epoch;
    occupied |= ~mask;
    occupied++;
    occupied &= mask;
  } while(occupied);
  return 0;
}

// recursively find a magic that provides 1:1 mapping of mask population to key 
//   key = bits placed so far     (starts at 0)
//   p = bits still to place      (starts at mask)
//   magic = maps the placed bits (starts at 0)
Bitboard Puzzle(Bitboard key, Bitboard p, Bitboard magic)
{
  int i, k;
  cnt++;
  for(k = 63; (~p & 1LL << k); k--) {}                 // search highest set bit
//  printf("Puzzle(%016llx, %016llx, %016llx)  %d %016llx\n", key, p, magic, k, 1LL << k);
  for(i = (k > keyStart ? k : keyStart); i < 64; i++) {                     // i = where we try to place it in the key
    if(!(key & 1LL << i)) {                            // there is room there 
      int shift = i - k;                               // how much we have to shift the mask to get it there
      Bitboard newBits = p << shift;
      Bitboard newKey = key + newBits;
      Bitboard newMagic;
      if(key & newBits & keyBits) continue;            // collision: already placed key bits must not be disturbed
      if((newKey ^ key) & key & keyBits) continue;     // also not by carry
      newBits -= (newKey ^ key) & keyBits;             // erase bits that are now placed
      newMagic = magic | 1LL << shift;
      if(!newBits) return newMagic;                    // nothing left to place: success!
      newMagic = Puzzle(newKey, newBits >> shift, newMagic); // try placing remaining bits of mask
      if(newMagic) return newMagic;                    // success: return result
    }
  }
  return 0; // fail
}

int main()
{
  int s;
  int size = 0;
  for(s=0; s<64; s++) {
    Bitboard res;
    Bitboard mask = MakeMask(s + (s & 070), Bishop);
//printf("%016llx\n", mask);
    keyStart = 64 - popcnt(mask);
    size += 1 << 64 - keyStart;
    keyBits = ~0LL << keyStart;
    res = Puzzle(0, mask, 0);
//printf("%016llx\n", res);
    if(Verify(mask, res, s + 1)) printf("collision\n");
    printf("%02o: mask = %016llx, magic = %016llx shift = %d in %d attempts\n", s, mask, res, keyStart, cnt); fflush(stdout);
  }
  printf("%d table elements (%d KB)\n", size, size*8/1024);
  return 0;
}