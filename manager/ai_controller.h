#ifndef AI_CONTROLLER_H
#define AI_CONTROLLER_H

#include "controller.h"
#include "program.h"

/**
 * Class to control an AI program playing Stratego
 * Inherits mostly from Program
 */
class AI_Controller : public Controller, private Program
{
	public:
		AI_Controller(const Piece::Colour & newColour, const char * executablePath, const double newTimeout = 60.0) : Controller(newColour, executablePath), Program(executablePath), timeout(newTimeout) {}
		virtual ~AI_Controller() {}

		

		virtual MovementResult QuerySetup(const char * opponentName,std::string setup[]);
		virtual MovementResult QueryMove(std::string & buffer);

		virtual bool Message(const char * message) 
		{
			//fprintf(stderr, "Sending message \"%s\" to AI program...\n", message);
			return Program::SendMessage(message);
		}
		virtual void Pause() {Program::Pause();} //Hack wrapper
		virtual void Continue() {Program::Continue();} //Hack wrapper

		virtual bool Valid() const {return Program::Running();}


	private:
		const double timeout; //Timeout in seconds for messages from the AI Program

};

#endif //AI_CONTROLLER_H
