/**
 * Class to manage a Stratego playing AI in Java
 * @author Sam Moore for the UCC::Progcomp 2012
 * @website http://progcomp.ucc.asn.au
 */

import java.lang.Exception;
import java.util.Vector;
import java.util.Random;
import java.util.Scanner;




class BasicAI
{


	
	/**
 	 * Moves a point in a direction, returns new point
	 * @param x x coord
	 * @param y y coord
	 * @param direction Indicates direction. Must be "LEFT", "RIGHT", "UP", "DOWN"
	 * @param multiplier Spaces to move
	 * @returns An array of length 2, containing the new x and y coords
	 * @throws Exception on unrecognised direction
	 */
	public static int[] Move(int x, int y, String direction, int multiplier) throws Exception
	{
		//NOTE: The board is indexed so that the top left corner is x = 0, y = 0
		//	Does not check that coordiantes would be valid in the board.

		if (direction.compareTo("DOWN") == 0)
			y += multiplier; //Moving down increases y
		else if (direction.compareTo("UP") == 0)
			y -= multiplier; //Moving up decreases y
		else if (direction.compareTo("LEFT") == 0)
			x -= multiplier; //Moving left decreases x
		else if (direction.compareTo("RIGHT") == 0)
			x += multiplier;
		else
		{
			throw new Exception("BasicAI.Move - Unrecognised direction " + direction);
		}

		int result[] = new int[2];
		result[0] = x; result[1] = y;
		return result;
	}

	/**
	 * Returns the "opposite" colour to that given
	 * @param colour Must be "RED" or "BLUE"
	 * @returns The alternate String to colour
	 * @throws Exception if colour is not "RED" or "BLUE"
	 */
	public static String OppositeColour(String colour) throws Exception
	{
		if (colour.compareTo("BLUE") == 0)
			return "RED";
		else if (colour.compareTo("RED") == 0)
			return "BLUE";
		else
			throw new Exception("BasicAI.OppositeColour - Unrecognised colour " + colour);
	}

	/**
	 * Tests if a value is an integer
	 * I cry at using exceptions for this
	 */
	public static boolean IsInteger(String str)
	{
		try
		{
			Integer.parseInt(str);
		}
		catch (NumberFormatException e)
		{
			return false;
		}
		return true;
	}

	private Scanner scan;
	private int turn; //The turn number of the game
	private Piece board[][]; //The board
	private Vector<Piece> units; //All units
	private Vector<Piece> enemyUnits; //All enemy units
	private Piece lastMoved; //Last moved piece
	private String colour; //Colour of the AI
	private String opponentName; //Name of the AI's opponent
	private int width; //Width of the board (NOTE: Should always be 10)
	private int height; //Height of the board (NOTE: Should always be 10)

	private static int totalAllies[] = {6,1,1,2,3,4,4,4,5,8,1,1}; //Numbers of allies, B -> F
	private static int totalEnemies[] = {6,1,1,2,3,4,4,4,5,8,1,1}; //Numbers of enemies, B -> F
	private static int hiddenEnemies[] = {6,1,1,2,3,4,4,4,5,8,1,1}; //Number of hidden enemies of each type
	private static int hiddenAllies[] = {6,1,1,2,3,4,4,4,5,8,1,1}; //Number of hidden allies of each type

	private static String directions[] = {"UP", "DOWN", "LEFT", "RIGHT"}; //All available directions
	
	private static Random rand = new Random(); //A random number generator
	
	/**
	 * Constructor
 	 * Sets up a board, prepares to play
	 */
	public BasicAI()
	{
		scan = new Scanner(System.in);
		turn = 0;
		board = null;
		units = new Vector<Piece>();
		enemyUnits = new Vector<Piece>();

		lastMoved = null;
		colour = null;
		opponentName = null;

		//HACK to get rid of stupid Javac warnings
		if (lastMoved == null && opponentName == null);
	}

	/**
	 * Implements Setup phase of protocol described in manager program man page
	 * Always uses the same setup. Override to create custom setups.
	 */
	public void Setup() throws Exception
	{
		String input = scan.nextLine();
		Vector<String> setup = Reader.readTokens(input); //Wierd java way of doing input from stdin, see Reader.java
		if (setup.size() != 4)
		{
			throw new Exception("BasicAI.Setup - Expected 4 tokens, got " + setup.size());
		}	
		colour = setup.elementAt(0);
		opponentName = setup.elementAt(1);
		width = Integer.parseInt(setup.elementAt(2));
		height = Integer.parseInt(setup.elementAt(3));

		if (width != 10 || height != 10)
			throw new Exception("BasicAI.Setup - Expected width and height of 10, got " + width + " and " + height);

		board = new Piece[width][height];
		for (int x=0; x < board.length; ++x)
		{
			for (int y = 0; y < board[x].length; ++y)
				board[x][y] = null;
		}

		//TODO: Modify this setup
		if (colour.compareTo("RED") == 0)
			System.out.println("FB8sB479B8\nBB31555583\n6724898974\n967B669999");
		else if (colour.compareTo("BLUE") == 0)
			System.out.println("967B669999\n6724898974\nBB31555583\nFB8sB479B8");
		else
			throw new Exception("BasicAI.Setup - Unrecognised colour of " + colour);

	}

	/**
	 * Cycles a move
 	 */
	public void MoveCycle() throws Exception
	{
		InterpretResult();
		ReadBoard();
		MakeMove();
		InterpretResult();
	}

	/**
	 * Makes a move
	 * TODO: Rewrite move algorithm (currently uses random)
	 */
	public void MakeMove() throws Exception
	{
		if (units.size() <= 0)
			throw new Exception("BasicAI.MakeMove - No units left!");

		int index = rand.nextInt(units.size() - 1); //Pick index of unit to move
		int startIndex = index; //Remember it
		
		while (true) //Don't worry, there is a break
		{
			Piece piece = units.elementAt(index);
			if (piece == null)
				throw new Exception("BasicAI.MakeMove - null unit ???");
	
			if (piece.Mobile())
			{
				int dirIndex = rand.nextInt(directions.length); //Pick a random direction index
				int startDirIndex = dirIndex; //Remember
				while (true)
				{
					int p[] = Move(piece.x, piece.y, directions[dirIndex], 1);
					if (p[0] >= 0 && p[0] < width && p[1] >= 0 && p[1] < height)
					{
						Piece target = board[p[0]][p[1]];
						if (target == null || (target.colour != piece.colour && target.colour != "NONE" && target.colour != "BOTH"))
						{
							System.out.println(""+piece.x + " " + piece.y + " " + directions[dirIndex]);
							return;
						}
						
					}
					dirIndex = (dirIndex + 1) % directions.length;
					if (dirIndex == startDirIndex)
						break;
				}
			}
			index = (index + 1) % units.size();
			if (index == startIndex)
			{
				System.out.println("NO_MOVE");
				break;
			}
		}
		
	}
	
	/**
	 * Reads the board
	 */
	public void ReadBoard() throws Exception
	{
		for (int y = 0; y < height; ++y)
		{
			String row = scan.nextLine();
			if (row.length() != width)
				throw new Exception("BasicAI.ReadBoard - Row " + y + " has width " + row.length() + " instead of " + width);
			for (int x = 0; x < width; ++x)
			{
				if (turn == 0)
				{
					switch (row.charAt(x))
					{
						case '.':
							break;
						case '#':
							board[x][y] = new Piece(OppositeColour(colour), '?', x, y);
							enemyUnits.add(board[x][y]);
							break;
						case '+':
							board[x][y] = new Piece("NONE", '+', x, y);
							break;
						default:
							board[x][y] = new Piece(colour, row.charAt(x), x, y);
							units.add(board[x][y]);
							break;	
					}
						
				}
			}
		}
	}

	/**
	 * Removes a unit from the game
	 */
	private void KillUnit(Piece kill) throws Exception
	{
		Vector<Piece> removeFrom = null;
		if (kill.colour.compareTo(colour) == 0)
		{
			totalAllies[Piece.Index(kill.rank)] -= 1;
			removeFrom = units;
		}
		else if (kill.colour.compareTo(OppositeColour(colour)) == 0)
		{
			totalEnemies[Piece.Index(kill.rank)] -= 1;
			removeFrom = enemyUnits;
		}
		if (removeFrom == null)
			throw new Exception("BasicAI.KillUnit - Can't identify unit with colour " + kill.colour + "!");

		for (int ii=0; ii < removeFrom.size(); ++ii)
		{
			if (removeFrom.elementAt(ii) == kill)
			{
				removeFrom.remove(ii);
				return;
			}				
		}
		throw new Exception("BasicAI.KillUnit - Couldn't find unit in unit list.");
	}

	/**
	 * Interprets the result of a move, updates all relevant variables
	 */
	public void InterpretResult() throws Exception
	{
		String input = scan.nextLine();
		Vector<String> result = Reader.readTokens(input);
		if (turn == 0)
			return;
		if (result.elementAt(0).compareTo("QUIT") == 0)
			System.exit(0);
		if (result.elementAt(0).compareTo("NO_MOVE") == 0)
			return;

		if (result.size() < 4)
		{
			throw new Exception("BasicAI.InterpretResult - Expect at least 4 tokens, got " + result.size());
		}

		int x = Integer.parseInt(result.elementAt(0));
		int y = Integer.parseInt(result.elementAt(1));
		String direction = result.elementAt(2);

		int multiplier = 1;
		String outcome = result.elementAt(3);
		int outIndex = 3;
		if (IsInteger(outcome))
		{
			multiplier = Integer.parseInt(outcome);
			outcome = result.elementAt(4);
			outIndex = 4;
		}
		int p[] = Move(x,y,direction, multiplier);

		Piece attacker = board[x][y];
		board[x][y] = null;
		if (attacker == null)
			throw new Exception("BasicAI.InterpretResult - Couldn't find a piece to move at (" + x +"," + y+")");

		lastMoved = attacker;

		Piece defender = board[p[0]][p[1]];
		

		attacker.x = p[0]; attacker.y = p[1];
		attacker.positions.add(0, p);

		if (result.size() >= outIndex + 3)
		{
			if (defender == null)
				throw new Exception("BasicAI.InterpretResult - Result suggests a defender at ("+p[0]+","+p[1]+"), but none found");
			attacker.rank = result.elementAt(outIndex+1).charAt(0); //ranks are 1 char long
			if (attacker.beenRevealed == false)
			{
				if (attacker.colour.compareTo(colour) == 0)
					hiddenAllies[Piece.Index(attacker.rank)] -= 1;
				else if (attacker.colour.compareTo(OppositeColour(colour)) == 0)
					hiddenEnemies[Piece.Index(attacker.rank)] -= 1;
				else
					throw new Exception("BasicAI.InterpretResult - Colour " + attacker.colour + " for moving piece makes no sense.");
			}
			attacker.beenRevealed = true;
			defender.rank = result.elementAt(outIndex+2).charAt(0); //ranks are 1 char long
			if (defender.beenRevealed == false)
			{
				if (defender.colour.compareTo(colour) == 0)
					hiddenAllies[Piece.Index(defender.rank)] -= 1;
				else if (attacker.colour.compareTo(OppositeColour(colour)) == 0)
					hiddenEnemies[Piece.Index(defender.rank)] -= 1;
				else
					throw new Exception("BasicAI.InterpretResult - Colour " + attacker.colour + " for defending piece makes no sense.");
			}
			defender.beenRevealed = true;
			
		}
		if (outcome.compareTo("OK") == 0)
			board[p[0]][p[1]] = attacker;
		else if (outcome.compareTo("KILLS") == 0)
		{
			board[p[0]][p[1]] = attacker;
			KillUnit(defender);
		}
		else if (outcome.compareTo("DIES") == 0)
		{
			KillUnit(attacker);
		}
		else if (outcome.compareTo("BOTHDIE") == 0)
		{
			board[p[0]][p[1]] = null;
			KillUnit(attacker);
			KillUnit(defender);
		}
		else
		{
			System.exit(0); //Game over
		}
		

	}

	/**
	 * The main function!
	 */
	public static void main(String [] args)
	{
		try
		{
			BasicAI theAI = new BasicAI();
			theAI.Setup();
			while (true)
				theAI.MoveCycle();
		}
		catch (Exception e)
		{
			System.out.println("EXCEPTION: " + e.getMessage());
		}
	}

}
