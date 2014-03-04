#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include <string>
#include "thread_util.h"

#ifndef NETWORK_H
#define NETWORK_H


class Network
{
	public:
		Network(int newPort = 4560);
		virtual ~Network();
		bool Valid() const {return sfd != -1;}

		bool SendMessage(const char * print, ...); //Sends a formated message (NOTE: Prints a newline)
		bool SendMessage(const std::string & buffer) {return SendMessage(buffer.c_str());} //Sends a C++ string message
		bool GetMessage(std::string & buffer, double timeout=-1); //Retrieves a message, or waits for a timeout (if positive)

	protected:
		int sfd;
		int port;
		FILE * file;
		
};

class Server : public Network
{
	public:
		Server(int newPort = 4560);
		virtual ~Server() {}
		
};

class Client : public Network
{
	public:
		Client(const char * server = "127.0.0.1", int newPort = 4560);
		virtual ~Client() {}
};

#endif //NETWORK_H
