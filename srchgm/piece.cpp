/*
  Fairy-Stockfish, a UCI chess variant playing engine derived from Stockfish
  Copyright (C) 2018-2022 Fabian Fichter

  Fairy-Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Fairy-Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <map>
#include <string>
#include <utility>

#include "types.h"
#include "piece.h"

namespace Stockfish {

PieceMap pieceMap; // Global object


namespace Betza {
  const std::map<char, std::vector<std::pair<int, int>>> leaperAtoms = {
      {'W', {std::make_pair(1, 0)}},
      {'F', {std::make_pair(1, 1)}},
      {'D', {std::make_pair(2, 0)}},
      {'N', {std::make_pair(2, 1)}},
      {'A', {std::make_pair(2, 2)}},
      {'H', {std::make_pair(3, 0)}},
      {'L', {std::make_pair(3, 1)}},
      {'C', {std::make_pair(3, 1)}},
      {'J', {std::make_pair(3, 2)}},
      {'Z', {std::make_pair(3, 2)}},
      {'G', {std::make_pair(3, 3)}},
      {'K', {std::make_pair(1, 0), std::make_pair(1, 1)}},
  };
  const std::map<char, std::vector<std::pair<int, int>>> riderAtoms = {
      {'R', {std::make_pair(1, 0)}},
      {'B', {std::make_pair(1, 1)}},
      {'Q', {std::make_pair(1, 0), std::make_pair(1, 1)}},
  };
  const std::string verticals = "fbvh";
  const std::string horizontals = "rlsh";
  // from_betza creates a piece by parsing Betza notation
  // https://en.wikipedia.org/wiki/Betza%27s_funny_notation
  PieceInfo* from_betza(const std::string& betza, const std::string& name) {
      PieceInfo* p = new PieceInfo();
      p->name = name;
      p->betza = betza;
      std::vector<MoveModality> moveModalities = {};
      bool hopper = false;
      bool rider = false;
      bool lame = false;
      bool initial = false;
      int distance = 0;
      int fExtension = 0;
      int fsExtension = 0;
      BentType bent = SINGLE_LEG;
      std::vector<std::string> prelimDirections = {};
      for (std::string::size_type i = 0; i < betza.size(); i++)
      {
          char c = betza[i];
          // Modality
          if (c == 'm' || c == 'c')
              moveModalities.push_back(c == 'c' ? MODALITY_CAPTURE : MODALITY_QUIET);
          // Hopper
          else if (c == 'p' || c == 'g')
          {
              hopper = true;
              // Grasshopper
              if (c == 'g')
                  distance = 0xFFFE; // range 1
          }
          // Lame leaper
          else if (c == 'n')
              lame = true;
          // Initial move
          else if (c == 'i')
              initial = true;
          // Directional modifiers
          else if (verticals.find(c) != std::string::npos || horizontals.find(c) != std::string::npos)
          {
              if (i + 1 < betza.size())
              {
                  char c2 = betza[i+1];
                  // Can modifiers be combined?
                  if (   c2 == c
                      || (verticals.find(c) != std::string::npos && horizontals.find(c2) != std::string::npos)
                      || (horizontals.find(c) != std::string::npos && verticals.find(c2) != std::string::npos))
                  {
                      prelimDirections.push_back(std::string(1, c) + c2);
                      i++;
                      continue;
                  }
              }
              prelimDirections.push_back(std::string(2, c));
          }
          // Multi-leg
          else if(betza.size() > i + 3 && (c == 'a' || (c == 'y' && betza[++i] == 'a'))) // detect yaf(s)
          {
              if(betza[i+1] == 'f' && c == 'y')
              {
                  if(betza[i+2] == 's') { i++; bent = BENT_SLIDER; }
                  else bent = SKI_SLIDER;
                  i++; distance |= 1; // suppress 1-step moves
                  moveModalities.clear();
              }
          }
          // Move atom
          else if (leaperAtoms.find(c) != leaperAtoms.end() || riderAtoms.find(c) != riderAtoms.end())
          {
              const auto& atoms = riderAtoms.find(c) != riderAtoms.end() ? riderAtoms.find(c)->second
                                                                         : leaperAtoms.find(c)->second;
              // Check for rider
              if (riderAtoms.find(c) != riderAtoms.end())
                  rider = true;
              if (i + 1 < betza.size()) {
                  if (betza[++i] == 'X')
                      fExtension++;
                  else if (betza[i] == 'Y')
                      fsExtension++;
                  else if (isdigit(betza[i]) || betza[i] == c)
                  {
                      rider = true;
                      // limited distance riders
                      if (isdigit(betza[i])) {
                          int range = betza[i] - '0';
                          if(range)
                              distance |= (0xFFFF << range & 0xFFFF);
                      }
                  } else i--;
              }
              if (!rider && lame)
                  distance = -1;
              // No modality qualifier means m+c
              if (moveModalities.size() == 0)
              {
                  moveModalities.push_back(MODALITY_QUIET);
                  moveModalities.push_back(MODALITY_CAPTURE);
              }
              // Define moves
              for (const auto& atom : atoms)
              {
                  int y = atom.first + 3*fExtension + 2*fsExtension;
                  int x = atom.second + 2*fsExtension;
                  std::vector<std::string> directions = {};
                  if(bent != SINGLE_LEG) {
                      if(bent == BENT_SLIDER && c == 'F') y = BENT; // yafsF = griffon
                      if(bent == BENT_SLIDER && c == 'W') y = BENT, x = BENT - 1; // yafsW = manticore
                      rider = true;
                  }
                  if(y > 1 && (x == 0 || x == y)) { // radial distant leap
                      if(rider) { // true rider
                          if(y == 2) distance |= 0x55555555; else
                          if(y == 3) distance |= 0xB6DBB6DB; else
                          if(y == 4) distance |= 0x77777777;
                          x /= y; y = 1; // use (masked) slider magics
                      } else if(lame) {  // lame leaper
                          rider = true;  // treat as slider
                          distance = ~(1 << (y - 1)) & 0xFFFF; // with only one target on ray
                          x /= y; y = 1;
                      }
                  }
                  // Split directions for orthogonal pieces
                  // This is required e.g. to correctly interpret fsW for soldiers
                  for (auto s : prelimDirections)
                      if (atoms.size() == 1 && atom.second == 0 && s[0] != s[1] && bent != BENT_SLIDER)
                      {
                          directions.push_back(std::string(2, s[0]));
                          directions.push_back(std::string(2, s[1]));
                      }
                      else
                          directions.push_back(s);
                  // Add moves
                  for (auto modality : moveModalities)
                  {
                      auto& v = hopper ? p->hopper[initial][modality]
                               : rider ? p->slider[initial][modality]
                                       : p->steps[initial][modality];
                      auto has_dir = [&](std::string s) {
                        return std::find(directions.begin(), directions.end(), s) != directions.end();
                      };
                      if (directions.size() == 0 || has_dir("ff") || has_dir("vv") || has_dir("rf") || has_dir("rv") || has_dir("fh") || has_dir("rh") || has_dir("hr"))
                          v[step(y, x)] = distance;
                      if (directions.size() == 0 || has_dir("bb") || has_dir("vv") || has_dir("lb") || has_dir("lv") || has_dir("bh") || has_dir("lh") || has_dir("hr"))
                          v[step(-y, -x)] = distance;
                      if (directions.size() == 0 || has_dir("rr") || has_dir("ss") || has_dir("br") || has_dir("bs") || has_dir("bh") || has_dir("rh") || has_dir("hr"))
                          v[step(-x, y)] = distance;
                      if (directions.size() == 0 || has_dir("ll") || has_dir("ss") || has_dir("fl") || has_dir("fs") || has_dir("fh") || has_dir("lh") || has_dir("hr"))
                          v[step(x, -y)] = distance;
                      if (directions.size() == 0 || has_dir("rr") || has_dir("ss") || has_dir("fr") || has_dir("fs") || has_dir("fh") || has_dir("rh") || has_dir("hl"))
                          v[step(x, y)] = distance;
                      if (directions.size() == 0 || has_dir("ll") || has_dir("ss") || has_dir("bl") || has_dir("bs") || has_dir("bh") || has_dir("lh") || has_dir("hl"))
                          v[step(-x, -y)] = distance;
                      if (directions.size() == 0 || has_dir("bb") || has_dir("vv") || has_dir("rb") || has_dir("rv") || has_dir("bh") || has_dir("rh") || has_dir("hl"))
                          v[step(-y, x)] = distance;
                      if (directions.size() == 0 || has_dir("ff") || has_dir("vv") || has_dir("lf") || has_dir("lv") || has_dir("fh") || has_dir("lh") || has_dir("hl"))
                          v[step(y, -x)] = distance;
                  }
              }
              // Reset state
              moveModalities.clear();
              prelimDirections.clear();
              hopper = false;
              rider = false;
              lame = false;
              initial = false;
              distance = 0;
              fExtension = 0;
              fsExtension = 0;
              bent = SINGLE_LEG;
          }
      }
      return p;
  }
  // Special multi-leg betza description for Janggi elephant
  PieceInfo* janggi_elephant_piece() {
      PieceInfo* p = from_betza("nZ", "janggiElephant");
      p->betza = "mafsmafW"; // for compatibility with XBoard/Winboard
      return p;
  }
}

void PieceMap::init(const Variant* v) {
  clear_all();
  add(PAWN, Betza::from_betza("fmWfceF", "pawn"));
  add(KNIGHT, Betza::from_betza("N", "knight"));
  add(BISHOP, Betza::from_betza("B", "bishop"));
  add(ROOK, Betza::from_betza("R", "rook"));
  add(QUEEN, Betza::from_betza("Q", "queen"));
  add(FERS, Betza::from_betza("F", "fers"));
  add(ALFIL, Betza::from_betza("A", "alfil"));
  add(FERS_ALFIL, Betza::from_betza("FA", "fersAlfil"));
  add(SILVER, Betza::from_betza("FfW", "silver"));
  add(AIWOK, Betza::from_betza("RNF", "aiwok"));
  add(BERS, Betza::from_betza("RF", "bers"));
  add(ARCHBISHOP, Betza::from_betza("BN", "archbishop"));
  add(CHANCELLOR, Betza::from_betza("RN", "chancellor"));
  add(AMAZON, Betza::from_betza("QN", "amazon"));
  add(KNIBIS, Betza::from_betza("mNcB", "knibis"));
  add(BISKNI, Betza::from_betza("mBcN", "biskni"));
  add(KNIROO, Betza::from_betza("mNcR", "kniroo"));
  add(ROOKNI, Betza::from_betza("mRcN", "rookni"));
  add(SHOGI_PAWN, Betza::from_betza("fW", "shogiPawn"));
  add(LANCE, Betza::from_betza("fR", "lance"));
  add(SHOGI_KNIGHT, Betza::from_betza("fN", "shogiKnight"));
  add(GOLD, Betza::from_betza("WfF", "gold"));
  add(DRAGON_HORSE, Betza::from_betza("BW", "dragonHorse"));
  add(CLOBBER_PIECE, Betza::from_betza("cW", "clobber"));
  add(BREAKTHROUGH_PIECE, Betza::from_betza("fmWfF", "breakthrough"));
  add(IMMOBILE_PIECE, Betza::from_betza("", "immobile"));
  add(CANNON, Betza::from_betza("mRcpR", "cannon"));
  add(JANGGI_CANNON, Betza::from_betza("pR", "janggiCannon"));
  add(SOLDIER, Betza::from_betza("fsW", "soldier"));
  add(HORSE, Betza::from_betza("nN", "horse"));
  add(ELEPHANT, Betza::from_betza("nA", "elephant"));
  add(JANGGI_ELEPHANT, Betza::janggi_elephant_piece());
  add(BANNER, Betza::from_betza("RcpRnN", "banner"));
  add(WAZIR, Betza::from_betza("W", "wazir"));
  add(COMMONER, Betza::from_betza("K", "commoner"));
  add(CENTAUR, Betza::from_betza("KN", "centaur"));
  add(KING, Betza::from_betza("K", "king"));
  // Add custom pieces
  for (PieceType pt = CUSTOM_PIECES; pt <= CUSTOM_PIECES_END; ++pt)
      add(pt, Betza::from_betza(v != nullptr ? v->customPiece[pt - CUSTOM_PIECES] : "", ""));
}

void PieceMap::add(PieceType pt, const PieceInfo* p) {
  insert(std::pair<PieceType, const PieceInfo*>(pt, p));
}

void PieceMap::clear_all() {
  for (auto const& element : *this)
      delete element.second;
  clear();
}

} // namespace Stockfish
