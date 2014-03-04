#ifndef __LL_CHALLENGE__
#define __LL_CHALLENGE__

#ifdef __cplusplus
extern "C" {
#endif

#define kBoardSize 10

typedef enum { kUnknown=0,
	kMarshall=1,kGeneral,kColonel,kMajor,kCaptain,
	kLieutenant,kSergeant,kMiner,kScout,kSpy,
	kBomb,kFlag,
	
	kEmpty,
	kWater,
	kMoved, // fake rank for moved pieces
	kAddForRankish // add this in for enemies when calculating the CRC
} PieceRank;

#define kPieceCharacters "0123456789sBF.+MA"

typedef enum {kNoColor, kRed, kBlue} PlayerColor;

typedef struct PieceType {
	PieceRank	thePieceRank;  /* rank of a piece */
	PlayerColor thePieceColor; /* color of a piece */
} PieceType;

typedef PieceType Board[kBoardSize][kBoardSize];
/* Used to provide test code with board configuration.  Red starts 
   in rows 0..3, Blue starts in rows 6..9 */
/* Squares [4][2], [4][3], [4][6], [4][7] and
           [5][2], [5][3], [5][6], [5][7] are water and cannot
           be occupied */

typedef struct PiecePosition {
	long row;  /* 0..9 */
	long col;  /* 0..9 */
} PiecePosition;

typedef struct MoveResult {
	PieceType rankOfAttacker;
	/* after a strike, returns identity of attacker */
	PieceType rankOfDefender;
	/* after a strike, returns identity of defender */
	Boolean attackerRemoved;
	/* true after a strike against a piece of equal or greater rank,
	   or against a bomb when the attacker is not a Miner */
	Boolean defenderRemoved;
	/* true after a strike by a piece of equal or greater rank,
	   or against a bomb when the attacker is a Miner, 
	   or against a Marshall by a Spy */
	Boolean victory;
	/* true after a strike against the Flag */
	Boolean legalMove;
	/* true unless you
	     - move into an occupied square, or 
	     - move or strike in a direction other than forward, backward, or sideways, or
	     - move more than one square (except Scouts), or
	     - move a Bomb or a Flag,
	     - move into Water, or
	     - strike a square not occupied by an opponent, or
	     - make any other illegal move  */
} MoveResult;

#ifdef __cplusplus
}
#endif

#endif
