#include <time.h>
#include <chrono>

#ifndef MYTIME_H
#define MYTIME_H
const char* timePointToChar(
    const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>& tp) 
{
    time_t ttp = std::chrono::system_clock::to_time_t(tp);
    return ctime(&ttp);
}


char* toTime(std::chrono::system_clock::time_point target) {
	time_t temp = std::chrono::system_clock::to_time_t(target);
	char* result = ctime(&temp);
	for (char *ptr = result; *ptr != '\0'; ptr++)
	{
		if (*ptr == '\n' || *ptr == '\r')
			*ptr = '\0';
	}
	return result;
}
#endif