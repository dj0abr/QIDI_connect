/*
 * Remote software
 * for QIDI 3d printers
 * 
 * System Requirements:
 * Linux
 * runs on small SBC like Raspberry PI or Odroid or others
 * 
 * (c) Kurt Moraw, DJ0ABR
 * License: GPL V3
 * 
 * udp.c
 * -----
 * QIDI 3D printers communicate via UDP
 */

#include "qidi_connect.h"

#define QIDI_PORT 3000

void *UDPrx(void *dummy);

int txmsgnum = 0;
int txconfirmed = 1;

struct sockaddr_in from_sock;
int UDP_sock = -1;
char broadcastIP[20];
char qidi_IP[20] = "000.000.000.000";

/*
 * open UDP socket for outgoing and incoming messages
 */
int init_udp()
{
struct sockaddr_in sin;

	UDP_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (UDP_sock == -1){
		printf("Cannot create socket, errno=%d\n", errno);
		return 1;
	}
	
	int broadcastPermission = 1;
    if (setsockopt(UDP_sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0){
        printf("setsockopt error\n");
        close(UDP_sock);
        return 1;
    }
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons(0);
	sin.sin_addr.s_addr = INADDR_ANY;
	if (bind(UDP_sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) != 0)
	{
        printf("Cannot bind socket, errno=%d\n", errno);
		close(UDP_sock);
        return 1;
	}
	
	printf("socket bound\n");
	
	getBroadcastIP();
    
    // start UDP rx thread
    pthread_t UDPrxThread;

	pthread_create(&UDPrxThread, NULL, UDPrx, NULL);

	
	return 0;
}

void closeUDP()
{
    close(UDP_sock);
}

void getBroadcastIP()
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (!ifa->ifa_addr) 
        {
            continue;
        }
        
        // check it is IP4
        if (ifa->ifa_addr->sa_family == AF_INET) 
        { 
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            
            // Filter local IP
            if(!strcmp(ifa->ifa_name,"lo")) continue;
            
            // Filter virtual addresses
            if(ifa->ifa_name[0] == 'v') continue;
            
            // own IP found
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
            strcpy(broadcastIP,addressBuffer);
            // replace last address with 255 to get the broadcast address
            char *hp = strrchr(broadcastIP,'.');
            if(hp)
            {
                *hp=0;
                strcat(broadcastIP,".255");
                printf("broadcast IP: <%s>\n",broadcastIP);
                // finished
                break;
            }
        } 
    }
    
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}

// send a broadcast message to QIDI port
void sendBroadcast(char *buf, int len)
{
struct sockaddr_in sin;

    printf("TX broadcast message no:%d <%s> to IP: %s port: %d\n",txmsgnum,buf,broadcastIP,QIDI_PORT);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(QIDI_PORT);
	sin.sin_addr.s_addr = inet_addr(broadcastIP);
	if(sendto(UDP_sock, buf, len, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)
    {
        printf("broadcast sendto failed\n");
    }
    txmsgnum++;
    txconfirmed = 0;
}

char lastTXmessage[100] = {0};

// send a UDP message to Qidi 3D printer
void sendToQidi_binary(unsigned char *buf, int len)
{
struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_port = htons(QIDI_PORT);
	sin.sin_addr.s_addr = inet_addr(qidi_IP);
	if(sendto(UDP_sock, buf, len, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)
    {
        printf("sendto failed\n");
        return;
    }
    txconfirmed = 0;
}

void sendToQidi(char *buf)
{
struct sockaddr_in sin;
int len = strlen(buf);

    strcpy(lastTXmessage,buf);

    printf("TX UDP message no:%d <%s> to IP: %s port: %d\n",txmsgnum,buf,qidi_IP,QIDI_PORT);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(QIDI_PORT);
	sin.sin_addr.s_addr = inet_addr(qidi_IP);
	if(sendto(UDP_sock, buf, len, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)
    {
        printf("sendto failed\n");
        return;
    }
    txmsgnum++;
    txconfirmed = 0;
}

// this threads waits for messages from the QIDI printer
void *UDPrx(void *dummy)
{
socklen_t fromlen;  
struct sockaddr_in from_sock;
char internalRXbuffer[RXBUFLEN];

    // this thread must terminate itself because
    // the parent does not want to wait
    pthread_detach(pthread_self());

   	while (keeprunning)
	{
		int rxlen = recvfrom(UDP_sock, internalRXbuffer, RXBUFLEN, 0, (struct sockaddr *)&from_sock, &fromlen);
        internalRXbuffer[rxlen] = 0;

        printf("from QIDI: %s",internalRXbuffer);
        internalRXbuffer[rxlen] = 0;
        writeRXbuffer(internalRXbuffer);
    }
    
    pthread_exit(NULL); // self terminate this thread
}

// thread safe fifo for the received data 

pthread_mutex_t ssb_crit_sec;
#define LOCK	pthread_mutex_lock(&(ssb_crit_sec))
#define UNLOCK	pthread_mutex_unlock(&(ssb_crit_sec))

#define FIFOLEN 100
char rxbuf[FIFOLEN][RXBUFLEN];
int wridx = 0, rdidx = 0;

void writeRXbuffer(char *s)
{
	LOCK;
	if (((wridx + 1) % FIFOLEN) == rdidx)
	{
        printf("RX fifo full !\n");
		UNLOCK;
		return;
	}

	strcpy(rxbuf[wridx],s);
	if (++wridx == FIFOLEN) wridx = 0;
	UNLOCK;
}

char *readRXbuffer()
{
static char *s;

	LOCK;
	if (rdidx == wridx)
	{
		// no data available
		UNLOCK;
		return NULL;
	}
	
	s = rxbuf[rdidx];
	if (++rdidx == FIFOLEN) rdidx = 0;
	UNLOCK;

	return s;
}



