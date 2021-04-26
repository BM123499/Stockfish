/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2021 The Stockfish developers (see AUTHORS file)

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
#include "misc.h"

namespace Stockfish {

uint8_t PopCnt16[1 << 16];
uint8_t SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard SquareBB[SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];

namespace {
  // If using PEXT indexing, do not use reduced table size.
  Bitboard SlideAttackTable[HasPext ? 0x19000 + 0x1480 : 88772] = {};

  template<PieceType Pt>
  void init_magics(Magic<Pt> magics[]);
}

// If using magic bitboards, it's possible to reduce the size of the
// attack table (~694 kB instead of 841 kB) by using specific offsets
// in the table for each piece, to allow overlaps wherever possible.
// We use magics and offsets originally found by Volker Annuss:
// www.talkchess.com/forum3/viewtopic.php?p=727500#p727500
#define sat SlideAttackTable

Magic< ROOK > RookMagics[SQUARE_NB] = {
    { 0x00280077ffebfffeULL, sat + 26304 }, { 0x2004010201097fffULL, sat + 35520 },
    { 0x0010020010053fffULL, sat + 38592 }, { 0x0040040008004002ULL, sat +  8026 },
    { 0x7fd00441ffffd003ULL, sat + 22196 }, { 0x4020008887dffffeULL, sat + 80870 },
    { 0x004000888847ffffULL, sat + 76747 }, { 0x006800fbff75fffdULL, sat + 30400 },
    { 0x000028010113ffffULL, sat + 11115 }, { 0x0020040201fcffffULL, sat + 18205 },
    { 0x007fe80042ffffe8ULL, sat + 53577 }, { 0x00001800217fffe8ULL, sat + 62724 },
    { 0x00001800073fffe8ULL, sat + 34282 }, { 0x00001800e05fffe8ULL, sat + 29196 },
    { 0x00001800602fffe8ULL, sat + 23806 }, { 0x000030002fffffa0ULL, sat + 49481 },
    { 0x00300018010bffffULL, sat +  2410 }, { 0x0003000c0085fffbULL, sat + 36498 },
    { 0x0004000802010008ULL, sat + 24478 }, { 0x0004002020020004ULL, sat + 10074 },
    { 0x0001002002002001ULL, sat + 79315 }, { 0x0001001000801040ULL, sat + 51779 },
    { 0x0000004040008001ULL, sat + 13586 }, { 0x0000006800cdfff4ULL, sat + 19323 },
    { 0x0040200010080010ULL, sat + 70612 }, { 0x0000080010040010ULL, sat + 83652 },
    { 0x0004010008020008ULL, sat + 63110 }, { 0x0000040020200200ULL, sat + 34496 },
    { 0x0002008010100100ULL, sat + 84966 }, { 0x0000008020010020ULL, sat + 54341 },
    { 0x0000008020200040ULL, sat + 60421 }, { 0x0000820020004020ULL, sat + 86402 },
    { 0x00fffd1800300030ULL, sat + 50245 }, { 0x007fff7fbfd40020ULL, sat + 76622 },
    { 0x003fffbd00180018ULL, sat + 84676 }, { 0x001fffde80180018ULL, sat + 78757 },
    { 0x000fffe0bfe80018ULL, sat + 37346 }, { 0x0001000080202001ULL, sat +   370 },
    { 0x0003fffbff980180ULL, sat + 42182 }, { 0x0001fffdff9000e0ULL, sat + 45385 },
    { 0x00fffefeebffd800ULL, sat + 61659 }, { 0x007ffff7ffc01400ULL, sat + 12790 },
    { 0x003fffbfe4ffe800ULL, sat + 16762 }, { 0x001ffff01fc03000ULL, sat +     0 },
    { 0x000fffe7f8bfe800ULL, sat + 38380 }, { 0x0007ffdfdf3ff808ULL, sat + 11098 },
    { 0x0003fff85fffa804ULL, sat + 21803 }, { 0x0001fffd75ffa802ULL, sat + 39189 },
    { 0x00ffffd7ffebffd8ULL, sat + 58628 }, { 0x007fff75ff7fbfd8ULL, sat + 44116 },
    { 0x003fff863fbf7fd8ULL, sat + 78357 }, { 0x001fffbfdfd7ffd8ULL, sat + 44481 },
    { 0x000ffff810280028ULL, sat + 64134 }, { 0x0007ffd7f7feffd8ULL, sat + 41759 },
    { 0x0003fffc0c480048ULL, sat +  1394 }, { 0x0001ffffafd7ffd8ULL, sat + 40910 },
    { 0x00ffffe4ffdfa3baULL, sat + 66516 }, { 0x007fffef7ff3d3daULL, sat +  3897 },
    { 0x003fffbfdfeff7faULL, sat +  3930 }, { 0x001fffeff7fbfc22ULL, sat + 72934 },
    { 0x0000020408001001ULL, sat + 72662 }, { 0x0007fffeffff77fdULL, sat + 56325 },
    { 0x0003ffffbf7dfeecULL, sat + 66501 }, { 0x0001ffff9dffa333ULL, sat + 14826 }
};

Magic<BISHOP> BishopMagics[SQUARE_NB] = {
    { 0x007fbfbfbfbfbfffULL, sat +  5378 }, { 0x0000a060401007fcULL, sat +  4093 },
    { 0x0001004008020000ULL, sat +  4314 }, { 0x0000806004000000ULL, sat +  6587 },
    { 0x0000100400000000ULL, sat +  6491 }, { 0x000021c100b20000ULL, sat +  6330 },
    { 0x0000040041008000ULL, sat +  5609 }, { 0x00000fb0203fff80ULL, sat + 22236 },
    { 0x0000040100401004ULL, sat +  6106 }, { 0x0000020080200802ULL, sat +  5625 },
    { 0x0000004010202000ULL, sat + 16785 }, { 0x0000008060040000ULL, sat + 16817 },
    { 0x0000004402000000ULL, sat +  6842 }, { 0x0000000801008000ULL, sat +  7003 },
    { 0x000007efe0bfff80ULL, sat +  4197 }, { 0x0000000820820020ULL, sat +  7356 },
    { 0x0000400080808080ULL, sat +  4602 }, { 0x00021f0100400808ULL, sat +  4538 },
    { 0x00018000c06f3fffULL, sat + 29531 }, { 0x0000258200801000ULL, sat + 45393 },
    { 0x0000240080840000ULL, sat + 12420 }, { 0x000018000c03fff8ULL, sat + 15763 },
    { 0x00000a5840208020ULL, sat +  5050 }, { 0x0000020008208020ULL, sat +  4346 },
    { 0x0000804000810100ULL, sat +  6074 }, { 0x0001011900802008ULL, sat +  7866 },
    { 0x0000804000810100ULL, sat + 32139 }, { 0x000100403c0403ffULL, sat + 57673 },
    { 0x00078402a8802000ULL, sat + 55365 }, { 0x0000101000804400ULL, sat + 15818 },
    { 0x0000080800104100ULL, sat +  5562 }, { 0x00004004c0082008ULL, sat +  6390 },
    { 0x0001010120008020ULL, sat +  7930 }, { 0x000080809a004010ULL, sat + 13329 },
    { 0x0007fefe08810010ULL, sat +  7170 }, { 0x0003ff0f833fc080ULL, sat + 27267 },
    { 0x007fe08019003042ULL, sat + 53787 }, { 0x003fffefea003000ULL, sat +  5097 },
    { 0x0000101010002080ULL, sat +  6643 }, { 0x0000802005080804ULL, sat +  6138 },
    { 0x0000808080a80040ULL, sat +  7418 }, { 0x0000104100200040ULL, sat +  7898 },
    { 0x0003ffdf7f833fc0ULL, sat + 42012 }, { 0x0000008840450020ULL, sat + 57350 },
    { 0x00007ffc80180030ULL, sat + 22813 }, { 0x007fffdd80140028ULL, sat + 56693 },
    { 0x00020080200a0004ULL, sat +  5818 }, { 0x0000101010100020ULL, sat +  7098 },
    { 0x0007ffdfc1805000ULL, sat +  4451 }, { 0x0003ffefe0c02200ULL, sat +  4709 },
    { 0x0000000820806000ULL, sat +  4794 }, { 0x0000000008403000ULL, sat + 13364 },
    { 0x0000000100202000ULL, sat +  4570 }, { 0x0000004040802000ULL, sat +  4282 },
    { 0x0004010040100400ULL, sat + 14964 }, { 0x00006020601803f4ULL, sat +  4026 },
    { 0x0003ffdfdfc28048ULL, sat +  4826 }, { 0x0000000820820020ULL, sat +  7354 },
    { 0x0000000008208060ULL, sat +  4848 }, { 0x0000000000808020ULL, sat + 15946 },
    { 0x0000000001002020ULL, sat + 14932 }, { 0x0000000401002008ULL, sat + 16588 },
    { 0x0000004040404040ULL, sat +  6905 }, { 0x007fff9fdf7ff813ULL, sat + 16076 }
};

#undef sat

/// safe_destination() returns the bitboard of target square for the given step
/// from the given square. If the step is off the board, returns empty bitboard.

inline Bitboard safe_destination(Square s, int step) {
    Square to = Square(s + step);
    return is_ok(to) && distance(s, to) <= 2 ? square_bb(to) : Bitboard(0);
}


/// Bitboards::pretty() returns an ASCII representation of a bitboard suitable
/// to be printed to standard output. Useful for debugging.

std::string Bitboards::pretty(Bitboard b) {

  std::string s = "+---+---+---+---+---+---+---+---+\n";

  for (Rank r = RANK_8; r >= RANK_1; --r)
  {
      for (File f = FILE_A; f <= FILE_H; ++f)
          s += b & make_square(f, r) ? "| X " : "|   ";

      s += "| " + std::to_string(1 + r) + "\n+---+---+---+---+---+---+---+---+\n";
  }
  s += "  a   b   c   d   e   f   g   h\n";

  return s;
}


/// Bitboards::init() initializes various bitboard tables. It is called at
/// startup and relies on global objects to be already zero-initialized.

void Bitboards::init() {

  for (unsigned i = 0; i < (1 << 16); ++i)
      PopCnt16[i] = uint8_t(std::bitset<16>(i).count());

  for (Square s = SQ_A1; s <= SQ_H8; ++s)
      SquareBB[s] = (1ULL << s);

  for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
      for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
          SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));

  init_magics< ROOK >( RookMagics );
  init_magics<BISHOP>(BishopMagics);

  for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
  {
      PawnAttacks[WHITE][s1] = pawn_attacks_bb<WHITE>(square_bb(s1));
      PawnAttacks[BLACK][s1] = pawn_attacks_bb<BLACK>(square_bb(s1));

      for (int step : {-9, -8, -7, -1, 1, 7, 8, 9} )
         PseudoAttacks[KING][s1] |= safe_destination(s1, step);

      for (int step : {-17, -15, -10, -6, 6, 10, 15, 17} )
         PseudoAttacks[KNIGHT][s1] |= safe_destination(s1, step);

      PseudoAttacks[QUEEN][s1]  = PseudoAttacks[BISHOP][s1] = attacks_bb<BISHOP>(s1, 0);
      PseudoAttacks[QUEEN][s1] |= PseudoAttacks[  ROOK][s1] = attacks_bb<  ROOK>(s1, 0);

      for (PieceType pt : { BISHOP, ROOK })
          for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
          {
              if (PseudoAttacks[pt][s1] & s2)
              {
                  LineBB[s1][s2]    = (attacks_bb(pt, s1, 0) & attacks_bb(pt, s2, 0)) | s1 | s2;
                  BetweenBB[s1][s2] = (attacks_bb(pt, s1, square_bb(s2)) & attacks_bb(pt, s2, square_bb(s1)));
              }
              BetweenBB[s1][s2] |= s2;
          }
  }
}

namespace {

  Bitboard sliding_attack(PieceType pt, Square sq, Bitboard occupied) {

    Bitboard attacks = 0;
    Direction   RookDirections[4] = {NORTH, SOUTH, EAST, WEST};
    Direction BishopDirections[4] = {NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};

    for (Direction d : (pt == ROOK ? RookDirections : BishopDirections))
    {
        Square s = sq;
        while (safe_destination(s, d) && !(occupied & s))
            attacks |= (s += d);
    }

    return attacks;
  }


  // init_magics() computes all rook and bishop attacks at startup. Either magic
  // bitboards or PEXT indexing are used to look up attacks of sliding pieces.
  // As a reference see www.chessprogramming.org/Magic_Bitboards. In particular,
  // here we use the so called "fixed shift fancy magic bitboards" approach.
  template<PieceType Pt>
  void init_magics(Magic<Pt> magics[]) {
    int size = 0;

    for (Square s = SQ_A1; s <= SQ_H8; ++s)
    {
        // Board edges are not considered in the relevant occupancies
        Bitboard edges = ((Rank1BB | Rank8BB) & ~rank_bb(s)) | ((FileABB | FileHBB) & ~file_bb(s));

        Magic<Pt>& m = magics[s];

        // Given a square 's', the mask is the bitboard of sliding attacks from
        // 's' computed on an empty board.
        m.mask = sliding_attack(Pt, s, 0) & ~edges;

        if constexpr (HasPext)
        {
            // For PEXT, use the starting offset if on the first square, and use the
            // previous square's end offset as the current square's starting offset.
            // Rooks are stored in entries 0 through 0x18FFFF,
            // bishops are stored in entries 0x190000 through 0x19147F.
            constexpr unsigned startOffset = Pt == ROOK ? 0 : 0x19000;
            m.attacks = s == SQ_A1 ? SlideAttackTable + startOffset : magics[s - 1].attacks + size;
        }

        // Use Carry-Rippler trick to enumerate all subsets of masks[s] and
        // store the corresponding sliding attack bitboard in the attack table.
        Bitboard occupied = 0;
        size = 0;
        do {
            Bitboard reference = sliding_attack(Pt, s, occupied);

            unsigned idx = m.index(occupied);
            assert(m.attacks[idx] == 0 || m.attacks[idx] == reference);
            m.attacks[idx] = reference;

            size++;
            occupied = (occupied - m.mask) & m.mask;
        } while (occupied);
    }
  }
}

} // namespace Stockfish
