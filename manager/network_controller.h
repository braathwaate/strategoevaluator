#include "controller.h"
#include "ai_controller.h"
#include "human_controller.h"
#include "network.h"

#ifndef NETWORK_CONTROLLER_H
#define NETWORK_CONTROLLER_H

class NetworkController : public Controller
{
	public:
		NetworkController(const Piece::Colour & colour, Network * newNetwork) : Controller(colour), network(newNetwork) {}
		virtual ~NetworkController() {}

		//virtual void Message(const char * message) = 0; 
		virtual bool Valid() const {return network->Valid();}

	protected:
		Network * network;
};

class NetworkSender : public NetworkController
{
	public:
		NetworkSender(const Piece::Colour & colour, Controller * newController, Network * newNetwork) : NetworkController(colour, newNetwork), controller(newController) {}
		virtual ~NetworkSender() {delete controller;}

		virtual bool Valid() const {return NetworkController::Valid() && controller->Valid();}

		virtual bool Message(const char * message) 
		{
			//fprintf(stderr,"NetworkSender sending message %s to underlying controller\n", message);
			return (controller->Message(message));
		}

		virtual MovementResult QuerySetup(const char * opponentName, std::string setup[]);
		virtual MovementResult QueryMove(std::string & buffer);

	private:
		Controller * controller;

};

class NetworkReceiver : public NetworkController
{
	public:
		NetworkReceiver(const Piece::Colour & colour, Network * newNetwork) : NetworkController(colour, newNetwork) {}
		virtual ~NetworkReceiver() {}

		virtual bool Message(const char * message) {return true;} //Do nothing (Counter intuitive much)
		virtual MovementResult QuerySetup(const char * opponentName, std::string setup[]);
		virtual MovementResult QueryMove(std::string & buffer);
		

};

#endif //NETWORK_CONTROLLER_H

//EOF
