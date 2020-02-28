
FLAGS=-lwiringPi -lwiringPiDev -lpthread -lm
PREFIX=/usr/local

all: motor emergency

motor: motor.c consts.h
	gcc -o motor motor.c ${FLAGS}
emergency: emergency.c consts.h
	gcc -o emergency emergency.c ${FLAGS}

install:
	cp emergency ${PREFIX}/bin/.
	cp motor ${PREFIX}/bin/.
	cp emergency-button.service /etc/systemd/system/.
