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
 * qconnect.c ... main entry point
 * 
 * using information from:
 * https://github.com/3daddict/QIDI-X-ONE
 * https://github.com/yandreev3/3dWiFiSend
 * 
 */

#include "qidi_connect.h"

void *getkey(void *dummy);

int keeprunning = 1;

int main()
{
    setvbuf(stdin, 0, _IONBF, 0);   // nonblocking key press read
    
    init_udp();     // init communication UDP sockets
    searchQidiIP(); // search for a QIDI 3d printer
    
    // start keyboard thread
    pthread_t keyThread;
	pthread_create(&keyThread, NULL, getkey, NULL);
    
    while(keeprunning)
    {
        qidi_loop();    // does the job
        usleep(1000);   // 1ms loop
    }
    
    closeUDP();
    keeprunning = 0;
    sleep(1);
}

void *getkey(void *dummy)
{
    pthread_detach(pthread_self());

   	while (keeprunning)
	{
        char c=getc(stdin);
        if(c >='a' && c <= 'z')
        {
            if(c == 'x')
                keeprunning = 0;
            
            command = c;
        }
    }
    
    pthread_exit(NULL); // self terminate this thread
}
