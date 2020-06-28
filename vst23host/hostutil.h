#ifndef hostutil_h
#define hostutil_h

#ifdef HOSTNODEBUG
#define log(fmt, ...)
#else
#include <cstdio>
#include <iostream>
#define log(fmt, ...) \
{ \
char message_buffer[300]; \
std::sprintf(message_buffer, fmt, ##__VA_ARGS__);  \
std::cout << "--- " << message_buffer << std::endl; \
}
#endif


#endif /* hostutil_h */
