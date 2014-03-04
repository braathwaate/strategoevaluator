#ifndef THREAD_UTIL_H
#define THREAD_UTIL_H

#include <stdlib.h>
#include <stdio.h>

#include <sstream>
#include <string>
#include <pthread.h> //Needed for threading
#include <signal.h> //Needed for killing the threads (why not in pthread.h?)

#include <assert.h>


class Thread
{
	public:
		Thread() : finished(false), thread(0),  started(false)
		{
			
		}

		virtual void Start() = 0;
	protected:
		void Start(void* (ThreadFunction)(void*))
		{
			assert(!started);
			started = true;
			pthread_create(&(thread), NULL, ThreadFunction, (void*)(this));
			pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
		}

	public:
		void Stop()
		{
			assert(started);
			if (!finished)
				pthread_cancel(thread);
			
			pthread_join(thread, NULL);
			started = false;
		}

		virtual ~Thread()
		{
			if (started)
				Stop();
		}

		bool Finished() const {return finished;}
	protected:
		
		bool finished;

	private:
		pthread_t thread;
	protected:
		bool started;
};

class GetterThread : public Thread
{
	public:
		GetterThread(FILE * newStream, std::string & newBuffer) : Thread(), stream(newStream), buffer(newBuffer)
		{
			
		}

		virtual ~GetterThread()
		{

		}

		virtual void Start() {assert(&buffer != NULL); Thread::Start(GetMessage);}

	private:
		FILE * stream;
	public:
		std::string & buffer;

		pthread_t thread;
		static pthread_mutex_t mutex;
		static void * GetMessage(void * p);

};


class TimerThread : public Thread
{
	public:
		TimerThread(int newCount) : Thread(), count(newCount)
		{
			
		}

		virtual ~TimerThread()
		{

		}

		virtual void Start() {Thread::Start(Timeout);}

	private:
		int count;

		static void * Timeout(void * p);

};

#endif //THREAD_UTIL_H

//EOF



