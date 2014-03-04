

#include "stratego.h"

using namespace std;

/**
 * Static variables
 */

//nothing, boulder, flag, spy, scout, miner, sergeant, lietenant, captain, major, colonel, general, marshal, bomb, error
char  Piece::tokens[] = {'.','*','F','s','9','8','7','6','5','4','3','2','1','B','?'};
int Piece::maxUnits[] = {0,0,1,1,8,5,4,4,4,3,2,1,1,6,0};



#ifdef BUILD_GRAPHICS
Piece::TextureManager Piece::textures;

Piece::TextureManager::~TextureManager()
{
	Array<Texture*>::Iterator i(*this);
	while (i.Good())
	{
		delete (*i);
		++i;
	}
}

Texture & Piece::TextureManager::operator[](const LUint & at)
{
	while (Array<Texture*>::Size() <= at)
	{
		char buffer[BUFSIZ];
		sprintf(buffer, "images/piece%lu.bmp", Array<Texture*>::Size());
		Array<Texture*>::Add(new Texture(buffer, false));
		
	}
	return *(Array<Texture*>::operator[](at));
}
#endif //BUILD_GRAPHICS

/**
 * Gets the type of a piece, based off a character token
 * @param fromToken - character identifying the piece
 * @returns The type of the piece
 */
Piece::Type Piece::GetType(char fromToken)
{
	for (int ii=0; ii <= (int)(Piece::BOMB); ++ii)
	{
		if (tokens[ii] == fromToken)
		{
			return Type(Piece::NOTHING + ii);
		}
	}
	return Piece::BOULDER;
}

/**
 * Gets the opposite to the indicated colour
 */
Piece::Colour Piece::OppositeColour(const Colour & colour)
{
	switch (colour)
	{
		case Piece::RED:
			return Piece::BLUE;
			break;
		case Piece::BLUE:
			return Piece::RED;
			break;
		case Piece::BOTH:
			return Piece::BOTH;
			break;
		case Piece::NONE:
			return Piece::NONE;
	}
}

/**
 * Construct a new, empty board
 * @param newWidth - the width of the board
 * @param newHeight - the height of the board
 */
Board::Board(int newWidth, int newHeight) : winner(Piece::NONE), width(newWidth), height(newHeight), board(NULL), pieces()
{
	board = new Piece**[width];
	for (int x=0; x < width; ++x)
	{
		board[x] = new Piece*[height];
		for (int y=0; y < height; ++y)
			board[x][y] = NULL;
	}
}

/**
 * Cleanup a board
 */
Board::~Board()
{
	for (int x=0; x < width; ++x)
	{
		for (int y=0; y < height; ++y)
			delete board[x][y];
		delete [] board[x];
	}
}

/**
 * Print textual representation of the board to a stream
 * @param stream - the stream to print information to
 * @param reveal - Pieces matching this colour will have their identify revealed, other pieces will be shown as '#'
 */
void Board::Print(FILE * stream, const Piece::Colour & reveal)
{
	for (int y=0; y < height; ++y)
	{
		for (int x=0; x < width; ++x)
		{
			Piece * piece = board[x][y];
			if (piece == NULL)
			{
				fprintf(stream, ".");
			}
			else if (piece->colour != Piece::NONE && (piece->colour == reveal || reveal == Piece::BOTH))
			{

				fprintf(stream, "%c", Piece::tokens[piece->type]);


			}
			else
			{
				switch (piece->colour)
				{
					case Piece::RED:
					case Piece::BLUE:
						fprintf(stream, "#");
						break;
					case Piece::NONE:
						fprintf(stream, "+");
						break;
					case Piece::BOTH:
						fprintf(stream, "$");
						break;
				}
			}
		}
		fprintf(stream, "\n");
	}
	
}

/**
 * Print textual representation of the board to a stream
 * @param stream - the stream to print information to
 * @param reveal - Pieces matching this colour will have their identify revealed, other pieces will be shown as '#'
 */
void Board::PrintPretty(FILE * stream, const Piece::Colour & reveal, bool showRevealed)
{
	for (int y=0; y < height; ++y)
	{
		for (int x=0; x < width; ++x)
		{
			Piece * piece = board[x][y];
			if (piece == NULL)
			{
				fprintf(stream, ".");
			}
			else if ((piece->colour != Piece::NONE && (piece->colour == reveal || reveal == Piece::BOTH))
					|| (piece->beenRevealed && showRevealed))
			{
				switch (piece->colour)	
				{
					case Piece::RED:
						fprintf(stream, "%c[%d;%d;%dm",0x1B,1,31,40);
						break;
					case Piece::BLUE:
						fprintf(stream, "%c[%d;%d;%dm",0x1B,1,34,40);
						break;
					default:
						break;
				}
				fprintf(stream, "%c", Piece::tokens[piece->type]);

			}
			else
			{
				switch (piece->colour)
				{
					case Piece::RED:
						fprintf(stream, "%c[%d;%d;%dm",0x1B,1,31,41);

						break;
					case Piece::BLUE:
						fprintf(stream, "%c[%d;%d;%dm",0x1B,1,34,44);
						break;
					case Piece::NONE:
						fprintf(stream, "%c[%d;%d;%dm",0x1B,1,37,47);
						break;
					case Piece::BOTH:
						//Should never see this
						fprintf(stream, "%c[%d;%d;%dm",0x1B,1,33,43);
						break;

				}	
				fprintf(stream, "#");
				
			}
			fprintf(stream, "%c[%d;%d;%dm",0x1B,0,7,0);
		}
		fprintf(stream, "\n");
	}
	
}


#ifdef BUILD_GRAPHICS
/**
 * Draw the board state to graphics
 * @param reveal - Pieces matching this colour will be revealed. If Piece::BOTH, all pieces will be revealed
 * @param showRevealed - If true, then all pieces that have taken part in combat will be revealed, regardless of colour.
 *			 If false, only pieces matching the colour reveal will be revealed
 */
void Board::Draw(const Piece::Colour & reveal, bool showRevealed)
{

	if (!Graphics::Initialised())
	{
		fprintf(stderr, "ERROR - Board::Draw called whilst graphics disabled!!!\n");
		exit(EXIT_FAILURE);
		
	}

	Graphics::ClearScreen();
	float scale = (float)(Piece::textures[(int)(Piece::NOTHING)].width()) / (float)(GRID_SIZE);
	for (int y=0; y < height; ++y)
	{
		for (int x=0; x < width; ++x)
		{
			Piece * piece = board[x][y];
			if (piece == NULL)
			{
				//Don't display anything

			}
			else if ((piece->colour != Piece::NONE && (piece->colour == reveal || reveal == Piece::BOTH))
					|| (piece->beenRevealed && showRevealed))
			{
				//Display the piece
				
				Piece::textures[(int)(piece->type)].DrawColour(x*GRID_SIZE*scale,y*GRID_SIZE*scale,0,scale, Piece::GetGraphicsColour(piece->colour));
				
			}
			else
			{
				switch (piece->colour)
				{
					case Piece::RED:
						Piece::textures[(int)(Piece::NOTHING)].DrawColour(x*GRID_SIZE*scale,y*GRID_SIZE*scale,0,scale, Piece::GetGraphicsColour(piece->colour));
						break;
					case Piece::BLUE:
						Piece::textures[(int)(Piece::NOTHING)].DrawColour(x*GRID_SIZE*scale,y*GRID_SIZE*scale,0,scale, Piece::GetGraphicsColour(piece->colour));
						break;
					case Piece::NONE:
						Piece::textures[(int)(Piece::BOULDER)].DrawColour(x*GRID_SIZE*scale,y*GRID_SIZE*scale,0,scale, Piece::GetGraphicsColour(piece->colour));
						break;
					case Piece::BOTH:
						Piece::textures[(int)(Piece::BOULDER)].DrawColour(x*GRID_SIZE*scale,y*GRID_SIZE*scale,0,scale, Piece::GetGraphicsColour(piece->colour));
						break;
				}
			}
		}
		
	}
	Graphics::UpdateScreen();
	
}
#endif //BUILD_GRAPHICS

/**
 * Adds a piece to the board
 * @param x - x-coord to place the piece at, starting at zero, must be less than board width
 * @param y - y-coord to place the piece at, starting at zero, must be less than board height
 * @param newType - the Type of the piece
 * @param newColour - the Colour of the piece
 * @returns true if and only if the piece could be successfully added.
 */
bool Board::AddPiece(int x, int y, const Piece::Type & newType, const Piece::Colour & newColour)
{
	if (board == NULL || x < 0 || y < 0 || x >= width || y >= width || board[x][y] != NULL)
		return false;

	Piece * piece = new Piece(newType, newColour);
	board[x][y] = piece;

	pieces.push_back(piece);
	return true;
}

/**
 * Gets a pointer to a piece at a board location
 * UNUSED
 * @param x - x-coord of the piece
 * @param y - y-coord of the piece
 * @returns pointer to the piece, or NULL if the board location was empty
 * @throws error if board is null or coords are invalid
 */
Piece * Board::GetPiece(int x, int y)
{
	assert(board != NULL);
	assert(x >= 0 && x < width && y >= 0 && y < height);
	return board[x][y];
}

/**
 * Moves a piece at a specified position in the specified direction, handles combat if necessary, updates state of the board
 * @param x - x-coord of the piece
 * @param y - y-coord of the piece
 * @param direction - Direction in which to move (UP, DOWN, LEFT or RIGHT)
 * @param colour - Colour which the piece must match for the move to be valid
 * @returns A MovementResult which indicates the result of the move
 */
MovementResult Board::MovePiece(int x, int y, const Direction & direction, int multiplier,const Piece::Colour & colour)
{
	if (board == NULL) 
	{
		return MovementResult(MovementResult::NO_BOARD);
	}
	if (!(x >= 0 && x < width && y >= 0 && y < height)) 
	{
		return MovementResult(MovementResult::INVALID_POSITION);
	}
	Piece * target = board[x][y];
	if (target == NULL) 
	{
		return MovementResult(MovementResult::NO_SELECTION);
	}
	if (!(colour == Piece::NONE || target->colour == colour)) 
	{
		return MovementResult(MovementResult::NOT_YOUR_UNIT);
	}
	if (target->type == Piece::FLAG || target->type == Piece::BOMB || target->type == Piece::BOULDER) 
	{
		return MovementResult(MovementResult::IMMOBILE_UNIT);
	}
	if (multiplier < 1)
		return MovementResult(MovementResult::INVALID_DIRECTION); //Don't allow moves that don't actually move forward
	if (multiplier > 1 && target->type != Piece::SCOUT)
	{
		return MovementResult(MovementResult::INVALID_DIRECTION); //Can only move a scout multiple times.
	}
	int x2 = x; int y2 = y;

	for (int ii=0; ii < multiplier; ++ii)
	{
		switch (direction)
		{
			case UP:
				--y2;
				break;
			case DOWN:
				++y2;
				break;
			case LEFT:
				--x2;
				break;
			case RIGHT:
				++x2;
				break;
		}
		if (!(x2 >= 0 && x2 < width && y2 >= 0 && y2 < height)) 
		{
			return MovementResult(MovementResult::INVALID_DIRECTION);
		}
		if (ii < multiplier-1 && board[x2][y2] != NULL)
		{
			return MovementResult(MovementResult::POSITION_FULL);
		}
	}
	Piece * defender = board[x2][y2];
	if (defender == NULL)
	{
		board[x][y] = NULL;
		board[x2][y2] = target;
	}
	else if (defender->colour != target->colour)
	{
		defender->beenRevealed = true;
		target->beenRevealed = true;

		Piece::Type defenderType = defender->type;
		Piece::Type attackerType = target->type;

		if (defender->colour == Piece::NONE) 
		{
			return MovementResult(MovementResult::POSITION_FULL);
		}
		if (defender->type == Piece::FLAG)
		{
			winner = target->colour;
			return MovementResult(MovementResult::VICTORY_FLAG);
		}
		else if (defender->type == Piece::BOMB)
		{
			if (target->type == Piece::MINER)
			{
				RemovePiece(defender);
				delete defender;
				board[x][y] = NULL;
				board[x2][y2] = target;
				return MovementResult(MovementResult::KILLS, attackerType, defenderType);
			}
			else
			{
				//Use this to destroy only the attacking piece, and not the bomb
				RemovePiece(target);
				delete target;
				board[x][y] = NULL;
				return MovementResult(MovementResult::DIES, attackerType, defenderType);

				/*
				//Use this to destroy both the bomb and the attacking piece
				RemovePiece(defender);
				RemovePiece(target);
				delete defender;
				delete target;
				board[x][y] = NULL;
				board[x2][y2] = NULL;
				return MovementResult(MovementResult::BOTH_DIE, attackerType, defenderType);
				*/
			}
		}
		else if (defender->type == Piece::MARSHAL && target->type == Piece::SPY)
		{
			RemovePiece(defender);
			delete defender;
			board[x][y] = NULL;
			board[x2][y2] = target;
			return MovementResult(MovementResult::KILLS, attackerType, defenderType);
		}
		else if (target->operator > (*defender))
		{
			RemovePiece(defender);
			delete defender;
			board[x][y] = NULL;
			board[x2][y2] = target;
			return MovementResult(MovementResult::KILLS, attackerType, defenderType);
		}
		else if (target->operator==(*defender))// && rand() % 2 == 0)
		{
			RemovePiece(defender);
			RemovePiece(target);
			delete defender;
			delete target;
			board[x][y] = NULL;
			board[x2][y2] = NULL;	
			return MovementResult(MovementResult::BOTH_DIE, attackerType, defenderType);
		}
		else
		{
			RemovePiece(target);
			delete target;
			board[x][y] = NULL;
			return MovementResult(MovementResult::DIES, attackerType, defenderType);
		}
	}
	else
	{
		return MovementResult(MovementResult::POSITION_FULL);
	}
	return MovementResult(MovementResult::OK);
}	

/**
 * Removes a piece from the board
 * @param piece The piece to remove
 * @returns true iff the piece actually existed
 */
bool Board::RemovePiece(Piece * piece)
{
	bool result = false;
	for (int x = 0; x < width; ++x)
	{
		for (int y = 0; y < height; ++y)	
		{
			if (board[x][y] == piece)
			{
				result = true;
				board[x][y] = NULL;
			}
		}
	}

	vector<Piece*>::iterator i = pieces.begin();
	while (i != pieces.end())
	{
		if ((*i) == piece)
		{
			i = pieces.erase(i);
			result = true;
			continue;
		}
		++i;
	}
	return result;
}

/**
 * Returns the total value of pieces belonging to colour
 * @param colour the colour
 * @returns the total value of pieces belonging to colour.
 *	(Redundant repetition <3)
 */
int Board::TotalPieceValue(const Piece::Colour & colour) const
{
	int result = 0;
	for (vector<Piece*>::const_iterator i = pieces.begin(); i != pieces.end(); ++i)
	{
		if ((*i)->colour == colour || colour == Piece::BOTH)
		{
			result += (*i)->PieceValue();
		}
	}
	return result;
}

/**
 * Returns the total number of mobile pieces belonging to colour
 * @param colour the colour
 * @returns the total value of mobile pieces belonging to colour.
 *	(Redundant repetition <3)
 */
int Board::MobilePieces(const Piece::Colour & colour) const
{
	int result = 0;
	for (vector<Piece*>::const_iterator i = pieces.begin(); i != pieces.end(); ++i)
	{
		if ((*i)->colour == colour || colour == Piece::BOTH)
		{
			if ((*i)->type <= Piece::MARSHAL && (*i)->type >= Piece::SPY)
				result++;
		}
	}
	return result;
}


