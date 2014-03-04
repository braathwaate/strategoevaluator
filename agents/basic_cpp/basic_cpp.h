/**
 * "basic_cpp", a sample Stratego AI for the UCC Programming Competition 2012
 * Declarations for classes Piece, Board and Basic_Cpp
 * @author Sam Moore (matches) [SZM]
 * @website http://matches.ucc.asn.au/stratego
 * @email progcomp@ucc.asn.au or matches@ucc.asn.au
 * @git git.ucc.asn.au/progcomp2012.git
 */

#ifndef BASIC_CPP_H
#define BASIC_CPP_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>





/**
 * enum for possible ranks of pieces
 */
typedef enum {UNKNOWN=14,BOMB=13,MARSHAL=12, GENERAL=11, COLONEL=10, MAJOR=9, CAPTAIN=8, LIEUTENANT=7, SERGEANT=6, MINER=5, SCOUT=4, SPY=3, FLAG=2,BOULDER=1, NOTHING=0} Rank;

/**
 * enum for possible colours of pieces and the AI
 */		
typedef enum {RED=0, BLUE=1, NONE, BOTH} Colour;


/**
 * Class to represent a piece on the board
 * TODO: Add features required for Pieces as used by your AI
 * 	For example, can replace the single member "rank" with two, "maxRank" and "minRank" 
 *	Or add an array of probability values for EVERY rank!
 */
class Piece
{
	public:
		static  char tokens[]; //The tokens used to identify various pieces

		Piece(int newX, int newY,const Colour & newColour, const Rank & newRank = UNKNOWN)
			: x(newX), y(newY), colour(newColour), rank(newRank) {}
		virtual ~Piece() {}

		bool Mobile() const {return rank != BOMB && rank != FLAG;}
		
		static Colour Opposite(const Colour & colour) {return colour == RED ? BLUE : RED;}

		int x; int y;
		const Colour colour; //The colour of the piece
		Rank rank; //The rank of the piece

		static Rank GetRank(char token); //Helper to get rank from character

};

/**
 * enum for Directions that a piece can move in
 */
typedef enum {UP=0, DOWN=1, LEFT=2, RIGHT=3, DIRECTION_ERROR=4} Direction;

/**
 * Class to represent a board
 */
class Board
{
	public:
		Board(int width, int height); //Construct a board with width and height
		virtual ~Board(); //Destroy the board

		Piece * Get(int x, int y) const; //Retrieve single piece
		Piece *  Set(int x, int y, Piece * newPiece); //Add piece to board

		int Width() const {return width;} //getter for width
		int Height() const {return height;} //getter for height

		bool ValidPosition(int x, int y) const {return (x >= 0 && x < width && y >= 0 && y < height);} //Helper - is position within board?
	private:

		int width; //The width of the board
		int height; //The height of the board
		Piece ** * board; //The pieces on the board
};

/**
 * Basic AI class
 * TODO: Make sure that if Piece is changed, BasicAI is updated to be consistent
 * TODO: More complex AI's should inherit off this class.
 *	It is recommended that only MakeMove is altered - hence the other functions are not virtual.
 */
class BasicAI
{
	public:
		BasicAI(); //Constructor
		virtual ~BasicAI(); //Destructor

		bool Setup(); //Implements setup protocol
		bool MoveCycle(); //Implements the MoveCycle protocol
		bool ReadBoard(); //Reads the board as part of the MoveCycle protocol
		bool InterpretResult(); //Interprets the result of a move
		virtual bool MakeMove(); //Makes a move - defaults to randomised moves
		bool DebugPrintBoard(); //DEBUG - Prints the board to stderr
	protected:
		int turn;
		Board * board; //The board
		std::vector<Piece*> units; //Allied units
		std::vector<Piece*> enemyUnits; //Enemy units

		bool ForgetUnit(Piece * forget); //Delete and forget about a unit

		Colour colour;
		std::string colourStr;
};

/**
 * Purely static class of Helper functions
 */
class Helper
{
	public:
		static Direction StrToDir(const std::string & str); //Helper - Convert string to a Direction enum
		static void DirToStr(const Direction & dir, std::string & buffer); //Helper - Convert Direction enum to a string
		static void MoveInDirection(int & x, int & y, const Direction & dir, int multiplier = 1); //Helper - Move a point in a direction
		static int Tokenise(std::vector<std::string> & buffer, std::string & str, char split = ' '); //Helper - Split a string into tokens
		static int Integer(std::string & fromStr); //Helper - convert a string to an integer
		static void ReadLine(std::string & buffer); //Helper - read a line from stdin
	private:
		//By making these private we ensure that no one can create an instance of Helper
		//Yes we could use namespaces instead, but I prefer this method because you can use private static member variables
		//Not that I am. But you can.
		Helper() {}
		~Helper() {}
};



#endif //BASIC_CPP_H

//EOF

