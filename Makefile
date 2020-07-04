CFLAGS?=-O3 -Wall -std=gnu99
LDLIBS+= -lpthread -lm
CC?=gcc
PROGNAME=qidi_connect
OBJ=qidi_connect.o udp.o qidi_communication.o qidi_message.o showdata.o gettime.o

all: qidi_connect

qidi_connect: $(OBJ)
	cd gcodestat-master && $(MAKE)
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	cd gcodestat-master && $(MAKE) clean && rm -f gcodestat
	rm -f *.o qidi_connect
