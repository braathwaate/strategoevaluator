#include "network.h"
#include <stdarg.h>

using namespace std;

Network::Network(int newPort) : sfd(-1), port(newPort), file(NULL)
{
    	sfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
	{
		perror("Network::Network - Error creating TCP socket");
		exit(EXIT_FAILURE);
	}
}

Network::~Network()
{
	if (Valid())
	{
		if (shutdown(sfd, SHUT_RDWR) == -1)
		{
			perror("Network::~Network - shutting down socket... ");
			close(sfd);
			sfd = -1;
		}
	}
	close(sfd);
}

Server::Server(int newPort) : Network(newPort)
{
	struct   sockaddr_in name;
//	char   buf[1024];
//	int    cc;

	
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	name.sin_port = htons(port);

	if (bind( sfd, (struct sockaddr *) &name, sizeof(name) ) < 0)
	{
		perror("Server::Server - Error binding socket");
		close(sfd); sfd = -1;
		exit(EXIT_FAILURE);
	}

	if (listen(sfd,1) < 0)
	{
		perror("Server::Server - Error listening on socket");
		close(sfd); sfd = -1;
		exit(EXIT_FAILURE);
	}
	int psd = accept(sfd, 0, 0);
	close(sfd);
	sfd = psd;
	if (sfd < 0)
	{
		perror("Server::Server - Error accepting connection");
		close(sfd); sfd = -1;
		exit(EXIT_FAILURE);
	}



	/*
	for(;;) 
	{
		cc=recv(sfd,buf,sizeof(buf), 0) ;
		if (cc == 0) exit (0);
		buf[cc] = '\0';
		printf("message received: %s\n", buf);
	}
	*/
}

Client::Client(const char * serverAddress, int newPort) : Network(newPort)
{
	struct	sockaddr_in server;
	struct  hostent *hp;


	server.sin_family = AF_INET;
	hp = gethostbyname(serverAddress);
	bcopy ( hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
	server.sin_port = htons(port);

	if (connect(sfd, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		fprintf(stderr, "Client::Client - Error connecting to server at address %s: ", serverAddress);
		perror("");
		close(sfd); sfd = -1;
		exit(EXIT_FAILURE);
	}




	/*
        for (;;) {
	   send(sfd, "HI", 2,0 );
           sleep(2);
        }
	*/
}

/**
 * Sends a message accross the network
 * WARNING: Always terminates the message with a newline '\n'
 * @returns true if the message was successfully sent; false if it was not (ie: the network was not connected!)
 */
bool Network::SendMessage(const char * print, ...)
{
	if (!Valid()) //Is the process running...
		return false; 

	if (file == NULL)
	{
		file = fdopen(sfd, "r+");
		setbuf(file, NULL);
	}

	va_list ap;
	va_start(ap, print);

	if (vfprintf(file, print, ap) < 0 || fprintf(file, "\n") < 0)
	{
		va_end(ap);
		return false;
	}
	va_end(ap);

	return true;
}

/**
 * Retrieves a message from the network, waiting a maximum amount of time
 * @param buffer - C++ string to store the resultant message in
 * @param timeout - Maximum amount of time to wait before failure. If timeout <= 0, then GetMessage will wait indefinately.
 * @returns true if the response was recieved within the specified time, false if it was not, or an EOF was recieved, or the process was not running.
 */
bool Network::GetMessage(string & buffer, double timeout)
{
	if (!Valid() || timeout == 0)
		return false;

	if (file == NULL)
	{
		file = fdopen(sfd, "r+");
		setbuf(file, NULL);
	}

	struct timeval tv;
	fd_set readfds;
	
	tv.tv_sec = (int)(timeout);
	tv.tv_usec = (timeout - (double)((int)timeout)) * 1000000;

	FD_ZERO(&readfds);
	FD_SET(sfd, &readfds);

	select(sfd+1, &readfds, NULL, NULL, &tv);

	if (!FD_ISSET(sfd, &readfds))
		return false; //Timed out
	//fprintf(stderr, "Got message!\n");
	for (char c = fgetc(file); c != '\n' && (int)(c) != EOF; c = fgetc(file))
	{	
		//fprintf(stderr, "%c", c);
		buffer += c;
	}
	//fprintf(stderr, "%s\n", buffer.c_str());
	return true;


	/* Old way, which is apparently terrible

	assert(&buffer != NULL);
	GetterThread getterThread(file, buffer);
	assert(&(getterThread.buffer) != NULL);

	TimerThread timerThread(timeout*1000000);

	getterThread.Start();
	if (timeout > 0)
		timerThread.Start();

	
	while (!getterThread.Finished())
	{
		if (timeout > 0 && timerThread.Finished())
		{
			getterThread.Stop();
			timerThread.Stop();
			return false;
		}
	}

	getterThread.Stop();
	if (timeout > 0)
		timerThread.Stop();

	

	if (buffer.size() == 1 && buffer[0] == EOF)
		return false;
	return true;
	*/


}

