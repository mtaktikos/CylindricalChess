/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2022 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <bitset>

#include "bitboard.h"
#include "magic.h"
#include "misc.h"
#include "piece.h"

namespace Stockfish {

uint8_t PopCnt16[1 << 16];
uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard SquareBB[SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard PseudoAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard PseudoMoves[2][COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard LeaperAttacks[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard LeaperMoves[2][COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
Bitboard BoardSizeBB[FILE_NB][RANK_NB];
RiderType AttackRiderTypes[PIECE_TYPE_NB];
RiderType MoveRiderTypes[2][PIECE_TYPE_NB];
PieceSet nonSlidingTypes;

Magic RookMagicsH[SQUARE_NB];
Magic RookMagicsV[SQUARE_NB];
Magic BishopMagics[SQUARE_NB];
Magic CannonMagicsH[SQUARE_NB];
Magic CannonMagicsV[SQUARE_NB];
Magic LameDabbabaMagics[SQUARE_NB];
Magic HorseMagics[SQUARE_NB];
Magic ElephantMagics[SQUARE_NB];
Magic JanggiElephantMagics[SQUARE_NB];
Magic CannonDiagMagics[SQUARE_NB];
Magic NightriderMagics[SQUARE_NB];
Magic CamelriderMagics[SQUARE_NB];
Magic ZebrariderMagics[SQUARE_NB];
Magic GrasshopperMagicsH[SQUARE_NB];
Magic GrasshopperMagicsV[SQUARE_NB];
Magic GrasshopperMagicsD[SQUARE_NB];
Magic RookMagicsHT[SQUARE_NB];
Magic RookMagicsHB[SQUARE_NB];
Magic RookMagicsVL[SQUARE_NB];
Magic RookMagicsVR[SQUARE_NB];
Magic DiagMagicsDT[SQUARE_NB];
Magic DiagMagicsDB[SQUARE_NB];
Magic DiagMagicsAT[SQUARE_NB];
Magic DiagMagicsAB[SQUARE_NB];
Magic CustomMagics[12][SQUARE_NB];

Magic* magics[32] = {BishopMagics, RookMagicsH, RookMagicsV, CannonMagicsH, CannonMagicsV,
                   LameDabbabaMagics, HorseMagics, ElephantMagics, JanggiElephantMagics, CannonDiagMagics, NightriderMagics,
                   GrasshopperMagicsH, GrasshopperMagicsV, GrasshopperMagicsD, CamelriderMagics, ZebrariderMagics,
                   RookMagicsHT, RookMagicsHB, RookMagicsVL, RookMagicsVR,
                   DiagMagicsDT, DiagMagicsDB, DiagMagicsAT, DiagMagicsAB };

RiderType nonSlidingRides;
RiderType asymmetricalRides;
int nrOfRides;
int riderBaseType[32];
int riderRangeMask[32];

namespace {

// Some magics need to be split in order to reduce memory consumption.
// Otherwise on a 12x10 board they can be >100 MB.
#ifdef LARGEBOARDS
  Bitboard RookTableH[0x11800];  // To store horizontal rook attacks
  Bitboard RookTableV[0x4800];  // To store vertical rook attacks
  Bitboard BishopTable[0x33C00]; // To store bishop attacks
  Bitboard CannonTableH[0x11800];  // To store horizontal cannon attacks
  Bitboard CannonTableV[0x4800];  // To store vertical cannon attacks
  Bitboard LameDabbabaTable[0x500];  // To store lame dabbaba attacks
  Bitboard HorseTable[0x500];  // To store horse attacks
  Bitboard ElephantTable[0x400];  // To store elephant attacks
  Bitboard JanggiElephantTable[0x1C000];  // To store janggi elephant attacks
  Bitboard CannonDiagTable[0x33C00]; // To store diagonal cannon attacks
  Bitboard NightriderTable[0xD200]; // To store nightrider attacks
  Bitboard CamelriderTable[0x8D0]; // To store nightrider attacks
  Bitboard ZebrariderTable[0x3D0]; // To store nightrider attacks
  Bitboard GrasshopperTableH[0x11800];  // To store horizontal grasshopper attacks
  Bitboard GrasshopperTableV[0x4800];  // To store vertical grasshopper attacks
  Bitboard GrasshopperTableD[0x33C00]; // To store diagonal grasshopper attacks
  Bitboard DiagTableD[0x3900]; // To store diagonal grasshopper attacks
  Bitboard DiagTableA[0x3900]; // To store diagonal grasshopper attacks
#else
  Bitboard RookTableH[0xA00];  // To store horizontal rook attacks
  Bitboard RookTableV[0xA00];  // To store vertical rook attacks
  Bitboard BishopTable[0x1480]; // To store bishop attacks
  Bitboard CannonTableH[0xA00];  // To store horizontal cannon attacks
  Bitboard CannonTableV[0xA00];  // To store vertical cannon attacks
  Bitboard LameDabbabaTable[0x240];  // To store lame dabbaba attacks
  Bitboard HorseTable[0x240];  // To store horse attacks
  Bitboard ElephantTable[0x1A0];  // To store elephant attacks
  Bitboard JanggiElephantTable[0x5C00];  // To store janggi elephant attacks
  Bitboard CannonDiagTable[0x1480]; // To store diagonal cannon attacks
  Bitboard NightriderTable[0x500]; // To store nightrider attacks
  Bitboard CamelriderTable[0xD0]; // To store camelrider attacks
  Bitboard ZebrariderTable[0x90]; // To store zebrarider attacks
  Bitboard GrasshopperTableH[0xA00];  // To store horizontal grasshopper attacks
  Bitboard GrasshopperTableV[0xA00];  // To store vertical grasshopper attacks
  Bitboard GrasshopperTableD[0x1480]; // To store diagonal grasshopper attacks
  Bitboard DiagTableD[0x640]; // To store diagonal grasshopper attacks
  Bitboard DiagTableA[0x640]; // To store diagonal grasshopper attacks
#endif

  // Rider directions
  const std::map<DirectionCode, int> RookDirectionsV { {step(1, 0), 0}, {step(-1, 0), 0}};
  const std::map<DirectionCode, int> RookDirectionsH { {step(0, 1), 0}, {step(0, -1), 0} };
  const std::map<DirectionCode, int> BishopDirections { {step(1, 1), 0}, {step(-1, 1), 0}, {step(-1, -1), 0}, {step(1, -1), 0} };

  const std::map<DirectionCode, int> allDirections[] { BishopDirections, RookDirectionsH, RookDirectionsV };

  enum MovementType { RIDER, HOPPER, LAME_LEAPER, HOPPER_RANGE };

  template <MovementType MT>
#ifdef PRECOMPUTED_MAGICS
  Bitboard* init_magics(Bitboard table[], Magic magics[], std::string betza, const Bitboard magicsInit[]);
#else
  Bitboard* init_magics(Bitboard table[], Magic magics[], std::string betza);
#endif

  template <MovementType MT>
  Bitboard one_ride(DirectionCode v, int limit, Square sq, Bitboard occupied, Color c, bool mask) {

        bool hurdle = false;
        Direction d = board_step(v);
        Direction x = h_step(v);
        Direction y = v_step(v);
        int lim = limit;
        Square ss = sq;
        Bitboard attack = 0;

        if(c != WHITE) d = -d, x = -x, y = -y;

        if(std::abs(x) == BENT) { // bent slider
//if(mask) sync_cout << y << "," << x << sync_endl;
            if(std::abs(y) < 4) {
                d = x/BENT; ss += y*FILE_NB;
            } else {
                int a = std::abs(y);
                y = y * FILE_NB / a; x /= BENT; // unit board steps in requested direction
                d = x + y; ss += x * (BENT - a) - d;
//sync_cout << "A: ss=" << ss << " d=" << d << " x=" << x << " y=" << y << " lim=" << lim << sync_endl;
            }
        } else
        if(std::abs(y) == BENT) { // bent slider
//if(mask) sync_cout << y << "," << x << sync_endl;
            if(std::abs(x) < 4) {
                d = y*FILE_NB/BENT; ss += x;
            } else {
                int a = std::abs(x);
                x /= a; y = y * FILE_NB / BENT;
                d = y + x; ss += y * (BENT - a) - d;
//sync_cout << "B: ss=" << ss << " d=" << d << " x=" << x << " y=" << y << " lim=" << lim << sync_endl;
            }
        }

        if(MT == HOPPER_RANGE && limit == 0xFFFE) lim = 0; // give grasshopper unlimited total range

        for (Square s = ss + d;
             is_ok(s) && distance(s, s == ss + d ? sq : s - d) <= 3;
             s += d)
        {
            if(mask && (!is_ok(s + d) || distance(s, s + d) > 3)) break; // forelast square is not blocker

            if (MT != HOPPER || hurdle)
            {
                if(!(lim & 1) || mask) attack |= s;
                lim >>= 1;
            }

            if (occupied & s)
            {
                if (MT == HOPPER && !hurdle)
                    hurdle = true;
                else
                    break;
            }
        }

        return attack;
  }

  template <MovementType MT>
  Bitboard sliding_attack(std::map<DirectionCode, int> directions, Square sq, Bitboard occupied, Color c = WHITE, bool mask = false) {
    assert(MT != LAME_LEAPER);

    Bitboard attack = 0;

    for (auto const& [v, limit] : directions)
    {
        attack |= one_ride<MT>(v, limit & 0xFFFF, sq, occupied, c, mask);
    }
    return attack;
  }

  Bitboard lame_leaper_path(DirectionCode d, Square s) {
    Direction dr = d > 0 ? NORTH : SOUTH;
    Direction df = h_step(d) < 0 ? WEST : EAST;
    Square to = s + board_step(d);
    Bitboard b = 0;
    if (!is_ok(to) || distance(s, to) >= 4)
        return b;
    while (s != to)
    {
        int diff = std::abs(file_of(to) - file_of(s)) - std::abs(rank_of(to) - rank_of(s));
        if (diff > 0)
            s += df;
        else if (diff < 0)
            s += dr;
        else
            s += df + dr;

        if (s != to)
            b |= s;
    }
    return b;
  }

  Bitboard lame_leaper_path(std::map<DirectionCode, int> directions, Square s) {
    Bitboard b = 0;
    for (const auto& i : directions)
        b |= lame_leaper_path(i.first, s);
    return b;
  }

  Bitboard lame_leaper_attack(std::map<DirectionCode, int> directions, Square s, Bitboard occupied) {
    Bitboard b = 0;
    for (const auto& i : directions)
    {
        Direction d = board_step(i.first);
        Direction h = h_step(i.first);
        Square to = s + d;
        if (is_ok(to) && file_of(s) + h < FILE_NB && file_of(s) + h >= 0 && !(lame_leaper_path(i.first, s) & occupied))
            b |= to;
    }
    return b;
  }

}

/// safe_destination() returns the bitboard of target square for the given step
/// from the given square. If the step is off the board, returns empty bitboard.

inline Bitboard safe_destination(Square s, DirectionCode c) {
    Direction step = board_step(c);
    Direction h = h_step(c);
    Square to = Square(s + step);
    return is_ok(to) && file_of(s) + h >= 0 && file_of(s) + h < FILE_NB ? square_bb(to) : Bitboard(0);
}


/// Bitboards::pretty() returns an ASCII representation of a bitboard suitable
/// to be printed to standard output. Useful for debugging.

std::string Bitboards::pretty(Bitboard b) {

  std::string s = "+---+---+---+---+---+---+---+---+---+---+---+---+\n";

  for (Rank r = RANK_MAX; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_MAX; ++f)
          s += b & make_square(f, r) ? "| X " : "|   ";

      s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+---+---+---+---+\n";
  }
  s += "  a   b   c   d   e   f   g   h   i   j   k   l\n";

  return s;
}

std::string small(Bitboard b) {
  std::string s = "";
  for (Rank r = RANK_MAX; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_MAX; ++f)
          s += b & make_square(f, r) ? "1" : "0";

      s +=  "\n";
  }
  return s;
}

std::string very_small(Bitboard b) {
  std::string s = "";
  static std::string digits[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f" };
  int k = 0;
  for(int i=127; i>=0; i--) {
    k = 2*k + (b >> i & 1);
    if(!(i & 3)) s += digits[k], k = 0;
    if(!(i % 12)) s += " ";
  }
  s += "\n";
  return s;
}

RiderType assign_magic(RiderType r, int limit) {
  int n = lsb(r);
  int i;
  limit >>= 16;
//std::cout << "assign " << r << "," << n << std::endl;
  for(i = 0; i < nrOfRides; i++)
      if(riderBaseType[i] == n && riderRangeMask[i] == limit) return RiderType(1 << i);

  if(nrOfRides == 32) return r; // use standard mask if overflow
//sync_cout << "new magic[" << i << "], type = " << n << " limit = " << limit << sync_endl;
  i = nrOfRides++; // allocate new magic
  nonSlidingRides |= RiderType(1 << i);
  asymmetricalRides |= RiderType(1 << i);
  riderBaseType[i] = n;
  riderRangeMask[i] = limit;
  Magic* m = CustomMagics[i - 20];
  magics[i] = m;
  for(Square s = SQ_A1; s < SQUARE_NB; ++s)
  {
      Bitboard b = 0;
      m[s] = magics[n][s];
      for (auto const& [v, limit2] : allDirections[n]) {
          b |= one_ride<RIDER>(v, limit, s, 0, WHITE, false);
      }
      m[s].mask &= b; // restrict mask according to high bits of limit
//sync_cout << s << " " << nonSlidingRides << " \n" << Stockfish::small(m[s].mask) << sync_endl;
  }
  return RiderType(1 << i);
}

bool is_leap_of(std::string betza, DirectionCode d) {
  PieceInfo* p = Betza::from_betza(betza, "");
  bool b = (p->steps[false][MODALITY_CAPTURE].find(d) != p->steps[false][MODALITY_CAPTURE].end());
  if(!b) b = (p->slider[false][MODALITY_CAPTURE].find(d) != p->slider[false][MODALITY_CAPTURE].end());
  delete p;
  return b;
}

/// Bitboards::init_pieces() initializes piece move/attack bitboards and rider types

void Bitboards::init_pieces() {

  for (PieceType pt = PAWN; pt <= KING; ++pt)
  {
      const PieceInfo* pi = pieceMap.find(pt)->second;

      // Detect rider types
      for (auto modality : {MODALITY_QUIET, MODALITY_CAPTURE})
      {
          for (bool initial : {false, true})
          {
              // We do not support initial captures
              if (modality == MODALITY_CAPTURE && initial)
                  continue;
              auto& riderTypes = modality == MODALITY_CAPTURE ? AttackRiderTypes[pt] : MoveRiderTypes[initial][pt];
              riderTypes = NO_RIDER;
              for (auto const& [d, limit] : pi->steps[initial][modality])
              {
                  if (limit && is_leap_of("D", d))
                      riderTypes |= RIDER_LAME_DABBABA;
                  if (limit && is_leap_of("N", d))
                      riderTypes |= RIDER_HORSE;
                  if (limit && is_leap_of("A", d))
                      riderTypes |= RIDER_ELEPHANT;
                  if (limit && is_leap_of("Z", d))
                      riderTypes |= RIDER_JANGGI_ELEPHANT;
              }
              for (auto const& [d, limit] : pi->slider[initial][modality])
              {
                  int l;
                  for(l = limit | 0xFFFF0000; ~l & 1; l >>= 1) {} if(~l) nonSlidingTypes |= pt;
                  if (is_leap_of("B", d))
                      riderTypes |= assign_magic(RIDER_BISHOP, limit);
                  if (is_leap_of("sR", d))
                      riderTypes |= assign_magic(RIDER_ROOK_H, limit);
                  if (is_leap_of("vR", d))
                      riderTypes |= assign_magic(RIDER_ROOK_V, limit);
                  if (is_leap_of("NN", d))
                      riderTypes |= RIDER_NIGHTRIDER;
                  if (is_leap_of("CC", d))
                      riderTypes |= RIDER_CAMELRIDER;
                  if (is_leap_of("ZZ", d))
                      riderTypes |= RIDER_ZEBRARIDER;
                  if (is_leap_of("sfyafsF", d))
                      riderTypes |= RIDER_ROOK_HT;
                  if (is_leap_of("rvyafsF", d))
                      riderTypes |= RIDER_ROOK_VR;
                  if (is_leap_of("bsyafsF", d))
                      riderTypes |= RIDER_ROOK_HB;
                  if (is_leap_of("lvyafsF", d))
                      riderTypes |= RIDER_ROOK_VL;
                  if (is_leap_of("rfblyafsW", d))
                      riderTypes |= RIDER_DIAG_DT;
                  if (is_leap_of("frlbyafsW", d))
                      riderTypes |= RIDER_DIAG_DB;
                  if (is_leap_of("lfbryafsW", d))
                      riderTypes |= RIDER_DIAG_AT;
                  if (is_leap_of("flrbyafsW", d))
                      riderTypes |= RIDER_DIAG_AB;
              }
              for (auto const& [d, limit] : pi->hopper[initial][modality])
              {
                  if (is_leap_of("sR", d))
                      riderTypes |= limit == 1 ? RIDER_GRASSHOPPER_H : RIDER_CANNON_H;
                  if (is_leap_of("vR", d))
                      riderTypes |= limit == 1 ? RIDER_GRASSHOPPER_V : RIDER_CANNON_V;
                  if (is_leap_of("B", d))
                      riderTypes |= limit == 1 ? RIDER_GRASSHOPPER_D : RIDER_CANNON_DIAG;
              }
          }
      }

      // Initialize move/attack bitboards
      for (Color c : { WHITE, BLACK })
      {
          for (Square s = SQ_A1; s <= SQ_MAX; ++s)
          {
              for (auto modality : {MODALITY_QUIET, MODALITY_CAPTURE})
              {
                  for (bool initial : {false, true})
                  {
                      // We do not support initial captures
                      if (modality == MODALITY_CAPTURE && initial)
                          continue;
                      auto& pseudo = modality == MODALITY_CAPTURE ? PseudoAttacks[c][pt][s] : PseudoMoves[initial][c][pt][s];
                      auto& leaper = modality == MODALITY_CAPTURE ? LeaperAttacks[c][pt][s] : LeaperMoves[initial][c][pt][s];
                      pseudo = 0;
                      leaper = 0;
                      for (auto const& [d, limit] : pi->steps[initial][modality])
                      {
                          pseudo |= safe_destination(s, DirectionCode(c == WHITE ? d : -d));
                          if (!limit)
                              leaper |= safe_destination(s, DirectionCode(c == WHITE ? d : -d));
                      }
                      pseudo |= sliding_attack<RIDER>(pi->slider[initial][modality], s, 0, c);
                      pseudo |= sliding_attack<HOPPER_RANGE>(pi->hopper[initial][modality], s, 0, c);
//if(pt == CUSTOM_PIECE_2 && !initial && s == 3)
//sync_cout << pt << "@" << s << ":\n" << Stockfish::small(pseudo) << sync_endl;
                  }
              }
          }
      }
      if(AttackRiderTypes[pt] & nonSlidingRides) nonSlidingTypes |= pt;
  }
}


/// Bitboards::init() initializes various bitboard tables. It is called at
/// startup and relies on global objects to be already zero-initialized.

void Bitboards::init() {

  Bitboard* b1;
  Bitboard* b2;

  for (unsigned i = 0; i < (1 << 16); ++i)
      PopCnt16[i] = uint8_t(std::bitset<16>(i).count());

  for (Square s = SQ_A1; s <= SQ_MAX; ++s)
      SquareBB[s] = make_bitboard(s);

  for (File f = FILE_A; f <= FILE_MAX; ++f)
      for (Rank r = RANK_1; r <= RANK_MAX; ++r)
          BoardSizeBB[f][r] = forward_file_bb(BLACK, make_square(f, r)) | SquareBB[make_square(f, r)] | (f > FILE_A ? BoardSizeBB[f - 1][r] : Bitboard(0));

  for (Square s1 = SQ_A1; s1 <= SQ_MAX; ++s1)
      for (Square s2 = SQ_A1; s2 <= SQ_MAX; ++s2)
              SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));
/*
Bitboard b = sliding_attack<RIDER>(Betza::from_betza("yafsW", "")->slider[false][MODALITY_QUIET], Square(64), 0);
sync_cout << small(b) << sync_endl;
exit(0);
*/
#ifdef PRECOMPUTED_MAGICS
  init_magics<RIDER>(RookTableH, RookMagicsH, "sR", RookMagicHInit);
  init_magics<RIDER>(RookTableV, RookMagicsV, "vR", RookMagicVInit);
  init_magics<RIDER>(BishopTable, BishopMagics, "B", BishopMagicInit);
  init_magics<HOPPER>(CannonTableH, CannonMagicsH, "sR", CannonMagicHInit);
  init_magics<HOPPER>(CannonTableV, CannonMagicsV, "vR", CannonMagicVInit);
  init_magics<LAME_LEAPER>(LameDabbabaTable, LameDabbabaMagics, "D", LameDabbabaMagicInit);
  init_magics<LAME_LEAPER>(HorseTable, HorseMagics, "NN", HorseMagicInit);
  init_magics<LAME_LEAPER>(ElephantTable, ElephantMagics, "AA", ElephantMagicInit);
  init_magics<LAME_LEAPER>(JanggiElephantTable, JanggiElephantMagics, "ZZ", JanggiElephantMagicInit);
  init_magics<HOPPER>(CannonDiagTable, CannonDiagMagics, "B", CannonDiagMagicInit);
  init_magics<RIDER>(NightriderTable, NightriderMagics, "NN", NULL); // Nightrider magics are fro wromg masks
  init_magics<RIDER>(CamelriderTable, CamelriderMagics, "CC", NULL);
  init_magics<RIDER>(ZebrariderTable, ZebrariderMagics, "ZZ", NULL);
  init_magics<HOPPER>(GrasshopperTableH, GrasshopperMagicsH, "sR", NULL);
  init_magics<HOPPER>(GrasshopperTableV, GrasshopperMagicsV, "vR", GrasshopperMagicVInit);
  init_magics<HOPPER>(GrasshopperTableD, GrasshopperMagicsD, "B", GrasshopperMagicDInit);
  b1 = init_magics<RIDER>(DiagTableD, DiagMagicsDT, "rfblyafsW", NULL);
  b2 = init_magics<RIDER>(DiagTableA, DiagMagicsAT, "lfbryafsW", NULL);
#else
  init_magics<RIDER>(RookTableH, RookMagicsH, "sR");
  init_magics<RIDER>(RookTableV, RookMagicsV, "vR");
  init_magics<RIDER>(BishopTable, BishopMagics, "B");
  init_magics<HOPPER>(CannonTableH, CannonMagicsH, "sR");
  init_magics<HOPPER>(CannonTableV, CannonMagicsV, "vR");
  init_magics<LAME_LEAPER>(LameDabbabaTable, LameDabbabaMagics, "DD");
  init_magics<LAME_LEAPER>(HorseTable, HorseMagics, "NN");
  init_magics<LAME_LEAPER>(ElephantTable, ElephantMagics, "AA");
  init_magics<LAME_LEAPER>(JanggiElephantTable, JanggiElephantMagics, "ZZ");
  init_magics<HOPPER>(CannonDiagTable, CannonDiagMagics, "B");
  init_magics<RIDER>(NightriderTable, NightriderMagics, "NN");
  init_magics<RIDER>(CamelriderTable, CamelriderMagics, "CC");
  init_magics<RIDER>(ZebrariderTable, ZebrariderMagics, "ZZ");
  init_magics<HOPPER>(GrasshopperTableH, GrasshopperMagicsH, "sR");
  init_magics<HOPPER>(GrasshopperTableV, GrasshopperMagicsV, "vR");
  init_magics<HOPPER>(GrasshopperTableD, GrasshopperMagicsD, "B");
  b1 = init_magics<RIDER>(DiagTableD, DiagMagicsDT, "rfblyafsW");
  b2 = init_magics<RIDER>(DiagTableA, DiagMagicsAT, "lfbryafsW");
#endif

  nrOfRides = 24;
  asymmetricalRides = ASYMMETRICAL_RIDERS;
  nonSlidingRides = NON_SLIDING_RIDERS;
  for(int i = 0; i < 24; i++) riderBaseType[i] = i;

  // Magics that can use attacks tables from others
  for(Square s = SQ_A1; s <= SQ_MAX; ++s)
  {
    RookMagicsHT[s] = RookMagicsH[rank_of(s) < RANK_NB - 1 ? s + FILE_NB : s - FILE_NB];
    RookMagicsHB[s] = RookMagicsH[rank_of(s) > 0 ? s - FILE_NB : s + FILE_NB];
    RookMagicsVR[s] = RookMagicsV[file_of(s) < FILE_NB - 1 ? s + 1 : s - 1];
    RookMagicsVL[s] = RookMagicsV[file_of(s) > 0 ? s - 1 : s + 1];
    if(file_of(s) != FILE_MAX && rank_of(s) != RANK_1) DiagMagicsDB[s] = DiagMagicsDT[s - FILE_NB + 1];
    if(file_of(s) != FILE_A && rank_of(s) != RANK_1) DiagMagicsAB[s] = DiagMagicsAT[s - FILE_NB - 1];
  }

#ifdef PRECOMPUTED_MAGICS
  init_magics<RIDER>(b1, DiagMagicsDB, "frlbyafsW", NULL);
  init_magics<RIDER>(b2, DiagMagicsAB, "flrbyafsW", NULL);
#else
  init_magics<RIDER>(b1, DiagMagicsDB, "frlbyafsW");
  init_magics<RIDER>(b2, DiagMagicsAB, "flrbyafsW");
#endif

  init_pieces();

  for (Square s1 = SQ_A1; s1 <= SQ_MAX; ++s1)
  {
#if 1
      for (PieceType pt : { BISHOP, ROOK })
          for (Square s2 = SQ_A1; s2 <= SQ_MAX; ++s2)
          {
              if (PseudoAttacks[WHITE][pt][s1] & s2)
              {
                  LineBB[s1][s2]    = (attacks_bb(WHITE, pt, s1, 0) & attacks_bb(WHITE, pt, s2, 0)) | s1 | s2;
                  BetweenBB[s1][s2] = (attacks_bb(WHITE, pt, s1, square_bb(s2)) & attacks_bb(WHITE, pt, s2, square_bb(s1)));
              }
              BetweenBB[s1][s2] |= s2;
          }
#else
#ifdef LARGEBOARDS
      int n = 5;
#else
      int n = 3;
#endif
      for(int dx = -n; dx <= n; dx++) for(int dy = -n; dy <= n; dy++) {
          int r = dx*dx + dy*dy;
          if(!r || r > 2 && dx*dy*(dx*dx - dy*dy) == 0 || r == 20) continue; // skip (N,0), (N,N) and (4,2) long leaps
          Direction d = FILE_NB * dy + dx;
          for(Square s2 = s1 + d; is_ok(s2) && distance(s1, s2) <= n; s2 += d) {
              for(Square s = s2; s != s1; s -= d) BetweenBB[s1][s2] |= s;
          }
      }
#endif
  }
}

namespace {

/// Fast generator for magic multipliers.
/// Works by purposeful backtracking rather than random trying:
/// recursively maps not-already-placed mask bits in the lookup key without spoiling
/// any bits that were placed earlier.

Bitboard puzzle(Bitboard key, Bitboard maskBitsToMap, Bitboard magic, int keyStart)
{
  int i, k;
  Bitboard keyBits = AllSquares << keyStart;

  for(k = SQ_MAX; ~maskBitsToMap & Square(k); k--) {} // find mask bit to map

  for(i = (k > keyStart ? k : keyStart); i <= SQUARE_BIT_MASK; i++)
  {
      int shift = i - k;
      Bitboard newBits = maskBitsToMap << shift;

      if(key & newBits & keyBits) continue; // does not fit in, shift more

      Bitboard newKey = key + newBits;
      Bitboard changedKeyBits = (newKey ^ key) & keyBits;

      if(changedKeyBits & key) continue; // carry spoiled key

      Bitboard newMagic = magic | Bitboard(1) << shift;
      Bitboard remainingMask = maskBitsToMap - (changedKeyBits >> shift);

      if(remainingMask)
          newMagic = puzzle(newKey, remainingMask, newMagic, keyStart);

      if(newMagic) return newMagic; // done
  }

  return 0; // fail
}

  // init_magics() computes all rook and bishop attacks at startup. Magic
  // bitboards are used to look up attacks of sliding pieces. As a reference see
  // www.chessprogramming.org/Magic_Bitboards. In particular, here we use the so
  // called "fancy" approach.

  template <MovementType MT>
#ifdef PRECOMPUTED_MAGICS
  Bitboard* init_magics(Bitboard table[], Magic magics[], std::string betza, const Bitboard magicsInit[]) {
#else
  Bitboard* init_magics(Bitboard table[], Magic magics[], std::string betza) {
#endif

    // Optimal PRNG seeds to pick the correct magics in the shortest time
#ifndef PRECOMPUTED_MAGICS
#ifdef LARGEBOARDS
    int seeds[][RANK_NB] = { { 734, 10316, 55013, 32803, 12281, 15100,  16645, 255, 346, 89123 },
                             { 734, 10316, 55013, 32803, 12281, 15100,  16645, 255, 346, 89123 } };
#else
    int seeds[][RANK_NB] = { { 8977, 44560, 54343, 38998,  5731, 95205, 104912, 17020 },
                             {  728, 10316, 55013, 32803, 12281, 15100,  16645,   255 } };
#endif
#endif

    Bitboard* occupancy = new Bitboard[1 << (FILE_NB + RANK_NB - 4)];
    Bitboard* reference = new Bitboard[1 << (FILE_NB + RANK_NB - 4)];
    Bitboard* nextEmpty = table;
    Bitboard b;
    int* epoch = new int[1 << (FILE_NB + RANK_NB - 4)]();
    int cnt = 0, size = 0, totalSize = 0;
    PieceInfo* pi = Betza::from_betza(betza, "");
    std::map<DirectionCode, int> directions = pi->slider[false][MODALITY_CAPTURE];

    for (Square s = SQ_A1; s <= SQ_MAX; ++s)
    {
        // Given a square 's', the mask is the bitboard of sliding attacks from
        // 's' computed on an empty board. The index must be big enough to contain
        // all the attacks for each possible subset of the mask and so is 2 power
        // the number of 1s of the mask. Hence we deduce the size of the shift to
        // apply to the 64 or 32 bits word to get the index.
        Magic& m = magics[s];
        if(m.attacks) continue; // this square has already been done

        // The mask for hoppers is unlimited distance, even if the hopper is limited distance (e.g., grasshopper)
        m.mask  = (MT == LAME_LEAPER ? lame_leaper_path(directions, s) : sliding_attack<MT == HOPPER ? HOPPER_RANGE : MT>(directions, s, 0, WHITE, true));
#ifdef LARGEBOARDS
        m.shift = 128 - popcount(m.mask);
#else
        m.shift = (Is64Bit ? 64 : 32) - popcount(m.mask);
#endif

        // Set the offset for the attacks table of the square. We have individual
        // table sizes for each square with "Fancy Magic Bitboards".
        m.attacks = nextEmpty += size;

        // Use Carry-Rippler trick to enumerate all subsets of masks[s] and
        // store the corresponding sliding attack bitboard in reference[].
        b = size = 0;
        do {
            occupancy[size] = b;
            reference[size] = MT == LAME_LEAPER ? lame_leaper_attack(directions, s, b) : sliding_attack<MT>(directions, s, b);

            if (HasPext)
                m.attacks[pext(b, m.mask)] = reference[size];

            size++;
            b = (b - m.mask) & m.mask;
        } while (b);

        if (HasPext)
            continue;

        if(!m.mask) m.magic = m.mask; else
#ifdef PRECOMPUTED_MAGICS
        // With large boards we always use an intelligent first try, as randomly trying until we succeed takes very long
        if(magicsInit) m.magic = magicsInit[s]; else // use precomputed if there is one
#endif
        m.magic = puzzle(0, m.mask, 0, m.shift);
//      sync_cout << s << ": " << very_small(m.mask) << very_small(m.magic) << sync_endl;

        {
            int i;

            // A good magic must map every possible occupancy to an index that
            // looks up the correct sliding attack in the attacks[s] database.
            // Note that we build up the database for square 's' as a side
            // effect of verifying the magic. Keep track of the attempt count
            // and save it in epoch[], little speed-up trick to avoid resetting
            // m.attacks[] after every failed attempt.
            for (++cnt, i = 0; i < size; ++i)
            {
                unsigned idx = m.index(occupancy[i]);

                if (epoch[idx] < cnt)
                {
                    epoch[idx] = cnt;
                    m.attacks[idx] = reference[i];
                }
                else if (m.attacks[idx] != reference[i])
                    break;
            }

            if(i < size) { // Panic! Generated magic was incorrect
                sync_cout << "# verification of magic multiplier for " << s << " failed at " << i << "/" << size << sync_endl;
            }
        }
        totalSize += size;
    }
#ifndef NDEBUG
    sync_cout << "# magic_init: size = " << totalSize << "(" << (totalSize*16 / 1024) << " KB)" << sync_endl;
#endif

    delete[] occupancy;
    delete[] reference;
    delete[] epoch;
    delete pi;

    return nextEmpty + size;
  }
}

} // namespace Stockfish
