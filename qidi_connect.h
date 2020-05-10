/*
 central header file for qconnect
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <wchar.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/file.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <math.h>
#include <ifaddrs.h>
#include <sys/stat.h>

#define RXBUFLEN 1000

int init_udp();
void sendToQidi_binary(unsigned char *buf, int len);
void sendToQidi(char *buf);
void getBroadcastIP();
void sendBroadcast(char *buf, int len);
int searchQidiIP();
void init_qidi();
void writeRXbuffer(char *s);
char *readRXbuffer();
void closeUDP();
void qidi_loop();
int decodeM4001(char *s);
int decodeM4000(char *s);
void show_data();
int decodeM20(char *s);
void startUDP();
void showperc(int perc, int offset, int filesize);

extern char broadcastIP[20];
extern int keeprunning;
extern char qidi_IP[20];
extern char lastTXmessage[100];
extern int bedtemp;
extern int bedtargettemp;
extern int head1temp;
extern int head1targettemp;
extern int head2temp;
extern int head2targettemp;
extern double posX;
extern double posY;
extern double posZ;
extern int fan1rpm;
extern int fan2rpm;
extern int loopstat;
extern int machinetype;
extern int bedsizeX;
extern int bedsizeY;
extern int machinesizeZ;
extern int nozzlenumber;
extern int hotbedenabled;
extern int txconfirmed;
extern char uploadfilename[256];
extern int verbose;
extern int printstat;
extern int printprogress;
extern char htmldir[256];

