#include "network_controller.h"

using namespace std;

MovementResult NetworkSender::QuerySetup(const char * opponentName, string setup[])
{
	//fprintf(stderr,"	NetworkSender::QuerySetup... ");
	MovementResult result = controller->QuerySetup(opponentName, setup);
	
	for (int ii=0; ii < 4; ++ii)
		assert(network->SendMessage("%s",setup[ii].c_str())); //TODO: Proper error check

	//fprintf(stderr,"Done!\n");
	return result;
}

MovementResult NetworkReceiver::QuerySetup(const char * opponentName, string setup[])
{
	//fprintf(stderr,"	NetworkReceiver::QuerySetup... ");
	for (int ii=0; ii < 4; ++ii)
	{
		assert(network->GetMessage(setup[ii], 20000));
	}
	//fprintf(stderr,"Done!\n");
	return MovementResult::OK;
}

MovementResult NetworkSender::QueryMove(string & buffer)
{
	MovementResult result = controller->QueryMove(buffer);
	network->SendMessage("%s", buffer.c_str());
	return result;
}

MovementResult NetworkReceiver::QueryMove(string & buffer)
{
	assert(network->GetMessage(buffer, 20000));
	return MovementResult::OK;
}
