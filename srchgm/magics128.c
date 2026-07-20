#include <stdio.h>

typedef unsigned long long int Bitboard;

Bitboard keyBits;
int keyStart;
int cnt;

int Bishop[] = { 13, -13, 11, -11, 0 };
int Rook[] = { 1, -1, 12, -12, 0 };
int Nightrider[] = { 23, 25, -23, -25, 14, 10, -14, -10, 0 };
int Camelrider[] = { 35, 37, -35, -37, 15, 9, -15, -9 };
int Zebrarider[] = { 34, 38, -34, -38, 27, 21, -27, -21 };

int popcnt(Bitboard b) {
  int res = 0;
  while(b) res += b & 1, b >>= 1;
  return res;
}

Bitboard MakeRay(int sqr, int steps, Bitboard *b_lo)
{
  Bitboard b = 0;
  int s = sqr + steps, next = s + steps;
  int dif = next % 12 - s % 12;
  int dif2 = s%  12 - sqr % 12;
  if(next < 0 || next >= 120 || dif > 4 || dif < -4 || dif2 > 4 || dif2 < -4) return 0; // terminal squares not in mask
  b = MakeRay(s, steps, b_lo);
  if(s >= 64) b |= 1LL << s; else *b_lo |= 1LL << s;
  return b;
}

Bitboard MakeMask(int sqr, int *steps, Bitboard *b_lo)
{
  Bitboard b = 0;
  *b_lo = 0;
  while(*steps) {
    b |= MakeRay(sqr, *steps++, b_lo);
  }
  return b;
}

// verify that all occupation patterns produce a unique index
int Verify(Bitboard mask, Bitboard mask_lo, Bitboard magic, Bitboard magic_lo, int epoch)
{
  static char table[4096];
  int i;
  Bitboard occupied = 0, occupied_lo = 0;
  do {
    int index = magic*occupied_lo + magic_lo*occupied >> (keyStart & 63);
    if(table[index] == epoch) return 1;
    table[index] = epoch;
    occupied_lo |= ~mask_lo;
    occupied_lo++;
    if(!occupied_lo) {
      occupied |= ~mask;
      occupied++;
      occupied &= mask;
    }
    occupied_lo &= mask_lo;
  } while(occupied || occupied_lo);
  return 0;
}

// recursively find a magic that provides 1:1 mapping of mask population to key 
//   key = bits placed so far     (starts at 0)
//   p = bits still to place      (starts at mask)
//   magic = maps the placed bits (starts at 0)
Bitboard Puzzle(Bitboard key, Bitboard p, Bitboard p_lo, Bitboard magic, Bitboard magic_lo, Bitboard *lo_result)
{

  int i, k;
  cnt++;
  for(k = 127; ((k >= 64 ? ~p : ~p_lo) & 1LL << (k & 63)); k--) {}                 // search highest set bit//
  printf("Puzzle(%016llx, %016llx%016llx, x%016llx%016llx)  %d\n", key, p, p_lo, magic, magic_lo, k);
  for(i = (k > keyStart ? k : keyStart); i < 128; i++) {// i = where we try to place it in the key
    if(!(key & 1LL << i & 63)) {                            // there is room there 
      int shift = i - k;                               // how much we have to shift the mask to get it there
      Bitboard newBits = (shift >= 64 ? p_lo << shift - 64 : p << shift | p_lo >> 64 - shift);
      Bitboard newKey = key + newBits;
      Bitboard newMagicH = magic, newMagicL = magic_lo;
      Bitboard newBitsL, erase;
printf("%d %d %d %016llx\n", shift, i, keyStart, newBits);
      if(key & newBits & keyBits) continue;            // collision: already placed key bits must not be disturbed
      if((newKey ^ key) & key & keyBits) continue;     // also not by carry
      erase = (newKey ^ key) & keyBits;                // remember bits that are now placed
printf("%016llx\n", erase);
      if(shift >= 64) newMagicH |= 1LL << shift - 64;
      else newMagicL  |= 1LL << shift;
printf("%016llx%016llx\n", newMagicH, newMagicL);
      if(newBits == erase) {          // nothing left to place: success!
         *lo_result = newMagicL;
         return newMagicH;
      }
      newBits = erase;
      if(shift >= 64) newBitsL = newBits >> shift - 64, newBits = 0;
      else newBitsL = newBits << 64 - shift, newBits >>= shift;
      newMagicH = Puzzle(newKey, p - newBits, p_lo - newBitsL, newMagicH, newMagicL, lo_result); // try placing remaining bits of mask
      if(newMagicH || newMagicL) {
        *lo_result = newMagicL;
        return newMagicH;      // success: return result
      }
if(cnt>20) break;
    }
  }
  *lo_result = 0;
  return 0; // fail
}

int main()
{
  int s;
  int size = 0;
  for(s=2; s<120; s++) {
    Bitboard res, res_lo = 0, mask_lo, magic_lo = 0;
    Bitboard mask = MakeMask(s, Bishop, &mask_lo);
    keyStart = 128 - popcnt(mask) - popcnt(mask_lo);
    size += 1 << 128 - keyStart;
    keyBits = ~0LL << keyStart;
    res = Puzzle(0, mask, mask_lo, 0, magic_lo, &magic_lo);
    if(Verify(mask, mask_lo, res, magic_lo, s + 1)) printf("collision\n");
    printf("%c%d: &%016llx%016llx, *%016llx%016llx, >>%d %d (%d calls)\n", s%12 + 'a', s/12 + 1, mask, mask_lo, res, magic_lo, keyStart, 128 - keyStart, cnt); fflush(stdout);
break;
  }
  printf("%d table elements (0x%08x, %d KB)\n", size, size, size*8/1024);
  return 0;
}