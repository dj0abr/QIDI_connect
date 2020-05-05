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
int verbose = 0;

// check if qidi_connect is already running
void isRunning()
{
    int num = 0;
    char s[256];
    sprintf(s,"ps -e | grep qidi_connect");
    
    FILE *fp = popen(s,"r");
    if(fp)
    {
        // gets the output of the system command
        while (fgets(s, sizeof(s)-1, fp) != NULL) 
        {
            if(strstr(s,"qidi_connect") && !strstr(s,"grep"))
            {
                if(++num == 2)
                {
                    printf("qidi_connect is already running, do not start twice !");
                    pclose(fp);
                    exit(0);
                }
            }
        }
        pclose(fp);
    }
}

void INThandler(int sig)
{
    printf("\nstopped by Ctrl-C, cleaning system ...");
    closeUDP();
    keeprunning = 0;
    sleep(1);
	exit(0);
}

int main(int argc, char *argv[])
{
int opt;

    // check if it is already running, if yes then exit
    isRunning();
    
    // this handler captures the Ctrl-C key and closes
    // all processes before exiting
    signal(SIGINT, INThandler);
    
    // read command line options
    while((opt = getopt(argc,argv,"Vvu:")) != -1)
	{
		switch(opt)
		{
			case 'u':	strcpy(uploadfilename,optarg);
                        printf("Upload file:%s, press 'u' to upload\n",uploadfilename);
						break;
            case 'v':   verbose = 1;
                        break;
			case 'V': 	printf("QIDI Linux Remote Control, V0.11, by Kurt Moraw, DJ0ABR\n");
						exit(0);
			default:	break;	
		}
	}
    
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
