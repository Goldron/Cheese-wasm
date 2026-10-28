////////////////////////////////////////////////////////////////////////////////
//
// Cheese, Chess engine.
//
// Copyright (C) 2006-2025 Patrice Duhamel. All rights reserved.
//
// Cheese is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Cheese is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "bitboard.h"

#include "util/logfile.h"
#include "move.h"
#include "moveparser.h"
#include "util/timer.h"

#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace Cheese::BitBoards {

#ifndef SYSTEM64BIT

const std::array<BitBoard, nb_squares> magic_rook_mask =
{
	0x000101010101017eULL, 0x000202020202027cULL, 0x000404040404047aULL, 0x0008080808080876ULL,
	0x001010101010106eULL, 0x002020202020205eULL, 0x004040404040403eULL, 0x008080808080807eULL,
	0x0001010101017e00ULL, 0x0002020202027c00ULL, 0x0004040404047a00ULL, 0x0008080808087600ULL,
	0x0010101010106e00ULL, 0x0020202020205e00ULL, 0x0040404040403e00ULL, 0x0080808080807e00ULL,
	0x00010101017e0100ULL, 0x00020202027c0200ULL, 0x00040404047a0400ULL, 0x0008080808760800ULL,
	0x00101010106e1000ULL, 0x00202020205e2000ULL, 0x00404040403e4000ULL, 0x00808080807e8000ULL,
	0x000101017e010100ULL, 0x000202027c020200ULL, 0x000404047a040400ULL, 0x0008080876080800ULL,
	0x001010106e101000ULL, 0x002020205e202000ULL, 0x004040403e404000ULL, 0x008080807e808000ULL,
	0x0001017e01010100ULL, 0x0002027c02020200ULL, 0x0004047a04040400ULL, 0x0008087608080800ULL,
	0x0010106e10101000ULL, 0x0020205e20202000ULL, 0x0040403e40404000ULL, 0x0080807e80808000ULL,
	0x00017e0101010100ULL, 0x00027c0202020200ULL, 0x00047a0404040400ULL, 0x0008760808080800ULL,
	0x00106e1010101000ULL, 0x00205e2020202000ULL, 0x00403e4040404000ULL, 0x00807e8080808000ULL,
	0x007e010101010100ULL, 0x007c020202020200ULL, 0x007a040404040400ULL, 0x0076080808080800ULL,
	0x006e101010101000ULL, 0x005e202020202000ULL, 0x003e404040404000ULL, 0x007e808080808000ULL,
	0x7e01010101010100ULL, 0x7c02020202020200ULL, 0x7a04040404040400ULL, 0x7608080808080800ULL,
	0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL
};

const std::array<BitBoard, nb_squares> magic_rook_magic =
{
	0x0018008450202280ULL, 0x0600180000106140ULL, 0x0081780000801260ULL, 0x0080180000804408ULL,
	0x020c008000800204ULL, 0x2002245040400902ULL, 0x08440900010002a3ULL, 0x000020c000400241ULL,
	0x21c0012008004040ULL, 0x1260080820002020ULL, 0x1082004008020804ULL, 0x0010448110940408ULL,
	0x18000c2001020112ULL, 0x0402007011004104ULL, 0x0623081400010091ULL, 0x1044028112410101ULL,
	0x0020408100060404ULL, 0x04412010020a0808ULL, 0x1080440021020208ULL, 0x0024180214010104ULL,
	0x0008408002413008ULL, 0x00804449010a0041ULL, 0x1086060401010102ULL, 0x0880040100104201ULL,
	0x48410029204300aeULL, 0x0924449421c10021ULL, 0x016001c002201109ULL, 0x001408600cc02a06ULL,
	0x3004200800101a02ULL, 0x01a6215002000882ULL, 0x1012050008800401ULL, 0x0100209222040263ULL,
	0x0040202805820e44ULL, 0x0040620000100323ULL, 0x00802088074410a0ULL, 0x4040440008900048ULL,
	0x0100088000001005ULL, 0x0100040429400803ULL, 0x0100050050010406ULL, 0x0040088800000141ULL,
	0x4200100e00a1300cULL, 0x4640400000002018ULL, 0x004200802800201aULL, 0x24240808440020c1ULL,
	0x081200200d00094eULL, 0x040408083030600aULL, 0x088042000c006801ULL, 0x40240208002c0043ULL,
	0x002020400100620dULL, 0x0a89010000424060ULL, 0x0011104002004030ULL, 0x40602202200100a2ULL,
	0x0201404040000906ULL, 0x04060200400b140aULL, 0x0403110005080401ULL, 0x00004040010800c1ULL,
	0x0900301900005280ULL, 0x0482483100500891ULL, 0x0008230108004030ULL, 0x0004402660200c12ULL,
	0x4010020508004108ULL, 0x010028014020140fULL, 0x11300a0102002097ULL, 0x001424d11000409bULL
};

std::array<BitBoard *, nb_squares> magic_rook_ptr;

const std::array<unsigned int, nb_squares> magic_rook_shift =
{
	20, 21, 21, 21, 21, 21, 21, 20,
	21, 22, 22, 22, 22, 22, 22, 21,
	21, 22, 22, 22, 22, 22, 22, 21,
	21, 22, 22, 22, 22, 22, 22, 21,
	21, 22, 22, 22, 22, 22, 22, 21,
	21, 22, 22, 22, 22, 22, 22, 21,
	21, 22, 22, 22, 22, 22, 22, 21,
	20, 21, 21, 21, 21, 21, 21, 20
};

const std::array<BitBoard, nb_squares> magic_bishop_mask =
{
	0x0040201008040200ULL, 0x0000402010080400ULL, 0x0000004020100a00ULL, 0x0000000040221400ULL,
	0x0000000002442800ULL, 0x0000000204085000ULL, 0x0000020408102000ULL, 0x0002040810204000ULL,
	0x0020100804020000ULL, 0x0040201008040000ULL, 0x00004020100a0000ULL, 0x0000004022140000ULL,
	0x0000000244280000ULL, 0x0000020408500000ULL, 0x0002040810200000ULL, 0x0004081020400000ULL,
	0x0010080402000200ULL, 0x0020100804000400ULL, 0x004020100a000a00ULL, 0x0000402214001400ULL,
	0x0000024428002800ULL, 0x0002040850005000ULL, 0x0004081020002000ULL, 0x0008102040004000ULL,
	0x0008040200020400ULL, 0x0010080400040800ULL, 0x0020100a000a1000ULL, 0x0040221400142200ULL,
	0x0002442800284400ULL, 0x0004085000500800ULL, 0x0008102000201000ULL, 0x0010204000402000ULL,
	0x0004020002040800ULL, 0x0008040004081000ULL, 0x00100a000a102000ULL, 0x0022140014224000ULL,
	0x0044280028440200ULL, 0x0008500050080400ULL, 0x0010200020100800ULL, 0x0020400040201000ULL,
	0x0002000204081000ULL, 0x0004000408102000ULL, 0x000a000a10204000ULL, 0x0014001422400000ULL,
	0x0028002844020000ULL, 0x0050005008040200ULL, 0x0020002010080400ULL, 0x0040004020100800ULL,
	0x0000020408102000ULL, 0x0000040810204000ULL, 0x00000a1020400000ULL, 0x0000142240000000ULL,
	0x0000284402000000ULL, 0x0000500804020000ULL, 0x0000201008040200ULL, 0x0000402010080400ULL,
	0x0002040810204000ULL, 0x0004081020400000ULL, 0x000a102040000000ULL, 0x0014224000000000ULL,
	0x0028440200000000ULL, 0x0050080402000000ULL, 0x0020100804020000ULL, 0x0040201008040200ULL
};

const std::array<BitBoard, nb_squares> magic_bishop_magic =
{
	0x141840c004ea7063ULL, 0x0700402304101106ULL, 0x00400100006440e4ULL, 0x0000000011080861ULL,
	0x4008000100040420ULL, 0x2e24000000020112ULL, 0x2420000402224544ULL, 0x08040410420a08b2ULL,
	0x1004410206001b27ULL, 0x0803004210004a84ULL, 0x04850a0211200c8cULL, 0x031000000340460aULL,
	0x2c00400006280112ULL, 0x5008048009300c98ULL, 0x0405601900141254ULL, 0x00842104200a10d3ULL,
	0x20022200080602a0ULL, 0x282170c0580810e4ULL, 0x080011000006080cULL, 0x0620424006202002ULL,
	0x02420092044a6024ULL, 0x1520410020021001ULL, 0x01a8040000040001ULL, 0x0401042000022271ULL,
	0x1010419600044400ULL, 0x0910014010011201ULL, 0x0c08002840012204ULL, 0x0401000210052210ULL,
	0x2810140000105040ULL, 0x0430040108082188ULL, 0x0048040002040080ULL, 0x0024060104011242ULL,
	0x00180500080c2424ULL, 0x2050230020420225ULL, 0x0048002000040044ULL, 0x3020210002480401ULL,
	0x0009100200186904ULL, 0x2004280000010302ULL, 0x200201000c011801ULL, 0x00b10041220204c2ULL,
	0x4444080140080414ULL, 0x0800038000220813ULL, 0x0102180203010422ULL, 0x0121210006000021ULL,
	0x0240040000040811ULL, 0x0889004000101010ULL, 0x0408110520b00109ULL, 0x08250048000802ccULL,
	0x241f10021001080aULL, 0x092a101044020455ULL, 0x0604118114040445ULL, 0x608a210002430081ULL,
	0x020900000010480eULL, 0x4403201d00203020ULL, 0x0a03400000081104ULL, 0x0079300021041022ULL,
	0x00e049210c10262bULL, 0x2091100040020061ULL, 0x0e05040040221001ULL, 0x0884040608002018ULL,
	0x5002022004201084ULL, 0x111012400a080025ULL, 0x23040c5400491002ULL, 0x07c540a004082101ULL
};

std::array<BitBoard *, nb_squares> magic_bishop_ptr;

const std::array<unsigned int, nb_squares> magic_bishop_shift =
{
	26, 27, 27, 27, 27, 27, 27, 26,
	27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 25, 25, 25, 25, 27, 27,
	27, 27, 25, 23, 23, 25, 27, 27,
	27, 27, 25, 23, 23, 25, 27, 27,
	27, 27, 25, 25, 25, 25, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27,
	26, 27, 27, 27, 27, 27, 27, 26
};

#else

const std::array<BitBoard, nb_squares> magic_rook_mask =
{
	0x000101010101017eULL, 0x000202020202027cULL, 0x000404040404047aULL, 0x0008080808080876ULL,
	0x001010101010106eULL, 0x002020202020205eULL, 0x004040404040403eULL, 0x008080808080807eULL,
	0x0001010101017e00ULL, 0x0002020202027c00ULL, 0x0004040404047a00ULL, 0x0008080808087600ULL,
	0x0010101010106e00ULL, 0x0020202020205e00ULL, 0x0040404040403e00ULL, 0x0080808080807e00ULL,
	0x00010101017e0100ULL, 0x00020202027c0200ULL, 0x00040404047a0400ULL, 0x0008080808760800ULL,
	0x00101010106e1000ULL, 0x00202020205e2000ULL, 0x00404040403e4000ULL, 0x00808080807e8000ULL,
	0x000101017e010100ULL, 0x000202027c020200ULL, 0x000404047a040400ULL, 0x0008080876080800ULL,
	0x001010106e101000ULL, 0x002020205e202000ULL, 0x004040403e404000ULL, 0x008080807e808000ULL,
	0x0001017e01010100ULL, 0x0002027c02020200ULL, 0x0004047a04040400ULL, 0x0008087608080800ULL,
	0x0010106e10101000ULL, 0x0020205e20202000ULL, 0x0040403e40404000ULL, 0x0080807e80808000ULL,
	0x00017e0101010100ULL, 0x00027c0202020200ULL, 0x00047a0404040400ULL, 0x0008760808080800ULL,
	0x00106e1010101000ULL, 0x00205e2020202000ULL, 0x00403e4040404000ULL, 0x00807e8080808000ULL,
	0x007e010101010100ULL, 0x007c020202020200ULL, 0x007a040404040400ULL, 0x0076080808080800ULL,
	0x006e101010101000ULL, 0x005e202020202000ULL, 0x003e404040404000ULL, 0x007e808080808000ULL,
	0x7e01010101010100ULL, 0x7c02020202020200ULL, 0x7a04040404040400ULL, 0x7608080808080800ULL,
	0x6e10101010101000ULL, 0x5e20202020202000ULL, 0x3e40404040404000ULL, 0x7e80808080808000ULL
};

const std::array<BitBoard, nb_squares> magic_rook_magic =
{
	0x0180001420804001ULL, 0x2040200010004000ULL, 0x6100082001001441ULL, 0x2100180420100100ULL,
	0x6200040200d00820ULL, 0x1600110200041008ULL, 0x4880408001000200ULL, 0x1180008000c02100ULL,
	0x0102002200410082ULL, 0x10c1002140010180ULL, 0x0111002000704100ULL, 0x0802002200401008ULL,
	0x0242003422001008ULL, 0x03120008041a0150ULL, 0x4042000102000408ULL, 0x12550001000a40a2ULL,
	0x0008688000401180ULL, 0x221000c010406008ULL, 0x4050008011200080ULL, 0x0801010060100008ULL,
	0x0442020008902044ULL, 0x0101010002040018ULL, 0x0020040008021001ULL, 0x1402020004006195ULL,
	0x0280184040002000ULL, 0x0101008d004000a0ULL, 0x0001004300200012ULL, 0x0100100100202902ULL,
	0x0002100500080100ULL, 0x0045004900040082ULL, 0x0010020400101548ULL, 0x00012042000c0081ULL,
	0x08a0244000800080ULL, 0x00d00a2010400042ULL, 0x0010040802200020ULL, 0x000200200a005040ULL,
	0x0018001101000428ULL, 0x1002009002002c18ULL, 0x0200100204000809ULL, 0x0001011142002084ULL,
	0x048000a000404010ULL, 0x0120100040044020ULL, 0x38a5002002490010ULL, 0x0050050010210008ULL,
	0x0044000800110100ULL, 0x2102001004020008ULL, 0x18a2008805020014ULL, 0x0001468100460004ULL,
	0x1082708000400080ULL, 0x0010004002200840ULL, 0x4020044110210100ULL, 0x0010081000210100ULL,
	0x0080101500080100ULL, 0x010100080c000300ULL, 0x2001001402000100ULL, 0x0401000153920900ULL,
	0x0080204111860102ULL, 0x0203002140081281ULL, 0x0200408873002001ULL, 0x20400410010008a1ULL,
	0x0801001004280083ULL, 0x0002001008010482ULL, 0x0001000400d20005ULL, 0x4004008401014022ULL
};

std::array<BitBoard *, nb_squares> magic_rook_ptr;

const std::array<unsigned int, nb_squares> magic_rook_shift =
{
	52, 53, 53, 53, 53, 53, 53, 52,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	53, 54, 54, 54, 54, 54, 54, 53,
	52, 53, 53, 53, 53, 53, 53, 52
};

const std::array<BitBoard, nb_squares> magic_bishop_mask =
{
	0x0040201008040200ULL, 0x0000402010080400ULL, 0x0000004020100a00ULL, 0x0000000040221400ULL,
	0x0000000002442800ULL, 0x0000000204085000ULL, 0x0000020408102000ULL, 0x0002040810204000ULL,
	0x0020100804020000ULL, 0x0040201008040000ULL, 0x00004020100a0000ULL, 0x0000004022140000ULL,
	0x0000000244280000ULL, 0x0000020408500000ULL, 0x0002040810200000ULL, 0x0004081020400000ULL,
	0x0010080402000200ULL, 0x0020100804000400ULL, 0x004020100a000a00ULL, 0x0000402214001400ULL,
	0x0000024428002800ULL, 0x0002040850005000ULL, 0x0004081020002000ULL, 0x0008102040004000ULL,
	0x0008040200020400ULL, 0x0010080400040800ULL, 0x0020100a000a1000ULL, 0x0040221400142200ULL,
	0x0002442800284400ULL, 0x0004085000500800ULL, 0x0008102000201000ULL, 0x0010204000402000ULL,
	0x0004020002040800ULL, 0x0008040004081000ULL, 0x00100a000a102000ULL, 0x0022140014224000ULL,
	0x0044280028440200ULL, 0x0008500050080400ULL, 0x0010200020100800ULL, 0x0020400040201000ULL,
	0x0002000204081000ULL, 0x0004000408102000ULL, 0x000a000a10204000ULL, 0x0014001422400000ULL,
	0x0028002844020000ULL, 0x0050005008040200ULL, 0x0020002010080400ULL, 0x0040004020100800ULL,
	0x0000020408102000ULL, 0x0000040810204000ULL, 0x00000a1020400000ULL, 0x0000142240000000ULL,
	0x0000284402000000ULL, 0x0000500804020000ULL, 0x0000201008040200ULL, 0x0000402010080400ULL,
	0x0002040810204000ULL, 0x0004081020400000ULL, 0x000a102040000000ULL, 0x0014224000000000ULL,
	0x0028440200000000ULL, 0x0050080402000000ULL, 0x0020100804020000ULL, 0x0040201008040200ULL
};

const std::array<BitBoard, nb_squares> magic_bishop_magic =
{
	0x0da0200a428908b0ULL, 0x0203100c01007003ULL, 0x0208080102300b24ULL, 0x0088204040600000ULL,
	0x01042c2068804080ULL, 0x4106170420190200ULL, 0x6084122964201028ULL, 0x2022482207304000ULL,
	0x0400224a0a280122ULL, 0x4c02058818010070ULL, 0x4042104c41902040ULL, 0x4200084841000200ULL,
	0x01000c0b08220400ULL, 0x300b0104606d0020ULL, 0x608400a404200e00ULL, 0x0400010328020290ULL,
	0x0021001002100104ULL, 0x0108005001630421ULL, 0x0020401005014012ULL, 0x4008000120404000ULL,
	0x00040102060a0002ULL, 0x020a0041004202a0ULL, 0x4104204202866801ULL, 0x5081400824040400ULL,
	0x0809200040040104ULL, 0x0c04200002080900ULL, 0x1008045006020200ULL, 0x0044010000200880ULL,
	0x20600a0000c05000ULL, 0x2001004102082002ULL, 0x4001020000e80c08ULL, 0x0064004020821082ULL,
	0x0004108500404404ULL, 0x0002109000445b22ULL, 0x12020082001021e4ULL, 0x0864201100880184ULL,
	0x008c040400025100ULL, 0x0001080220020211ULL, 0x0002008100022804ULL, 0x0222048304212400ULL,
	0x0004100c100c0400ULL, 0x08004c26204008c4ULL, 0x1006010402000100ULL, 0x0601044010460a02ULL,
	0x0108280500400400ULL, 0x0004250042000100ULL, 0x68083014009240c0ULL, 0x0803014062040100ULL,
	0x002201100ad201c4ULL, 0x010025052821002cULL, 0x4000004602d02000ULL, 0x3000000042020000ULL,
	0x0800003002020000ULL, 0x0100187021421014ULL, 0x103b1010009128c0ULL, 0x1020028c10c03208ULL,
	0x484a486610100402ULL, 0x1840321c60980880ULL, 0x0000200064260834ULL, 0x10000102089c0400ULL,
	0x0000060010020200ULL, 0x040200c006ac0d10ULL, 0x08801420360240a0ULL, 0x0042440f28020288ULL
};

std::array<BitBoard *, nb_squares> magic_bishop_ptr;

const std::array<unsigned int, nb_squares> magic_bishop_shift =
{
	58, 59, 59, 59, 59, 59, 59, 58,
	59, 59, 59, 59, 59, 59, 59, 59,
	59, 59, 57, 57, 57, 57, 59, 59,
	59, 59, 57, 55, 55, 57, 59, 59,
	59, 59, 57, 55, 55, 57, 59, 59,
	59, 59, 57, 57, 57, 57, 59, 59,
	59, 59, 59, 59, 59, 59, 59, 59,
	58, 59, 59, 59, 59, 59, 59, 58
};

#endif // SYSTEM64BIT

// size of the magic keys
#ifdef SYSTEM64BIT
constexpr int magic_key_size = 64;
#else
constexpr int magic_key_size = 32;
#endif

// bishop magic table size
constexpr int magictable_bishop_size = 5248;

// rook magic table size
constexpr int magictable_rook_size = 102400;

// magic tables
std::array<BitBoard, magictable_rook_size> magicTableRook;

std::array<BitBoard, magictable_bishop_size> magicTableBishop;

// add a square bitboard if the destination is valid
constexpr void add_valid_square(Square sq, int dx, int dy, BitBoard &bb)
{
	int f = static_cast<int>(squareFile(sq)) + dx;
	int r = static_cast<int>(squareRank(sq)) + dy;

	if ((f >= 0) &&
		(f < nb_files) &&
		(r >= 0) &&
		(r < nb_ranks)) {
		bb |= square[sq + static_cast<SquareDir>(dx + dy * nb_files)];
	}
}

// pawn attacks
const std::array<std::array<BitBoard, nb_squares>, nb_sides> pawn_attack = 
[]() {
	std::array<std::array<BitBoard, nb_squares>, nb_sides> table {};
	for (Square sq=a1; sq<=h8; ++sq) {
		add_valid_square(sq, -1,  1, table[white][sq]);
		add_valid_square(sq,  1,  1, table[white][sq]);
		add_valid_square(sq, -1, -1, table[black][sq]);
		add_valid_square(sq,  1, -1, table[black][sq]);
	}
	return table;
} ();

// knight attacks
const std::array<BitBoard,nb_squares> knight_attack = []() {
	std::array<BitBoard,nb_squares> table {};
	for (Square sq=a1; sq<=h8; ++sq) {
		add_valid_square(sq, -1,  2, table[sq]);
		add_valid_square(sq,  1,  2, table[sq]);
		add_valid_square(sq, -1, -2, table[sq]);
		add_valid_square(sq,  1, -2, table[sq]);
		add_valid_square(sq,  2, -1, table[sq]);
		add_valid_square(sq,  2,  1, table[sq]);
		add_valid_square(sq, -2, -1, table[sq]);
		add_valid_square(sq, -2,  1, table[sq]);
	}
	return table;
} ();

// king attacks
const std::array<BitBoard, nb_squares> king_attack = []() {
	std::array<BitBoard, nb_squares> table {};
	for (Square sq=a1; sq<=h8; ++sq) {
		add_valid_square(sq, -1, -1, table[sq]);
		add_valid_square(sq, -1,  0, table[sq]);
		add_valid_square(sq, -1,  1, table[sq]);
		add_valid_square(sq,  0, -1, table[sq]);
		add_valid_square(sq,  0,  1, table[sq]);
		add_valid_square(sq,  1, -1, table[sq]);
		add_valid_square(sq,  1,  0, table[sq]);
		add_valid_square(sq,  1,  1, table[sq]);
	}
	return table;
} ();

constexpr BitBoard square_index(int f, int r) {
	return (1ULL << (r * nb_files + f));
};

// bitboards for a single square
const std::array<BitBoard, nb_squares> square = []() {
	std::array<BitBoard, nb_squares> table {};
	for (int n=0; n<nb_squares; ++n) {
		table[n] = (1ULL << n);
	}
	return table;
} ();

// all diagonal 45 R
const std::array<BitBoard, nb_squares> diag_45r = []() {
	constexpr SquareDir ur = dir_up + dir_right;
	std::array<BitBoard, nb_squares> table {};
	for (Square p=a1; p<=h8; ++p) {

		table[p] = square[p];

		// up right
		for (Square n=p+ur; ((n <= h8) && 
			(squareFile(n) > squareFile(p))); n+=ur) {
			table[p] |= square[n];
		}

		// down left
		for (Square n=p-ur; ((n >= a1) && 
			(squareFile(n) < squareFile(p))); n-=ur) {
			table[p] |= square[n];
		}
	}
	return table;
} ();

// all diagonal 45 L
const std::array<BitBoard, nb_squares> diag_45l = []() {
	constexpr SquareDir ul = dir_up + dir_left;
	std::array<BitBoard, nb_squares> table {};
	for (Square p=a1; p<=h8; ++p) {

		table[p] = square[p];

		// up left
		for (Square n=p-ul; ((n >= a1) && 
			(squareFile(n) > squareFile(p))); n-=ul) {
			table[p] |= square[n];
		}

		// down right
		for (Square n=p+ul; ((n <= h8) && 
			(squareFile(n) < squareFile(p))); n+=ul) {
			table[p] |= square[n];
		}
	}
	return table;
} ();

const std::array<std::array<BitBoard, nb_squares>, nb_squares> between = []() {
	std::array<std::array<BitBoard, nb_squares>, nb_squares> table;
	for (Square y=a1; y<=h8; ++y) {

		Rank yr = squareRank(y);
		File yf = squareFile(y);

		for (Square x=a1; x<=h8; ++x) {

			Rank xr = squareRank(x);
			File xf = squareFile(x);
			int	dx = xf - yf;
			int	dy = xr - yr;

			table[y][x] = 0ULL;
			if ((dx != 0) || (dy != 0)) {

				if ((std::abs(dx) == std::abs(dy)) ||
					(dx == 0) || (dy == 0)) {

					int nb = (dx != 0) ? std::abs(dx) : std::abs(dy);
					int sx = 0;
					int sy = 0;
					if (dx != 0) {
						sx = (dx > 0) ? 1 : -1;
					}
					if (dy != 0) {
						sy = (dy > 0) ? 1 : -1;
					}

					for (int n=1; n<nb; n++) {
						table[y][x] |=
							square[makeSquare(yf + sx * n, yr + sy * n)];
					}
				}
			}
		}
	}
	return table;
} ();

// direction from a square to another
const std::array<std::array<std::int8_t, nb_squares>, nb_squares> direction = 
[]() {
	std::array<std::array<std::int8_t, nb_squares>, nb_squares> table {};
	for (Square x=a1; x<=h8; ++x) {
		for (Square y=a1; y<=h8; ++y) {

			int	dx = squareFile(x) - squareFile(y);
			int	dy = squareRank(x) - squareRank(y);

			// default = no direction
			SquareDir d = dir_none;

			if (dx < 0) {
				d = d + dir_left;
			}
			if (dx > 0) {
				d = d + dir_right;
			}
			if (dy < 0) {
				d = d + dir_down;
			}
			if (dy > 0) {
				d = d + dir_up;
			}

			table[y][x] = static_cast<std::int8_t>(d);
		}
	}
	return table;
} ();

// files bitboards
const std::array<BitBoard, nb_files> file = []() {
	std::array<BitBoard, nb_files> table {};
	for (int f=0; f<nb_files; ++f) {
		BitBoard bb = 0ULL;
		for (int r=0; r<nb_ranks; ++r) {
			bb |= square_index(f, r);
		}
		table[f] = bb;
	}
	return table;
} ();

// ranks bitboards
const std::array<BitBoard, nb_ranks> rank = []() {
	std::array<BitBoard, nb_ranks> table {};	
	for (int r=0; r<nb_ranks; ++r) {
		BitBoard bb = 0ULL;
		for (int f=0; f<nb_files; ++f) {
			bb |= square_index(f, r);
		}
		table[r] = bb;
	}
	return table;
} ();

// bitboards for files around
const std::array<BitBoard, nb_files> files_around = []() {
	std::array<BitBoard, nb_files> table {};
	for (int f=0; f<nb_files; ++f) {
		BitBoard bb = 0ULL;
		for (int r=0; r<nb_ranks; ++r) {
			if (f > 0) {
				bb |= square_index(f - 1, r);
			}
			if (f < (nb_files - 1)) {
				bb |= square_index(f + 1, r);
			}
		}
		table[f] = bb;
	}
	return table;
} ();

// all white squares
const BitBoard white_squares = []() {
	BitBoard bb = 0ULL;
	for (int f=0; f<nb_files; ++f) {
		for (int r=0; r<nb_ranks; ++r) {
			if (((f + r + 1) % 2) != 0) {
				bb |= square_index(f, r);
			}
		}
	}
	return bb;
} ();

// all black squares
const BitBoard black_squares = []() {
	BitBoard bb = 0ULL;
	for (int f=0; f<nb_files; ++f) {
		for (int r=0; r<nb_ranks; ++r) {
			if (((f + r) % 2) != 0) {
				bb |= square_index(f, r);
			}
		}
	}
	return bb;
} ();

constexpr BitBoard BitBoardFull = std::numeric_limits<std::uint64_t>::max();

constexpr int max_sliding_dir = 4;
using ArrayPaths = std::array<std::pair<SquareDir, int>, max_sliding_dir>;

////////////////////////////////////////////////////////////////////////////////
constexpr BitBoard SlidingAttack(Square sq, BitBoard occ, 
	const ArrayPaths &paths) {
	BitBoard bb = 0ULL;
	for (const auto & [dir, count] : paths) {
		Square p = sq;
		for (int n=0; n<count; n++) {
			p += dir;
			bb |= square[p];
			if ((occ & square[p]) != 0ULL) {
				break;
			}
		}
	}
	return bb;	
}

////////////////////////////////////////////////////////////////////////////////
BitBoard RookAttack(Square sq, BitBoard occ)
{
	int f1 = static_cast<int>(squareFile(sq));
	int f2 = static_cast<int>(file_h) - f1;
	int r1 = static_cast<int>(squareRank(sq));
	int r2 = static_cast<int>(rank_8) - r1;

	const ArrayPaths paths = {{
		{ dir_right, f2 },
		{ dir_left,  f1 },
		{ dir_up,    r2 },
		{ dir_down,  r1 }
	}};

	return SlidingAttack(sq, occ, paths);
}

////////////////////////////////////////////////////////////////////////////////
// generate bishop attack without using bitboard (slow)
BitBoard BishopAttack(Square sq, BitBoard occ)
{
	int f1 = static_cast<int>(squareFile(sq));
	int f2 = static_cast<int>(file_h) - f1;
	int r1 = static_cast<int>(squareRank(sq));
	int r2 = static_cast<int>(rank_8) - r1;

	const ArrayPaths paths = {{
		{ dir_up_right,   std::min(f2, r2) },
		{ dir_up_left,    std::min(f1, r2) },
		{ dir_down_right, std::min(f2, r1) },
		{ dir_down_left,  std::min(f1, r1) }
	}};

	return SlidingAttack(sq, occ, paths);
}

////////////////////////////////////////////////////////////////////////////////
// Generate magic table from magic keys & shifts
template<Piece type, int size>
void createMagicTable(std::array<BitBoard, size> &data,
	std::array<BitBoard *, nb_squares> &ptr, 
	const std::array<BitBoard, nb_squares> &mask, 
	const std::array<BitBoard, nb_squares> &magic,
	const std::array<unsigned int, nb_squares> &shift)
{
	static_assert((type == bishop) || (type == rook));

	data = {};
	unsigned int dbOffset = 0;

	for (Square n=a1; n<=h8; ++n) {

		ptr[n] = &data[dbOffset];

		BitBoard occ = 0;
		BitBoard oset = mask[n];

		do {
			auto index = magicAttack(occ, oset, magic[n], shift[n]);
			data[dbOffset + index] = (type == bishop) ?
				BishopAttack(n, occ) : RookAttack(n, occ);
			occ = (occ - oset) & oset;
		} while (occ != 0);

		dbOffset += (1 << (magic_key_size - shift[n]));
	}
}

////////////////////////////////////////////////////////////////////////////////
// Generate magic tables for bishop and rook
void createMagicTables()
{
	createMagicTable<bishop, magictable_bishop_size>(magicTableBishop, 
		magic_bishop_ptr, magic_bishop_mask, magic_bishop_magic,
		magic_bishop_shift);

	createMagicTable<rook, magictable_rook_size>(magicTableRook, 
		magic_rook_ptr, magic_rook_mask, magic_rook_magic,
		magic_rook_shift);
}

////////////////////////////////////////////////////////////////////////////////
constexpr BitBoard filterBishopAttack(BitBoard occ)
{
	return occ & ~(file[file_a] | file[file_h] | rank[rank_1] | rank[rank_8]);
}

////////////////////////////////////////////////////////////////////////////////
constexpr BitBoard filterRookAttack(BitBoard occ, Square sq)
{
	BitBoard o = occ;
	if (squareFile(sq) != file_a) {
		o &= ~file[file_a];
	}
	if (squareFile(sq) != file_h) {
		o &= ~file[file_h];
	}
	if (squareRank(sq) != rank_1) {
		o &= ~rank[rank_1];
	}
	if (squareRank(sq) != rank_8) {
		o &= ~rank[rank_8];
	}
	return o;
}

////////////////////////////////////////////////////////////////////////////////
bool testMagicKey(Square sq, BitBoard magickey, int numbit, Piece type,
	unsigned int &testcount, int &testcollision, std::uint64_t &maxvalue)
{
	constexpr std::size_t max_index = (1 << 12);

	// max size = 1 << (64 - numbit)
	std::array<std::uint64_t, max_index>	indexList;
	indexList.fill(BitBoardFull);

	BitBoard occ = 0;

	// possible attack on the square for the piece on an empty board
	// (but we can remove borders for sliding pieces)
	BitBoard oset = (type == bishop) ?
		filterBishopAttack(BishopAttack(sq, occ)) :
		filterRookAttack(RookAttack(sq, occ), sq);

	unsigned int count = 0;
	int collision = 0;
	std::uint64_t maxval = 0;

	bool r = true;
	do {

		auto index = magicAttack(occ, oset, magickey, numbit);

		if (index > maxval) {
			maxval = index;
		}

		BitBoard bAtt = (type == bishop)  ?
			BishopAttack(sq, occ) :
			RookAttack(sq, occ);

		if (indexList[index] == BitBoardFull) {
			indexList[index] = bAtt;
			++count;
		} else
		if (indexList[index] == bAtt) {
			++collision;
		} else {
			r = false;
			break;
		}

		occ = (occ - oset) & oset;

	} while (occ != 0);

	testcount = count;
	testcollision = collision;
	maxvalue = maxval;

	return r;
}

////////////////////////////////////////////////////////////////////////////////
int findMagicKey(Square sq, unsigned int nb, int nbbit, Piece type,
	BitBoard &magic, unsigned int &shift, std::mt19937_64 &rnd)
{
	BitBoard bestKey = 0;
	std::uint64_t maxval = 0;
	std::uint64_t bestmaxvalue = 0;
	unsigned int testcount = 0;
	unsigned int bestCount = 0;
	int testcollision = 0;
	int bestNumBit = 0;
	int bestCollision = -1;
	int	numbit = magic_key_size - nbbit;
	int max_bits = 12;

	std::uniform_int_distribution<std::uint64_t> dist;

	while (numbit >= (magic_key_size - max_bits)) {

		unsigned int i = 0;

		while (i < nb) {

			BitBoard magickey = dist(rnd) & dist(rnd) & dist(rnd);

			// test the magic key & bit
			if (testMagicKey(sq, magickey, numbit, type, testcount,
				testcollision, maxval) == 1) {

				if (maxval > bestmaxvalue) {
					bestmaxvalue = maxval;
				}

				if (testcollision > bestCollision) {
					bestKey = magickey;
					bestNumBit = numbit;
					bestCount = testcount;
					bestCollision = testcollision;
				}
			}

			++i;
		}

		if (bestCollision != -1) {
			break;
		}

		--numbit;
	}

	magic = bestKey;
	shift = bestNumBit;

	Logger::debug() << "Square "
			  << ChessMoveParser::square_to_string(sq)
			  << " : "
			  << "0x" << std::setfill('0') << std::setw(16) << std::hex
			  << bestKey << std::dec
			  << " with " << bestNumBit << " bits (count = "
			  << bestCount << ", collision = " << bestCollision << ')';

	// return the size we need in the table
	return (1 << (magic_key_size - bestNumBit));
}

////////////////////////////////////////////////////////////////////////////////
void print_bitboard_array(const std::array<BitBoard, nb_squares> &data, 
	int nb)
{
	std::cout << '\n';
	for (int p=0; p<nb_squares; ++p) {
		if ((p != 0) && ((p % nb) == 0)) {
			std::cout << '\n';
		}
		std::cout << "0x" << std::setfill('0') << std::setw(16) << std::hex
				  << data[p] << "ULL, ";
	}
	std::cout << '\n';
}

////////////////////////////////////////////////////////////////////////////////
void print_shift_array(const std::array<unsigned int, nb_squares> &data, 
	int nb)
{
	std::cout << '\n';
	for (int p=0; p<nb_squares; ++p) {
		if ((p != 0) && ((p % nb) == 0)) {
			std::cout << '\n';
		}
		std::cout << std::dec << data[p] << ", ";
	}
	std::cout << '\n';
}

////////////////////////////////////////////////////////////////////////////////
void buildMagicTables()
{
	std::array<BitBoard, nb_squares> table_rook_mask;
	std::array<BitBoard, nb_squares> table_rook_magic;
	std::array<unsigned int, nb_squares> table_rook_shift;
	std::array<BitBoard, nb_squares> table_bishop_mask;
	std::array<BitBoard, nb_squares> table_bishop_magic;
	std::array<unsigned int, nb_squares> table_bishop_shift;

	std::random_device rd;	
	std::seed_seq sd { rd(), rd(), rd(), rd() };
	std::mt19937_64 mtrnd(sd);

	unsigned int szb = 0;
	unsigned int szr = 0;

	constexpr unsigned int max_search = 1000000;
	constexpr int nb_bits_bishop = 5;
	constexpr int nb_bits_rook = 10;

	// generate tables
	for (Square p=a1; p<=h8; ++p) {
		szb += findMagicKey(p, max_search, nb_bits_bishop, bishop, 
			table_bishop_magic[p], table_bishop_shift[p], mtrnd);
	}

	for (Square n=a1; n<=h8; ++n) {
		table_bishop_mask[n] = filterBishopAttack(BishopAttack(n, 0ULL));
	}

	for (Square p=a1; p<=h8; ++p) {
		szr += findMagicKey(p, max_search, nb_bits_rook, rook,
			table_rook_magic[p], table_rook_shift[p], mtrnd);
	}

	for (Square n=a1; n<=h8; ++n) {
		table_rook_mask[n] = filterRookAttack(RookAttack(n, 0ULL), n);
	}

	// print results
	std::cout << "\nbishop table size = " << std::dec << szb << '\n';

	std::cout << "\nbishop mask : \n";
	print_bitboard_array(table_bishop_mask, 4);

	std::cout << "\nbishop magic : \n";
	print_bitboard_array(table_bishop_magic, 4);

	std::cout << "\nbishop shift : \n";
	print_shift_array(table_bishop_shift, 8);

	std::cout << "\nrook table size = " << std::dec << szr << '\n';

	std::cout << "\nrook mask : \n";
	print_bitboard_array(table_rook_mask, 4);

	std::cout << "\nrook magic : \n";
	print_bitboard_array(table_rook_magic, 4);

	std::cout << "\nrook shift : \n";
	print_shift_array(table_rook_shift, 8);

	std::cout << std::endl;	
}

////////////////////////////////////////////////////////////////////////////////
void init()
{
	if constexpr (find_magic_keys) {

		Logger::debug() << "Init Bitboards";
		Timer timer;
		timer.start();

		buildMagicTables();

		auto timesearch = timer.getElapsedTime();
		Logger::debug() << "time " << Timer::timeToString(timesearch) << " ms";
	}

	createMagicTables();
}

////////////////////////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &os, const OutputBitBoard &bb)
{
	const std::string_view strline = "|---|---|---|---|---|---|---|---|";
	os << std::setfill('0') << std::setw(16) << std::hex << bb.value;
	std::string tmp;
	os << '\n' << strline << '\n';
	for (Rank y=rank_1; y<=rank_8; ++y) {
		tmp = '|';
		for (File x=file_a; x<=file_h; ++x) {
			tmp += ' ';
			Square sq = makeSquare(x, static_cast<Rank>(rank_8 - y));
			tmp += ((bb.value & BitBoards::square[sq]) != 0ULL) ? 'x' : ' ';
			tmp += ' ';
			tmp += '|';
		}
		tmp += '\n';
		os << tmp;
		os << strline << '\n';
	}
	os << std::endl;
	return os;
}

} // namespace Cheese::BitBoards
