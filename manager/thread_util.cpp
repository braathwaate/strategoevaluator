#include "thread_util.h"

#include <sstream>
#include <string>

using namespace std;

pthread_mutex_t GetterThread::mutex = PTHREAD_MUTEX_INITIALIZER;

void * GetterThread::GetMessage(void * p)
{
	
	GetterThread * getter = (GetterThread*)(p);
	
	stringstream inputStream;

	char s = fgetc(getter->stream);
	while (s != '\n' && s != EOF)
	{
		
		inputStream << s;
		s = fgetc(getter->stream);
	}
	if (s == EOF)
	{
		getter->buffer = "";
		getter->buffer += s;
		return NULL;
	}	

	pthread_mutex_lock(&mutex);
		getter->buffer = inputStream.str();
	pthread_mutex_unlock(&mutex);
	
	getter->finished = true;

	return NULL;
}

void * TimerThread::Timeout(void * p)
{
	TimerThread * timer = (TimerThread*)(p);
	usleep(timer->count);
	timer->finished = true;
	return NULL;
}

