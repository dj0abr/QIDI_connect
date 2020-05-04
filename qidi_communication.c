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
                    
void writeLine(char *line, int ln);

char command = 0;
int writestatus = 0;

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
            
            printf("the 3d printer has the IP: <%s>\n",qidi_IP);
            
            // connect to the printer (and disconnect any previous connection)
            loopstat = 4001;
            
            return 1;
        }
        
        sleep(1);
        
        if(++cnt >= 10)
        {
            printf("no QIDI 3D printer found within 10 seconds ... giving up\n");
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
#define CMD_TIMEOUT 1000    // ms

void qidi_loop()
{
char *s;
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
                            loopstat = 4000;
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
                        loopstat = 4000;
                        timeout = 0;
                    }
                    break;
    }
    
    // check for a user command
    if(command == 'f') // display files on SD card
        loopstat = 20;
    
    if(command == 'u') // uplod a gcode file
        loopstat = 28;
    
    command = 0;
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

int writefile()
{
char str[100];
char *s;
int timeout = 0;
static int chunknum = 0;

    switch (writestatus)
    {
        case 0: sprintf(str,"M28 %s","testfile.gcode");   // TODO: enter file name
                printf("send file: %s\n",str);
                sendToQidi(str);
                writestatus = 1;
                chunknum = 0;
                break;
        case 1: // wait for "ok" confirmation to M28
                s = readRXbuffer();
                if(s)
                {
                    if(strstr(s,"ok"))
                    {
                        // confirmed, begin sending the file
                        writestatus = 2;
                        break;
                    }
                }
                if(++timeout > CMD_TIMEOUT)
                {
                    // qidi does not respond, give up writing file
                    printf("upload ERROR 1\n");
                    return 1;
                }
                break;
                
        case 2:  // send gcode data
                writeLine("01234567890123456789",chunknum++);
                //writestatus = 2;
                // if last data, change to FINISHED
                writestatus = 3;
                break;
                
        case 3: // wait for confirmation for data chunk
                s = readRXbuffer();
                if(s)
                {
                    if(strstr(s,"ok"))
                    {
                        // confirmed, continue sending the file
                        writestatus = 4; //2;
                        sleep(1);
                        break;
                    }
                }
                if(++timeout > CMD_TIMEOUT)
                {
                    // qidi does not respond, give up writing file
                    printf("upload ERROR 1\n");
                    return 1;
                }
                break;
                
        case 4: sendToQidi("M29");
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
                        return 1;
                        break;
                    }
                }
                if(++timeout > CMD_TIMEOUT)
                {
                    // qidi does not respond, give up writing file
                    printf("upload ERROR 2\n");
                    return 1;
                }
                break;
    }
    return 0;
}

// sends one line (chunk) of a gcode file to the 3d printer
// max line legth: 256 Bytes
void writeLine(char *line, int ln)
{
unsigned char s[256+6+1];
int len = strlen(line);

    if(len > 256)
    {
        printf("line too long\n");
        return;
    }
    
    printf("write line: %s\n",line);
    
    /*
     * concat at the end of the line: 4 Bytes...length, 1 byte chksum, 1 byte = 0x83
     */
    
    // length
    memcpy(s,line,len);
    s[len+3] = ln & 0xff;
    s[len+2] = ln >> 8;
    s[len+1] = ln >> 16;
    s[len+0] = ln >> 24;
    
    // calc chksum
    unsigned char chksum = 0;
    for(int i=0; i<(len+4); i++)
        chksum ^= s[i];
    
    s[len+4] = chksum;
    s[len+5] = 0x83;
        
    sendToQidi_binary(s, len+6);
}
