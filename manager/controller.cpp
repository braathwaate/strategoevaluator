#include "controller.h"

#include <sstream>
#include "game.h"

using namespace std;

/**
 * Queries the player to setup their pieces
 *
 */

MovementResult Controller::Setup(const char * opponentName)
{
	string setup[4] = {"","","",""};
	MovementResult query = this->QuerySetup(opponentName, setup);
	if (query != MovementResult::OK)
		return query;

	

	int usedUnits[(int)(Piece::BOMB)];
	for (int ii = 0; ii <= (int)(Piece::BOMB); ++ii)
		usedUnits[ii] = 0;

	int yStart = 0;
	switch (colour)
	{
		case Piece::RED:
			yStart = 0;
			break;
		case Piece::BLUE:
			yStart = Game::theGame->theBoard.Height()-4;
			break;
		default:
			return MovementResult::COLOUR_ERROR; 
			break;
	}


	for (int y = 0; y < 4; ++y)
	{
		if ((int)setup[y].length() != Game::theGame->theBoard.Width())
			return MovementResult::BAD_RESPONSE;

		for (int x = 0; x < Game::theGame->theBoard.Width(); ++x)
		{
			Piece::Type type = Piece::GetType(setup[y][x]);
			if (type != Piece::NOTHING)
			{
				usedUnits[(int)(type)]++;
				if (usedUnits[type] > Piece::maxUnits[(int)type])
				{
					//fprintf(stderr, "Too many units of type %c\n", Piece::tokens[(int)(type)]);
					return MovementResult::BAD_RESPONSE;
				}
				Game::theGame->theBoard.AddPiece(x, yStart+y, type, colour);
			}
		}
	}
	if (usedUnits[(int)Piece::FLAG] <= 0)
	{
		return MovementResult::BAD_RESPONSE; //You need to include a flag!
	}
	
	//Added 9/04/12 - Check all pieces that can be placed are placed
	for (int ii = 0; ii <= (int)(Piece::BOMB); ++ii)
	{
		if (usedUnits[ii] != Piece::maxUnits[ii])
			return MovementResult::BAD_RESPONSE; //You must place ALL possible pieces
	}
	

	return MovementResult::OK;

}


/**
 * Queries the player to respond to a state of Game::theGame->theBoard
 * @param buffer String which is used to store the player's responses
 * @returns The result of the response and/or move if made
 */
MovementResult Controller::MakeMove(string & buffer)
{
	buffer.clear();
	MovementResult query = this->QueryMove(buffer);
	if (query != MovementResult::OK)
		return query;

	/* 
	//Removed 3/01/12 NO_MOVE now not allowed, SURRENDER is undocumented and not necessary
	if (buffer == "NO_MOVE")
	{
		buffer += " OK";
		return MovementResult::OK;
	}
	*/
	//Restored SURRENDER 9/04/12, because... it kind of seems necessary?	
	if (buffer == "SURRENDER")
	{
		buffer += " OK";
		return MovementResult::SURRENDER;
	}



	int x; int y; string direction="";
	stringstream s(buffer);
	s >> x;
	s >> y;
	

	s >> direction;



	Board::Direction dir;
	if (direction == "UP")
	{
		dir = Board::UP;
	}
	else if (direction == "DOWN")
	{
		dir = Board::DOWN;
	}
	else if (direction == "LEFT")
	{
		dir = Board::LEFT;
	}
	else if (direction == "RIGHT")
	{
		dir = Board::RIGHT;
	}	
	else
	{

		if (Game::theGame->allowIllegalMoves)
			return MovementResult::OK;
		else
			return MovementResult::BAD_RESPONSE; //Player gave bogus direction - it will lose by default.	
	}

	int multiplier = 1;
	if (s.peek() != EOF)
		s >> multiplier;
	MovementResult moveResult = Game::theGame->theBoard.MovePiece(x, y, dir, multiplier, colour);

	s.clear(); 	s.str("");

	//I stored the ranks in the wrong order; rank 1 is the marshal, 2 is the general etc...
	//So I am reversing them in the output... great work
	s << Piece::tokens[(int)(moveResult.attackerRank)] << " " << Piece::tokens[(int)(moveResult.defenderRank)];	
	switch (moveResult.type)
	{
		case MovementResult::OK:
			buffer += " OK";
			break;
		case MovementResult::VICTORY_FLAG:
			buffer += " VICTORY_FLAG";
			break;
		case MovementResult::VICTORY_ATTRITION:
			buffer += " VICTORY_ATTRITION";
			break;
		case MovementResult::KILLS:
			buffer += " KILLS ";
			buffer += s.str();

			break;
		case MovementResult::DIES:
			buffer += " DIES ";
			buffer += s.str();
			break;
		case MovementResult::BOTH_DIE:
			buffer += " BOTHDIE ";
			buffer += s.str();
			break;	
		default:
			buffer += " ILLEGAL";
			break;			
		
	}

	
	if (!Board::LegalResult(moveResult))
	{
		
		if (Game::theGame->allowIllegalMoves)
		{
			
			return MovementResult::OK; //HACK - Illegal results returned as legal! (Move not made)
		}
		else if (this->HumanController()) //Cut human controllers some slack and let them try again...
		{
			//Yes, checking type of object is "not the C++ way"
			//	But sometimes its bloody useful to know!!!
			Message("Bad move: \'" + buffer + "\' <- Please try again!");
			buffer = "";
			return this->MakeMove(buffer);
		}
	}

	return moveResult; 	

}

/**
 * Fixes the name of the controller
 * Should be called AFTER the constructor, since it modifies the name string, which might be used in the constructor
 */
void Controller::FixName()
{
	for (unsigned int ii=0; ii < name.size(); ++ii)
	{
		if (name[ii] == ' ')
		{
			name.erase(ii); //Just erase everything after the first whitespace
			break;
		}
	}
	//This is kind of hacky; I added it so that I could pass arguments to the AIs
	//Because simulate.py doesn't like extra arguments showing up in the AI name at the end of the game (its fine until then)
	//So I'll just remove them, after they have been passed! What could possibly go wrong?
	// - Last entry in Sam Moore's diary, 2012
}

