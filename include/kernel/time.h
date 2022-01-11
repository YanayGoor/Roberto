#ifndef ROBERTO_TIME_H
#define ROBERTO_TIME_H

#include <stdint.h>

typedef uint64_t rtime_t;

void time_init(void);
rtime_t get_time(void);
void sleep(unsigned int seconds);
void msleep(unsigned int milliseconds);

#endif // ROBERTO_TIME_H
