/*
 * UCC 2012 Programming Competition Entry
 * - "Ramen"
 *
 * By John Hodge [TPG]
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#if ENABLE_DEBUG
# define DEBUG(s, a...)	fprintf(stderr, "DEBUG: "s"\n" ,## a)
#else
# define DEBUG(...)	do{}while(0)
#endif
#define ASSERT(val)	do{if(!(val)){fprintf(stderr, "ASSERTION FAILED - " #val " at %s:%i\n", __FILE__, __LINE__);exit(-1);} }while(0)


#define true	1
#define false	0
typedef char	BOOL;

typedef struct sMove	tMove;

enum eDirections
{
	DIR_INVAL,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN
};

enum eColours
{
	COLOUR_RED,
	COLOUR_BLUE
};

enum eResult
{
	RESULT_INVAL,
	RESULT_ILLEGAL,
	RESULT_OK,
	RESULT_KILL,
	RESULT_DIES,
	RESULT_BOTHDIE,
	RESULT_VICTORY
};

struct sMove
{
	char	x, y;
	enum eDirections	dir;	// eDirections
	char	dist;

	enum eResult	result;
	char	attacker;
	char	defender;
};

extern int	giBoardWidth;
extern int	giBoardHeight;
extern char	*gaBoardState;

extern void	AI_Initialise(enum eColours Colour, const char *Opponent);
extern void	AI_HandleMove(int bMyMove, const tMove *Move);
extern void	AI_DoMove(tMove *MyMove);

#endif

