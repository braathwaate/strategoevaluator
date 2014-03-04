/*
 * UCC 2012 Programming Competition Entry
 * - "Ramen"
 *
 * By John Hodge [TPG]
 */
#define ENABLE_DEBUG	0
#define SHOW_TARGET_MAP	0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "interface.h"
#include "ai_common.h"

// === CONSTANTS ===
//! \brief Maximum recusion depth
#define MAX_SEARCH_DEPTH	5

//! \brief Threshold before repeat modifier is applied
#define REPEAT_THRESHOLD	3
//! \brief Modifier applied to a repeated move
#define REPEAT_MOVE_MODIFY(score)	do{score /= (giNumRepeatedMove <= 5 ? 2 : 10); score -= giNumRepeatedMove*10;}while(0);

//! \brief Number of moves by this AI before the defensive modifier kicks in
#define DEFENSIVE_THRESHOLD	20
//! \brief Modifier applied to offensive moves when > DEFENSIVE_THRESHOLD defensive moves have been done in a row
#define DEFENSIVE_MODIFY(score)	do{score *= 1+(giTurnsSinceLastTake/15);}while(0)
/**
 * \name AI move outcome scores
 * \{
 */
#define OC_LOSE	-100
#define OC_DRAW	0
#define OC_UNK	40
#define OC_WIN	100
#define OC_FLAG	150
/**
 * \}
 */

// === PROTOTYPES ===
// - Wrapper and Initialisation
void	AI_Initialise(enum eColours Colour, const char *Opponent);
void	AI_HandleMove(int bMyMove, const tMove *Move);
void	UpdateStates(const tMove *OpponentMove);
void	AI_DoMove(tMove *MyMove);
// - Management
tPiece	*GetPieceByPos(int X, int Y);
void	MovePieceTo(tPiece *Piece, int X, int Y);
void	UpdateRank(tPiece *Piece, char RankChar);
void	PieceExposed(tPiece *Piece);
void	RemovePiece(tPiece *Piece);
// -- AI Core 
 int	GetBestMove(tPlayerStats *Attacker, tPlayerStats *Defender, tMove *Move, int Level);
 int	GetPositionScore(tPlayerStats *Attacker, tPlayerStats *Defender, int Level, int X, int Y, tPiece *Piece);
 int	GetScore(tPiece *This, tPiece *Target);
 int	GetRawScore(tPiece *This, tPiece *Target);
// -- Helpers
static inline int	GetOutcome(int Attacker, int Defender);
static inline int	ABS(int Val);
static inline int	RANGE(int Min, int Val, int Max);

// === GLOBALS ===
enum eColours	gMyColour;
tPiece	gBlockPiece = {.Team = 2};
 int	giGameStateSize;
tGameState	*gpCurrentGameState;
BOOL	gbFirstTurn = true;
//tPiece	**gaBoardPieces;
const char *gsOpponentDbFilename;
// -- State variables to avoid deadlocking
 int	giNumRepeatedMove;
 int	giTurnsSinceLastTake;
tMove	gLastMove;
tPiece	*gLastMove_Target, *gLastMove_Piece;

// === CODE ===
void AI_Initialise(enum eColours Colour, const char *Opponent)
{
	gMyColour = Colour;

	// TODO: Get opponent filename
	gsOpponentDbFilename = DB_GetOpponentFile(Opponent);
	
	// Select setup
//	setup_id = rand() % 3;
	int setup_id = 1;
	switch( setup_id )
	{
//	case 0:	// Bomb-off (dick move)
//		// 39 pieces, bombs blocking gates, high level pieces backing up
//		{
//		const char *setup[] = {
//			"8.88979993\n",
//			"6689995986\n",
//			"F72434s174\n",
//			"BB56BB55BB\n"
//			};
//		if( Colour == COLOUR_RED )
//			for(int i = 0; i < 4; i ++ )	printf(setup[i]);
//		else
//			for(int i = 4; i --; )		printf(setup[i]);
//		break;
//		}
	case 1:
		{
		const char *setup[] = {
			"FB8sB479B8\n",
			"BB31555583\n",
			"6724898974\n",
			"967B669999\n"
			};
		if( Colour == COLOUR_RED )
			for(int i = 0; i < 4; i ++ )	printf(setup[i]);
		else
			for(int i = 4; i --; )		printf(setup[i]);
		}
		break ;
	default:
		exit(1);
	}


	giGameStateSize = sizeof(tGameState) + giBoardHeight*giBoardWidth*sizeof(tPieceRef);
	gpCurrentGameState = calloc( giGameStateSize, 1 );
	gpCurrentGameState->Opponent.Colour = !Colour;
	gpCurrentGameState->MyExposed.Colour = Colour;
	gpCurrentGameState->MyActual.Colour = Colour;
//	gaBoardPieces = calloc( giBoardHeight*giBoardWidth, sizeof(tPiece*) );
}

void AI_int_InitialiseBoardState(void)
{
	 int	piece_index = 0;
	 int	my_piece_index = 0;
	for( int y = 0; y < giBoardHeight; y ++ )
	{
		for( int x = 0; x < giBoardWidth; x ++ )
		{
			tPiece	*p;
			char b;
			
			b = gaBoardState[y*giBoardWidth+x];
	
			if( b == '.' )	continue ;
			if( b == '+' )
			{
				gpCurrentGameState->BoardState[ y*giBoardWidth+x ].Team = 3;
				continue ;
			}

			if( b == '#' )
			{
				if( piece_index >= N_PIECES ) {
					piece_index ++;
					continue ;
				}
				p = &gpCurrentGameState->Opponent.Pieces[piece_index++];
				p->Rank = RANK_UNKNOWN;
				p->X = x;	p->StartX = x;
				p->Y = y;	p->StartY = y;
				p->bHasMoved = false;
				p->Team = !gMyColour;
				gpCurrentGameState->BoardState[ y*giBoardWidth+x ].Team = 2;
				gpCurrentGameState->BoardState[ y*giBoardWidth+x ].Index = piece_index - 1;
				DEBUG("Enemy at %i,%i", x, y);
			}
			else
			{
				if( my_piece_index >= N_PIECES ) {
					my_piece_index ++;
					continue ;
				}
				p = &gpCurrentGameState->MyActual.Pieces[my_piece_index++];
				p->X = x;
				p->Y = y;
				p->Team = gMyColour;
				UpdateRank(p, b);
				gpCurrentGameState->MyActual.nRanks[p->Rank] ++;
				gpCurrentGameState->BoardState[ y*giBoardWidth+x ].Team = 1;
				gpCurrentGameState->BoardState[ y*giBoardWidth+x ].Index = my_piece_index - 1;
			}

			
		}
	}
	gpCurrentGameState->Opponent.nPieces = piece_index;
	if( piece_index > N_PIECES )
		DEBUG("GAH! Too many opposing pieces (%i > 40)", piece_index);
	if( my_piece_index > N_PIECES )
		DEBUG("GAH! Too many of my pieces (%i > 40)", my_piece_index);

	// Catch for if I don't put enough pieces out (shouldn't happen)
	while( my_piece_index < N_PIECES ) {
		gpCurrentGameState->MyActual.Pieces[my_piece_index].bDead = true;
		gpCurrentGameState->MyActual.Pieces[my_piece_index].Rank = RANK_UNKNOWN;
		my_piece_index ++;
	}

	// Load guesses at what each piece is
	DB_LoadGuesses(gsOpponentDbFilename, !gMyColour);
	gpCurrentGameState->Opponent.bGuessValid = true;
}

void AI_HandleMove(int bMyMove, const tMove *Move)
{
	if( gbFirstTurn )
	{
		gbFirstTurn = false;
		
		AI_int_InitialiseBoardState();

		// Reverse the first move
		if( Move->dir != DIR_INVAL )
		{
			tPiece	*p;
			switch(Move->dir)
			{
			case DIR_INVAL:	ASSERT(Move->dir != DIR_INVAL);	break;
			case DIR_LEFT:	p = GetPieceByPos( Move->x-1, Move->y );	break ;
			case DIR_RIGHT:	p = GetPieceByPos( Move->x+1, Move->y );	break ;
			case DIR_UP:	p = GetPieceByPos( Move->x, Move->y-1 );	break ;
			case DIR_DOWN:	p = GetPieceByPos( Move->x, Move->y+1 );	break ;
			}
			MovePieceTo( p, Move->x, Move->y );
			p->StartX = Move->x;
			p->StartY = Move->y;
		}
	}

	if(Move->result == RESULT_VICTORY)
	{
		// TODO: Distiguish between victory conditions?
		// - Note flag location?

		// TODO: Save back initial board state
		DB_WriteBackInitialState(gsOpponentDbFilename, !gMyColour, gpCurrentGameState->Opponent.Pieces);
	}

	if( !bMyMove )
	{
		if( Move->dir != DIR_INVAL )
			UpdateStates(Move);
	}
	else
	{
		tPiece	*p = GetPieceByPos(Move->x, Move->y);
		ASSERT(p);

	 	int newx = p->X, newy = p->Y;
		switch(Move->dir)
		{
		case DIR_INVAL:	break;
		case DIR_LEFT:	newx -= Move->dist;	break;
		case DIR_RIGHT:	newx += Move->dist;	break;
		case DIR_UP:	newy -= Move->dist;	break;
		case DIR_DOWN:	newy += Move->dist;	break;
		}
		tPiece	*target = GetPieceByPos(newx, newy);

		switch(Move->result)
		{
		case RESULT_ILLEGAL:	break;
		case RESULT_INVAL:	break;
		case RESULT_OK:
			MovePieceTo(p, newx, newy);
			break;
		case RESULT_KILL:
			UpdateRank(target, Move->defender);
			RemovePiece(target);
			MovePieceTo(p, newx, newy);
			PieceExposed(p);	// TODO: Update oponent's view
			giTurnsSinceLastTake = 0;
			break;
		case RESULT_DIES:
		case RESULT_VICTORY:
			UpdateRank(target, Move->defender);
			PieceExposed(p);
			RemovePiece(p);
			giTurnsSinceLastTake = 0;
			break;
		case RESULT_BOTHDIE:
			UpdateRank(target, Move->defender);
			PieceExposed(p);
			RemovePiece(p);
			RemovePiece(target);
			giTurnsSinceLastTake = 0;
			break;
		}
	}
}

void UpdateStates(const tMove *OpponentMove)
{
	// --- Get moved piece, update position ---
	tPiece	*moved_piece = GetPieceByPos(OpponentMove->x, OpponentMove->y);
	// - Sanity
	ASSERT( moved_piece );
	ASSERT( moved_piece->Team == !gMyColour );
	// - Only scouts can move multiple squares
	if( moved_piece->Rank == RANK_UNKNOWN && OpponentMove->dist > 1 )
		UpdateRank(moved_piece, '9');
	// - Update position
	 int newx = moved_piece->X, newy = moved_piece->Y;
	switch(OpponentMove->dir)
	{
	case DIR_INVAL:	break;
	case DIR_LEFT:	newx -= OpponentMove->dist;	break;
	case DIR_RIGHT:	newx += OpponentMove->dist;	break;
	case DIR_UP:	newy -= OpponentMove->dist;	break;
	case DIR_DOWN:	newy += OpponentMove->dist;	break;
	}
	tPiece	*my_piece = GetPieceByPos(newx, newy);
	
	// Check if one of my pieces has been taken
	switch( OpponentMove->result )
	{
	case RESULT_ILLEGAL:	break;
	case RESULT_INVAL:	break;
	case RESULT_OK:
		MovePieceTo(moved_piece, newx, newy);
		break;
	case RESULT_KILL:
	case RESULT_VICTORY:
		UpdateRank(moved_piece, OpponentMove->attacker);
		PieceExposed(my_piece);
		RemovePiece(my_piece);
		MovePieceTo(moved_piece, newx, newy);
		break;
	case RESULT_DIES:
		UpdateRank(moved_piece, OpponentMove->attacker);
		PieceExposed(my_piece);
		RemovePiece(moved_piece);
		break;
	case RESULT_BOTHDIE:
		UpdateRank(moved_piece, OpponentMove->attacker);
		RemovePiece(moved_piece);
		PieceExposed(my_piece);
		RemovePiece(my_piece);
		break;
	}

	// Update rank if revealed
	if( moved_piece->Rank == RANK_UNKNOWN )
		UpdateRank(moved_piece, gaBoardState[moved_piece->Y*giBoardWidth+moved_piece->X]);

	// - Update piece states
	DEBUG("Updating piece states");
	for( int y = 0; y < giBoardHeight; y ++ )
	{
		for( int x = 0; x < giBoardWidth; x ++ )
		{
			char	c = gaBoardState[y*giBoardWidth+x];
			if( c == '.' )	continue;
			if( c == '+' )	continue;
			tPiece *p = GetPieceByPos(x, y);
			if(!p) DEBUG("c = %c", c);
			ASSERT(p);
			if( p->Team == gMyColour )	continue ;
			if( p->Rank == RANK_UNKNOWN && c != '#' )
				UpdateRank(p, c);
		}
	}
}

void AI_DoMove(tMove *MyMove)
{
#if 1
	// Sanity checks
	for( int i = 0; i < N_PIECES; i ++ )
	{
		tPiece	*p = &gpCurrentGameState->MyActual.Pieces[i];
		if(p->bDead)	continue;

		if( p != GetPieceByPos(p->X, p->Y) ) {
			DEBUG("Piece %p(%i,%i R%i) not at stated position",
				p, p->X, p->Y, p->Rank);
		}
	}
#endif

	DEBUG("Deciding on move");
	GetBestMove(&gpCurrentGameState->MyActual, &gpCurrentGameState->Opponent, MyMove, 0);
}

tPiece *GetPieceByPos(int X, int Y)
{
	tPieceRef	*pr = &gpCurrentGameState->BoardState[Y*giBoardWidth+X];
	switch( pr->Team )
	{
	case 0:	return NULL;
	case 1:	return &gpCurrentGameState->MyActual.Pieces[ (int)pr->Index ];
	case 2:	return &gpCurrentGameState->Opponent.Pieces[ (int)pr->Index ];
	case 3:	return &gBlockPiece;
	}
	return NULL;
}

void MovePieceTo(tPiece *Piece, int X, int Y)
{
	DEBUG("Moved %p(%i,%i) to (%i,%i)",
		Piece, Piece->X, Piece->Y, X, Y);
	
	gpCurrentGameState->BoardState[Y*giBoardWidth + X]
		= gpCurrentGameState->BoardState[Piece->Y*giBoardWidth + Piece->X];
	gpCurrentGameState->BoardState[Piece->Y*giBoardWidth + Piece->X].Team = 0;

	Piece->X = X;
	Piece->Y = Y;

	if( !Piece->bHasMoved )
	{
		if( Piece->Team == gMyColour )
		{
			gpCurrentGameState->MyExposed.nMoved ++;
		}
		else
		{
			gpCurrentGameState->Opponent.nMoved ++;
		}
	}
	
	Piece->bHasMoved = true;
}

void UpdateRank(tPiece *Piece, char RankChar)
{
	enum eRanks rank;

	rank = CharToRank(RankChar);

	if( Piece->Rank == rank )
		return ;

	if( Piece->Rank != RANK_UNKNOWN )
	{
		if(Piece->Rank != rank )
		{
			DEBUG("Rank of piece %p(%i,%i) has changed, was %i now %i",
				Piece, Piece->X, Piece->Y, Piece->Rank, rank);
			Piece->Rank = rank;
		}
		return ;
	}

	if( Piece->Team == !gMyColour && rank != RANK_UNKNOWN )
	{
		if( gpCurrentGameState->Opponent.nRanks[rank] >= MAX_RANK_COUNTS[rank] ) {
			DEBUG("ERROR: Bookkeeping failed, >%i units of rank %i on board",
				MAX_RANK_COUNTS[rank], rank);
		}
		DEBUG("Found a %i", rank);
		gpCurrentGameState->Opponent.nRanks[rank] ++;
		if( gpCurrentGameState->Opponent.nIdentified == gpCurrentGameState->Opponent.nPieces ) {
			DEBUG("ERROR: Bookkeeping failed, >%i units identified",
				gpCurrentGameState->Opponent.nPieces);
		}
		gpCurrentGameState->Opponent.nIdentified ++;

		if( Piece->GuessedRank != RANK_UNKNOWN && Piece->GuessedRank != rank )
		{
			fprintf(stderr, "Assumption failed, saved %c != act %c",
				cRANK_CHARS[Piece->GuessedRank], cRANK_CHARS[rank]);
			gpCurrentGameState->Opponent.bGuessValid = false;
		}

	}
	Piece->Rank = rank;
	if( Piece->Team == !gMyColour )
	{
		// Expensive? What's that?
		DB_WriteBackInitialState(gsOpponentDbFilename, !gMyColour, gpCurrentGameState->Opponent.Pieces);
	}
}

void PieceExposed(tPiece *Piece)
{
	ASSERT(Piece->Team == gMyColour);
	if( Piece->bExposed == false )
	{
		gpCurrentGameState->MyExposed.nRanks[Piece->Rank] ++;
		gpCurrentGameState->MyExposed.nIdentified ++;
		Piece->bExposed = true;
	}
}

/**
 * \brief Remove a piece from the board
 */
void RemovePiece(tPiece *Piece)
{
	tPlayerStats	*owner;
	gpCurrentGameState->BoardState[Piece->Y*giBoardWidth + Piece->X].Team = 0;
	if( Piece->Team == !gMyColour ) {
		owner = &gpCurrentGameState->Opponent;
	}
	else {
		owner = &gpCurrentGameState->MyExposed;
		gpCurrentGameState->MyActual.nRanks[Piece->Rank] --;
	}
	owner->nKilledRanks[Piece->Rank] ++;
	owner->nRanks[Piece->Rank] --;
	owner->nIdentified --;
	owner->nPieces --;
	Piece->bDead = true;
}

// ----------------------------------------------------------------------------
// - AI Core
// ----------------------------------------------------------------------------
#define TARGET_GRID_W	10
#define TARGET_GRID_H	10
#define TARGET_GRID_SIZE	(TARGET_GRID_W*TARGET_GRID_H)
typedef struct sGridSlot {
	tPiece	*p;
	char	dist;
	char	complexity;
	enum eDirections	firstdir;
	char	firstdist;
	char	bDirect;
} tTargetGrid[TARGET_GRID_SIZE];
int GetTargetsFrom(tPiece *Piece, tTargetGrid *grid)
{
	 int	n_targets;
	
	memset(*grid, 0, sizeof(*grid));

	int cur_dist = 1;
	int b_updates = 0;

	void _check_dir(struct sGridSlot *pgs, struct sGridSlot *gs, int x, int y, enum eDirections dir)
	{
		if( !gs )	return ;
		if( gs->dist )	return ;
		if( pgs->p )	return ;

		tPiece *p = GetPieceByPos(x, y);
		if( p && (p == &gBlockPiece || p->Team == Piece->Team) )
			p = (void*)-1;
		gs->dist = cur_dist + 1;
		gs->p = p;
		DEBUG("%p at %i,%i %i away", p, x, y, cur_dist);
		if( pgs->firstdir == DIR_INVAL || (pgs->firstdir == dir && pgs->bDirect) ) {
			gs->bDirect = 1;
			gs->firstdir = dir;
			gs->firstdist = pgs->firstdist + 1;
		}
		else {
			gs->firstdist = pgs->firstdist;
			gs->firstdir = pgs->firstdir;
			gs->bDirect = 0;
		}
		b_updates = 1;
	}

	(*grid)[ Piece->X + Piece->Y * TARGET_GRID_W ].dist = -1;

	do {
		b_updates = 0;
		for( int i = 0; i < TARGET_GRID_SIZE; i ++ )
		{
			int x = i % TARGET_GRID_W;
			int y = i / TARGET_GRID_H;
			struct sGridSlot	*gs = &(*grid)[i];

			struct sGridSlot	*gs_u = NULL, *gs_d = NULL;
			struct sGridSlot	*gs_l = NULL, *gs_r = NULL;

			if( !gs->dist )	continue ;

			// Get adjacent cells
			if( y > 0 )
				gs_u = &(*grid)[i - TARGET_GRID_W];
			if( x > 0 )
				gs_l = &(*grid)[i - 1];
			if( y < TARGET_GRID_H - 1 )
				gs_d = &(*grid)[i + TARGET_GRID_W];
			if( x < TARGET_GRID_W - 1 )
				gs_r = &(*grid)[i + 1];
			
			_check_dir(gs, gs_u, x, y-1, DIR_UP);
			_check_dir(gs, gs_d, x, y+1, DIR_DOWN);
			_check_dir(gs, gs_l, x-1, y, DIR_LEFT);
			_check_dir(gs, gs_r, x+1, y, DIR_RIGHT);
		}

		cur_dist ++;
	} while(b_updates);

#if SHOW_TARGET_MAP
	fprintf(stderr, "%p Type %c\n", Piece, cRANK_CHARS[Piece->Rank]);
	for( int i = 0; i < 10*10; i ++ )
	{
		tPiece	*np = (*grid)[i].p;
		if( i == Piece->X + Piece->Y * TARGET_GRID_W )
			fprintf(stderr, "?");
		else if( (*grid)[i].dist == 0 )
			fprintf(stderr, "#");	// Unreachable
		else if( !np )
			fprintf(stderr, " ");	// Empty
		else if( np == (void*)-1 )
			fprintf(stderr, ".");	// My team/block
		else
			fprintf(stderr, "X");	// Viable target!
		if( i % 10 == 9 )
			fprintf(stderr, "\n");
	}
#endif

	DEBUG("Getting targets");
	n_targets = 0;
	for( int i = 0; i < TARGET_GRID_SIZE; i ++ )
	{
		if( (*grid)[i].p == (void*)-1 )
			(*grid)[i].p = NULL;
		if( (*grid)[i].p ) {
			DEBUG("Target (%i,%i) %p %i dist",
				i%10, i/10, (*grid)[i].p, (*grid)[i].dist);
			(*grid)[i].dist -= 1;
			n_targets ++;
		}
	}

	return n_targets;
}

int GetBestMove(tPlayerStats *Attacker, tPlayerStats *Defender, tMove *Move, int Level)
{
	// Avoid infinite recursion
	if( Level == MAX_SEARCH_DEPTH )	return 1;

	 int	best_score = INT_MIN;
	tMove	best_move;
	tPiece	*best_p = NULL;
	tPiece	*best_target = NULL;
	tTargetGrid	grid;

	// - Check possible moves
	for( int i = 0; i < N_PIECES; i ++ )
	{
		tPiece	*p = &Attacker->Pieces[i];
		 int	p_score = 0;	// Piece score
		struct sGridSlot	*bt_gs = NULL;
		 int	bt_score;	// Best target score

		// Dead, ignore
		if( p->bDead )
			continue ;
		// These cannot move
		if( p->Rank == RANK_BOMB || p->Rank == RANK_FLAG )
			continue ;

		// Get what pieces are able to be attacked from this piece
		int nt = GetTargetsFrom(p, &grid);
		DEBUG("(%i,%i) %i targets", p->X, p->Y, nt);
		if( nt <= 0 )	continue ;

		// Find the best target of those
		for( int j = 0; j < TARGET_GRID_SIZE; j ++ )
		{
			struct sGridSlot *gs = &grid[j];
			if( !gs->p )	continue ;

			int t_score = GetScore(p, gs->p);

#if 1
			if( gs->p == gLastMove_Target && p == gLastMove_Piece && giNumRepeatedMove > REPEAT_THRESHOLD)
			{
				REPEAT_MOVE_MODIFY(t_score);
			}
#endif

			// TODO: For scouts, use gs->complexity
			// TODO: Don't use a linear relationship on distance
			p_score += t_score / (gs->dist < 2 ? 1 : 2);

			// Best target
			if( !bt_gs || t_score > bt_score ) {
				bt_score = t_score;
				bt_gs = gs;
			}
		}

		DEBUG("p_score = %i, bt_score = %i", p_score, bt_score);

		// Best move is towards that piece
		if( best_move.dir == DIR_INVAL || best_score < p_score )
		{
			best_move.dir = bt_gs->firstdir;
			best_move.x = p->X;
			best_move.y = p->Y;
			best_move.dist = (p->Rank == RANK_SCOUT) ? bt_gs->firstdist : 1;
			best_score = p_score;
			best_p = p;
			best_target = bt_gs->p;
		}
	}


	if( Move )
	{
		ASSERT(best_move.dir != DIR_INVAL);
		*Move = best_move;
		
		if( ((Move->dir-1)^1) == gLastMove.dir-1 && Move->dist == gLastMove.dist
		&& Move->x == gLastMove.x && Move->y == gLastMove.y )
		{
			giNumRepeatedMove ++;
			DEBUG("Up to %i repititions", giNumRepeatedMove);
		}
		else
			giNumRepeatedMove = 0;

		// TODO: Recurse once on this to determine what the other team will do
		// Record that move, then check when the move is performed to see if we were right.

		gLastMove = *Move;
		gLastMove_Target = best_target;
		gLastMove_Piece = best_p;
		giTurnsSinceLastTake ++;
	}

	DEBUG("best_score = %i", best_score);

	return best_score;
}

/**
 * \brief 
 */
int GetScore(tPiece *This, tPiece *Target)
{
	tPlayerStats	*attacker, *defender;
	int score;

	if( This->Team == gMyColour ) {
		defender = &gpCurrentGameState->Opponent;
		attacker = &gpCurrentGameState->MyExposed;
	}
	else {
		attacker = &gpCurrentGameState->Opponent;
		defender = &gpCurrentGameState->MyExposed;
	}

	score = GetRawScore(This, Target);

	if( This->Team == gMyColour )
	{
		switch( This->Rank )
		{
		case RANK_MARSHAL:	// Marshal has balls of steel if the spy and enemy marshal are dead
			if( defender->nKilledRanks[RANK_MARSHAL] && defender->nKilledRanks[RANK_SPY] )
				score *= 2;
			break;
		case RANK_GENERAL:	// General always attacks!
			score *= 2;
			break;
		case RANK_SCOUT:
			score = score * gpCurrentGameState->MyActual.nRanks[RANK_SCOUT] / MAX_RANK_COUNTS[RANK_SCOUT] + score;
			break;
		default:
			break;
		}
	}

	return score;
}

int GetRawScore(tPiece *This, tPiece *Target)
{
	tPlayerStats	*this_team, *target_team;
	
	ASSERT( This->Team != Target->Team );

	if( This->Team == gMyColour ) {
		target_team = &gpCurrentGameState->Opponent;
		this_team = &gpCurrentGameState->MyExposed;
	}
	else {
		this_team = &gpCurrentGameState->Opponent;
		target_team = &gpCurrentGameState->MyExposed;
	}

	// Both ranks known, used fixed rules
	if( This->Rank != RANK_UNKNOWN && Target->Rank != RANK_UNKNOWN )
	{
		return GetOutcome(This->Rank, Target->Rank);
	}
	// If it's our move, and the guesses are valid, then use the guess
	else if( This->Team == gMyColour
		&& gpCurrentGameState->Opponent.bGuessValid
		&& Target->GuessedRank != RANK_UNKNOWN
		)
	{
		return GetOutcome(This->Rank, Target->GuessedRank);
	}
	else
	{
		 int	sum = 0;
		 int	max = Target->bHasMoved ? RANK_SPY : RANK_FLAG;
		 int	count = 0;

		if( target_team->nIdentified >= target_team->nPieces ) {
			DEBUG("%i >= %i, what the fsck",
				target_team->nIdentified, target_team->nPieces);
		}
		ASSERT(target_team->nPieces > target_team->nIdentified);

		for( int i = RANK_MARSHAL; i <= max; i ++ )
		{
			 int	n_unk = MAX_RANK_COUNTS[i] - (target_team->nRanks[i] + target_team->nKilledRanks[i]);
			if( n_unk == 0 )
				continue ;
			ASSERT( n_unk > 0 );

			// TODO: Fiddle with outcome score depending on respective ranks
			sum += n_unk * GetOutcome(This->Rank, i);
			count += n_unk;
		}

//		if( Target->bHasMoved )
//			sum /= target_team->nPieces - target_team->nMoved;
//		else
//			sum /= target_team->nPieces - target_team->nIdentified;
		sum /= count;

		if( sum > OC_FLAG ) {
			fprintf(stderr, "sum (%i) > OC_WIN (%i) -- nUnIdent=%i\n",
				sum, OC_WIN, target_team->nPieces - target_team->nIdentified);
			ASSERT( sum <= OC_FLAG );
		}

		return sum - ABS(sum) / 10;
	}
}

static inline int GetOutcome(int Attacker, int Defender)
{
	if( Attacker == 0 )	return OC_UNK;
	if( Defender == 0 )	return OC_UNK;

	if( Defender == RANK_FLAG )
		return OC_FLAG;

	if( Attacker != RANK_MINER && Defender == RANK_BOMB )
		return OC_LOSE;
	if( Attacker == RANK_MINER && Defender == RANK_BOMB )
		return OC_WIN;

	if( Attacker == RANK_SPY && Defender == RANK_MARSHAL )
		return OC_WIN;

	if( Attacker == Defender )
		return OC_DRAW;

	if( Attacker < Defender )
		return OC_WIN;
	else
		return OC_LOSE;
}


static inline int ABS(int Val)
{
	return Val < 0 ? -Val : Val;
}

static inline int RANGE(int Min, int Val, int Max)
{
	return Min <= Val && Val <= Max;
}

