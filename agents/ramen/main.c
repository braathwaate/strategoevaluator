/*
 * UCC 2012 Programming Competition Entry
 * - "Ramen"
 *
 * By John Hodge [TPG]
 */
#define ENABLE_DEBUG	0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "interface.h"

// === CONSTANTS ===
static const char *DIR_NAMES[] = {"INVL", "LEFT", "RIGHT", "UP", "DOWN"};

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
void	GetMove(char *line, tMove *Move);
void	ReadBoardState(FILE *stream, char *dest);
 int	RunRegex(regex_t *regex, const char *string, int nMatches, regmatch_t *matches, const char *errorMessage);
void	CompileRegex(regex_t *regex, const char *pattern, int flags);

// === GLOBALS ===
regex_t	gRegex_move;
regex_t	gRegex_res;
 int	giBoardWidth;
 int	giBoardHeight;
char	*gaBoardState;

// === CODE ===
int main(int argc, char *argv[])
{
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	// $X $Y $DIRECTION [$MULTIPLIER=1] $OUTCOME
	CompileRegex(&gRegex_move, "([0-9]+) ([0-9]+) ([A-Z]+)( [0-9]+)? (.*)", REG_EXTENDED);
	// (KILLS|DIES|BOTHDIE) $ATTACKER_RANK $DEFENDER_RANK
	CompileRegex(&gRegex_res, "([A-Z_]+) (.) (.)", REG_EXTENDED);

	{
		 int	colour_id;
		char	colour[6];
		char	opponent[128];
		fscanf(stdin, "%s %s %i %i", colour, opponent, &giBoardWidth, &giBoardHeight);

		if( strcmp(colour, "RED") == 0 )
			colour_id = COLOUR_RED;
		else if( strcmp(colour, "BLUE") == 0 )
			colour_id = COLOUR_BLUE;
		else {
			fprintf(stderr, "Oops... nutty manager, colour = %s\n", colour);
			colour_id = COLOUR_RED;
		}

		DEBUG("colour=%i, opponent='%s', dims = %ix%i", colour_id, opponent, giBoardWidth, giBoardHeight);

		AI_Initialise(colour_id, opponent);
	}
	
	gaBoardState = malloc(giBoardWidth*giBoardHeight);

	for( ;; )
	{
		tMove	mymove, opponent_move;
		char	line[32];

//		DEBUG("Waiting for move");
		ASSERT( fgets(line, sizeof(line), stdin) != NULL );
//		DEBUG("pm line = '%s'", line);

		if( strcmp(line, "\n") == 0 )
			continue ;

		if( strcmp(line, "START\n") == 0 )
		{
//			DEBUG("New game");
			ReadBoardState(stdin, gaBoardState);
			// TODO: Check if this hasn't happened before
			opponent_move.x = 0;
			opponent_move.y = 0;
			opponent_move.dist = 0;
			opponent_move.dir = 0;
		}
		else if( strncmp(line, "QUIT", 4) == 0 )
		{
			// TODO: Result?
			break ;
		}
		else if( strcmp(line, "VICTORY_FLAG\n") == 0 )
		{
			// I win!
			break;
		}
		else
		{
//			DEBUG("GetMove");
			GetMove(line, &opponent_move);
//			DEBUG("Read board state");
			ReadBoardState(stdin, gaBoardState);
		}
		DEBUG("Opposing move %i,%i dir %i dist %i",
			opponent_move.x, opponent_move.y, opponent_move.dir, opponent_move.dist);

		// Silly opponent, you lost
		if( opponent_move.result == RESULT_VICTORY )
			break;

		// Determine move
		AI_HandleMove(0, &opponent_move);
		AI_DoMove(&mymove);
		DEBUG("Chose move %i,%i %i %i", mymove.x, mymove.y, mymove.dir, mymove.dist);
		printf("%i %i %s %i\n", mymove.x, mymove.y, DIR_NAMES[mymove.dir], mymove.dist);

		// Get result of the move
		ASSERT( fgets(line, sizeof(line), stdin) != NULL );
//		DEBUG("res line = '%s'", line);
//
		GetMove(line, &mymove);
		AI_HandleMove(1, &mymove);

		// I WON!
		if( mymove.result == RESULT_VICTORY )
			break;

//		DEBUG("Move over");
	}

	return 0;
}

void GetMove(char *line, tMove *Move)
{
	regmatch_t	matches[1+5];

	// regex (\d+) (\d+) ([A-Z]*)(?: (\d+))?
	RunRegex(&gRegex_move, line, 1+5, matches, "Move line");

	char *xstr = line + matches[1].rm_so;
	char *ystr = line + matches[2].rm_so;
	char *dirstr = line + matches[3].rm_so;

	Move->x = atoi(xstr);
	Move->y = atoi(ystr);
//	DEBUG("(%i,%i)", Move->x, Move->y);
	// Direction
	     if( strncmp(dirstr, "UP",    2) == 0 )
		Move->dir = DIR_UP;
	else if( strncmp(dirstr, "DOWN",  4) == 0 )
		Move->dir = DIR_DOWN;
	else if( strncmp(dirstr, "LEFT",  4) == 0 )
		Move->dir = DIR_LEFT;
	else if( strncmp(dirstr, "RIGHT", 5) == 0 )
		Move->dir = DIR_RIGHT;
	else {
		fprintf(stderr, "Is the manager nuts? Dir = %.*s unk\n",
			matches[3].rm_eo + matches[3].rm_so, dirstr
			);
		fprintf(stderr, "line = '%s'\n", line);
		Move->dir = DIR_INVAL;
	}
	if( matches[4].rm_so >= 0 )
		Move->dist = atoi(line + matches[4].rm_so + 1);
	else
		Move->dist = 1;
	
	// Outcome
	char	*outcome = line + matches[5].rm_so;
	if( strncmp(outcome, "OK", 2) == 0 )
		Move->result = RESULT_OK;
	else if( strncmp(outcome, "ILLEGAL", 7) == 0 )
		Move->result = RESULT_ILLEGAL;
	else if( strncmp(outcome, "VICTORY_FLAG", 12) == 0 )
		Move->result = RESULT_VICTORY;
	else if( strncmp(outcome, "VICTORY_ATTRITION", 17) == 0 )
		Move->result = RESULT_VICTORY;
	else
	{
		regmatch_t res_matches[3+1];
		RunRegex(&gRegex_res, outcome, 3+1, res_matches, "Result portion");

		char *res_str = outcome + res_matches[1].rm_so;
		     if( strncmp(res_str, "KILLS ", 6) == 0 )
			Move->result = RESULT_KILL;
		else if( strncmp(res_str, "DIES ", 5) == 0 )
			Move->result = RESULT_DIES;
		else if( strncmp(res_str, "BOTHDIE ", 8) == 0 )
			Move->result = RESULT_BOTHDIE;
		else {
			fprintf(stderr, "Is the manager nuts? Result = %.*s\n",
				res_matches[1].rm_eo + res_matches[1].rm_so, res_str
			       );
			Move->result = RESULT_INVAL;
		}

		Move->attacker = *(outcome + res_matches[2].rm_so);
		Move->defender = *(outcome + res_matches[3].rm_so);
	}
}

void ReadBoardState(FILE *stream, char *dest)
{
	for( int i = 0; i < giBoardHeight; i ++ )
	{
		char	tmp[giBoardWidth+2];
		fgets(tmp, sizeof(tmp), stream);
		DEBUG("BS %.*s", giBoardWidth, tmp);
		memcpy(dest+i*giBoardWidth, tmp, giBoardWidth);
	}
}

int RunRegex(regex_t *regex, const char *string, int nMatches, regmatch_t *matches, const char *errorMessage)
{
	 int	ret;
	
	ret = regexec(regex, string, nMatches, matches, 0);
	if( ret ) {
		size_t  len = regerror(ret, regex, NULL, 0);
		char    errorStr[len];
		regerror(ret, regex, errorStr, len);
		fprintf(stderr, "string = '%s'\n", string);
		fprintf(stderr, "%s\n%s", errorMessage, errorStr);
		exit(-1);
	}
	
	return ret;
}

void CompileRegex(regex_t *regex, const char *pattern, int flags)
{
	 int	ret = regcomp(regex, pattern, flags);
	if( ret ) {
		size_t	len = regerror(ret, regex, NULL, 0);
		char    errorStr[len];
		regerror(ret, regex, errorStr, len);
		fprintf(stderr, "Regex compilation failed - %s\n", errorStr);
		exit(-1);
	}
}
