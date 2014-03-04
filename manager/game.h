#ifndef MAIN_H
#define MAIN_H

#include "stratego.h"
#include "ai_controller.h"
#include "human_controller.h"
#include "network_controller.h"
#include <cstring>


/**
 * Class to manage the game
 * Bit messy since I keep adding on parameters :P
 */
class Game
{
	public:
		Game(const char * redPath, const char * bluePath, const bool enableGraphics, double newStallTime = 1.0, const bool allowIllegal=false, FILE * newLog = NULL, const Piece::Colour & newRevealed = Piece::BOTH, int maxTurns = 5000, const bool printBoard = false, double newTimeoutTime = 2.0, const char * newImageOutput = "");
		Game(const char * fromFile, const bool enableGraphics, double newStallTime = 1.0, const bool allowIllegal=false, FILE * newLog = NULL, const Piece::Colour & newRevealed = Piece::BOTH, int maxTurns = 5000, const bool printBoard = false, double newTimeoutTime = 2.0, const char * newImageOutput = "");
		virtual ~Game();

		

		void Wait(double wait); 

		Piece::Colour Setup(const char * redName, const char * blueName);
		MovementResult CheckVictoryAttrition();
		MovementResult Play();
		void PrintEndMessage(const MovementResult & result);
		

		static void HandleBrokenPipe(int signal);
		void ReadUserCommand();
		
		const Piece::Colour Turn() const {return turn;}
		void ForceTurn(const Piece::Colour & newTurn) {turn = newTurn;}
		int TurnCount() const {return turnCount;}

		static Game * theGame;
		static int Tokenise(std::vector<std::string> & buffer, std::string & str, char split = ' '); //Helper - Split a string into tokens

	private:
		void MakeControllers(const char * redPath, const char * bluePath); //Create a controller based off a path
	public:
		int logMessage(const char * format, ...);
		static void PrintResults(const MovementResult & result, std::string & buffer);
		FILE * GetLogFile() const {return log;}
		Controller * red;
		Controller * blue;
	private:
		Piece::Colour turn;
		
	public:
		Board theBoard;
	private:
		const bool graphicsEnabled;
		double stallTime;
	public:
		const bool allowIllegalMoves;

	private:
		FILE * log;
		
	public:
		const Piece::Colour reveal;
		int turnCount;

		static bool gameCreated;

		FILE * input;

		int maxTurns;
		const bool printBoard;

	private:
		double timeoutTime;
		std::string imageOutput;
		
};

class FileController : public Controller
{
	public:
		FileController(const Piece::Colour & newColour, FILE * newFile) : Controller(newColour, "file"), file(newFile) {}
		virtual ~FileController() {}

		virtual bool Message(const char * string) {return true;} //Don't send messages
		virtual MovementResult QuerySetup(const char * opponentName, std::string setup[]);
		virtual MovementResult QueryMove(std::string & buffer);
		virtual bool Valid() const {return file != NULL;}

	private:
		FILE * file;


};

std::string itostr(int i);


#endif //MAIN_H
