#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <wiringPi.h>

#include "consts.h"

#define EMERGENCY_REFRESH_RATE 500

void setup() {
	wiringPiSetup();
	pinMode(ENABLE, OUTPUT);
	pinMode(EMERGENCY, INPUT);
	
	// Set this process onto the asked CPU
	cpu_set_t  mask;
	CPU_ZERO(&mask);
	CPU_SET(CPU_ID, &mask);
	int result = sched_setaffinity(0, sizeof(mask), &mask);
}

int main() {
	struct timespec ts;
	ts.tv_sec  = 0;
	ts.tv_nsec = EMERGENCY_REFRESH_RATE;

	while (true) {
		int emergency = digitalRead(EMERGENCY);
		digitalWrite(ENABLE, emergency);
		nanosleep(&ts, &ts);
	}

	return 0;
}
