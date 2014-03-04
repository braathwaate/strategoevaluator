/**
 * Class to represent a Piece
 * @author Sam Moore
 * @website http://progcomp.ucc.asn.au
 */

import java.lang.Exception;
import java.util.Vector;

class Piece
{
	//Normally in the Java Way (TM) you would have to make these private, and add "Getters" and "Setters" and all sorts of crap.
	// Disclaimer: The author is not responsible for the repercussions of not following the Java Way (TM)
	public int x; //x coord
	public int y; //y coord
	public char rank; //Rank of the piece
	public String colour; //The colour of the Piece
	public Vector<int[]> positions; //All positions the piece has been at
	public boolean beenRevealed; //True if the piece has been revealed in combat

	public static char ranks[] = {'B','1','2','3','4','5','6','7','8','9','s','F', '?', '+'}; //List of all the possible ranks
	//'?' is an unknown piece, '+' is an obstacle

	/**
	 * Constructor
	 * @param c The new colour
	 * @param r The new rank
	 * @param xx The new x coord
	 * @param yy The new y coord
	 */
	public Piece(String c, char r, int xx, int yy)
	{
		
		this.colour = c;
		this.rank = r;
		this.x = xx;
		this.y = yy;
		this.positions = new Vector<int[]>();
		this.beenRevealed = false;
		
		positions.add(new int[2]);
		positions.elementAt(0)[0] = x;
		positions.elementAt(0)[1] = y;
	}

	/**
 	 * @returns True if the piece can move, based on its rank
	 */
	public boolean Mobile()
	{
		return (rank != 'F' && rank != 'B' && rank != '+' && rank != '?');
	}

	/**
	 * @returns The value of the piece's rank
	 */
	public int ValuedRank()
	{
		for (int ii=0; ii < ranks.length; ++ii)
		{
			if (ranks[ii] == rank)
				return (ranks.length - 2 - ii);
		}
		return 0;
	}

	/**
 	 * @returns the index in ranks of a rank
	 * @throws Exception if the rank doesn't exist
	 */
	public static int Index(char rank) throws Exception
	{
		for (int ii=0; ii < ranks.length; ++ii)
		{
			if (ranks[ii] == rank)
				return ii;
		}
		throw new Exception("Piece.Index - No such rank as " + rank);
		
	}
}
