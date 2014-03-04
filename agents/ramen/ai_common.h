/*
 * UCC 2012 Programming Competition Entry
 * - "Ramen"
 *
 * By John Hodge [TPG]
 */
#ifndef _AI_COMMON_H_
#define _AI_COMMON_H_

#include "interface.h"

#define N_PIECES	40

enum eRanks
{
	RANK_UNKNOWN,	//  0
	RANK_MARSHAL,	//  1
	RANK_GENERAL,	//  2
	RANK_COLONEL,	//  3
	RANK_MAJOR,	//  4
	RANK_CAPTAIN,	//  5
	RANK_LIEUTENANT,//  6
	RANK_SERGEANT,	//  7
	RANK_MINER,	//  8
	RANK_SCOUT,	//  9
	RANK_SPY,	// 10
	RANK_BOMB,	// 11
	RANK_FLAG,	// 12
	N_RANKS
};
static const int MAX_RANK_COUNTS[N_RANKS] = {
	40, 1, 1, 2, 3, 4, 4, 4, 5, 8, 1, 6, 1
};
static const char cRANK_CHARS[N_RANKS] = "#123456789sBF";
static inline enum eRanks CharToRank(char ch)
{
	switch(ch)
	{
	case '1':	return RANK_MARSHAL;
	case '2':	return RANK_GENERAL;
	case '3':	return RANK_COLONEL;
	case '4':	return RANK_MAJOR;
	case '5':	return RANK_CAPTAIN;
	case '6':	return RANK_LIEUTENANT;
	case '7':	return RANK_SERGEANT;
	case '8':	return RANK_MINER;
	case '9':	return RANK_SCOUT;
	case 's':	return RANK_SPY;
	case 'B':	return RANK_BOMB;
	case 'F':	return RANK_FLAG;
	case '#':	return RANK_UNKNOWN;
	default:
		// Wut. Unkown
		DEBUG("Unknown character '%c'", ch);
		return RANK_UNKNOWN;
	}
}

/**
 */
typedef struct sPiece
{
	 int	X, Y;
	BOOL	bDead;
	enum eRanks	Rank;	// -1 = unknown
	BOOL	bHasMoved;
	enum eColours	Team;
	// TODO: Keep last moved
	
	BOOL	bExposed;	// Marks when the piece is known by the other team

	 int	StartX, StartY;	// Used to save initial layout
	enum eRanks	GuessedRank;	// Only used it bGuessValid is set
} tPiece;

typedef struct sPlayerStats
{
	enum eColours	Colour;
	 int	nPieces;
	 int	nMoved;
	 int	nIdentified;
	 int	nRanks[N_RANKS];
	 int	nKilledRanks[N_RANKS];
	tPiece	Pieces[N_PIECES];
	BOOL	bGuessValid;
} tPlayerStats;

typedef struct sPieceRef
{
	char	Index;	// Index into tPlayerStats.Pieces
	char	Team;	// 0 = Empty, 1 = Me, 2 = Opponent, 3 = Block
} tPieceRef;

typedef struct sGameState
{
	tPlayerStats	Opponent;
	tPlayerStats	MyExposed;
	tPlayerStats	MyActual;
	tPieceRef	BoardState[];	// 
} tGameState;

// --- Database
extern char	*DB_GetOpponentFile(const char *Opponent);
extern void	DB_LoadGuesses(const char *DBFile, enum eColours Colour);
extern void	DB_WriteBackInitialState(const char *DBFile, enum eColours Colour, tPiece *Pieces);

#endif

