#include <sstream>

#include <stdarg.h>

#include <cassert>

#include "thread_util.h"
#include "program.h"
#include <vector>
#include <string.h>
#include <stdio.h>

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


using namespace std;


/**
 * Constructor
 * @param executablePath - path to the program that will be run
 *
 * Creates two pipes - one for each direction between the parent process and the AI program
 * Forks the process. 
 *	The child process closes unused sides of the pipe, and then calls exec to replace itself with the AI program
 *	The parent process closes unused sides of the pipe, and sets up member variables - associates streams with the pipe fd's for convenience.
 */
Program::Program(const char * executablePath) : input(NULL), output(NULL), pid(0), paused(false)
{
	
		
	/*
	vector<char*> args;
	if (executablePath[0] != '"')
		args.push_back((char*)executablePath);
	else
		args.push_back((char*)(executablePath)+1);
	char * token = NULL;
	do
	{
		token = strstr(args[args.size()-1], " ");
		if (token == NULL)
			break;

		*token = '\0';
		do
		{
			++token;
			if (*token == '"')
				*token = '\0';
		}
		while (*token != '\0' && iswspace(*token));

		if (*token != '\0' && !iswspace(*token))
		{
			args.push_back(token);
		}
		else
			break;
	}
	while (token != NULL);

	char **  arguments = NULL;
        if (args.size() > 0)
	{
		arguments = new char*[args.size()];
		for (unsigned int i=0; i < args.size(); ++i)
			arguments[i] = args[i];
	}
	*/
	//See if file exists and is executable...
	if (access(executablePath, X_OK) != 0)
	{
		pid = -1;
		return;
	}
	
	int readPipe[2]; int writePipe[2];
	assert(pipe(readPipe) == 0);
	assert(pipe(writePipe) == 0);

	pid = fork();
	if (pid == 0)
	{
		close(readPipe[0]);  //close end that parent reads from
		close(writePipe[1]); //close end that parent writes to

		//TODO: Fix possible bug here if the process is already a daemon
		assert(writePipe[0] != 0 && readPipe[1] != 1);
		dup2(writePipe[0],0); close(writePipe[0]); //pipe end child reads from goes to STDIN
		dup2(readPipe[1], 1); close(readPipe[1]); //pipe end child writes to goes to STDOUT

		//TODO: Somehow force the exec'd process to be unbuffered
		setbuf(stdin, NULL); //WARNING: These lines don't appear to have any affect
		setbuf(stdout, NULL); //You should add them at the start of the wrapped program.
					//If your wrapped program is not written in C/C++, you will probably have a problem
				

		if (access(executablePath, X_OK) == 0) //Check we STILL have permissions to start the file
		{
			execl(executablePath, executablePath, (char*)(NULL)); ///Replace process with desired executable
			//execv(executablePath,arguments); ///Replace process with desired executable
		}
		perror("execv error:\n");
		fprintf(stderr, "Program::Program - Could not run program \"%s\"!\n", executablePath);
		exit(EXIT_FAILURE); //We will probably have to terminate the whole program if this happens
	}
	else
	{
		close(writePipe[0]); //close end that child writes to
		close(readPipe[1]); //close end that child reads from

		input = fdopen(readPipe[0],"r"); output = fdopen(writePipe[1],"w");
		setbuf(input, NULL);
		setbuf(output, NULL);
	}
	
}

/**
 * Destructor
 * Writes EOF to the wrapped program and then closes all streams
 * Kills the wrapped program if it does not exit within 1 second.
 */
Program::~Program()
{
	if (Running()) //Check if the process created is still running...
	{
		//fputc(EOF, output); //If it was, tell it to stop with EOF

		TimerThread timer(2); //Wait for 2 seconds
		timer.Start();		
		while (!timer.Finished())
		{
			if (!Running())
			{
				timer.Stop();
				break;
			}
		}
		timer.Stop();
		kill(pid, SIGKILL);
	}
	if (pid > 0)
	{
		fclose(input);
		fclose(output);
	}
	
}

/**
 * Forces the program to pause by sending SIGSTOP
 * Program can be resumed by calling Continue() (which sends SIGCONT)
 * @returns true if the program could be paused, false if it couldn't (probably because it wasn't running)
 */
bool Program::Pause()
{
	if (pid > 0 && kill(pid,SIGSTOP) == 0)
	{
		paused = true;
		return true;
	}
	return false;
}

/**
 * Causes a paused program to continue
 * @returns true if the program could be continued, false if it couldn't (probably because it wasn't running)
 */
bool Program::Continue()
{
	if (pid > 0 && kill(pid,SIGCONT) == 0)
	{
		paused = false;
		return true;
	}
	return false;
}

/**
 * @returns true iff the program is paused
 */
bool Program::Paused() const
{
	return paused;
}


/**
 * Sends a message to the wrapped AI program
 * WARNING: Always prints a new line after the message (so don't include a new line)
 *	This is because everything is always line buffered.
 * @returns true if the message was successfully sent; false if it was not (ie: the process was not running!)
 */
bool Program::SendMessage(const char * print, ...)
{
	if (!Running()) //Is the process running...
		return false; 

	va_list ap;
	va_start(ap, print);

	if (vfprintf(output, print, ap) < 0 || fprintf(output, "\n") < 0)
	{
		va_end(ap);
		return false;
	}
	va_end(ap);
	



	return true;
}


/**
 * Retrieves a message from the wrapped AI program, waiting a maximum amount of time
 * @param buffer - C++ string to store the resultant message in
 * @param timeout - Maximum amount of time to wait before failure. If timeout <= 0, then GetMessage will wait indefinately.
 * @returns true if the response was recieved within the specified time, false if it was not, or an EOF was recieved, or the process was not running.
 */
bool Program::GetMessage(string & buffer, double timeout)
{
	if (!Running() || timeout == 0)
		return false;

	struct timeval tv;
	fd_set readfds;
	
	tv.tv_sec = (int)(timeout);
	tv.tv_usec = (timeout - (double)((int)timeout)) * 1000000;
	
	int fd = fileno(input);

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	select(fd+1, &readfds, NULL, NULL, &tv);

	if (!FD_ISSET(fd, &readfds))
		return false; //Timed out
	//fprintf(stderr, "Got message!\n");
	for (char c = fgetc(input); c != '\n' && (int)(c) != EOF; c = fgetc(input))
	{	
		//fprintf(stderr, "%c", c);
		buffer += c;
	}
	//fprintf(stderr, "%s\n", buffer.c_str());
	//fprintf(stderr,"DONE\n");
	return true;

	/* Old way, using threads, which apparently is terrible
	assert(&buffer != NULL);
	GetterThread getterThread(input, buffer);
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

/**
 * Returns true iff the process is running
 * @returns what I just said, fool
 */
bool Program::Running() const
{
	return (pid > 0 && kill(pid,0) == 0);
}




