#ifndef PROGRAM_H
#define PROGRAM_H

#include "thread_util.h"

#include <string>
#include <unistd.h> //Needed to check permissions

/**
 * A wrapping class for an external program, which can exchange messages with the current process through stdin/stdout
 * Useful for attaching control of an operation to an external process - for example, AI for a game
 */
class Program
{
	public:
		Program(const char * executeablePath); //Constructor
		virtual ~Program(); //Destructor




		bool SendMessage(const char * print, ...); //Sends a formated message (NOTE: Prints a newline)
		bool SendMessage(const std::string & buffer) {return SendMessage(buffer.c_str());} //Sends a C++ string message
		bool GetMessage(std::string & buffer, double timeout=-1); //Retrieves a message, or waits for a timeout (if positive)

		bool Running() const;
		bool Paused() const;
		bool Pause();
		bool Continue();
		

	protected:
		FILE * input;	//Stream used for sending information TO the process
		FILE * output; //Stream used for retrieving information FROM the process

	private:
		pid_t pid; //Process ID of the program wrapped
		bool paused;
		
};

#endif //PROGRAM_H


