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

// Bigger number : slower
#define START_SLEEP 19000000 // => minimum speed
#define MIN_SLEEP    2000000 // => maximum speed

// Bigger number : faster acceleration
#define ACCELERATION   10000 // => acceleration

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef enum command_type { SET_DIST, SET_HOME } command_type;
typedef enum direction { FORWARD = 1, BACKWARD = -1 } direction;

typedef struct command {
	command_type type;
	int value;
} command;

typedef struct actions {
	int * times;
	direction * dirs;
	int current;
	int size;
} actions;

int DIR_PIN;
int STEP_PIN;

bool parse_command(char * buffer, command * cmd) {
	char command_str[1024];
	int value = 0;

	int r = sscanf(buffer, "%s %d", command_str, &value);

	if (r != 2) {
		return false;
	}
	if (strcmp(command_str, "SET_DIST") == 0) {
		cmd->type = SET_DIST;
		cmd->value = value;
		return true;
	}
	else if (strcmp(command_str, "SET_HOME") == 0) {
		cmd->type = SET_HOME;
		cmd->value = value;
		return true;
	}
	// not a valid command
	return false;
}

bool check_command(command * cmd) {
	int count;
	char buffer[1024];
	count = read(STDIN_FILENO, buffer, 1024);
	if(count < 0 && errno == EAGAIN) {
		return false;
	}
	else if(count >= 0) {
		buffer[count] = '\0';
		return parse_command(buffer, cmd);
	}
	else {
		printf("Error while reading: %i | %s", errno, strerror(errno));
		return false;
	}
}

direction get_dir(int current_pos, int dest_pos) {
	if (current_pos > dest_pos) {
		return BACKWARD;
	}
	return FORWARD;
}

void update_actions(int current_pos, int dest_pos, actions * actions) {
	
	int delta = abs(current_pos - dest_pos);
	int ramp_up_steps = delta / 2;
	int k = actions->current;
	int slow_down_steps = 0;

	direction dir = get_dir(current_pos, dest_pos);
	if (dir != actions->dirs[k]) {
		slow_down_steps = abs(actions->times[k] - START_SLEEP) / ACCELERATION + 1;
	}

	int total_steps = delta + slow_down_steps;

	if (total_steps == 0) {
		actions->current = actions->size - 1;
		return;
	}
	int * times = malloc(sizeof(int) * total_steps);
	int * dirs  = malloc(sizeof(direction) * total_steps);

	times[0] = actions->times[k];
	dirs[0]  = actions->dirs[k];
	int j = 1;

	for (; j < slow_down_steps ; j++) {
		times[j] = MIN(times[j - 1] + ACCELERATION, START_SLEEP);
		dirs[j]  = actions->dirs[k];
	}

	for (int i = j; i < ramp_up_steps ; i++) {
		times[i] = MAX(times[i - 1] - ACCELERATION, MIN_SLEEP);
		dirs[i]  = dir;
	}

	times[total_steps - 1] = START_SLEEP;
	dirs[total_steps - 1]  = dir;
	for (int i = total_steps - 2 ; i >= ramp_up_steps ; i--) {
		times[i] = MAX(times[i + 1] - ACCELERATION, MIN_SLEEP);
		dirs[i] = dir;
	}

	free(actions->times);
	free(actions->dirs);
	actions->times = times;
	actions->dirs  = dirs;
	actions->current = 0;
	actions->size = total_steps;
}

void setup() {
	wiringPiSetup();
	pinMode(DIR_PIN, OUTPUT);
	pinMode(STEP_PIN, OUTPUT);
	digitalWrite(DIR_PIN, 0);
	
	// Allow reading from STDIN without blocking
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	if(fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK))
		exit(1);

	// Set this process onto the asked CPU
	cpu_set_t  mask;
	CPU_ZERO(&mask);
	CPU_SET(CPU_ID, &mask);
	int result = sched_setaffinity(0, sizeof(mask), &mask);
}

void usage(char * argv[]) {
	fprintf(stderr, "Usage: %s <azimuth/altitude>\n", argv[0]);
}

int main(int argc, char * argv[]){

	if (argc != 2) {
		usage(argv);
		exit(1);
	}
	else if (strcmp(argv[1], "azimuth") == 0) {
		STEP_PIN = AZ_STEP;
		DIR_PIN  = AZ_DIR;
	}
	else if (strcmp(argv[1], "altitude") == 0) {
		STEP_PIN = AL_STEP;
		DIR_PIN  = AL_DIR;
	}
	else {
		usage(argv);
		exit(1);
	}

	command last_command;
	actions actions;
	struct timespec ts;

	int current_dir = FORWARD;
	int current_pos = 0;
	int dest_pos    = 0;

	setup();

	ts.tv_sec  = 0;
	ts.tv_nsec = MIN_SLEEP;

	actions.times = malloc(sizeof(int) * 1);
	actions.dirs  = malloc(sizeof(direction) * 1);
	actions.times[0] = START_SLEEP;
	actions.dirs[0]  = FORWARD;
	actions.current = 0;
	actions.size = 1;
	update_actions(current_pos, dest_pos, &actions);

	while (true) {

		if (check_command(&last_command)) {
			switch (last_command.type) {
				case SET_HOME:
					current_pos = 0;
					dest_pos = 0;
					break;
				case SET_DIST:
					dest_pos = last_command.value;
					break;
			}
			update_actions(current_pos, dest_pos, &actions);
		}

		if (current_pos != dest_pos && actions.current < actions.size) {
			current_dir = actions.dirs[actions.current];
			if (current_dir == FORWARD) {
				current_pos += 1;
				digitalWrite(DIR_PIN, 1);
			}
			else {
				current_pos -= 1;
				digitalWrite(DIR_PIN, 0);
			}
			digitalWrite(STEP_PIN, 1);
			digitalWrite(STEP_PIN, 0);
		
			ts.tv_nsec = actions.times[actions.current];

			if (actions.current + 1 < actions.size)
				actions.current += 1;
		}
		nanosleep(&ts, &ts);
	}

	return 0;
}
