#define ASSERTIONS 1
#define DEBUG_RULES 0
#define DEBUG 0
#define SAVE_OUTPUT 1

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cstdlib>
#include <iostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <sstream>

using namespace std;    // two of two, yay!

typedef bool Boolean;
typedef unsigned long UInt32;

#include "peternlewis.h"

const char *gPieceCharacters = kPieceCharacters;
#if SAVE_OUTPUT
ofstream gOutputFile;
#endif

template <class T>
inline std::string to_string (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}


/*
	Author: Peter N Lewis
	
	Originally: Submitted in 1997 to MacTech Programming Challenge and won.
	<http://www.mactech.com/articles/mactech/Vol.13/13.11/Nov97Challenge/index.html>

	Assumptions:
		Only time we spend thinking is counted against out 10 seconds (not time in GetMove/ReportMove)
		
	Method:
		Basically we keep track of the board and what we know and what they know.
		Each opponent piece has a bit map associated with it describing what pieces it could be.
		As we see more pieces, the bit map is culled.  If the piece moves, the bomb & flag bits are removed.
		If we've seen all Scouts (for example), then the Scout bit is removed from all remaining pieces.
		If all but one bit is remvoed, then we know what the piece is.
		
		At each turn, we simply apply a sequence of actions (listed below) and take the first action that works.
		
		It does very little in the way of lookahead (it plans out a path, but doesn't remember it and
		doesn't take it to account any movement by the opposition)
		
		It keeps a CRC of recent board positions (since the last strike) and doesn't replay any boards
		(we want to win, not draw!).
		
		If we exceed 10 seconds thinking time, we resign.  Not that this is particularly likely,
		in the games I tried, it spend less than half a second total.

	Optimizations:
		None.
	
	Comment:
		It actually plays a half decent game!  The end game is not as good as I'd like, but time is up!
*/

/*
USE SPY
	If our spy is next to their 1, kill it
	
DEFEND AGAINST SPY
	if we have seen the spy, ignore this case
	
	If an unknown piece is next to the 1, then
		run, attack, have another piece attack, or ignore depending on a table

ATTACK WEAKER
	If a known piece is next to a weaker known piece, attack it
		except if it places that piece in a dangerous location

EXPLORE ATTACK
	If a 6,7,9 is next to an unknown piece, attack it

RETREAT
	If a known piece is next to a stronger known piece, run away
		(preferably towards something that can kill it
		or if it's lowly, towards an unknown piece)
		
SCOUT
	Try advancing scouts rapidly

ATTACK DISTANT
	If a known piece is distant, but a clear path leads a slightly better piece towards it, advance the better piece
		(includes miners)

EXPLORE DISTANT
	Try exploring (advance lowly pieces towards unknown pieces)

ATTACK KNOWN WITH SAME DISTANT
	If a known piece can be attacked by a known identical piece, attack it
	
FIND FLAG
	When few unmoved pieces remain, start assuming they are bombs/flags

MOVE FORWARD
	Move any piece we can forward

MOVE
	Move any piece we can

RESIGN
	Give up
*/

static void Output( string what )
{
#if SAVE_OUTPUT
	gOutputFile << "<< " << what << "\n";
#endif
	cout << what << "\n";
}

static void SaveInput( string what )
{
#if SAVE_OUTPUT
	gOutputFile << ">> " << what << "\n";
#endif
}

static void Log( string what )
{
#if SAVE_OUTPUT
	gOutputFile << what << "\n";
#endif
	cerr << what << "\n";
}

static void Debug( string what )
{
#if SAVE_OUTPUT
	gOutputFile << what << "\n";
#endif
#if DEBUG
	cerr << what << "\n";
#endif
}

static void Assert( short must, string what )
{
	if ( !must ) {
#if ASSERTIONS
		Log( string("Assert failed! ") + what + "\n" );
#endif
	}
}

std::vector<string> gLineBuffer;

enum {
	kNoNothing = 0x00001FFE,
	kStationaryBits = ((1 << kBomb) | (1 << kFlag))
};

enum {
	kRepeatedBoards = 1000
};

typedef struct Square {
	PlayerColor color;
	PieceRank rank;
	UInt32 possibilities;
} Square;

typedef Square OurBoard[kBoardSize][kBoardSize];

typedef int Counts[kFlag+1];

typedef UInt32 BoardPossibilities[kBoardSize][kBoardSize];

typedef struct Storage {
	OurBoard board;
	Counts our_pieces;
	Counts their_pieces;
	Boolean do_getmove;
	Boolean victory;
	Square blankSquare;
	PlayerColor playerColor;
	PlayerColor theirColor;
	BoardPossibilities dangers;
	BoardPossibilities known_dangers;
	UInt32 repeated_board[kRepeatedBoards];
	UInt32 repeated_board_count;
} Storage, *StoragePtr;

static void DumpBoard( StoragePtr storage )
{
#if DEBUG
	Debug( "DumpBoard:" );
	for ( int row = 0; row < kBoardSize; row++ ) {
		string line;
		for ( int col = 0; col < kBoardSize; col++ ) {
			PieceRank rank = storage->board[row][col].rank;
			if ( kMarshall <= rank && rank <= kFlag ) {
				char left, right;
				if ( storage->board[row][col].color == kRed ) {
					left = '{';
					right = '}';
				} else {
					left = '[';
					right = ']';
				}
				line += left;
				line += gPieceCharacters[rank];
				line += right;
				line += ' ';
			} else {
				line += ' ';
				if ( rank == kEmpty ) {
					line += '.';
				} else if ( rank == kWater ) {
					line += '*';
				} else if ( rank == kMoved ) {
					line += 'M';
				} else if ( rank == kAddForRankish ) {
					line += '%';
				} else if ( rank == kUnknown ) {
					line += '?';
				} else {
					line += '@';
				}
				line += ' ';
				line += ' ';
			}
		}
		Debug( line );
	}
	Debug( "" );
#endif
}

static PieceRank HelperGetRank(char token)
{
	for ( unsigned int ii=0; ii <= strlen(gPieceCharacters); ++ii ) {
		if (gPieceCharacters[ii] == token) {
			return (PieceRank)(ii);
		}
	}
	return kUnknown;
}

/**
 * Tokenise a string
 */
static int HelperTokenise(std::vector<string> & buffer, std::string & str, char split = ' ')
{
	buffer.clear();
	string token = "";
	for (unsigned int x = 0; x < str.size(); ++x)
	{
		if (str[x] == split && token.size() > 0)
		{
			buffer.push_back(token);
			token = "";
		}
		if (str[x] != split)
			token += str[x];
	}
	if (token.size() > 0)
		buffer.push_back(token);
	return buffer.size();
}

/**
 * Convert string to integer
 */
static int HelperInteger(std::string & fromStr)
{
	stringstream s(fromStr);
	int result = 0;
	s >> result;
	return result;
}

/**
 * Read in a line from stdin
 */
static void HelperReadLine()
{
	std::string buffer = "";
	for (char c = cin.get(); c != '\n' && cin.good(); c = cin.get()) {		
		buffer += c;
	}
	SaveInput( buffer );
	HelperTokenise( gLineBuffer, buffer );
	if ( gLineBuffer.size() == 0 ) {
		Log( "No tokens on line" );
		exit( 99 );
	}
	if ( gLineBuffer.size() == 0 ) {
		Log( "No tokens on line" );
		exit( 99 );
	}
	if ( gLineBuffer[0] == "QUIT" || gLineBuffer[0] == "NO_MOVE") {
		Log( "QUIT token found" );
		exit( 99 );
	}
}

static void HelperJunkLine( int lines = 1 )
{
	while ( lines > 0 ) {
		HelperReadLine();
		lines--;
	}
}

enum MoveOutcome {
	kMR_OK,
	kMR_Kills,
	kMR_Dies,
	kMR_BothDie
};

static void HelperReadMove( StoragePtr storage, PiecePosition& from, PiecePosition& to, MoveOutcome& result, PieceRank& attacker, PieceRank& defender )
{
	HelperReadLine();
	if ( gLineBuffer.size() < 4 ) {
		Log( "Less than 4 tokens" );
		exit( 99 );
	}
	from.col = HelperInteger( gLineBuffer[0] );
	from.row = HelperInteger( gLineBuffer[1] );
	int move = HelperInteger( gLineBuffer[3] );
	if ( move == 0 ) {
		gLineBuffer.insert( gLineBuffer.begin()+3, "1" );
		move = 1;
	}
	to = from;
	std::string dir = gLineBuffer[2];
	if (dir == "UP") {
		to.row -= move;
	} else if (dir == "DOWN") {
		to.row += move;
	} else if (dir == "LEFT") {
		to.col -= move;
	} else if (dir == "RIGHT") {
		to.col += move;
	} else {
		Log( "DIRECTION ERROR" );
		exit( 99 );
	}
	attacker = kUnknown;
	defender = kUnknown;
	std::string res = gLineBuffer[4];
	if ( res == "ILLEGAL" ) {
		Log( "Opponent (hopefully) made ILLEGAL move" );
		exit( 99 );
	} else if ( res == "OK" ) {
		result = kMR_OK;
	} else {
		if ( res == "KILLS" ) {
			result = kMR_Kills;
		} else if ( res == "DIES" ) {
			result = kMR_Dies;
		} else if ( res == "BOTHDIE" ) {
			result = kMR_BothDie;
		} else {
			Log( string("Unknown move result ") + res );
			exit( 99 );
		}
		attacker = HelperGetRank( gLineBuffer[5][0] );
		defender = HelperGetRank( gLineBuffer[6][0] );
	}

}

#ifdef false

static const char *board_setup_1[4] = { // 1 = Marshal, ..., 9 = Scout, : = Spy, ; = Bomb, < = Flag
	"8;<;77;6;7",
	"75;586896;",
	"6989954893",
	"943159:249",
};

static const char *bobs_board_setup[4] = { // 1 = Marshal, ..., 9 = Scout, : = Spy, ; = Bomb, < = Flag
	"<8;645:1;6",
	"8277984893",
	";;5756;7;8",
	"9949359969",
};
						
#endif

static const char *board_setup[4] = { // 1 = Marshal, ..., 9 = Scout, : = Spy, ; = Bomb, < = Flag
//	"8;<;67;;77",
	"8;<;67;7;7",
	"48;3862;89",
	"6359954865",
	"997159:499",
};

static const char *start_piece_counts = "0112344458161";

static int dR[4] = { 1, 0, -1, 0 };
static int dC[4] = { 0, -1, 0, 1 };

#if ASSERTIONS

static void AssertValidBoard( StoragePtr storage )
{
	int piece;
	int count1 = 0;
	int count2 = 0;
	int row, col;
	
	for ( piece = kMarshall; piece <= kFlag; piece++ ) {
		count1 += storage->their_pieces[piece];
	}

	for ( row = 0; row < kBoardSize; row++ ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( storage->board[row][col].color == storage->theirColor
					&& storage->board[row][col].rank == kUnknown ) {
				count2++;
			}
		}
	}
	
	Assert( count1 == count2, "count1 == count2" );
}

#else

#define AssertValidBoard( storage )

#endif

static void PositionPieces(
  StoragePtr storage,        /* 1MB of preinitialized storage for your use */
  PlayerColor playerColor,  /* you play red or blue, with red playing first */
  Board *theBoard           /* provide the initial position of your pieces */
)
{
	int row, our_row, their_row, col, board_col;
	PlayerColor theirColor;
	int piece;
	Boolean reverse = (time( NULL ) & 1) != 0;
	
	Assert( strlen(board_setup[0]) == kBoardSize, "strlen(board_setup[0]) == kBoardSize" );
	Assert( strlen(board_setup[1]) == kBoardSize, "strlen(board_setup[1]) == kBoardSize" );
	Assert( strlen(board_setup[2]) == kBoardSize, "strlen(board_setup[2]) == kBoardSize" );
	Assert( strlen(board_setup[3]) == kBoardSize, "strlen(board_setup[3]) == kBoardSize" );
	
	for ( row = 0; row <= 3; row++ ) {
		if ( playerColor == kRed ) {
			our_row = row;
			their_row = (kBoardSize-1)-row;
			theirColor = kBlue;
		} else {
			their_row = row;
			our_row = (kBoardSize-1)-row;
			theirColor = kRed;
		}
		for ( col = 0; col < 10; col++ ) {
			board_col = reverse ? (kBoardSize-1) - col : col;
			(*theBoard)[our_row][col].thePieceRank = (PieceRank) (board_setup[row][board_col] - '0');
			(*theBoard)[our_row][col].thePieceColor = playerColor;
			
			storage->board[our_row][col].color = playerColor;
			storage->board[our_row][col].rank = (*theBoard)[our_row][col].thePieceRank;
			storage->board[our_row][col].possibilities = kNoNothing;

			storage->board[their_row][col].color = theirColor;
			storage->board[their_row][col].rank = kUnknown;
			storage->board[their_row][col].possibilities = kNoNothing;
		}
	}

	for ( row = 4; row <= 5; row++ ) {
		for( col = 0; col < kBoardSize; col++ ) {
			storage->board[row][col].color = (PlayerColor)kNoColor;
			storage->board[row][col].rank = (PieceRank) ((col/2 % 2 == 1) ? kWater : kEmpty);
			storage->board[row][col].possibilities = 0;
		}
	}
	
	for ( piece = kMarshall; piece <= kFlag; piece++ ) {
		storage->our_pieces[piece] = start_piece_counts[piece] - '0';
		storage->their_pieces[piece] = start_piece_counts[piece] - '0';
	}
	
	storage->do_getmove = (playerColor == kBlue);
	storage->victory = false;
	storage->blankSquare = storage->board[4][0];
	storage->playerColor = playerColor;
	storage->theirColor = playerColor == kRed ? kBlue : kRed;
	storage->repeated_board_count = 0;

	AssertValidBoard( storage );
}

static void Learn( StoragePtr storage, Boolean them, int row, int col, PieceRank rank )
{
	Boolean gotall;
	PlayerColor thiscolor;
	int r, c;
	
	if ( storage->board[row][col].rank == kUnknown ) {
	
		if ( rank == kMoved ) {
			UInt32 possibilities = storage->board[row][col].possibilities;
			possibilities &= ~kStationaryBits;
			
			if ( (possibilities & (possibilities-1)) == 0 ) { // only one bit on! Now we know!
				int newrank;
				newrank = 0;
				while ( (possibilities & 1) == 0 ) {
					possibilities >>= 1;
					newrank++;
				}
				rank = (PieceRank)newrank;
			} else {
				storage->board[row][col].possibilities = possibilities;
			}
		}
		
		if ( rank != kMoved ) {
			storage->board[row][col].rank = rank;
			storage->board[row][col].possibilities = (1 << rank);
			if ( them ) {
				gotall = --storage->their_pieces[rank] == 0;
			} else {
				gotall = --storage->our_pieces[rank] == 0;
			}
			if ( gotall ) {
				thiscolor = storage->board[row][col].color;
				for ( r = 0; r < kBoardSize; r++ ) {
					for ( c = 0; c < kBoardSize; c++ ) {
						if ( storage->board[r][c].rank == kUnknown 
								&& storage->board[r][c].color == thiscolor ) {
							UInt32 possibilities = storage->board[r][c].possibilities;
							possibilities &= ~ (1 << rank);
							storage->board[r][c].possibilities = possibilities;
							if ( (possibilities & (possibilities-1)) == 0 ) { // only one bit on!
								int newrank;
								newrank = 0;
								while ( (possibilities & 1) == 0 ) {
									possibilities >>= 1;
									newrank++;
								}
								Learn( storage, them, r, c, (PieceRank)newrank );
							}
						}
					}
				}
			}
		}
	} else {
		Assert( rank == kMoved || storage->board[row][col].rank == rank, "rank == kMoved || storage->board[row][col].rank == rank" );
	}
}

static void HandleTheirMove( StoragePtr storage, const PiecePosition moveFrom, const PiecePosition moveTo, Boolean moveStrike, const MoveResult moveResult )
{
	Assert( moveResult.legalMove, "moveResult.legalMove" ); // They must have made a legal move or we would not be called
	Assert( !moveResult.victory, "!moveResult.victory" ); // If they won we would not be called
	if ( moveStrike ) {
		Learn( storage, true, moveFrom.row, moveFrom.col, moveResult.rankOfAttacker.thePieceRank );
		Learn( storage, false, moveTo.row, moveTo.col, moveResult.rankOfDefender.thePieceRank );
		if ( moveResult.attackerRemoved && moveResult.defenderRemoved ) {
			storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
			storage->board[moveTo.row][moveTo.col] = storage->blankSquare;
		} else if ( moveResult.attackerRemoved ) {
//			if ( storage->board[moveTo.row][moveTo.col].rank == kBomb ) {
				storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
//			} else {
//				storage->board[moveFrom.row][moveFrom.col] = storage->board[moveTo.row][moveTo.col];
//				storage->board[moveTo.row][moveTo.col] = storage->blankSquare;
//			}
		} else {
			Assert( moveResult.defenderRemoved, "moveResult.defenderRemoved" );
			storage->board[moveTo.row][moveTo.col] = storage->board[moveFrom.row][moveFrom.col];
			storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
		}
	} else {
		storage->board[moveTo.row][moveTo.col] = storage->board[moveFrom.row][moveFrom.col];
		storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
		if ( abs(moveTo.row - moveFrom.row) + abs(moveTo.col - moveFrom.col) > 1 ) {
			Learn( storage, true, moveTo.row, moveTo.col, kScout );
		} else {
			Learn( storage, true, moveTo.row, moveTo.col, (PieceRank)kMoved );
		}
	}

	AssertValidBoard( storage );
}

static Boolean FindPiece( StoragePtr storage, PlayerColor color, PieceRank rank, int *row, int *col )
{
	int r, c;
	
	for ( r = 0; r < kBoardSize; r++ ) {
		for( c = 0; c < kBoardSize; c++ ) {
			if ( storage->board[r][c].color == color
					&& storage->board[r][c].rank == rank ) {
				*row = r;
				*col = c;
				return true;
			}
		}
	}
	return false;
}

static Boolean IsOnBoardWeak( int row, int col )
{
	return 0 <= row && row < kBoardSize && 0 <= col && col < kBoardSize;
}

static Boolean IsOnBoard( int row, int col )
{
	if ( 0 <= row && row < kBoardSize && 0 <= col && col < kBoardSize ) {
		if ( row <= 3 || row >= 6 ) {
			return true;
		}
		if ( col <= 1 || col >= 8 ) {
			return true;
		}
		if ( 4 <= col && col <= 5 ) {
			return true;
		}
	}
	return false;
}

static Boolean IsColorPiece( StoragePtr storage, int row, int col, PlayerColor color )
{
	Assert( IsOnBoardWeak( row, col ), "IsOnBoardWeak( row, col )" );
	return storage->board[row][col].color == color;
}

static Boolean IsOurPiece( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoardWeak( row, col ), "IsOnBoardWeak( row, col )" );
	return storage->board[row][col].color == storage->playerColor;
}

static Boolean IsTheirPiece( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoardWeak( row, col ), "IsOnBoardWeak( row, col )" );
	return storage->board[row][col].color == storage->theirColor;
}

static Boolean IsUnknownPiece( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoardWeak( row, col ), "IsOnBoardWeak( row, col )" );
	return storage->board[row][col].rank == kUnknown;
}

static Boolean IsRankPiece( StoragePtr storage, int row, int col, PieceRank rank )
{
	Assert( IsOnBoardWeak( row, col ), "IsOnBoardWeak( row, col )" );
	return storage->board[row][col].rank == rank;
}

static Boolean IsEmptySquare( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoardWeak( row, col ), "IsOnBoardWeak( row, col )" );
	return storage->board[row][col].rank == (PieceRank)kEmpty;
}

static Boolean IsWaterSquare( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoardWeak( row, col ), "IsOnBoardWeak( row, col )" );
	return storage->board[row][col].rank == (PieceRank)kWater;
}

static Boolean IsLowlyRank( PieceRank rank )
{
	return kCaptain <= rank && rank <= kScout && rank != kMiner;
}

static Boolean IsLowlyPiece( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoard( row, col ), "IsOnBoard( row, col )" );
	return IsLowlyRank( storage->board[row][col].rank );
}

static Boolean IsMovedPiece( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoard( row, col ), "IsOnBoard( row, col )" );
	return (storage->board[row][col].possibilities & kStationaryBits) == 0;
}

static Boolean IsRevealedPiece( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoard( row, col ), "IsOnBoard( row, col )" );
	Assert( IsOurPiece( storage, row, col ), "IsOurPiece( storage, row, col )" );
	UInt32 possibilities = storage->board[row][col].possibilities;
	return ( (possibilities & (possibilities-1)) == 0 );
}

static int CountAdjacentUnknownPieces( StoragePtr storage, PlayerColor color, int row, int col )
{
	int d;
	int unknowns = 0;
	
	for ( d = 0; d < 4; d++ ) {
		int r = row + dR[d];
		int c = col + dC[d];
		
		if ( IsOnBoard( r, c ) && IsColorPiece( storage, r, c, color ) && IsUnknownPiece( storage, r, c ) ) {
			unknowns++;
		}
	}
	
	return unknowns;
}

static const char *defend_spy_table = "RARROAOORARRRARRXAXAOAOOXAXAXAXA";
// Run/Attack/Other/Nothing, >1 unknown:other:danger:moved

static Boolean LowlyCanAttack( StoragePtr storage, int row, int col, int *otherRow, int *otherCol )
{
	for ( int d = 0; d < 4; d++ ) {
		int r = row + dR[d];
		int c = col + dC[d];
		
		if ( IsOnBoard( r, c ) 
				&& IsOurPiece( storage, r, c ) 
				&& IsLowlyPiece( storage, r, c ) ) {
			*otherRow = r;
			*otherCol = c;
			return true;
		}
	}	
	return false;
}

static void UpdateDangerPossibilities( StoragePtr storage )
{
	int row, col;
	
	for ( row = 0; row < kBoardSize; row++ ) {
		for( col = 0; col < kBoardSize; col++ ) {
			storage->dangers[row][col] = 0;
			storage->known_dangers[row][col] = 0;
		}
	}
	
	for ( row = 0; row < kBoardSize; row++ ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsTheirPiece( storage, row, col ) ) {
				UInt32 possibilities = (storage->board[row][col].possibilities & ~kStationaryBits);
				UInt32 known_possibilities = 0;
				
				if ( storage->board[row][col].rank != kUnknown ) {
					known_possibilities = possibilities;
				}
				
				for ( int d = 0; d < 4; d++ ) {
					int r = row + dR[d];
					int c = col + dC[d];
					
					if ( IsOnBoard( r, c ) ) {
						storage->dangers[r][c] |= possibilities;
						storage->known_dangers[r][c] |= known_possibilities;
					}
				}	

			}
		}
	}
	
}

static UInt32 GetDangerPossibilities( StoragePtr storage, int row, int col )
{
	Assert( IsOnBoard( row, col ), "IsOnBoard( row, col )" );
	return storage->dangers[row][col];
}

static Boolean PossibilitiesCouldKill( PieceRank rank, UInt32 possibilities )
{
	if ( (possibilities & ~kStationaryBits) == 0 ) {
		return false;
	}
	
	switch ( rank ) {
		case kFlag:
			return true;
		case kBomb:
			return (possibilities & (1 << kMiner)) != 0;
		case kMarshall:
			return (possibilities & ((1 << kMarshall) + (1<< kSpy))) != 0;
		default:
			return (possibilities & ((1 << (rank+1)) - 1)) != 0;
	}
}

static Boolean PossibilitiesCouldKillSafely( PieceRank rank, UInt32 possibilities )
{
	if ( (possibilities & ~kStationaryBits) == 0 ) {
		return false;
	}
	
	switch ( rank ) {
		case kFlag:
			return true;
		case kBomb:
			return (possibilities & (1 << kMiner)) != 0;
		case kMarshall:
			return (possibilities & ((1<< kSpy))) != 0;
		default:
			return (possibilities & ((1 << rank) - 1)) != 0;
	}
}

static Boolean WillKillPossibilities( PieceRank rank, UInt32 possibilities )
{
	Assert( possibilities != 0, "possibilities != 0" );
	
	switch ( rank ) {
		case kFlag:
			return false;
		case kBomb:
			return false;
		case kMiner:
			return (possibilities & ~((1 << kScout) + (1 << kBomb) + (1 << kFlag))) == 0;
		case kSpy:
			return (possibilities & ~(1 << kMarshall)) == 0;
		default:
			return (possibilities & (((1 << (rank + 1)) - 1) + (1 << kBomb))) == 0;
	}
}

static Boolean WillKillOrSuicidePossibilities( PieceRank rank, UInt32 possibilities )
{
	Assert( possibilities != 0, "possibilities != 0" );
	
	switch ( rank ) {
		case kFlag:
			return false;
		case kBomb:
			return false;
		case kMiner:
			return (possibilities & ~((1 << kScout) + (1 << kMiner) + (1 << kBomb) + (1 << kFlag))) == 0;
		case kSpy:
			return (possibilities & ~((1 << kMarshall) + (1 << kSpy))) == 0;
		default:
			return (possibilities & (((1 << rank) - 1) + (1 << kBomb))) == 0;
	}
}

static Boolean WillPossibilitiesKill( UInt32 possibilities, PieceRank rank )
{
	Assert( possibilities != 0, "possibilities != 0" );
	possibilities &= ~kStationaryBits;
	if ( possibilities == 0 ) {
		return false;
	}

	switch ( rank ) {
		case kFlag:
			return true;
		case kBomb:
			return possibilities == (1 << kMiner);
		default:
			return (possibilities & ~((1 << (rank+1))-1)) == 0;
	}
}

static Boolean FindSafeSquare( StoragePtr storage, int row, int col, int *safeRow, int *safeCol )
{
	Assert( IsOnBoard( row, col ), "IsOnBoard( row, col )" );
	
	PieceRank rank = storage->board[row][col].rank;
	int doff = (storage->playerColor == kBlue ? 0 : 2); // Try backwards first
	
	for ( int d = 0; d < 4; d++ ) {
		int dr = dR[(d + doff) % 4];
		int dc = dC[(d + doff) % 4];
		int r = row + dr;
		int c = col + dc;
		
		while ( IsOnBoard( r, c ) && IsEmptySquare( storage, r, c ) ) {
			if ( !PossibilitiesCouldKill( rank, GetDangerPossibilities( storage, r, c ) ) ) {
				*safeRow = r;
				*safeCol = c;
				return true;
			}
			if ( rank != kScout ) {
				break;
			}
			r += dr;
			c += dc;
		}
	}	
	return false;
}

static void CountEnemies( StoragePtr storage, int row, int col, int *knowns, int *unknowns )
{
	*knowns = 0;
	*unknowns = 0;
	
	for ( int d = 0; d < 4; d++ ) {
		int r = row + dR[d];
		int c = col + dC[d];
		
		if ( IsOnBoard( r, c ) && IsTheirPiece( storage, r, c ) ) {
			if ( storage->board[r][c].rank == kUnknown ) {
				*unknowns += 1;
			} else {
				*knowns += 1;
			}
		}
	}	
}

/*
static Boolean CanRun( StoragePtr storage, int row, int col, int *runRow, int *runCol )
{	
	for ( int d = 0; d < 4; d++ ) {
		int r = row + dR[(d + (storage->playerColor == kBlue ? 0 : 2)) % 4]; // Try backwards first
		int c = col + dC[(d + (storage->playerColor == kBlue ? 0 : 2)) % 4];
		
		if ( IsOnBoard( r, c ) && (storage->board[r][c].rank == kEmpty) ) {
			*runRow = r;
			*runCol = c;
			return true;
		}
	}	
	return false;
}
*/

static Boolean FindSafePath( StoragePtr storage, Boolean very_safe, Boolean suicide_ok, int from_row, int from_col, int to_row, int to_col, int *best_path, int *first_row, int *first_col )
{
	Assert( IsOurPiece( storage, from_row, from_col ), "IsOurPiece( storage, from_row, from_col )" );
	
	PieceRank rank = storage->board[from_row][from_col].rank;
	BoardPossibilities *dangers = very_safe ? &storage->dangers : &storage->known_dangers;
	
	if ( abs( from_row - to_row ) + abs( from_col - to_col ) > *best_path ) {
		return false;
	}
	
	if ( abs( from_row - to_row ) + abs( from_col - to_col ) == 1 ) {
		*best_path = 0;
		*first_row = to_row;
		*first_col = to_col;
		return true;
	}
	
	int path_length_to[kBoardSize][kBoardSize];
	PiecePosition que[kBoardSize * kBoardSize];
	int que_start = 0;
	int que_fin = 0;
	int que_next_len = 0;
	int current_len = 0;
	int row, col;
	
	for ( row = 0; row < kBoardSize; row++ ) {
		for( col = 0; col < kBoardSize; col++ ) {
			path_length_to[row][col] = -1;
		}
	}

	que[que_fin].row = from_row;
	que[que_fin].col = from_col;
	path_length_to[from_row][from_col] = 0;
	que_fin++;
	que_next_len = que_fin;
	
	while ( que_fin > que_start ) {
		row = que[que_start].row;
		col = que[que_start].col;
		que_start++;
		
		for ( int d = 0; d < 4; d++ ) {
			int dr = dR[d];
			int dc = dC[d];
// scout moves NYI
			int r = row + dr;
			int c = col + dc;
			
			if ( IsOnBoard( r, c ) && path_length_to[r][c] == -1 
						&& IsEmptySquare( storage, r, c ) ) {
				if ( suicide_ok
								? !PossibilitiesCouldKillSafely( rank, (*dangers)[r][c] )
								: !PossibilitiesCouldKill( rank, (*dangers)[r][c] ) ) {
					path_length_to[r][c] = current_len + 1;
					if ( abs( to_row - r ) + abs( to_col - c ) == 1 ) {
						*best_path = current_len + 1;
						while ( current_len > 0 ) {
							for ( int d = 0; d < 4; d++ ) {
								int backr = r + dR[d];
								int backc = c + dC[d];

								if ( path_length_to[backr][backc] == current_len ) {
									r = backr;
									c = backc;
									break;
								}
							}
							current_len--;
						}
						*first_row = r;
						*first_col = c;
						return true;
					}
					que[que_fin].row = r;
					que[que_fin].col = c;
					que_fin++;
				} else {
					path_length_to[r][c] = 1000; // Cant go here
				}
			}
		}
		
		if ( que_start == que_next_len ) {
			que_next_len = que_fin;
			current_len++;
		}
	}
	
	return false;
}

static UInt32 CalcBoardCRC( StoragePtr storage, int from_row, int from_col, int to_row, int to_col )
{
	Assert( !IsOnBoard( from_row, from_col ) || IsOurPiece( storage, from_row, from_col ), "!IsOnBoard( from_row, from_col ) || IsOurPiece( storage, from_row, from_col )" );
	Assert( !IsOnBoard( to_row, to_col ) || IsEmptySquare( storage, to_row, to_col ), "!IsOnBoard( to_row, to_col ) || IsEmptySquare( storage, to_row, to_col )" );
	
	UInt32 result = 0;
	
	int row, col;
	int rankish;
	
	for ( row = 0; row < kBoardSize; row++ ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( row == from_row && col == from_col ) {
				rankish = 0;
			} else if ( row == to_row && col == to_col ) {
				rankish = storage->board[from_row][from_col].rank;
			} else if ( IsEmptySquare( storage, row, col ) || IsWaterSquare( storage, row, col ) ) {
				rankish = 0;
			} else if ( IsOurPiece( storage, row, col ) ) {
				rankish = storage->board[row][col].rank;
			} else {
				rankish = storage->board[row][col].rank + kAddForRankish;
			}
			result += rankish; // Hmm, not a very good CRC
			result = result * 11 + (result >> 25);
		}
	}

	return result;	
}

static Boolean OKMove( StoragePtr storage, int from_row, int from_col, int to_row, int to_col )
{
	if ( IsTheirPiece( storage, to_row, to_col ) ) {
		return true;
	}
	
	UInt32 crc = CalcBoardCRC( storage, from_row, from_col, to_row, to_col );
	for ( UInt32 i = 0; i < storage->repeated_board_count; i++ ) {
		if ( crc == storage->repeated_board[i] ) {
			return false;
		}
	}
	return true;
}

static void AppendRepeatedBoard( StoragePtr storage )
{
	UInt32 crc = CalcBoardCRC( storage, -1, -1, -1, -1 );
	
	if ( storage->repeated_board_count == kRepeatedBoards ) {
		storage->repeated_board_count--;
		memcpy( &storage->repeated_board[0], &storage->repeated_board[1], storage->repeated_board_count * sizeof(storage->repeated_board[0]) );
	}
	storage->repeated_board[storage->repeated_board_count++] = crc;
}

#if DEBUG_RULES
	#define RETURN( x ) Log( x ); return
#else
	#define RETURN( x ) return
#endif

static void FigureOutOurMove( StoragePtr storage, PiecePosition *moveFrom, PiecePosition *moveTo )
{
	int ourRow, ourCol, theirRow, theirCol, row, col, runRow, runCol;
	int rowFirst = storage->playerColor == kRed ? 0 : kBoardSize - 1;
	int rowLast = storage->playerColor == kRed ? kBoardSize - 1 : 0;
	int rowAdd = storage->playerColor == kRed ? 1 : -1;
	int bestUnknowns;
	int bestPath;
	int thisPath;

	UpdateDangerPossibilities( storage );
	
// USE SPY
	if ( FindPiece( storage, storage->theirColor, kMarshall, &theirRow, &theirCol )
			&& FindPiece( storage, storage->playerColor, kSpy, &ourRow, &ourCol ) 
			&& abs( theirRow - ourRow ) + abs( theirCol - ourCol ) == 1 ) {
		moveFrom->row = ourRow;
		moveFrom->col = ourCol;
		moveTo->row = theirRow;
		moveTo->col = theirCol;
		RETURN( "USE SPY" );
	}

// DEFEND AGAINST SPY
	if (storage->their_pieces[kSpy] > 0) {
		if ( FindPiece( storage, storage->playerColor, kMarshall, &ourRow, &ourCol ) ) {
			int unknowns = CountAdjacentUnknownPieces( storage, storage->theirColor, ourRow, ourCol );
			
			if ( unknowns ) {
				int base_index = 0;
				Boolean canrun = FindSafeSquare( storage, ourRow, ourCol, &runRow, &runCol );
				if ( !canrun ) {
					base_index += 16;
				}
				if ( unknowns > 1 ) {
					base_index += 8;
				}

				for ( int d = 0; d < 4; d++ ) {
					int r = ourRow + dR[d];
					int c = ourCol + dC[d];
					int otherRow, otherCol;
					
					if ( IsOnBoard( r, c )
							&& IsTheirPiece( storage, r, c )
							&& IsUnknownPiece( storage, r, c ) ) {
						int index = base_index;
						if ( LowlyCanAttack( storage, r, c, &otherRow, &otherCol ) ) {
							index += 4;
						}
						if ( CountAdjacentUnknownPieces( storage, storage->theirColor, r, c ) > 0 ) {
							index += 2;
						}
						if ( IsMovedPiece( storage, r, c ) ) {
							index += 1;
						}
						
						if ( defend_spy_table[index] == 'A' ) { // Attack
							moveFrom->row = ourRow;
							moveFrom->col = ourCol;
							moveTo->row = r;
							moveTo->col = c;
							RETURN( "DEFEND AGAINST SPY 1" );
						} else if ( defend_spy_table[index] == 'O' ) { // Attack
							moveFrom->row = otherRow;
							moveFrom->col = otherCol;
							moveTo->row = r;
							moveTo->col = c;
							RETURN( "DEFEND AGAINST SPY 2" );
						}
					}
				}
				
				if ( canrun && OKMove( storage, ourRow, ourCol, runRow, runCol ) ) {
					moveFrom->row = ourRow;
					moveFrom->col = ourCol;
					moveTo->row = runRow;
					moveTo->col = runCol;
					RETURN( "DEFEND AGAINST SPY 3" );
				}
				// Give up!  Next rule…
			}
		}
	}

// ATTACK WEAKER
	for ( row = rowFirst; 0 <= row && row < kBoardSize; row += rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsTheirPiece( storage, row, col ) ) {
				UInt32 enemy = storage->board[row][col].possibilities;
				UInt32 danger = GetDangerPossibilities( storage, row, col );

				int bestDir = -1;
				Boolean isBestRevealed = true;
				PieceRank bestRank = kUnknown;

				for ( int d = 0; d < 4; d++ ) {
					int r = row + dR[d];
					int c = col + dC[d];
					
					if ( IsOnBoard( r, c ) && IsOurPiece( storage, r, c ) ) {
						if ( !PossibilitiesCouldKill( storage->board[r][c].rank, danger ) ) {
							if ( WillKillPossibilities( storage->board[r][c].rank, enemy ) ) {
								Boolean thisRevealed = IsRevealedPiece( storage, r, c );
								if ( isBestRevealed || !thisRevealed ) {
									if ( bestDir == -1 || (storage->board[r][c].rank > bestRank) ) {
										bestDir = d;
										bestRank = storage->board[r][c].rank;
										isBestRevealed = thisRevealed;
									}
								}
							}
						}
					}
				}
				if ( bestDir != -1 ) {
					moveFrom->row = row + dR[bestDir];
					moveFrom->col = col + dC[bestDir];
					moveTo->row = row;
					moveTo->col = col;
					RETURN( "ATTACK WEAKER" );
				}
			}
		}
	}

// EXPLORE ATTACK
	for ( int rnk = kScout; rnk >= kMarshall; rnk-- ) {
		PieceRank rank = (PieceRank) rnk;
		if ( IsLowlyRank( rank ) ) {

			for ( row = rowLast; 0 <= row && row < kBoardSize; row -= rowAdd ) {
				for( col = 0; col < kBoardSize; col++ ) {
					if ( IsOurPiece( storage, row, col )
							&& IsRankPiece( storage, row, col, rank ) ) {

						for ( int d = 0; d < 4; d++ ) {
							int r = row + dR[d];
							int c = col + dC[d];
							
							if ( IsOnBoard( r, c ) 
										&& IsTheirPiece( storage, r, c ) 
										&& IsRankPiece( storage, r, c, kUnknown ) ) {
								moveFrom->row = row;
								moveFrom->col = col;
								moveTo->row = r;
								moveTo->col = c;
								RETURN( "EXPLORE ATTACK" );
							}
						}
					}
				}
			}
		
		}
	}
	
// RETREAT
	for ( row = rowLast; 0 <= row && row < kBoardSize; row -= rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsOurPiece( storage, row, col )
					&& IsMovedPiece( storage, row, col ) ) {

				for ( int d = 0; d < 4; d++ ) {
					int r = row + dR[d];
					int c = col + dC[d];
					
					if ( IsOnBoard( r, c ) 
								&& IsTheirPiece( storage, r, c ) 
								&& WillPossibilitiesKill( storage->board[r][c].possibilities, storage->board[row][col].rank ) ) {
						bestPath = 1000;
						for ( int to_row = rowLast; 0 <= to_row && to_row < kBoardSize; to_row -= rowAdd ) {
							for( int to_col = 0; to_col < kBoardSize; to_col++ ) {
								thisPath = bestPath;
								if ( IsTheirPiece( storage, to_row, to_col ) 
											&& (IsRankPiece( storage, to_row, to_col, kUnknown ) 
														|| WillKillPossibilities( storage->board[row][col].rank, storage->board[to_row][to_col].possibilities ))
											&& FindSafePath( storage, false, true, row, col, to_row, to_col, &thisPath, &runRow, &runCol ) 
											&& OKMove( storage, row, col, runRow, runCol ) ) {
									bestPath = thisPath;
									moveFrom->row = row;
									moveFrom->col = col;
									moveTo->row = runRow;
									moveTo->col = runCol;
								}
							}
						}
						if ( bestPath < 1000 ) {
							RETURN( "RETREAT" );
						}
					}
				}
			}
		}
	}

// SCOUT
	bestUnknowns = 0;
	
	for ( row = rowLast; 0 <= row && row < kBoardSize; row -= rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsOurPiece( storage, row, col ) 
						&& IsRankPiece( storage, row, col, kScout ) ) {
				for ( int d = 0; d < 4; d++ ) {
					int r = row + dR[d];
					int c = col + dC[d];
					
					while ( IsOnBoard( r, c ) && IsEmptySquare( storage, r, c ) ) {
						
						int knowns, unknowns;
						CountEnemies( storage, r, c, &knowns, &unknowns );
						if ( knowns == 0 && unknowns > bestUnknowns && OKMove( storage, row, col, r, c ) ) {
							bestUnknowns = unknowns;
							ourRow = row;
							ourCol = col;
							runRow = r;
							runCol = c;
						}
						r += dR[d];
						c += dC[d];
					}
				}
			}
		}
	}

	if ( bestUnknowns > 0 ) {
		moveFrom->row = ourRow;
		moveFrom->col = ourCol;
		moveTo->row = runRow;
		moveTo->col = runCol;
		RETURN( "SCOUT" );
	}

// ATTACK DISTANT

	bestPath = 1000;
	
	for ( row = rowFirst; 0 <= row && row < kBoardSize; row += rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsTheirPiece( storage, row, col ) ) {
				UInt32 possibilities = storage->board[row][col].possibilities;
				UInt32 danger = GetDangerPossibilities( storage, row, col );
				
				if ( (possibilities & ((1 << kBomb) | (1 << kMarshall))) != ((1 << kBomb) | (1 << kMarshall)) ) {
					for ( int r = rowFirst; 0 <= r && r < kBoardSize; r += rowAdd ) {
						for( int c = 0; c < kBoardSize; c++ ) {
							if ( IsOurPiece( storage, r, c ) ) {
								if ( WillKillPossibilities( storage->board[r][c].rank, possibilities ) ) {
									if ( storage->board[r][c].rank >= kCaptain || !PossibilitiesCouldKill( storage->board[r][c].rank, danger ) ) {
										thisPath = bestPath;
										if ( FindSafePath( storage, true, false, r, c, row, col, &thisPath, &runRow, &runCol ) ) {
											if ( OKMove( storage, r, c, runRow, runCol ) ) {
												bestPath = thisPath;
												moveFrom->row = r;
												moveFrom->col = c;
												moveTo->row = runRow;
												moveTo->col = runCol;
											}
										}
									}
								}
							}
						}
					}
				}

			}
		}
	}

	if ( bestPath < 1000 ) {
		RETURN( "ATTACK DISTANT" );
	}

// EXPLORE DISTANT

	bestPath = 1000;
	
	for ( row = rowFirst; 0 <= row && row < kBoardSize; row += rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsTheirPiece( storage, row, col ) && storage->board[row][col].rank == kUnknown ) {
				
				for ( int r = rowFirst; 0 <= r && r < kBoardSize; r += rowAdd ) {
					for( int c = 0; c < kBoardSize; c++ ) {
						if ( IsOurPiece( storage, r, c ) && IsLowlyPiece( storage, r, c ) ) {
							thisPath = bestPath;
							if ( FindSafePath( storage, false, true, r, c, row, col, &thisPath, &runRow, &runCol ) ) {
								if ( OKMove( storage, r, c, runRow, runCol ) ) {
									bestPath = thisPath;
									moveFrom->row = r;
									moveFrom->col = c;
									moveTo->row = runRow;
									moveTo->col = runCol;
								}
							}
						}
					}
				}

			}
		}
	}

	if ( bestPath < 1000 ) {
		RETURN( "EXPLORE DISTANT" );
	}

// ATTACK KNOWN WITH SAME DISTANT

	bestPath = 1000;
	
	for ( row = rowFirst; 0 <= row && row < kBoardSize; row += rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsTheirPiece( storage, row, col ) ) {
				UInt32 possibilities = storage->board[row][col].possibilities;
				
				if ( (possibilities & ((1 << kBomb) | (1 << kMarshall))) != ((1 << kBomb) | (1 << kMarshall)) ) {
					for ( int r = rowFirst; 0 <= r && r < kBoardSize; r += rowAdd ) {
						for( int c = 0; c < kBoardSize; c++ ) {
							if ( IsOurPiece( storage, r, c ) ) {
								if ( WillKillOrSuicidePossibilities( storage->board[r][c].rank, possibilities ) ) {
									thisPath = bestPath;
									if ( FindSafePath( storage, true, true, r, c, row, col, &thisPath, &runRow, &runCol ) ) {
										if ( OKMove( storage, r, c, runRow, runCol ) ) {
											bestPath = thisPath;
											moveFrom->row = r;
											moveFrom->col = c;
											moveTo->row = runRow;
											moveTo->col = runCol;
										}
									}
								}
							}
						}
					}
				}

			}
		}
	}

	if ( bestPath < 1000 ) {
		RETURN( "ATTACK KNOWN WITH SAME DISTANT" );
	}

// FIND FLAG
// NYI

// MOVE FORWARD
	
	for ( row = rowLast; 0 <= row && row < kBoardSize; row -= rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsOurPiece( storage, row, col ) ) {
				PieceRank rank = storage->board[row][col].rank;
				if ( rank != kBomb && rank != kFlag ) {
					int r = row + rowAdd;
					if ( IsOnBoard( r, col ) && !IsOurPiece( storage, r, col ) && OKMove( storage, row, col, r, col ) ) {
						moveFrom->row = row;
						moveFrom->col = col;
						moveTo->row = r;
						moveTo->col = col;
						RETURN( "MOVE FORWARD" );
					}
				}
			}
		}
	}

// MOVE

	for ( row = rowLast; 0 <= row && row < kBoardSize; row -= rowAdd ) {
		for( col = 0; col < kBoardSize; col++ ) {
			if ( IsOurPiece( storage, row, col ) ) {
				PieceRank rank = storage->board[row][col].rank;
				if ( rank != kBomb && rank != kFlag ) {
					
					for ( int d = 0; d < 4; d++ ) {
						int r = row + dR[d];
						int c = col + dC[d];
						
						if ( IsOnBoard( r, c ) && !IsOurPiece( storage, r, c ) && OKMove( storage, row, col, r, c ) ) {
							moveFrom->row = row;
							moveFrom->col = col;
							moveTo->row = r;
							moveTo->col = c;
							RETURN( "MOVE" );
						}
					}
				}
			}
		}
	}

// RESIGN
	moveFrom->row = -1;
	moveFrom->col = -1;
	moveTo->row = -1;
	moveTo->col = -1;
	RETURN( "RESIGN" );

}

static void HandleOurMove( StoragePtr storage, PiecePosition moveFrom, PiecePosition moveTo, const MoveResult moveResult )
{
	Boolean moveStrike;

	if ( IsOnBoard( moveTo.row, moveTo.col ) ) {
		moveStrike = storage->board[moveTo.row][moveTo.col].color != kNoColor;
	} else {
		moveStrike = false;
	}
	
	if ( moveResult.victory ) { // We Win! :-)
		storage->victory = true;
	} else if ( !moveResult.legalMove ) { // We Lose! :-(
	} else {
		if ( moveStrike ) {
			storage->repeated_board_count = 0;
			Learn( storage, true, moveTo.row, moveTo.col, moveResult.rankOfDefender.thePieceRank );
			Learn( storage, false, moveFrom.row, moveFrom.col, moveResult.rankOfAttacker.thePieceRank );

			if ( moveResult.attackerRemoved && moveResult.defenderRemoved ) {
				storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
				storage->board[moveTo.row][moveTo.col] = storage->blankSquare;
			} else if ( moveResult.attackerRemoved ) {
//				if ( storage->board[moveTo.row][moveTo.col].rank == kBomb ) {
					storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
//				} else {
//					storage->board[moveFrom.row][moveFrom.col] = storage->board[moveTo.row][moveTo.col];
//					storage->board[moveTo.row][moveTo.col] = storage->blankSquare;
//				}
			} else {
				Assert( moveResult.defenderRemoved, "moveResult.defenderRemoved" );
				storage->board[moveTo.row][moveTo.col] = storage->board[moveFrom.row][moveFrom.col];
				storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
			}

		} else {
			if ( abs( moveTo.row - moveFrom.row ) + abs( moveTo.col - moveFrom.col ) > 1 ) {
				Assert( storage->board[moveFrom.row][moveFrom.col].rank == kScout, "storage->board[moveFrom.row][moveFrom.col].rank == kScout" );
				Learn( storage, false, moveFrom.row, moveFrom.col, kScout );
			} else {
				Learn( storage, false, moveFrom.row, moveFrom.col, (PieceRank)kMoved );
			}
			storage->board[moveTo.row][moveTo.col] = storage->board[moveFrom.row][moveFrom.col];
			storage->board[moveFrom.row][moveFrom.col] = storage->blankSquare;
		}
		AppendRepeatedBoard( storage );
	}

	AssertValidBoard( storage );
}

/*
Boolean MakeAMove(
  StoragePtr storage,          / * 1MB of storage from PositionPieces * /
  PlayerColor playerColor,    / * you play red or blue, with red playing first * /
  GetOpponentMove *GetMove,  / * callback used to find about opponents last move * /
  ReportYourMove *ReportMove   / * callback used to make a move * /
)
{
	if ( storage->do_getmove ) {
		HandleTheirMove( storage, *GetMove );
	}
	storage->do_getmove = true;
	
	HandleOurMove( storage,  *ReportMove );

	return storage->victory;
}
*/

// Code to map UCC Challenge to MacTech Challenge

static PlayerColor ReadPlayerLine()
{
	HelperReadLine();
	
	std::string colourStr = gLineBuffer[0];

	if ( colourStr == "RED" ) {
		return kRed;
	} else if ( colourStr == "BLUE" ) {
		return kBlue;
	} else {
		Log( string("What color? ") + colourStr );
		exit( 99 );
	}
}

static PlayerColor OtherPlayerColor( PlayerColor player )
{
	return (player == kRed) ? kBlue : kRed;
}

int main(int argc, char ** argv)
{
	srand(time(NULL));
	cin.rdbuf()->pubsetbuf(NULL, 0);
	cout.rdbuf()->pubsetbuf(NULL, 0);
	cout.setf(std::ios::unitbuf);
#if SAVE_OUTPUT
	gOutputFile.open( (string("/tmp/peternlewis-output-") + to_string(time( NULL )) + "-" + to_string(rand() & 0xFFFF) + ".log").c_str() );
	gOutputFile.setf(std::ios::unitbuf);
#endif
	
	Storage storage;
	Board board;
	
	PlayerColor player = ReadPlayerLine();
	
	PositionPieces( &storage, player, &board );
	if ( player == kRed ) {
		for ( int r = 0; r <= 3; r++ ) {
			string line;
			for ( int c = 0; c < kBoardSize; c++ ) {
				line += gPieceCharacters[board[r][c].thePieceRank];
			}
			Output( line );
		}
	} else {
		for ( int r = kBoardSize - 4; r < kBoardSize; r++ ) {
			string line;
			for ( int c = 0; c < kBoardSize; c++ ) {
				line += gPieceCharacters[board[r][c].thePieceRank];
			}
			Output( line );
		}
	}

	bool expectStart = (player == kRed);
	while ( 1 ) {
		Debug( "LOOP" );
		if ( expectStart ) {
			HelperReadLine();
			expectStart = false;
		} else {
			PiecePosition from;
			PiecePosition to;
			MoveOutcome result;
			PieceRank attacker;
			PieceRank defender;
			HelperReadMove( &storage, from, to, result, attacker, defender );
			Debug( to_string(from.col) + "," + to_string(from.row) + " -> " + to_string(to.col) + "," + to_string(to.row) );
			MoveResult moveResult;
			moveResult.rankOfAttacker.thePieceRank = attacker;
			moveResult.rankOfAttacker.thePieceColor = OtherPlayerColor( player );
			moveResult.rankOfDefender.thePieceRank = defender;
			moveResult.rankOfDefender.thePieceColor = player;
			moveResult.attackerRemoved = (result == kMR_Dies) || (result == kMR_BothDie);
			moveResult.defenderRemoved = (result == kMR_Kills) || (result == kMR_BothDie);
			moveResult.victory = false;
			moveResult.legalMove = true;
			HandleTheirMove( &storage, from, to, (result != kMR_OK), moveResult );
		}
		HelperJunkLine( kBoardSize );
		DumpBoard( &storage );
		
		PiecePosition moveFrom;
		PiecePosition moveTo;

		FigureOutOurMove( &storage, &moveFrom, &moveTo );
		Debug( to_string(moveFrom.col) + ',' + to_string(moveFrom.row) + " -> " + to_string(moveTo.col) + ',' + to_string(moveTo.row) );
		if ( moveFrom.row < 0 ) {
			Output( "SURRENDER" );
			exit(EXIT_SUCCESS);
		}
		std::string dir;
		int move;
		if ( moveTo.col > moveFrom.col ) {
			dir = "RIGHT";
			move = moveTo.col - moveFrom.col;
		} else if ( moveTo.col < moveFrom.col ) {
			dir = "LEFT";
			move = moveFrom.col - moveTo.col;
		} else if ( moveTo.row < moveFrom.row ) {
			dir = "UP";
			move = moveFrom.row - moveTo.row;
		} else if ( moveTo.row > moveFrom.row ) {
			dir = "DOWN";
			move = moveTo.row - moveFrom.row;
		}
		Output( to_string(moveFrom.col) + ' ' + to_string(moveFrom.row) + ' ' + dir + ' ' + to_string(move) );
		{
			PiecePosition from;
			PiecePosition to;
			MoveOutcome result;
			PieceRank attacker;
			PieceRank defender;
			HelperReadMove( &storage, from, to, result, attacker, defender );
			MoveResult moveResult;
			moveResult.rankOfAttacker.thePieceRank = attacker;
			moveResult.rankOfAttacker.thePieceColor = player;
			moveResult.rankOfDefender.thePieceRank = defender;
			moveResult.rankOfDefender.thePieceColor = OtherPlayerColor( player );
			moveResult.attackerRemoved = (result == kMR_Dies) || (result == kMR_BothDie);
			moveResult.defenderRemoved = (result == kMR_Kills) || (result == kMR_BothDie);
			moveResult.victory = false;
			moveResult.legalMove = true;
			HandleOurMove( &storage, from, to, moveResult );
			DumpBoard( &storage );
		}
	}
	
	exit(EXIT_SUCCESS);
	return 0;
}
