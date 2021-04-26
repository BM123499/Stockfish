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
#include <array>

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

Magic<  ROOK> RookMagics[SQUARE_NB];
Magic<BISHOP> BishopMagics[SQUARE_NB];

namespace {
  using KnownMagicArray = std::array<KnownMagic, SQUARE_NB>;

  // If using magic bitboards, it's possible to reduce the size of the
  // attack table (~694 kB instead of 841 kB) by using specific offsets
  // in the table for each piece, to allow overlaps wherever possible.
  // We use magics and offsets originally found by Volker Annuss:
  // www.talkchess.com/forum3/viewtopic.php?p=727500#p727500
  constexpr KnownMagicArray KnownRookMagics = {
    KnownMagic { 0x00280077ffebfffeu,  26304 }, KnownMagic { 0x2004010201097fffu,  35520 },
    KnownMagic { 0x0010020010053fffu,  38592 }, KnownMagic { 0x0040040008004002u,   8026 },
    KnownMagic { 0x7fd00441ffffd003u,  22196 }, KnownMagic { 0x4020008887dffffeu,  80870 },
    KnownMagic { 0x004000888847ffffu,  76747 }, KnownMagic { 0x006800fbff75fffdu,  30400 },
    KnownMagic { 0x000028010113ffffu,  11115 }, KnownMagic { 0x0020040201fcffffu,  18205 },
    KnownMagic { 0x007fe80042ffffe8u,  53577 }, KnownMagic { 0x00001800217fffe8u,  62724 },
    KnownMagic { 0x00001800073fffe8u,  34282 }, KnownMagic { 0x00001800e05fffe8u,  29196 },
    KnownMagic { 0x00001800602fffe8u,  23806 }, KnownMagic { 0x000030002fffffa0u,  49481 },
    KnownMagic { 0x00300018010bffffu,   2410 }, KnownMagic { 0x0003000c0085fffbu,  36498 },
    KnownMagic { 0x0004000802010008u,  24478 }, KnownMagic { 0x0004002020020004u,  10074 },
    KnownMagic { 0x0001002002002001u,  79315 }, KnownMagic { 0x0001001000801040u,  51779 },
    KnownMagic { 0x0000004040008001u,  13586 }, KnownMagic { 0x0000006800cdfff4u,  19323 },
    KnownMagic { 0x0040200010080010u,  70612 }, KnownMagic { 0x0000080010040010u,  83652 },
    KnownMagic { 0x0004010008020008u,  63110 }, KnownMagic { 0x0000040020200200u,  34496 },
    KnownMagic { 0x0002008010100100u,  84966 }, KnownMagic { 0x0000008020010020u,  54341 },
    KnownMagic { 0x0000008020200040u,  60421 }, KnownMagic { 0x0000820020004020u,  86402 },
    KnownMagic { 0x00fffd1800300030u,  50245 }, KnownMagic { 0x007fff7fbfd40020u,  76622 },
    KnownMagic { 0x003fffbd00180018u,  84676 }, KnownMagic { 0x001fffde80180018u,  78757 },
    KnownMagic { 0x000fffe0bfe80018u,  37346 }, KnownMagic { 0x0001000080202001u,    370 },
    KnownMagic { 0x0003fffbff980180u,  42182 }, KnownMagic { 0x0001fffdff9000e0u,  45385 },
    KnownMagic { 0x00fffefeebffd800u,  61659 }, KnownMagic { 0x007ffff7ffc01400u,  12790 },
    KnownMagic { 0x003fffbfe4ffe800u,  16762 }, KnownMagic { 0x001ffff01fc03000u,      0 },
    KnownMagic { 0x000fffe7f8bfe800u,  38380 }, KnownMagic { 0x0007ffdfdf3ff808u,  11098 },
    KnownMagic { 0x0003fff85fffa804u,  21803 }, KnownMagic { 0x0001fffd75ffa802u,  39189 },
    KnownMagic { 0x00ffffd7ffebffd8u,  58628 }, KnownMagic { 0x007fff75ff7fbfd8u,  44116 },
    KnownMagic { 0x003fff863fbf7fd8u,  78357 }, KnownMagic { 0x001fffbfdfd7ffd8u,  44481 },
    KnownMagic { 0x000ffff810280028u,  64134 }, KnownMagic { 0x0007ffd7f7feffd8u,  41759 },
    KnownMagic { 0x0003fffc0c480048u,   1394 }, KnownMagic { 0x0001ffffafd7ffd8u,  40910 },
    KnownMagic { 0x00ffffe4ffdfa3bau,  66516 }, KnownMagic { 0x007fffef7ff3d3dau,   3897 },
    KnownMagic { 0x003fffbfdfeff7fau,   3930 }, KnownMagic { 0x001fffeff7fbfc22u,  72934 },
    KnownMagic { 0x0000020408001001u,  72662 }, KnownMagic { 0x0007fffeffff77fdu,  56325 },
    KnownMagic { 0x0003ffffbf7dfeecu,  66501 }, KnownMagic { 0x0001ffff9dffa333u,  14826 }
  };

  constexpr KnownMagicArray KnownBishopMagics = {
    KnownMagic { 0x007fbfbfbfbfbfffu,   5378 }, KnownMagic { 0x0000a060401007fcu,   4093 },
    KnownMagic { 0x0001004008020000u,   4314 }, KnownMagic { 0x0000806004000000u,   6587 },
    KnownMagic { 0x0000100400000000u,   6491 }, KnownMagic { 0x000021c100b20000u,   6330 },
    KnownMagic { 0x0000040041008000u,   5609 }, KnownMagic { 0x00000fb0203fff80u,  22236 },
    KnownMagic { 0x0000040100401004u,   6106 }, KnownMagic { 0x0000020080200802u,   5625 },
    KnownMagic { 0x0000004010202000u,  16785 }, KnownMagic { 0x0000008060040000u,  16817 },
    KnownMagic { 0x0000004402000000u,   6842 }, KnownMagic { 0x0000000801008000u,   7003 },
    KnownMagic { 0x000007efe0bfff80u,   4197 }, KnownMagic { 0x0000000820820020u,   7356 },
    KnownMagic { 0x0000400080808080u,   4602 }, KnownMagic { 0x00021f0100400808u,   4538 },
    KnownMagic { 0x00018000c06f3fffu,  29531 }, KnownMagic { 0x0000258200801000u,  45393 },
    KnownMagic { 0x0000240080840000u,  12420 }, KnownMagic { 0x000018000c03fff8u,  15763 },
    KnownMagic { 0x00000a5840208020u,   5050 }, KnownMagic { 0x0000020008208020u,   4346 },
    KnownMagic { 0x0000804000810100u,   6074 }, KnownMagic { 0x0001011900802008u,   7866 },
    KnownMagic { 0x0000804000810100u,  32139 }, KnownMagic { 0x000100403c0403ffu,  57673 },
    KnownMagic { 0x00078402a8802000u,  55365 }, KnownMagic { 0x0000101000804400u,  15818 },
    KnownMagic { 0x0000080800104100u,   5562 }, KnownMagic { 0x00004004c0082008u,   6390 },
    KnownMagic { 0x0001010120008020u,   7930 }, KnownMagic { 0x000080809a004010u,  13329 },
    KnownMagic { 0x0007fefe08810010u,   7170 }, KnownMagic { 0x0003ff0f833fc080u,  27267 },
    KnownMagic { 0x007fe08019003042u,  53787 }, KnownMagic { 0x003fffefea003000u,   5097 },
    KnownMagic { 0x0000101010002080u,   6643 }, KnownMagic { 0x0000802005080804u,   6138 },
    KnownMagic { 0x0000808080a80040u,   7418 }, KnownMagic { 0x0000104100200040u,   7898 },
    KnownMagic { 0x0003ffdf7f833fc0u,  42012 }, KnownMagic { 0x0000008840450020u,  57350 },
    KnownMagic { 0x00007ffc80180030u,  22813 }, KnownMagic { 0x007fffdd80140028u,  56693 },
    KnownMagic { 0x00020080200a0004u,   5818 }, KnownMagic { 0x0000101010100020u,   7098 },
    KnownMagic { 0x0007ffdfc1805000u,   4451 }, KnownMagic { 0x0003ffefe0c02200u,   4709 },
    KnownMagic { 0x0000000820806000u,   4794 }, KnownMagic { 0x0000000008403000u,  13364 },
    KnownMagic { 0x0000000100202000u,   4570 }, KnownMagic { 0x0000004040802000u,   4282 },
    KnownMagic { 0x0004010040100400u,  14964 }, KnownMagic { 0x00006020601803f4u,   4026 },
    KnownMagic { 0x0003ffdfdfc28048u,   4826 }, KnownMagic { 0x0000000820820020u,   7354 },
    KnownMagic { 0x0000000008208060u,   4848 }, KnownMagic { 0x0000000000808020u,  15946 },
    KnownMagic { 0x0000000001002020u,  14932 }, KnownMagic { 0x0000000401002008u,  16588 },
    KnownMagic { 0x0000004040404040u,   6905 }, KnownMagic { 0x007fff9fdf7ff813u,  16076 }
  };

  // If using PEXT indexing, do not use reduced table size.
  Bitboard SlideAttackTable[HasPext ? 0x19000 + 0x1480 : 88772] {};

  template<PieceType Pt>
  void init_magics(Bitboard table[], Magic<Pt> magics[]);
}

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

  init_magics<  ROOK>(SlideAttackTable, RookMagics);
  init_magics<BISHOP>(SlideAttackTable, BishopMagics);

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
  void init_magics(Bitboard table[], Magic<Pt> magics[]) {
    int size = 0;

    for (Square s = SQ_A1; s <= SQ_H8; ++s)
    {
        // Board edges are not considered in the relevant occupancies
        Bitboard edges = ((Rank1BB | Rank8BB) & ~rank_bb(s)) | ((FileABB | FileHBB) & ~file_bb(s));

        Magic<Pt>& m = magics[s];
        KnownMagic knownMagic = Pt == ROOK ? KnownRookMagics[s] : KnownBishopMagics[s];

        // Given a square 's', the mask is the bitboard of sliding attacks from
        // 's' computed on an empty board.
        m.mask = sliding_attack(Pt, s, 0) & ~edges;

        if constexpr (HasPext)
        {
            // For PEXT, use the starting offset if on the first square, and use the
            // previous square's end offset as the current square's starting offset.
            // Rooks are stored in entries 0 through 0x18FFFF,
            // bishops are stored in entries 0x190000 through 0x19147F.
            constexpr int startOffset = Pt == ROOK ? 0 : 0x19000;
            m.attacks = s == SQ_A1 ? table + startOffset : magics[s - 1].attacks + size;
        }
        else
        {
            // For magic bitboards indexing we use pre-computed magics values and
            // offsets. Since we are using the "fixed shift" approach we do not need
            // to calculate the shift, because the magic product will always resolve
            // a unique index using a 64-12 bit shift for rooks and 64-9 bit shift
            // for bishops.
            m.magic = knownMagic.magic;
            m.attacks = table + knownMagic.offset;
        }

        // Use Carry-Rippler trick to enumerate all subsets of masks[s] and
        // store the corresponding sliding attack bitboard in the attack table.
        Bitboard occupied = 0;
        size = 0;
        do {
            Bitboard reference = sliding_attack(Pt, s, occupied);

            unsigned index = m.index(occupied);
            assert(m.attacks[index] == 0 || m.attacks[index] == reference);
            m.attacks[index] = reference;

            size++;
            occupied = (occupied - m.mask) & m.mask;
        } while (occupied);
    }
  }
}

} // namespace Stockfish
