#include "human_controller.h"

#include "game.h"

#include <iostream> //Really I can't be bothered with fscanf any more

using namespace std;

MovementResult Human_Controller::QuerySetup(const char * opponentName, string setup[])
{
	
	static bool shownMessage = false;
	if (!shownMessage)
	{
		if (graphicsEnabled)
			fprintf(stdout, "WARNING: GUI not fully supported. You will be forced to use the default setup.\n");
		else
		{
			fprintf(stdout,"Enter %d x %d Setup grid\n", Game::theGame->theBoard.Width(), 4);
			fprintf(stdout,"Please enter one line at a time.\n");
			fprintf(stdout, "You must place at least the Flag (%c). Use '%c' for empty squares.\n", Piece::tokens[(int)Piece::FLAG], Piece::tokens[(int)Piece::NOTHING]);
		
			switch (colour)
			{
				case Piece::RED:
					fprintf(stdout, "You are RED and occupy the top 4 rows of the board.\n");
					fprintf(stdout, "NOTE: Enter \"DEFAULT\" to use the setup:\n");
					fprintf(stdout, "FB8sB479B8\nBB31555583\n6724898974\n967B669999\n");
					break;
				case Piece::BLUE:
					fprintf(stdout, "You are BLUE and occupy the bottom 4 rows of the board.\n");
					fprintf(stdout, "NOTE: Enter \"DEFAULT\" to use the setup:\n");
					fprintf(stdout, "967B669999\n6724898974\nBB31555583\nFB8sB479B8\n");
					break;
				default:
					fprintf(stdout, "WARNING: Unknown colour error! Please exit the game.\n");
					break;
			}	
		}	
		
		shownMessage = true;
	}

	if (graphicsEnabled)
	{
		switch(colour)
		{
			case Piece::RED:
				setup[0] = "FB8sB479B8"; 
				setup[1] = "BB31555583";
				setup[2] = "6724898974";
				setup[3] = "967B669999";
				break;
			case Piece::BLUE:
				setup[0] = "967B669999";
				setup[1] = "6724898974";
				setup[2] = "BB31555583";
				setup[3] = "FB8sB479B8";
				break;
			default:
				assert(false);
				break;
			}
		return MovementResult::OK;
	}

	for (int y = 0; y < 4; ++y)
	{
		cin >> setup[y];
		if (y == 0 && setup[0] == "DEFAULT")
		{
			switch(colour)
			{
				case Piece::RED:
					setup[0] = "FB8sB479B8"; 
					setup[1] = "BB31555583";
					setup[2] = "6724898974";
					setup[3] = "967B669999";
					break;
				case Piece::BLUE:
					setup[0] = "967B669999";
					setup[1] = "6724898974";
					setup[2] = "BB31555583";
					setup[3] = "FB8sB479B8";
					break;
				default:
					assert(false);
					break;
			}
			break;
		}
	}
	assert(cin.get() == '\n');
	
	return MovementResult::OK;
}

MovementResult Human_Controller::QueryMove(string & buffer)
{
	static bool shownMessage = false;
	if (!shownMessage)
	{
		if (!graphicsEnabled)
		{
			fprintf(stdout, "Please enter your move in the format:\n X Y DIRECTION [MULTIPLIER=1]\n");
			fprintf(stdout, "Where X and Y indicate the coordinates of the piece to move;\n DIRECTION is one of UP, DOWN, LEFT or RIGHT\n and MULTIPLIER is optional (and only valid for scouts (%c))\n", Piece::tokens[(int)(Piece::SCOUT)]);
			
		}
		shownMessage = true;
	}

	
	#ifdef BUILD_GRAPHICS
	if (graphicsEnabled)
	{
		
		fprintf(stdout, "Click to move!\n");
		SDL_Event event; int mouseClick = 0;

		int x[] = {-1, -1}; int y[] = {-1, -1};
		while (mouseClick < 2)
		{
			
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
						Game::theGame->logMessage("Exit called by human player!\n");
						exit(EXIT_SUCCESS);
						break;
					case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button)
					{
						case SDL_BUTTON_LEFT:
							SDL_GetMouseState(&x[mouseClick], &y[mouseClick]);
							x[mouseClick] /= 64; y[mouseClick] /= 64; //Adjust based on graphics grid size
							if (mouseClick == 0)
							{
								stringstream s("");
								s << x[0] << " " << y[0] << " ";
								buffer += s.str();
							}
							else if (mouseClick == 1)
							{
								int xDist = x[1] - x[0];
								int yDist = y[1] - y[0];
								int magnitude = max(abs(xDist), abs(yDist));
								if (abs(xDist) > abs(yDist))
								{
									if (xDist < 0)
										buffer += "LEFT";
									else
										buffer += "RIGHT";
								}
								else if (yDist < 0)
									buffer += "UP";
								else
									buffer += "DOWN";

								if (magnitude > 1)
								{
									stringstream s("");
									s << " " << magnitude;
									buffer += s.str();
								}
							}
							mouseClick++;
							break;
					}
					break;
				}
			}
		}
		fprintf(stdout, "Move complete!\n");
		
	}
	else
	#endif //BUILD_GRAPHICS
	{
		buffer.clear();
		for (char in = fgetc(stdin); in != '\n'; in = fgetc(stdin))
		{
			buffer += in;
		}
	}
	
	

	return MovementResult::OK;
	
}
