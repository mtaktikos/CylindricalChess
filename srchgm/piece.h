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

#ifndef PIECE_H_INCLUDED
#define PIECE_H_INCLUDED

#include <string>
#include <map>

#include "types.h"
#include "variant.h"

namespace Stockfish {

const int BENT = 9;

enum MoveModality {MODALITY_QUIET, MODALITY_CAPTURE, MOVE_MODALITY_NB};

/// PieceInfo struct stores information about the piece movements.

struct PieceInfo {
  std::string name = "";
  std::string betza = "";
  std::map<DirectionCode, int> steps[2][MOVE_MODALITY_NB] = {};
  std::map<DirectionCode, int> slider[2][MOVE_MODALITY_NB] = {};
  std::map<DirectionCode, int> hopper[2][MOVE_MODALITY_NB] = {};
};

struct PieceMap : public std::map<PieceType, const PieceInfo*> {
  void init(const Variant* v = nullptr);
  void add(PieceType pt, const PieceInfo* v);
  void clear_all();
};

extern PieceMap pieceMap;

inline std::string piece_name(PieceType pt) {
  return is_custom(pt) ? "customPiece" + std::to_string(pt - CUSTOM_PIECES + 1)
                       : pieceMap.find(pt)->second->name;
}

inline DirectionCode step(int y, int x) {
  return DirectionCode(y * WIDTH + x);
}

inline Direction h_step(DirectionCode v) {
  return Direction(((v + HWIDTH) & X_STEP_MASK) - HWIDTH);
}

inline Direction v_step(DirectionCode v) {
  return Direction((v - h_step(v)) / WIDTH);
}

inline Direction board_step(DirectionCode v) {
  return Direction(v_step(v) * FILE_NB + h_step(v));
}

namespace Betza {
  PieceInfo* from_betza(const std::string& betza, const std::string& name);
}

} // namespace Stockfish

#endif // #ifndef PIECE_H_INCLUDED
