/**
 * Contains declaration for MovementResult class
 */
#ifndef MOVERESULT_H
#define MOVERESULT_H

class Board;
class Piece;

/**
 * Class used to indicate the result of a move in stratego
 */
class MovementResult
{
	public:
		typedef enum {OK, DIES, KILLS, BOTH_DIE, NO_BOARD, INVALID_POSITION, NO_SELECTION, NOT_YOUR_UNIT, IMMOBILE_UNIT, INVALID_DIRECTION, POSITION_FULL, VICTORY_FLAG, VICTORY_ATTRITION, SURRENDER, BAD_RESPONSE, NO_MOVE, COLOUR_ERROR, ERROR, DRAW_DEFAULT, DRAW, BAD_SETUP} Type;

		MovementResult(const Type & result = OK, const Piece::Type & newAttackerRank = Piece::NOTHING, const Piece::Type & newDefenderRank = Piece::NOTHING)
			: type(result), attackerRank(newAttackerRank), defenderRank(newDefenderRank) {}
		MovementResult(const MovementResult & cpy) : type(cpy.type), attackerRank(cpy.attackerRank), defenderRank(cpy.defenderRank) {}
		virtual ~MovementResult() {}
		

		bool operator==(const Type & equType) const {return type == equType;}
		bool operator!=(const Type & equType) const {return type != equType;}

		Type type;
		Piece::Type attackerRank;
		Piece::Type defenderRank;
};

#endif //MOVERESULT_H

//EOF
