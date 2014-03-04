#ifndef HUMAN_CONTROLLER_H
#define HUMAN_CONTROLLER_H

#include "controller.h"

/**
 * Class to control a human player playing Stratego
 */
class Human_Controller : public Controller
{
	public:
		Human_Controller(const Piece::Colour & newColour, const bool enableGraphics) : Controller(newColour, "human"), graphicsEnabled(enableGraphics) {}
		virtual ~Human_Controller() {}

		virtual bool HumanController() const {return true;}
		virtual MovementResult QuerySetup(const char * opponentName, std::string setup[]);
		virtual MovementResult QueryMove(std::string & buffer); 
		virtual bool Message(const char * message) {return (strlen(message) <= 0 || fprintf(stderr, "%s\n", message) > 0);}
	
	private:
		const bool graphicsEnabled;


};

#endif //AI_CONTROLLER_H
