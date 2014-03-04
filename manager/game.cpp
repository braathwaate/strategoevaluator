#include "game.h"
#include <stdarg.h>
#include <string>

using namespace std;



Game* Game::theGame = NULL;
bool Game::gameCreated = false;

Game::Game(const char * redPath, const char * bluePath, const bool enableGraphics, double newStallTime, const bool allowIllegal, FILE * newLog, const  Piece::Colour & newReveal, int newMaxTurns, bool newPrintBoard, double newTimeoutTime, const char * newImageOutput) : red(NULL), blue(NULL), turn(Piece::RED), theBoard(10,10), graphicsEnabled(enableGraphics), stallTime(newStallTime), allowIllegalMoves(allowIllegal), log(newLog), reveal(newReveal), turnCount(0), input(NULL), maxTurns(newMaxTurns), printBoard(newPrintBoard), timeoutTime(newTimeoutTime), imageOutput(newImageOutput)
{
	gameCreated = false;
	if (gameCreated)
	{
		fprintf(stderr, "Game::Game - Error - Tried to create more than one Game!\n");
		exit(EXIT_FAILURE);
	}
	gameCreated = true;
	Game::theGame = this;
	signal(SIGPIPE, Game::HandleBrokenPipe);


	#ifdef BUILD_GRAPHICS
	if (graphicsEnabled && (!Graphics::Initialised()))
	{
			string s = "Stratego: ";
			s += string(redPath);
			s += " ";
			s += string(bluePath);
			Graphics::Initialise(s.c_str(), theBoard.Width()*GRID_SIZE, theBoard.Height()*GRID_SIZE);
	}
	#endif //BUILD_GRAPHICS



	MakeControllers(redPath, bluePath);

	if (red == NULL || blue == NULL)
	{
		fprintf(stderr, "Game::Game - Error creating controller: ");
		if (red == NULL)
		{
			if (blue == NULL)
				fprintf(stderr, " BOTH! (red: \"%s\", blue: \"%s\"\n", redPath, bluePath);
			else
				fprintf(stderr, " RED! (red: \"%s\")\n", redPath);
		}
		else
			fprintf(stderr, "BLUE! (blue: \"%s\")\n", bluePath);
		exit(EXIT_FAILURE);
	}
//	logMessage("Game initialised.\n");
}

Game::Game(const char * fromFile, const bool enableGraphics, double newStallTime, const bool allowIllegal, FILE * newLog, const  Piece::Colour & newReveal, int newMaxTurns, bool newPrintBoard, double newTimeoutTime,const char * newImageOutput) : red(NULL), blue(NULL), turn(Piece::RED), theBoard(10,10), graphicsEnabled(enableGraphics), stallTime(newStallTime), allowIllegalMoves(allowIllegal), log(newLog), reveal(newReveal), turnCount(0), input(NULL), maxTurns(newMaxTurns), printBoard(newPrintBoard), timeoutTime(newTimeoutTime), imageOutput(newImageOutput)
{
	gameCreated = false;
	if (gameCreated)
	{
		fprintf(stderr, "Game::Game - Error - Tried to create more than one Game!\n");
		exit(EXIT_FAILURE);
	}
	gameCreated = true;
	Game::theGame = this;
	signal(SIGPIPE, Game::HandleBrokenPipe);

	#ifdef BUILD_GRAPHICS
	if (graphicsEnabled && (!Graphics::Initialised()))
	{
			string s = "Stratego: (file) ";
			s += string(fromFile);
			Graphics::Initialise(s.c_str(), theBoard.Width()*GRID_SIZE, theBoard.Height()*GRID_SIZE);
	}
	#endif //BUILD_GRAPHICS

	input = fopen(fromFile, "r");

	red = new FileController(Piece::RED, input);
	blue = new FileController(Piece::BLUE, input);


}

Game::~Game()
{
	
	delete red;
	delete blue;

	if (log != NULL && log != stdout && log != stderr)
		fclose(log);

	if (input != NULL && input != stdin)
		fclose(input);
}

/**
 * Attempts to setup the board and controllers
 * @param redName the name of the red AI
 * @param blueName the name of the blue AI
 * @returns A colour, indicating if there were any errors
	Piece::NONE indicates no errors
	Piece::BOTH indicates errors with both AI
	Piece::RED / Piece::BLUE indicates an error with only one of the two AI
 */
Piece::Colour Game::Setup(const char * redName, const char * blueName)
{

	if (!red->Valid())
	{
		logMessage("Controller for Player RED is invalid!\n");
		if (!red->HumanController())
			logMessage("Check that executable \"%s\" exists and has executable permissions set.\n", redName);
	}
	if (!blue->Valid())
	{
		logMessage("Controller for Player BLUE is invalid!\n");
		if (!blue->HumanController())
			logMessage("Check that executable \"%s\" exists and has executable permissions set.\n", blueName);
	}
	if (!red->Valid())
	{
		if (!blue->Valid())
			return Piece::BOTH;
		return Piece::RED;
	}
	else if (!blue->Valid())
	{
		return Piece::BLUE;
	}

	for (int y = 4; y < 6; ++y)
	{
		for (int x = 2; x < 4; ++x)
		{
			theBoard.AddPiece(x,y,Piece::BOULDER, Piece::NONE);
		}
		for (int x = 6; x < 8; ++x)
		{
			theBoard.AddPiece(x,y,Piece::BOULDER, Piece::NONE);
		}
	}


	MovementResult redSetup = red->Setup(blueName);
	MovementResult blueSetup = blue->Setup(redName);


	Piece::Colour result = Piece::NONE;
	if (redSetup != MovementResult::OK)
	{	
		if (blueSetup != MovementResult::OK)
		{
			logMessage("BOTH players give invalid setup!\n");
			result = Piece::BOTH;
		}
		else
		{
			//logMessage("Player RED gave an invalid setup!\n");
			result = Piece::RED;
		}
		
	}
	else if (blueSetup != MovementResult::OK)
	{
		//logMessage("Player BLUE gave an invalid setup!\n");
		result = Piece::BLUE;
	}


	logMessage("%s RED SETUP\n", red->name.c_str());
	if (redSetup == MovementResult::OK)
	{
		for (int y=0; y < 4; ++y)
		{
			for (int x=0; x < theBoard.Width(); ++x)
			{
				if (theBoard.GetPiece(x, y) != NULL)
					logMessage("%c", Piece::tokens[(int)(theBoard.GetPiece(x, y)->type)]);
				else
					logMessage(".");
			}
			logMessage("\n");
		}	
	}
	else
	{
		logMessage("INVALID!\n");
	}

	logMessage("%s BLUE SETUP\n", blue->name.c_str());
	if (blueSetup == MovementResult::OK)
	{
		for (int y=0; y < 4; ++y)
		{
			for (int x=0; x < theBoard.Width(); ++x)
			{
				if (theBoard.GetPiece(x, theBoard.Height()-4+y) != NULL)
					logMessage("%c", Piece::tokens[(int)(theBoard.GetPiece(x, theBoard.Height()-4+y)->type)]);
				else
					logMessage(".");
			}
			logMessage("\n");
		}	
	}
	else
	{
		logMessage("INVALID!\n");
	}

	
	return result;

}

void Game::Wait(double wait)
{
	if (wait <= 0)
		return;




	#ifdef BUILD_GRAPHICS


	if (!graphicsEnabled)
	{
		usleep(1000000*wait); //Wait in seconds
		return;
	}

	TimerThread timer(wait*1000000); //Wait in seconds
	timer.Start();
	while (!timer.Finished())
	{
	
		SDL_Event  event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					timer.Stop();
					exit(EXIT_SUCCESS);
					break;
			}
		}
	}
	timer.Stop();

	#else
	usleep(wait*1000000); //Wait in seconds
	#endif //BUILD_GRAPHICS
	
}

void Game::HandleBrokenPipe(int sig)
{
	if (theGame == NULL)
	{
		fprintf(stderr, "ERROR - Recieved SIGPIPE during game exit!\n");
		exit(EXIT_FAILURE);
	}
	
	theGame->logMessage("SIGPIPE - Broken pipe (AI program no longer running)\n");

	MovementResult result = MovementResult::BAD_RESPONSE; 

	if (theGame->turn == Piece::RED)
	{
		if (theGame->red->Valid())
		{
			theGame->logMessage("	Strange; RED still appears valid.\n");
			if (theGame->blue->Valid())
			{
				theGame->logMessage("	BLUE also appears valid. Exiting with ERROR.\n");
				result = MovementResult::ERROR;
			}
			else
			{
				theGame->logMessage("BLUE is invalid. Wait for BLUE's turn to exit.\n");
				return;
			}
		}
	}
	if (theGame->turn == Piece::BLUE)
	{
		if (theGame->blue->Valid())
		{
			theGame->logMessage("	Strange; BLUE still appears valid.\n");
			if (theGame->red->Valid())
			{
				theGame->logMessage("	RED also appears valid. Exiting with ERROR.\n");
				result = MovementResult::ERROR;
			}
			else
			{
				theGame->logMessage("RED is invalid. Wait for RED's turn to exit.\n");
				return;
			}
		}
	}


	Game::theGame->PrintEndMessage(result);

	string buffer = "";
	PrintResults(result, buffer);

	//Message the AI's the quit message
	Game::theGame->red->Message("QUIT " + buffer);
	Game::theGame->blue->Message("QUIT " + buffer);

	//Log the message
	if (Game::theGame->GetLogFile() != stdout)
		Game::theGame->logMessage("%s\n", buffer.c_str());

	fprintf(stdout, "%s\n", buffer.c_str());
	exit(EXIT_SUCCESS);
}

void Game::PrintEndMessage(const MovementResult & result)
{
	if (turnCount == 0)
	{
		logMessage("Game ends in the SETUP phase - REASON: ");
	}
	else
	{
		if (turn == Piece::RED)
		{
			logMessage("Game ends on RED's turn - REASON: ");	
		}
		else if (turn == Piece::BLUE)
		{
			logMessage("Game ends on BLUE's turn - REASON: ");
		}
		else
		{
			logMessage("Game ends on ERROR's turn - REASON: ");
			
		}
	}
	switch (result.type)
	{
		case MovementResult::OK:
			logMessage("Status returned OK, unsure why game halted...\n");
			break;
		case MovementResult::DIES:
			logMessage("Status returned DIES, unsure why game halted...\n");
			break;
		case MovementResult::KILLS:
			logMessage("Status returned KILLS, unsure why game halted...\n");
			break;
		case MovementResult::BOTH_DIE:
			logMessage("Status returned BOTH_DIE, unsure why game halted...\n");
			break;
		case MovementResult::NO_BOARD:
			logMessage("Board does not exit?!\n");
			break;
		case MovementResult::INVALID_POSITION:
			logMessage("Coords outside board\n");
			break;
		case MovementResult::NO_SELECTION:
			logMessage("Move does not select a piece\n");
			break;
		case MovementResult::NOT_YOUR_UNIT:
			logMessage("Selected piece belongs to other player\n");
			break;
		case MovementResult::IMMOBILE_UNIT:
			logMessage("Selected piece is not mobile (FLAG or BOMB)\n");
			break;
		case MovementResult::INVALID_DIRECTION:
			logMessage("Selected unit cannot move that way\n");
			break;
		case MovementResult::POSITION_FULL:
			logMessage("Attempted move into square occupied by neutral or allied piece\n");
			break;
		case MovementResult::VICTORY_FLAG:
			logMessage("Captured the flag\n");
			break;
		case MovementResult::VICTORY_ATTRITION:
			logMessage("Destroyed all mobile enemy pieces\n");
			break;
		case MovementResult::BAD_RESPONSE:
			logMessage("Unintelligable response\n");
			break;
		case MovementResult::NO_MOVE:
			logMessage("Response timeout after %2f seconds.\n", timeoutTime);
			break;
		case MovementResult::COLOUR_ERROR:
			logMessage("Internal controller error - COLOUR_ERROR\n");
			break;
		case MovementResult::ERROR:
			logMessage("Internal controller error - Unspecified ERROR\n");
			break;
		case MovementResult::DRAW_DEFAULT:
			logMessage("Game declared a draw after %d turns\n", turnCount);
			break;
		case MovementResult::DRAW:
			logMessage("Game declared a draw because neither player has mobile pieces\n");
			break;
		case MovementResult::SURRENDER:
			logMessage("This player has surrendered!\n");
			break;
		case MovementResult::BAD_SETUP:
			switch (turn)
			{
				case Piece::RED:
					logMessage("An illegal setup was made by RED\n");
					break;
				case Piece::BLUE:
					logMessage("An illegal setup was made by BLUE\n");
					break;
				case Piece::BOTH:
					logMessage("An illegal setup was made by BOTH players\n");
					break;
				case Piece::NONE:
					logMessage("Unknown internal error.\n");
					break;
			}
			break;


	}

	if (printBoard)
	{
		system("clear");
		fprintf(stdout, "%d Final State\n", turnCount);
		theBoard.PrintPretty(stdout, Piece::BOTH);
		fprintf(stdout, "\n");
	}

	#ifdef BUILD_GRAPHICS
	if (graphicsEnabled && log == stdout)
	{
		logMessage("CLOSE WINDOW TO EXIT\n");
		theBoard.Draw(Piece::BOTH);
		while (true)
		{
			SDL_Event  event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
						exit(EXIT_SUCCESS);
						break;
				}
			}			
		}
	}
	else
	#endif //BUILD_GRAPHICS
	{
		if (log == stdout)
		{
			logMessage("PRESS ENTER TO EXIT\n");
			while (fgetc(stdin) != '\n');
			exit(EXIT_SUCCESS); //Might want to actually exit, you foolish fool
		}
	}

}
/** Checks for victory by attrition (destroying all mobile pieces)
 *
 *  @returns OK for no victory, 
 *	DRAW if both players have no pieces, or 
 *	VICTORY_ATTRITION  if the current player has won by attrition
 */
MovementResult Game::CheckVictoryAttrition()
{
        if (theBoard.MobilePieces(Piece::OppositeColour(turn)) == 0)
	{
		if (theBoard.MobilePieces(turn) == 0)
	                return MovementResult::DRAW;
	        else
		        return MovementResult::VICTORY_ATTRITION;
	}
	return MovementResult::OK;

}
MovementResult Game::Play()
{

	MovementResult result = MovementResult::OK;
	turnCount = 1;
	string buffer;

	Piece::Colour toReveal = reveal;
	
	
	

//	logMessage("Messaging red with \"START\"\n");
	red->Message("START");
	
	int moveCount = 0;

	while (!Board::HaltResult(result) && (turnCount < maxTurns || maxTurns < 0))
	{
		if (red->HumanController() && blue->HumanController())
			toReveal = Piece::RED;
		if (printBoard)
		{
			system("clear");
			if (turnCount == 0)
				fprintf(stdout, "START:\n");
			else
				fprintf(stdout, "%d BLUE:\n", turnCount);
			theBoard.PrintPretty(stdout, toReveal);
			fprintf(stdout, "\n\n");
		}

		#ifdef BUILD_GRAPHICS
		if (graphicsEnabled)
		{
			theBoard.Draw(toReveal);
			if (imageOutput != "")
			{
				string imageFile = "" + imageOutput + "/"+ itostr(moveCount) + ".bmp";
				Graphics::ScreenShot(imageFile.c_str());
			}

		}
		#endif //BUILD_GRAPHICS
		
		turn = Piece::RED;
		blue->Pause();
		red->Continue();
		if (!Board::HaltResult(result))
		{
			result = CheckVictoryAttrition();
		}
		if (Board::HaltResult(result))
			break;

		logMessage( "%d RED: ", turnCount);
		result = red->MakeMove(buffer);
		logMessage( "%s\n", buffer.c_str());

		if (!Board::HaltResult(result))
		{
			result = CheckVictoryAttrition();
		}
		if (Board::HaltResult(result))
			break;

		red->Message(buffer);
		blue->Message(buffer);




		if (stallTime >= 0)
			Wait(stallTime);
		else
			ReadUserCommand();

		if (blue->HumanController() && red->HumanController())
			toReveal = Piece::BLUE;
		if (printBoard)
		{
			system("clear");
			fprintf(stdout, "%d RED:\n", turnCount);
			theBoard.PrintPretty(stdout, toReveal);
			fprintf(stdout, "\n\n");
		}

		++moveCount;
		
		#ifdef BUILD_GRAPHICS
		if (graphicsEnabled)
		{
			theBoard.Draw(toReveal);
			if (imageOutput != "")
			{
				string imageFile = "" + imageOutput + "/" + itostr(moveCount) + ".bmp";
				Graphics::ScreenShot(imageFile.c_str());
			}
		}
		#endif //BUILD_GRAPHICS

		
		
		turn = Piece::BLUE;
		red->Pause();
		blue->Continue();
		if (!Board::HaltResult(result))
		{
			result = CheckVictoryAttrition();
		}
		if (Board::HaltResult(result))
			break;

		logMessage( "%d BLU: ", turnCount);
		result = blue->MakeMove(buffer);
		logMessage( "%s\n", buffer.c_str());

		if (!Board::HaltResult(result))
		{
			result = CheckVictoryAttrition();
		}
		if (Board::HaltResult(result))
			break;

		red->Message(buffer);
		blue->Message(buffer);


		if (theBoard.MobilePieces(Piece::RED) == 0)
			result = MovementResult::DRAW;

		if (theBoard.MobilePieces(Piece::RED) == 0)
		{
			if (theBoard.MobilePieces(Piece::BLUE) == 0)
				result = MovementResult::DRAW;
			else
				result = MovementResult::VICTORY_ATTRITION;
			break;			
		}

		if (stallTime >= 0)
			Wait(stallTime);
		else
			ReadUserCommand();
	
		++moveCount;

		++turnCount;
	}

	if ((maxTurns >= 0 && turnCount >= maxTurns) && result == MovementResult::OK)
	{
		result = MovementResult::DRAW_DEFAULT;
	}

	
	return result;

		

}

/**
 * Logs a message to the game's log file if it exists
 * @param format the format string
 * @param additional parameters - printed using va_args
 * @returns the result of vfprintf or a negative number if the log file does not exist
 */
int Game::logMessage(const char * format, ...)
{
	if (log == NULL)
		return -666;
		va_list ap;
	va_start(ap, format);

	int result = vfprintf(log, format, ap);
	va_end(ap);

	return result;
}

/**
 * Waits for a user command
 * Currently ignores the command.
 */
void Game::ReadUserCommand()
{
	fprintf(stdout, "Waiting for user to press enter... (type QUIT to exit)\n");
	string command("");
	for (char c = fgetc(stdin); c != '\n' && (int)(c) != EOF; c = fgetc(stdin))
	{
		command += c;
	}

	if (command == "QUIT")
	{
		fprintf(stdout, "Ordered to quit... exiting...\n");
		exit(EXIT_SUCCESS);
	}
}

MovementResult FileController::QuerySetup(const char * opponentName, std::string setup[])
{

	char c = fgetc(file);
	name = "";
	while (c != ' ')
	{
		name += c;
		c = fgetc(file);
	}

	while (fgetc(file) != '\n');

	for (int y = 0; y < 4; ++y)
	{
		setup[y] = "";
		for (int x = 0; x < Game::theGame->theBoard.Width(); ++x)
		{
			setup[y] += fgetc(file);
		}

		if (fgetc(file) != '\n')
		{
			return MovementResult::BAD_RESPONSE;
		}
	}
	return MovementResult::OK;

	
}

MovementResult FileController::QueryMove(std::string & buffer)
{
	//This bit is kind of hacky and terrible, and yes I am mixing C with C++
	//Yes I should have used fstream for the whole thing and it would be much easier.
	//Oh well.

	char buf[BUFSIZ];

	fgets(buf, sizeof(buf), file);
	char * s = (char*)(buf);
	while (*s != ':' && *s != '\0')
		++s;
	
	//Move forward to the start of the move information
	for (int i=0; i < 2; ++i)
	{
		if (*s != '\0' && *s != '\n')
			++s;
	}
	
	//Unfortunately we can't just copy the whole line
	buffer = string(s);
	//We have to remove the movement result tokens
	

	vector<string> tokens;
	Game::Tokenise(tokens, buffer, ' ');
	buffer.clear();

	if (tokens.size() < 1)
		return MovementResult::BAD_RESPONSE;
	buffer += tokens[0];

	
	if (tokens[0] == "NO_MOVE") //tokens[0] is either the x coordinate, or "NO_MOVE"
		return MovementResult::OK;
	if (tokens.size() < 2)
		return MovementResult::BAD_RESPONSE;
	buffer += " ";
	buffer += tokens[1]; //The y coordinate
	buffer += " ";
	buffer += tokens[2]; //The direction
	
	//Check for a possible multiplier. If tokens[3] is an integer it will be the multiplier, otherwise it won't be.
	if (tokens.size() > 3 && atoi(tokens[3].c_str()) != 0)
	{
		buffer += " ";
		buffer += tokens[3];
	}
	else
	{
		//(tokens[3] should include a new line)
		//buffer += "\n";
	}

	

	
	
	
	return MovementResult::OK;
}

/**
 * Tokenise a string
 */
int Game::Tokenise(std::vector<string> & buffer, std::string & str, char split)
{
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
 * Creates Controller baseds off strings. Takes into account controllers other than AI_Controller.
 * @param redPath - Either the path to an AI_Controller compatable executable, or one of %human or %network or %network:[IP_ADDRESS]
 * @param bluePath - Ditto
 * Sets this->red to a controller using redPath, and this->blue to a controller using bluePath
 * TODO: Make nicer (this function should be ~half the length)
 */
void Game::MakeControllers(const char * redPath, const char * bluePath)
{
	Network * redNetwork = NULL;
	Network * blueNetwork = NULL;
	//To allow for running two network controllers (I don't know why you would, but beside the point...) use two ports
	static const int port1 = 4560;
	static const int port2 = 4561;

	if (redPath[0] == '@')
	{
		if (strcmp(redPath, "@human") == 0)
			red = new Human_Controller(Piece::RED, graphicsEnabled);
		else
		{
			const char * network = strstr(redPath, "@network");
			if (network == NULL)
			{
				red = NULL;
				return;
			}
			network = strstr(network, ":");
		
			if (network == NULL)
			{
				logMessage("Creating server for red AI... ");
				redNetwork = new Server(port1);
				logMessage("Successful!\n");

			}
			else
			{
				network = (const char*)(network+1);
				logMessage("Creating client for red AI... ");
				redNetwork = new Client(network, port2);
				logMessage("Connected to address %s\n", network);
			}

			logMessage("	(Red's responses will be received over the connection)\n");
			red = new NetworkReceiver(Piece::RED, redNetwork);
		}		
	}
	else
		red = new AI_Controller(Piece::RED, redPath, timeoutTime);

	if (bluePath[0] == '@')
	{
		if (strcmp(bluePath, "@human") == 0)
			blue = new Human_Controller(Piece::BLUE, graphicsEnabled);
		else
		{
			const char * network = strstr(bluePath, "@network");
			if (network == NULL)
			{
				blue = NULL;
				return;
			}
			network = strstr(network, ":");
		
			if (network == NULL)
			{
				logMessage("Creating server for blue AI... ");
				blueNetwork = new Server(port2);
				logMessage("Successful!\n");

			}
			else
			{
				network = (const char*)(network+1);
				logMessage("Creating client for blue AI... ");
				blueNetwork = new Client(network, port1);
				logMessage("Connected to address %s\n", network);
			}
			logMessage("	(Blue's responses will be received over the connection)\n");
			blue = new NetworkReceiver(Piece::BLUE, blueNetwork);
		}		
	}
	else
		blue = new AI_Controller(Piece::BLUE, bluePath, timeoutTime);

	if (redNetwork != NULL)
	{
		
		blue = new NetworkSender(Piece::BLUE,blue, redNetwork);
		logMessage("	(Blue's responses will be copied over the connection)\n");
	}
	if (blueNetwork != NULL)
	{
		
		red = new NetworkSender(Piece::RED, red, blueNetwork);
		logMessage("	(Red's responses will be copied over the connection)\n");
	}

	red->FixName(); blue->FixName();
	
}

string itostr(int i)
{
	stringstream s;
	s << i;
	return s.str();
}


void Game::PrintResults(const MovementResult & result, string & buffer)
{
	stringstream s("");
	switch (Game::theGame->Turn())
	{
		case Piece::RED:
			s << Game::theGame->red->name << " RED ";
			break;
		case Piece::BLUE:
			s << Game::theGame->blue->name << " BLUE ";
			break;
		case Piece::BOTH:
			s << "neither BOTH ";
			break;
		case Piece::NONE:
			s << "neither NONE ";
			break;
	}

	if (!Board::LegalResult(result) && result != MovementResult::BAD_SETUP)
		s << "ILLEGAL ";
	else if (!Board::HaltResult(result))
		s << "INTERNAL_ERROR ";
	else
	{
		switch (result.type)
		{
			case MovementResult::VICTORY_FLAG:
			case MovementResult::VICTORY_ATTRITION: //It does not matter how you win, it just matters that you won!
				s <<  "VICTORY ";
				break;
			case MovementResult::SURRENDER:
				s << "SURRENDER ";
				break;
			case MovementResult::DRAW:
				s << "DRAW ";
				break;
			case MovementResult::DRAW_DEFAULT:
				s << "DRAW_DEFAULT ";
				break;
			case MovementResult::BAD_SETUP:
				s << "BAD_SETUP ";
				break;	
			default:
				s << "INTERNAL_ERROR ";
				break;	
		}
	}
	
	s << Game::theGame->TurnCount() << " " << Game::theGame->theBoard.TotalPieceValue(Piece::RED) << " " << Game::theGame->theBoard.TotalPieceValue(Piece::BLUE);

	buffer = s.str();
	

}

