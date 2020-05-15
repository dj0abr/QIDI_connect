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
 * qidi_communication.c
 * -----
 * QIDI 3D printers specific communication
 */

#include "qidi_connect.h"
                    
void writeLine(char *line, int len, int offset);

int writestatus = 0;
char uploadfilename[256];
char printfile[256] = {0};
char webfile[256] = {0};

/*
 * Find QIDI 3D printer in the network:
 * 
 * send UDP message: M99999 to port 3000 to IP: 192.168.0.255 (or similar)
 * 
 * QIDI response:
 * ok MAC:cc:50:e3:61:06:f9 IP:192.168.0.101 VER:V10.0.12 ID:4d,6f,28,4c,0a,ca,5b,9a NAME:xmax@x-max
 * 
 * this function currently works for ONE printer only
 */

// search for a QIDI 3d printer
int searchQidiIP()
{
char searchstring[] = "M99999";
int cnt = 0;

    while(1) {
        // send broadcast to query for a QIDI 3d printer
        if(txconfirmed == 1)
        {
            sendBroadcast(searchstring, strlen(searchstring));
        }
        
        // read response from QIDI 3d printer with a 1s timeout
        char *s = readRXbuffer();
        if(s != NULL)
        {
            // qidi found, extract data from the rx string
            // get IP address
            char *hp = strstr(s,"IP:");
            if(hp == NULL)
            {
                printf("3d printer found, but IP address not found:%s\n",s);
                return 0;
            }
            hp+=3;
            char *hpe = strchr(hp,' ');
            if(hpe == NULL)
            {
                printf("3d printer found, but invalid response:%s\n",s);
                return 0;
            }
            *hpe=0;
            strncpy(qidi_IP,hp,19);
            qidi_IP[19] = 0;
            
            printf("3D printer IP: <%s>\n",qidi_IP);
            
            // connect to the printer (and disconnect any previous connection)
            loopstat = 4001;
            
            return 1;
        }
        
        sleep(1);
        
        if(++cnt >= 2)
        {
            printf("no QIDI 3D printer found within 2 seconds ... try again\n");
            return 0;
        }
    }
    
    return 0;
}

// does the various jobs
void read_status();
int writefile();

int loopstat;

#define IDLE_TIME   2000
#define CMD_TIMEOUT 5000    // ms

void qidi_loop()
{
char *s;
char str[1000];
int res;
static int timeout = 0;
static int giveup = 0;

// status numbers are the same as the gcode command
// responses from the printer have a status number with additional 0

    switch (loopstat)
    {        
        case 4000:  // idle loop: read status
                    sendToQidi("M4000");
                    loopstat = 40000;
                    timeout = 0;
                    break;
            
        case 40000: // wait for response to M4000
                    s = readRXbuffer();
                    if(s) decodeM4000(s);
                    if(++timeout >= IDLE_TIME)
                        loopstat = 4000;    // back to idle
                    break;
                    
        case 4001:  // read machine parameters
                    sendToQidi("M4001");
                    loopstat = 40010;
                    timeout = 0;
                    break;
                    
        case 40010: // wait for response to M4001
                    s = readRXbuffer();
                    if(s)
                    {
                        res = decodeM4001(s);
                        if(res) 
                        {
                            loopstat = 20; // continue reading files on SD card
                            break;
                        }
                    }
                    if(++timeout >= CMD_TIMEOUT)
                        loopstat = 4001;    // repeat last command
                    break;
                    
        case 20:    // list files on SD card
                    sendToQidi("M20");
                    loopstat = 200;
                    timeout = 0;
                    giveup = 0;
                    break;
                    
        case 200:   // wait for response to M20
                    s = readRXbuffer();
                    if(s)
                    {
                        res = decodeM20(s);
                        if(res == 2) 
                        {
                            loopstat = 4000;
                            break;
                        }
                        if(res ==  1) break;
                    }
                    if(++timeout >= CMD_TIMEOUT)
                    {
                         //loopstat = 20;    // repeat last command (does not work, qidi unresponsive)
                         sendToQidi("M4000"); // wake up qidi in case the file list hangs
                         timeout = 0;
                         if(++giveup >= 5)
                         {
                             // give up with file list after 5 tries
                             printf("!!! list of files inclomplete !!!\n");
                             loopstat = 4000;
                             break;
                         }
                    }
                    break;
                    
        case 28:    // upload gcode file
                    writestatus = 0;
                    loopstat = 280;
                    timeout = 0;
                    break;
                    
        case 280:   if(writefile() == 1)
                    {
                        // finished writing file
                        loopstat = 20;  // re-read file list on SD card
                        timeout = 0;
                        // disable percentage file
                        showperc(-1,0,1);
                    }
                    
                    if(writefile() == 2)
                    {
                        // upload ERROR, file was not fully uploaded to the 3d printer
                        loopstat = 20;  // re-read file list on SD card
                        timeout = 0;
                        // disable percentage file
                        showperc(-2,0,1);
                    }
                    break;
                    
        case 30:     // delete file from SD card
                    sprintf(str,"M30 %s",printfile);
                    sendToQidi(str);
                    loopstat = 300;
                    timeout = 0;
                    break;
                    
        case 300:   // wait for delete confirmation
                    s = readRXbuffer();
                    if(s)
                    {
                        if(strstr(s,"ok N:"))
                        {
                            loopstat = 20;  // re-read file list on SD card
                            delete_finished++;
                            writeGUI();
                            break;
                        }
                    }
                    if(++timeout >= CMD_TIMEOUT)
                        loopstat = 30;    // repeat last command
                    break;
                    
        case 6030:  // start printing a file
                    sprintf(str,"M6030 \'%s\'",printfile);
                    sendToQidi(str);
                    loopstat = 60300;
                    timeout = 0;
                    break;
                    
        case 60300: // wait for response to M6030
                    s = readRXbuffer();
                    if(s)
                    {
                        if(strstr(s,"ok N:"))
                        {
                            loopstat = 4000;
                            break;
                        }
                    }
                    if(++timeout >= CMD_TIMEOUT)
                        loopstat = 6030;    // repeat last command
                    break;
    }
    
    // check for a WEB command, which is a file in the html folder
    
    // check for a filename to be printed, received from php via udp
    if(*webfile)
    {
        char *hp = strchr(webfile,'|');
        if(hp)
        {
            if(*webfile == 'd')
            {
                // delete file
                strcpy(printfile,hp+1);
                printf("DELETE: %s\n",printfile);
                loopstat = 30; // activate for start deleting
            }
            else
            {
                // start 3d printing
                strcpy(printfile,hp+1);
                printf("PRINT: %s\n",printfile);
                loopstat = 6030; // activate for start printing
            }
        }
        else
        {
            // file upload
            // !!! to allow upload of big files !!!
            // modify /etc/php.../php.ini, value: upload_max_filesize
            // and also: post_max_site (a bit more than upload_max_filesize
            // and also: memory_limit (even a bit more)
            // and possible: max_execution_time and: max_input_time if a big file upload timeouts
            strcpy(uploadfilename,webfile);
            printf("UPLOAD: %s\n",uploadfilename);
            loopstat = 28; // activate for uploading
        }
        
        *webfile = 0;
    }
}

/*
* Upload a gcode file (uncompressed)
* ==================================
* request: M28 filename
* answer: ok N:0
* send: file (or part of file)
* answer: ok
* if all bytes sent, save file: request: M29
* answer: Done saving file \r\n// filename
*
*/

#define CHUNKSIZE   1280    // same size as in qidi software
     
int writefile()
{
char str[1000];
char *s;
static int timeout = 0;
struct stat st;
int perc;
static int filesize;
static int offset = 0;
static char fileline[CHUNKSIZE+1];
static FILE *fr;
static int finished = 0;
static int rlen;
static int resend = 0;
static int lastperc = -1;
static int num_resends = 0;

    switch (writestatus)
    {
        case 0: // open the file to upload
                sprintf(str,"%s/phpdir/%s",htmldir,uploadfilename);
                fr = fopen(str,"r");
                if(!fr)
                {
                    printf("cannot open %s\n",uploadfilename);
                    return 2;
                }
                // get total size of the file
                if(stat(str,&st)==0)
                    filesize = st.st_size;
                
                if(filesize < 2)
                {
                    printf("file too small\n");
                    return 2;
                }
                
                sprintf(str,"M28 %s",uploadfilename);   // TODO: enter file name
                printf("send file: %s\n",str);
                sendToQidi(str);
                writestatus = 1;
                offset = 0;
                finished = 0;
                resend = 0;
                timeout = 0;
                break;
        case 1: // wait for "ok" confirmation to M28
                //printf("wait for ok on M28. Timeout:%d\n",timeout);
                s = readRXbuffer();
                if(s)
                {
                    if(strstr(s,"ok"))
                    {
                        if(verbose) printf("OK received\n");
                        // confirmed, begin sending the file
                        writestatus = 2;
                        timeout = 0;
                        break;
                    }
                }
                if(++timeout > CMD_TIMEOUT)
                {
                    // qidi does not respond, give up writing file
                    printf("upload ERROR timeout in 1\n");
                    fclose(fr);
                    return 2;
                }
                break;
                
        case 2:  // send gcode data
                if(resend == 0)
                {
                    rlen = fread(fileline, 1, CHUNKSIZE, fr);
                    num_resends=0;
                }
                else
                    printf("resend offset: %d\n",offset);
                
                writeLine(fileline,rlen,offset);
                //usleep(1); // helps
                if(rlen != CHUNKSIZE) finished = 1;
                else finished = 0;
                
                timeout = 0;
                writestatus = 3;
                
                // show bytes left
                if(filesize > 1)
                {
                    perc = (int)(100-((filesize-offset)*100)/filesize);
                    if(perc != lastperc)
                    {
                        if((perc%10) == 0)
                            printf("sent: %d %% (%d of %d)\n",perc,offset,filesize);
                        showperc(perc,offset,filesize);
                        lastperc = perc;
                    }
                }
                break;
                
        case 3: // wait for confirmation for data chunk
                s = readRXbuffer();
                if(s)
                {
                    timeout = 0;
                    if(strstr(s,"ok"))
                    {
                        offset += rlen;
                        // confirmed, continue sending the file
                        if(finished == 1)
                            writestatus = 4; //2;
                        else
                        {
                            writestatus = 2;
                            resend = 0;
                        }
                        break;
                    }
                    else
                    {
                        printf("transfer error, resend\n");
                        resend = 1; // resend last chunk
                        writestatus = 2;
                        num_resends++;
                        
                        if(++num_resends > 10)
                        {
                            // qidi does not respond, give up writing file
                            printf("upload ERROR 1\n");
                            fclose(fr);
                            return 2;
                        }
                    }
                }
                if(++timeout > CMD_TIMEOUT)
                {
                    printf("no response, resend\n");
                    resend = 1; // resend last chunk
                    writestatus = 2;
                    num_resends++;
                    
                    if(++num_resends > 10)
                    {
                        // qidi does not respond, give up writing file
                        printf("upload ERROR 1\n");
                        fclose(fr);
                        return 2;
                    }
                }
                break;
                
        case 4: // save file to SD card
                sendToQidi("M29");
                timeout = 0;
                writestatus = 5;
                break;
                
        case 5: // wait for "ok" confirmation to M29
                s = readRXbuffer();
                if(s)
                {
                    if(strstr(s,"ok"))
                    {
                        // confirmed, file upload OK
                        printf("upload OK\n");
                        fclose(fr);
                        return 1;
                    }
                    timeout = 0;
                }
                if(++timeout > CMD_TIMEOUT)
                {
                    // qidi does not respond, give up writing file
                    printf("upload ERROR 2\n");
                    fclose(fr);
                    return 2;
                }
                break;
    }
    return 0;
}

// sends one line (chunk) of a gcode file to the 3d printer
// max line legth: CHUNKSIZE Bytes
void writeLine(char *line, int len, int offset)
{
unsigned char s[CHUNKSIZE+6+1];

    if(verbose) printf("write line: %s\n",line);
    
    /*
     * concat at the end of the line: 4 Bytes...length, 1 byte chksum, 1 byte = 0x83
     */
    
    // length
    memcpy(s,line,len);
    s[len+0] = offset & 0xff;
    s[len+1] = offset >> 8;
    s[len+2] = offset >> 16;
    s[len+3] = offset >> 24;
    
    // calc chksum
    unsigned char chksum = 0;
    for(int i=0; i<(len+4); i++)
        chksum ^= s[i];
    
    s[len+4] = chksum;
    s[len+5] = 0x83;
        
    sendToQidi_binary(s, len+6);
}

// ===== UDP Receiver =====
// process to receive local UDP messages from the webserver (php)

void *getudp(void *dummy);
int pipe_sock;
char udpdata[256];

void startUDP()
{
    pipe_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (pipe_sock == -1)
    {
        printf("Failed to create socket,errno=%d\n", errno);
        return;
    }

    struct sockaddr_in sinc4fm;
    memset(&sinc4fm, 0, sizeof(struct sockaddr_in));
    sinc4fm.sin_family = AF_INET;
    sinc4fm.sin_port = htons(8899);
    sinc4fm.sin_addr.s_addr = INADDR_ANY;	// nur intern

    if (bind(pipe_sock, (struct sockaddr *)&sinc4fm, sizeof(struct sockaddr_in)) != 0)
    {
        printf("Failed to bind PIPE socket, errno=%d\n", errno);
        close(pipe_sock);
        return;
    }
    
    // start the receiver process
    pthread_t udpThread;
	pthread_create(&udpThread, NULL, getudp, NULL);
}

void *getudp(void *dummy)
{
socklen_t fromlen;
    
    pthread_detach(pthread_self());

   	while (keeprunning)
	{
        fromlen = sizeof(struct sockaddr_in);
        struct sockaddr_in frm_dmr;
        int len = recvfrom(pipe_sock, udpdata, 256, 0, (struct sockaddr *)&frm_dmr, &fromlen);
        udpdata[len] = 0;
        //printf("read %d bytes from sock: %d. Message:%s\n", len, 8899,udpdata);
        
        strcpy(webfile,udpdata); // it is a file for upload
    }
    
    pthread_exit(NULL); // self terminate this thread
}
