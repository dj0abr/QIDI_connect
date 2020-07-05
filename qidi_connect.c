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
 * https://github.com/Photonsters/anycubic-photon-docs/blob/master/photon-blueprints/ChituClientWifiProtocol-translated.txt
 * 
 */

#include "qidi_connect.h"

void *getkey(void *dummy);
void test();

int keeprunning = 1;
int verbose = 0;

char htmldir[256] = { "." };

// cleans white spaces from beginning, middle and end of a string
char *cleanPfadString(char *str, int cleanspace)
{
static char hs[256];
char *hp = str;
int idx = 0;

    while(*hp == ' ' || *hp == ',' || *hp == '\n' || *hp == '\r' || *hp == '\'' || *hp == '\"') hp++;
    while(*hp) hs[idx++] = *hp++;
    hp = hs+idx-1;
    while(*hp == ' ' || *hp == ',' || *hp == '\n' || *hp == '\r' || *hp == '\'' || *hp == '\"') *hp-- = 0;
    hs[idx] = 0;
    return hs;
}

int searchHTMLpath()
{
int ret=0;
char *sret;

    if(htmldir[0] == '.')
    {
        // search for the apache working directory
        printf("search Apache HTML path\n");
        printf("search /srv for htdocs\n");
        ret = system("find  /srv -xdev -name htdocs  -print > pfad 2>/dev/null");
        printf("search /var for htdocs\n");
        ret = system("find  /var -xdev -name htdocs  -print >> pfad 2>/dev/null");
        printf("search /var for html\n");
        ret = system("find  /var -xdev -name html  -print >> pfad 2>/dev/null");
        printf("search /usr for htdocs\n");
        ret = system("find  /usr -xdev -name htdocs  -print >> pfad 2>/dev/null");
        // if the directory was found its name is in file: "pfad"
        FILE *fp=fopen("./pfad","r");
        if(fp)
        {
            char p[256];
            sret = fgets(p,255,fp);
            if(sret == NULL)
            {
                printf("Path to apache webserver files not found: 3\n");
                exit(0);
            }
            char *cp= cleanPfadString(p,0);
            if(strlen(cp)>3)
            {
                strcpy(htmldir,cp);
                printf("Webserver Path: %s\n",htmldir);
                
                // copy html folder to Apache html path
                char s[300];
                sprintf(s,"cp html/* %s",htmldir);
                int r = system(s);
                if(r == -1)
                    printf("copying of HTML files failed\n");
                
                // create phpdir
                sprintf(s,"mkdir %s/phpdir",htmldir);
                r = system(s);
                if(r == -1)
                    printf("creating phpdir failed\n");
                
                // set phpdir access to 777. This is required for the upload funktion
                // to write files
                sprintf(s,"chmod 777 %s/phpdir",htmldir);
                r = system(s);
                if(r == -1)
                    printf("chmod for phpdir failed\n");
            }
            else
            {
                printf("Path to apache webserver files not found: 2\n");
                exit(0);
            }
            
            fclose(fp);
        }
        else
        {
            printf("Path to apache webserver files not found: 1\n");
            exit(0);
        }
    }
    return ret;
}


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
    showperc(-1,0,1);
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
			case 'V': 	printf("QIDI Linux Remote Control, V0.5, by Kurt Moraw, DJ0ABR\n");
						exit(0);
			default:	break;	
		}
	}
    
    setvbuf(stdin, 0, _IONBF, 0);   // nonblocking key press read

    searchHTMLpath();// search Apache HTML folder
    init_udp();     // init communication UDP sockets

    // start keyboard thread
    pthread_t keyThread;
	pthread_create(&keyThread, NULL, getkey, NULL);
    
    // start local UDP connection to php
    startUDP();
    
    // init perecntage file
    showperc(-1,0,1);
    
    while(keeprunning)
    {
        int r = searchQidiIP(); // search for a QIDI 3d printer
        if(r) break;
    }
    
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
            {
                printf("stopping program ... \n");
                keeprunning = 0;
            }
        }
    }
    
    pthread_exit(NULL); // self terminate this thread
}
