#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <wiringPi.h>

#define	AZ_DIR 22
#define AZ_STEP 23
#define EMERGENCY 4
#define ENABLE 21

#define START_SLEEP 50000000
#define MIN_SLEEP 5000000
#define ACCELERATION 0.99

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

enum command_type { SET_DIST, SET_HOME };
struct command {
	enum command_type type;
	int value;
};
struct sleep_times {
	int * times;
	int current_time;
	int size;
};

bool parse_command(char * buffer, struct command * cmd) {
	char command_str[50];
	int value;

	sscanf(buffer, "%s %d", command_str, &value);

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

bool check_command(struct command * cmd) {
	int count;
	char buffer[1024];
	count = read(STDIN_FILENO, buffer, 1024);
	if(count < 0 && errno == EAGAIN) {
		return false;
	}
	else if(count >= 0) {
		return parse_command(buffer, cmd);
	}
	else {
		printf("Error while reading: %i | %s", errno, strerror(errno));
		return false;
	}
}

void setup() {
	wiringPiSetup();
	pinMode(AZ_DIR, OUTPUT);
	pinMode(AZ_STEP, OUTPUT);
	digitalWrite(AZ_DIR, 0);
	
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	if(fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK))
		exit(1);
}

void update_sleep_times(int current_pos, int dest_pos, struct sleep_times * sleep_times) {
	
	int delta = abs(current_pos - dest_pos);
	int ramp_up_steps = delta / 2;
	int ramp_down_steps = delta - ramp_up_steps;
	int current = sleep_times->times[sleep_times->current_time];

	int * times = malloc(sizeof(int) * delta);

	times[0] = current;
	for (int i = 1 ; i < ramp_up_steps ; i++) {
		times[i] = MAX(times[i - 1] * ACCELERATION, MIN_SLEEP);
	}

	times[delta - 1] = START_SLEEP;
	for (int i = delta - 2 ; i >= ramp_up_steps ; i--) {
		times[i] = MAX(times[i + 1] * ACCELERATION, MIN_SLEEP);
	}

	for (int i = 0 ; i < delta ; i++) {
		printf("%i\n", times[i]);
	}

	free(sleep_times->times);
	sleep_times->times = times;
	sleep_times->current_time = 0;
	sleep_times->size = delta;
}

int main(void){
	struct command last_command;
	struct timespec ts;
	struct sleep_times sleep_times;

	int current_dir = 0;
	int current_pos = 0;
	int dest_pos = 1000;

	setup();

	ts.tv_sec = 0;
	ts.tv_nsec = 5000000;

	sleep_times.times = malloc(sizeof(int) * 1);
	sleep_times.times[0] = START_SLEEP;
	sleep_times.current_time = 0;
	sleep_times.size = 1;
	update_sleep_times(current_pos, dest_pos, &sleep_times);

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
			update_sleep_times(current_pos, dest_pos, &sleep_times);
		}

		if (current_pos != dest_pos) {
			digitalWrite(AZ_STEP, 1);
			digitalWrite(AZ_STEP, 0);

			if (current_dir == 0) {
				current_pos += 1;
			}
			else {
				current_pos -= 1;
			}
		
			ts.tv_nsec = sleep_times.times[sleep_times.current_time];
			sleep_times.current_time += 1;
		}
		nanosleep(&ts, &ts);
	}

	return 0;
}
