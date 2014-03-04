/**
 * "basic_cpp", a sample Stratego AI for the UCC Programming Competition 2012
 * Implementations of main function, and Helper functions
 *
 * @author Sam Moore (matches) [SZM]
 * @website http://matches.ucc.asn.au/stratego
 * @email progcomp@ucc.asn.au or matches@ucc.asn.au
 * @git git.ucc.asn.au/progcomp2012.git
 */

#include "basic_cpp.h" //Needs class Base_Cpp and the includes in this file

using namespace std;

/**
 * The characters used to represent various pieces
 * NOTHING, BOULDER, FLAG, SPY, SCOUT, MINER, SERGEANT, LIETENANT, CAPTAIN, MAJOR, COLONEL, GENERAL, MARSHAL, BOMB, UNKNOWN
 */
char  Piece::tokens[] = {'.','*','F','s','9','8','7','6','5','4','3','2','1','B','?'};

/**
 * Gets a rank from the character used to represent it
 * Basic lookup of Piece::tokens
 */
Rank Piece::GetRank(char token)
{
	for (int ii=0; ii <= 14; ++ii)
	{
		if (tokens[ii] == token)
			return (Rank)(ii);
	}
	return UNKNOWN;
}

/**
 * IMPLEMENTATION of Helper FOLLOWS
 */

/**
 * Convert string to direction
 */
Direction Helper::StrToDir(const string & dir)
{
	if (dir == "UP")
		return UP;
	else if (dir == "DOWN")
		return DOWN;
	else if (dir == "LEFT")
		return LEFT;
	else if (dir == "RIGHT")
		return RIGHT;
	else
		return DIRECTION_ERROR;
}

/**
 * Direction to String
 */
void Helper::DirToStr(const Direction & dir, std::string & buffer)
{
	switch (dir)
	{
		case UP:
			buffer = "UP";
			break;
		case DOWN:
			buffer = "DOWN";
			break;
		case LEFT:
			buffer = "LEFT";
			break;
		case RIGHT:
			buffer = "RIGHT";
			break;
		default:
			buffer = "DIRECTION_ERROR";
			break;
	}
}

/**
 * Move a point in a direction
 */
void Helper::MoveInDirection(int & x, int & y, const Direction & dir, int multiplier)
{
	switch (dir)
	{
		case UP:
			y -= multiplier;
			break;
		case DOWN:
			y += multiplier;
			break;
		case LEFT:
			x -= multiplier;
			break;
		case RIGHT:
			x += multiplier;
			break;
		default:
			break;
	}

}

/**
 * Tokenise a string
 */
int Helper::Tokenise(std::vector<string> & buffer, std::string & str, char split)
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
 * Convert string to integer
 */
int Helper::Integer(std::string & fromStr)
{
	stringstream s(fromStr);
	int result = 0;
	s >> result;
	return result;
}

/**
 * Read in a line from stdin
 */
void Helper::ReadLine(std::string & buffer)
{
	buffer = "";
	for (char c = cin.get(); c != '\n' && cin.good(); c = cin.get())
	{		
		buffer += c;
	}
}

/**
 * IMPLEMENTATION of Board FOLLOWS
 */

/**
 * Constructer for Board
 */
Board::Board(int w, int h) : width(w), height(h), board(NULL)
{
	//Construct 2D array of P*'s
	board = new Piece**[width];
	for (int x=0; x < width; ++x)
	{
		board[x] = new Piece*[height];
		for (int y=0; y < height; ++y)
			board[x][y] = NULL;
	}
}

/**
 * Destructor for board
 */
Board::~Board()
{
	//Destroy the 2D array of P*'s
	for (int x=0; x < width; ++x)
	{
		for (int y=0; y < height; ++y)
			delete board[x][y];
		delete [] board[x];
	}
}

/**
 * Retrieves a piece on the Board
 * @param x x coordinate
 * @param y y coordinate
 * @returns A Piece* for the piece, or NULL if there is no piece at the point given
 */
Piece * Board::Get(int x, int y) const
{
	if (ValidPosition(x, y))
		return board[x][y];
	return NULL;
}
/**
 * Sets a piece on the Board
 * @param x x coordinate
 * @param y y coordinate
 * @param newPiece
 * @param returns newPiece if successful, NULL if not
 */
Piece * Board::Set(int x, int y, Piece * newPiece)
{
	if (!ValidPosition(x, y))
		return NULL;
	board[x][y] = newPiece;
	assert(Get(x,y) == newPiece);
	return newPiece;
}

/**
 * IMPLEMENTATION of Base_Cpp FOLLOWS
 */

/**
 * Constructor for AI
 */
BasicAI::BasicAI() : turn(0), board(NULL), units(), enemyUnits(), colour(NONE), colourStr("")
{
	srand(time(NULL));
	cin.rdbuf()->pubsetbuf(NULL, 0);
	cout.rdbuf()->pubsetbuf(NULL, 0);
}

/**
 * Destructor for AI
 */
BasicAI::~BasicAI()
{
	if (board != NULL)
		delete board;
}


/**
 * Setup the AI
 * @returns true if successful, false on error
 */
bool BasicAI::Setup()
{
	
	cin >> colourStr; 


	std::string opponentName(""); //opponentName is unused, just read it
	cin >> opponentName; 
	
	int width = 0; int height = 0;
	cin >> width; cin >> height;

	while(cin.get() != '\n' && cin.good()); //trim newline
	
	board = new Board(width, height);

	if (colourStr == "RED")
	{
		colour = RED;
		cout << "FB8sB479B8\nBB31555583\n6724898974\n967B669999\n";
	}
	else if (colourStr == "BLUE")
	{
		colour = BLUE;
		cout << "967B669999\n6724898974\nBB31555583\nFB8sB479B8\n";
	}
	else 
		return false;

	return (board != NULL);
}

/**
 * Performs a move, including the saving states bits
 * @returns true if the game is to continue, false if it is to end
 */
bool BasicAI::MoveCycle()
{
	//cerr << "BasicAI at MoveCycle()\n";
	if (!InterpretResult())	
		return false;
	if (!ReadBoard())
		return false;
	if (!MakeMove())
		return false;

	turn++;
	return InterpretResult();
}

/**
 * Interprets the result of a move. Ignores the first move
 * @returns true if successful, false if there was an error
 */
bool BasicAI::InterpretResult()
{
	//cerr << "BasicAI at InterpretResult()\n";
	if (turn == 0)
	{
		while (cin.get() != '\n' && cin.good());
		return true;
	}


	string resultLine; Helper::ReadLine(resultLine);
	vector<string> tokens; Helper::Tokenise(tokens, resultLine, ' ');
	
	if (tokens.size() <= 0)
	{
		//cerr << "No tokens!\n";
		return false;
	}
	
	if (tokens[0] == "QUIT")
	{
		return false;
	}

	if (tokens[0] == "NO_MOVE")
	{
		return true;

	}

	if (tokens.size() < 4)
	{
		//cerr << "Only got " << tokens.size() << " tokens\n";
		return false;
	}
	

	int x = Helper::Integer(tokens[0]);
	int y = Helper::Integer(tokens[1]);
	
	

	Direction dir = Helper::StrToDir(tokens[2]);

	//We might want to actually check for the multiplier in the sample agents! 20/12/11
	unsigned int outIndex = 3;
	int multiplier = atoi(tokens[outIndex].c_str());
	if (multiplier == 0)
		multiplier = 1;
	else
		outIndex += 1;
	


	string & outcome = tokens[outIndex];


	int x2 = x; int y2 = y; Helper::MoveInDirection(x2,y2,dir, multiplier);

	Piece * attacker = board->Get(x,y);
	if (attacker == NULL)
	{
		//cerr << "No attacker!\n";
		return false;
	}
	Piece * defender = board->Get(x2,y2);
	if (outcome == "OK")
	{
		board->Set(x2,y2, attacker);
		board->Set(x,y,NULL);
		attacker->x = x2; attacker->y = y2;
	}
	else if (outcome == "KILLS")
	{
		if (defender == NULL)
		{
			//cerr << "No defender!\n";
			return false;
		}
		if (tokens.size() < outIndex+2)
			return false;


		board->Set(x2,y2, attacker);
		board->Set(x,y,NULL);
		attacker->x = x2; attacker->y = y2;
		attacker->rank = Piece::GetRank(tokens[outIndex+1][0]);
		ForgetUnit(defender);
	}
	else if (outcome == "DIES")
	{
		if (defender == NULL)
		{
			//cerr << "No defender!\n";
			return false;
		}
		if (tokens.size() < outIndex+3)
			return false;


		board->Set(x,y,NULL);
		defender->rank = Piece::GetRank(tokens[outIndex+2][0]);
		ForgetUnit(attacker);

		
	}
	else if (outcome == "BOTHDIE")
	{
		board->Set(x,y,NULL);
		board->Set(x2,y2, NULL);

		ForgetUnit(attacker);
		ForgetUnit(defender);
	}
	else if (outcome == "FLAG")
	{
		//cerr << "BasicAI - Flag was captured, exit!\n";
		return false;
	}
	else if (outcome == "ILLEGAL")
	{
		//cerr << "BasicAI - Illegal move, exit!\n";
		return false;
	}

	//cerr << "BasicAI finished InterpretResult()\n";
	return true;
}

/**
 * Performs a random move
 * TODO: Overwrite with custom move
 * @returns true if a move could be made (including NO_MOVE), false on error
 */
bool BasicAI::MakeMove()
{
	//cerr << "BasicAI at MakeMove()\n";
	if (units.size() <= 0)
	{
		//cerr << " No units!\n";
		return false;

	}
	
	int index = rand() % units.size();
	int startIndex = index;
	while (true)
	{
		

		Piece * piece = units[index];
		if (piece != NULL && piece->Mobile())
		{
			int dirIndex = rand() % 4;
			int startDirIndex = dirIndex;
			while (true)
			{
				int x = piece->x; int y = piece->y;
				assert(board->Get(x,y) == piece);
				Helper::MoveInDirection(x,y,(Direction)(dirIndex));
				if (board->ValidPosition(x,y))
				{
					Piece * target = board->Get(x,y);	
					if (target == NULL || (target->colour != piece->colour && target->colour != NONE))
					{
						string dirStr;
						Helper::DirToStr((Direction)(dirIndex), dirStr);
						cout << piece->x << " " << piece->y << " " << dirStr << "\n";
						return true;
					}
				}

				dirIndex = (dirIndex + 1) % 4;
				if (dirIndex == startDirIndex)
					break;
			}
		}

		index = (index+1) % (units.size());
		if (index == startIndex)
		{
			cout << "NO_MOVE\n";
			return true;
		}
	}
	return true;
}

/**
 * Reads in the board
 * On first turn, sets up Board
 * On subsquent turns, takes no action
 * @returns true on success, false on error
 */
bool BasicAI::ReadBoard()
{
	//cerr << "BasicAI at ReadBoard()\n";
	for (int y = 0; y < board->Height(); ++y)
	{
		string row;
		Helper::ReadLine(row);
		for (unsigned int x = 0; x < row.size(); ++x)
		{
			if (turn == 0)
			{
				switch (row[x])
				{
					case '.':
						break;
					case '#':
						board->Set(x,y, new Piece(x,y,Piece::Opposite(colour), UNKNOWN));
						enemyUnits.push_back(board->Get(x,y));
						break;
					case '+':
						board->Set(x,y, new Piece(x,y,NONE, BOULDER));
						break;
					default:
						board->Set(x,y,new Piece(x,y,colour, Piece::GetRank(row[x])));
						units.push_back(board->Get(x,y));
						break;
				}
			}
		}
	}
	return true;
}

/**
 * Removes a piece from memory
 * @param piece The piece to delete
 * @returns true if the piece was actually found
 */
bool BasicAI::ForgetUnit(Piece * piece)
{	
	//cerr << "BasicAI at ForgetUnit()\n";
	bool result = false;
	vector<Piece*>::iterator i = units.begin(); 
	while (i != units.end())
	{
		if ((*i) == piece)
		{
			i = units.erase(i); result = true;
			continue;
		}
		++i;
	}

	i = enemyUnits.begin();
	while (i != enemyUnits.end())
	{
		if ((*i) == piece)
		{
			i = enemyUnits.erase(i); result = true;
			continue;
		}
		++i;
	}


	delete piece;
	return result;
}


/**
 * The main function
 * @param argc
 * @param argv
 * @returns zero on success, non-zero on failure
 */
int main(int argc, char ** argv)
{

	srand(time(NULL));

	BasicAI * basicAI = new BasicAI();
	if (basicAI->Setup())
	{
		while (basicAI->MoveCycle());		
	}
	delete basicAI;
	exit(EXIT_SUCCESS);
	return 0;
}
