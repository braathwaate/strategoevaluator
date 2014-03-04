#include <sstream>

#include "game.h"
#include "stratego.h"

#include "ai_controller.h"

using namespace std;


/**
 * Queries the AI program to setup its pieces. Stores the setup in a st
 * @implements Controller::QuerySetup
 * @param
 * @returns A MovementResult
 */

MovementResult AI_Controller::QuerySetup(const char * opponentName, std::string setup[])
{
	switch (colour)
	{
		case Piece::RED:
			if (!SendMessage("RED %s %d %d", opponentName, Game::theGame->theBoard.Width(), Game::theGame->theBoard.Height()))
				return MovementResult::BAD_RESPONSE;
			break;
		case Piece::BLUE:
			if (!SendMessage("BLUE %s %d %d", opponentName, Game::theGame->theBoard.Width(), Game::theGame->theBoard.Height()))
				return MovementResult::BAD_RESPONSE;
			break;
		case Piece::NONE:
		case Piece::BOTH:
			return MovementResult::COLOUR_ERROR;
			break;
	}

	for (int y = 0; y < 4; ++y)
	{
		if (!GetMessage(setup[y], timeout))
			return MovementResult::BAD_RESPONSE;	
	}

	return MovementResult::OK;
}


/**
 * Queries the AI program to make a move
 * @implements Controller::QueryMove
 * @param buffer String which stores the AI program's response
 * @returns A MovementResult which will be MovementResult::OK if a move was made, or MovementResult::NO_MOVE if the AI did not respond
 */
MovementResult AI_Controller::QueryMove(string & buffer)
{
	if (!Running())
		return MovementResult::NO_MOVE; //AI has quit
	Game::theGame->theBoard.Print(output, colour);
	//Game::theGame->logMessage("DEBUG: About to get message from %d\n", colour);
	if (!GetMessage(buffer,timeout))
	{
		return MovementResult::NO_MOVE; //AI did not respond (within the timeout). It will lose by default.
	}
	//Game::theGame->logMessage("DEBUG: Got message \"%s\" from %d\n", buffer.c_str(), colour);
	return MovementResult::OK; //Got the message
}

