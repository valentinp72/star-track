#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <wiringPi.h>

#include "consts.h"

#define EMERGENCY_REFRESH_RATE 50000000

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
	setup();

	struct timespec ts;
	ts.tv_sec  = 0;
	ts.tv_nsec = EMERGENCY_REFRESH_RATE;

	int last_state = digitalRead(EMERGENCY);
	int tmp_state;
	digitalWrite(ENABLE, last_state);

	while (true) {
		tmp_state = digitalRead(EMERGENCY);
		if (tmp_state != last_state) {
			last_state = tmp_state;
			digitalWrite(ENABLE, last_state);
		}
		nanosleep(&ts, &ts);
	}

	return 0;
}
