#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "stratego.h"
#include <string>

/**
 * Class to control a player for Stratego
 * Abstract base class
 */

class Controller
{
	public:
		Controller(const Piece::Colour & newColour, const char * newName = "no-name") : colour(newColour), name(newName) {}
		virtual ~Controller() {}

		MovementResult Setup(const char * opponentName);

		MovementResult MakeMove(std::string & buffer);

		virtual bool HumanController() const {return false;} //Hacky... overrides in human_controller... avoids having to use run time type info

		bool Message(const std::string & buffer) {return Message(buffer.c_str());}
		virtual bool Message(const char * string) = 0;

		virtual MovementResult QuerySetup(const char * opponentName, std::string setup[]) = 0;
		virtual MovementResult QueryMove(std::string & buffer) = 0;
		virtual bool Valid() const {return true;}


		virtual void Pause() {} 	// Hack function (AI_Controller ONLY will overwrite with wrapper to Program::Pause)
		virtual void Continue() {}	// Hack function (AI_Controller '' '' wrapper to Program::Continue)

		const Piece::Colour colour; 

		virtual void FixName(); //Should be called after setup, sets the name of the controller

		std::string name;


};





#endif //CONTROLLER_H


