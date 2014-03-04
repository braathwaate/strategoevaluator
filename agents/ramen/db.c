/*
 * UCC 2012 Programming Competition Entry
 * - "Ramen"
 *
 * By John Hodge [TPG]
 */
#include <stdio.h>
#include <stdlib.h>
#include "ai_common.h"
#include <stdint.h>
#include <string.h>

#define TAG_BOARDSTATE	0x7342

typedef struct sTag
{
	uint16_t	Tag;
	uint16_t	Length;
} tTag;

typedef struct sSavedBoardState
{
	uint8_t 	W, H;
	uint16_t	Count;
	char	NormalisedBoard[];
} tSavedBoardState;

// === PROTOTYPES ===
tSavedBoardState	*DB_int_ReadState(FILE *fp, off_t *offset);
void	DB_int_AppendTag(FILE *fp, uint16_t Tag, uint16_t Size, void *Data);

// === CODE ===
char *DB_GetOpponentFile(const char *Opponent)
{
	uint32_t	checksum = 0;

	{
		 int	ofs = 0;
		const char *str = Opponent;
		while( *str )
		{
			checksum ^= *str << ofs;
			str ++;
			ofs += 5;
			ofs %= 32 - 5;
		}
	}
	
	const char	*filename = NULL;
	 int	filenamelen = 0;
	{
		const char *last_slash = NULL;
		const char *last_dot = NULL;
		const char *str = Opponent;
		while( *str )
		{
			if(*str == '/')	last_slash = str;
			if(*str == '.')	last_dot = str;
			str ++;
		}
		filename = last_slash + 1;
		if( last_slash > last_dot )
			filenamelen = str - filename;
		else
			filenamelen = last_dot - filename;
	}

	int len = snprintf(NULL, 0, "%08x_%.*s.ramen", checksum, filenamelen, filename);
	char *ret = malloc(len+1);
	snprintf(ret, len+1, "%08x_%.*s.ramen", checksum, filenamelen, filename);
//	fprintf(stderr, "DB File = '%s'\n", ret);
	return ret;
}

void DB_LoadGuesses(const char *DBFile, enum eColours Colour)
{
	FILE	*fp;
	off_t	offset = 0;
	tSavedBoardState	*saved_board = NULL;

	fp = fopen(DBFile, "r+");
	if(!fp)	return ;

	// Read board states, checking for a same state
	while( (saved_board = DB_int_ReadState(fp, &offset)) )
	{
		if( saved_board->W != giBoardWidth )
			continue ;
		if( saved_board->H != giBoardHeight )
			continue ;
		break;
	}

	// TODO: Combine counts of how many times a state has been played

	if( saved_board )
	{
		char	bs[giBoardWidth*4];
		 int	ofs = 0;


		if( Colour != COLOUR_RED )
		{
			ofs = giBoardHeight-4;
			char	*bs2 = saved_board->NormalisedBoard;
			memcpy(bs + giBoardWidth*0, bs2 + giBoardWidth*(4-1), giBoardWidth);
			memcpy(bs + giBoardWidth*1, bs2 + giBoardWidth*(4-2), giBoardWidth);
			memcpy(bs + giBoardWidth*2, bs2 + giBoardWidth*(4-3), giBoardWidth);
			memcpy(bs + giBoardWidth*3, bs2 + giBoardWidth*(4-4), giBoardWidth);
		}
		else
		{
			memcpy(bs, saved_board->NormalisedBoard, giBoardWidth*4);
		}
//		for( int i = 0; i < 4; i ++ ) {
//			fprintf(stderr, "%.*s\n", giBoardWidth, bs + giBoardWidth*i);
//		}

		// Set guessed ranks
		for( int i = 0; i < giBoardWidth*4; i ++ )
		{
			tPiece	*p = GetPieceByPos(i % giBoardWidth, i/giBoardWidth + ofs );
//			fprintf(stderr, "%c", bs[i]);
//			if(i % giBoardWidth == giBoardWidth-1)
//				fprintf(stderr, "\n");
			if( bs[i] == '\0' && p )
				break;
			if( bs[i] != '\0' && !p )
				break;
			if( p )
				p->GuessedRank = CharToRank(bs[i]);
		}
	}

	fclose(fp);
}

void DB_WriteBackInitialState(const char *DBFile, enum eColours Colour, tPiece *Pieces)
{
	char	bs[giBoardHeight*giBoardWidth];
	memset(bs, 0, sizeof(bs));

	for( int i = 0; i < N_PIECES; i ++ )
	{
		if( Pieces[i].StartY < 0 )	continue ;
		char	*bp = &bs[ Pieces[i].StartY*giBoardWidth + Pieces[i].StartX ];

		if( *bp != '\0' )
		{
			// Oops?
		}
		else
		{
			*bp = cRANK_CHARS[ Pieces[i].Rank ];
		}
	}

	// Normalise board to RED
	if( Colour != COLOUR_RED )
	{
		memcpy(bs + giBoardWidth*0, bs + giBoardWidth*(giBoardHeight-1), giBoardWidth);
		memcpy(bs + giBoardWidth*1, bs + giBoardWidth*(giBoardHeight-2), giBoardWidth);
		memcpy(bs + giBoardWidth*2, bs + giBoardWidth*(giBoardHeight-3), giBoardWidth);
		memcpy(bs + giBoardWidth*3, bs + giBoardWidth*(giBoardHeight-4), giBoardWidth);
	}


	off_t	offset;
	tSavedBoardState	*saved_board;
	FILE *fp = fopen(DBFile, "r+");
	if( !fp ) {
		fp = fopen(DBFile, "w");
	}

	// Read board states, checking for a same state
	while( (saved_board = DB_int_ReadState(fp, &offset)) )
	{
//		fprintf(stderr, "DBG: %i == %i? and %i == %i\n",
//			saved_board->W, giBoardWidth, saved_board->H, giBoardHeight
//			);

		if( saved_board->W != giBoardWidth )
			continue ;
		if( saved_board->H != giBoardHeight )
			continue ;

		BOOL	b_different = false;

		for( int i = 0; i < 4*giBoardWidth; i ++ )
		{
			if( saved_board->NormalisedBoard[i] == '#' || bs[i] == '#' )
				continue ;
			if( saved_board->NormalisedBoard[i] != bs[i] ) {
				fprintf(stderr, "DBG: '%c' != '%c'\n", saved_board->NormalisedBoard[i], bs[i]);
				b_different = true;
				break;
			}
		}

		if( b_different )	continue ;

		break;
	}

	if( saved_board )
	{
		saved_board->Count ++;
		fseek(fp, offset, SEEK_SET);
		// Merge
		for( int i = 0; i < 4*giBoardWidth; i ++ )
		{
			if( saved_board->NormalisedBoard[i] == '#' )
				saved_board->NormalisedBoard[i] = bs[i];
		}
		// Write back
		fwrite(saved_board, sizeof(*saved_board) + giBoardWidth*4, 1, fp);
	}
	else
	{
		saved_board = malloc( sizeof(*saved_board) + giBoardWidth*4 );
		saved_board->W = giBoardWidth;
		saved_board->H = giBoardHeight;
		saved_board->Count = 1;
		memcpy(saved_board->NormalisedBoard, bs, 4*giBoardWidth);
		DB_int_AppendTag(fp, TAG_BOARDSTATE, sizeof(*saved_board) + giBoardWidth*4, saved_board);
	}
	free(saved_board);

	fclose(fp);
}

tSavedBoardState *DB_int_ReadState(FILE *fp, off_t *offset)
{
	tTag	tag;
	tSavedBoardState	*ret = NULL;

	do {
		if( fread(&tag, sizeof(tag), 1, fp) != 1 )
			break ;
		if( tag.Tag == TAG_BOARDSTATE )
		{
			*offset = ftell(fp);
			ret = malloc(tag.Length);
			fread(ret, tag.Length, 1, fp);
		}
		fseek(fp, tag.Length, SEEK_CUR);
	} while(!ret);

	return ret;
}

void DB_int_AppendTag(FILE *fp, uint16_t Tag, uint16_t Size, void *Data)
{
	fseek(fp, 0, SEEK_END);
	fwrite(&Tag, sizeof(uint16_t), 1, fp);
	fwrite(&Size, sizeof(uint16_t), 1, fp);
	fwrite(Data, Size, 1, fp);
}
