CFLAGS?=-O3 -Wall -std=gnu99
LDLIBS+= -lpthread -lm
CC?=gcc
PROGNAME=qidi_connect
OBJ=qidi_connect.o udp.o qidi_communication.o qidi_message.o showdata.o

all: qidi_connect

qidi_connect: $(OBJ)
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o qidi_connect
