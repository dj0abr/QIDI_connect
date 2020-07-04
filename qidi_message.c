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
 * qidi_message.c
 * -----
 * handles messages received from the 3d printer
 */

/*
 * read machine specific parameters, also "connects" to the qidi 3d printer
 * ========================================================================
 * request: M4001
 * answer:
 *      ok X:0.010611 Y:0.010611 Z:0.002500 E:0.010700 T:0/303/255/300/1 U:'GBK' B:1
 *      T: machine type (0=cartesian, 1=delta)) / bed size X in mm / bed size Y in mm / size Z in mm / number of nozzles
 *      B: 1=hot bed enabled
 * 
 * read the actual status from the printer, must be done every 2s
 * ==============================================================
 * request: M4000
 * answer:
        ok. B:-50/0 E1:-52 / 0 E2: 76/0 X:0.000 Y:0.000 Z:0.000 F:0/0 D:0/0/0 T:0
        
        B: hot bed current temperature / target temperature
        E1: thermal Head 1 current temperature / target temperature
        E2: thermal Head 2 current temperature / target temperature
        X,Y,Z:coordinate position, in mm
        F: Extrusion Head 1 fan PWM / extrusion Head 2 fan the maximum value is 256
        D: current read position in gcode file / total gcode file size / pause
        T: actual printing time
 *
 * Show files on SD card
 * =====================
 * request: M20
 * answer:
        Message: Begin file list
        Message: File-0 Name and Size
        Message: File-n Name and Size
        End file list
 *
 * Upload a gcode file (uncompressed)
 * ==================================
 * request: M28 filename
 * answer: ok N:0
 * send: file (or part of file)
 * answer: ok
 * if all bytes sent, save file: request: M29
 * answer: Done saving file \r\n// filename
 *
 * Start Printing a gcode file
 * ===========================
 * request: M6020 'filename' (quot-mark = 0x27)
 * answer: ok N:0
 *
 * Filename of gcode file which is currently printing
 * ==================================================
 * request: M4006
 * answer: ok 'filename'
 *
 */


#include "qidi_connect.h"

#define TEMPARRLEN  500

long getElement_int(char *s, char *elem, int elemnum);
double getElement_float(char *s, char *elem, int elemnum);
void insert_bedlist(int temp, int targettemp);
void insert_nozzlelist(int temp, int targettemp);

int bedtemp = 0;
int bedtargettemp = 0;
int head1temp = 0;
int head1targettemp = 0;
int head2temp = 0;
int head2targettemp = 0;
double posX = 0;
double posY = 0;
double posZ = 0;
int fan1rpm = 0;
int fan2rpm = 0;
int printstat = 0;
int printprogress = 0;
int machinetype = 0;
int bedsizeX = 0;
int bedsizeY = 0;
int machinesizeZ = 0;
int nozzlenumber = 0;
int hotbedenabled = 0;
int noztemparr[TEMPARRLEN];
int bedtemparr[TEMPARRLEN];
unsigned long totalPrinttime = 0;

int SDfileUpdate = 0;
int delete_finished = 0;

int decodeM4000(char *s)
{
    if(s[0] == 'e')
    {
        printf("Qidi printer reports an error: %s\n",s);
        return 0;
    }
    
    bedtemp = getElement_int(s,"B:",0);
    if(bedtemp == -9999) return 0;
    
    bedtargettemp = getElement_int(s,"B:",1);
    if(bedtargettemp == -9999) return 0;
    
    insert_bedlist(bedtemp,bedtargettemp);
    
    head1temp = getElement_int(s,"E1:",0);
    if(head1temp == -9999) return 0;
    
    head1targettemp = getElement_int(s,"E1:",1);
    if(head1targettemp == -9999) return 0;
    
    insert_nozzlelist(head1temp,head1targettemp);
    
    head2temp = getElement_int(s,"E2:",0);
    if(head2temp == -9999) return 0;
    
    head2targettemp = getElement_int(s,"E2:",1);
    if(head2targettemp == -9999) return 0;
    
    posX = getElement_float(s,"X:",0);
    if(posX == -9999) return 0;
    
    posY = getElement_float(s,"Y:",0);
    if(posY == -9999) return 0;
    
    posZ = getElement_float(s,"Z:",0);
    if(posZ == -9999) return 0;
    
    fan1rpm = getElement_int(s,"F:",0);
    if(fan1rpm == -9999) return 0;
    fan1rpm = (fan1rpm*100)/255;
    
    fan2rpm = getElement_int(s,"F:",1);
    if(fan2rpm == -9999) return 0;
    fan2rpm = (fan2rpm*100)/255;
        
    printstat = getElement_int(s,"T:",0);
    if(printstat == -9999) return 0;
        
    long gcodefile_done = getElement_int(s,"D:",0);   // current read pos in gcode file
    if(gcodefile_done == -9999) return 0;
        
    long gcodefile_total = getElement_int(s,"D:",1);  // total gcode file size
    if(gcodefile_total == -9999) return 0;
    
    if(gcodefile_total != 0)
        printprogress = (int)(((double)gcodefile_done * 10000.0)/(double)gcodefile_total); // resolution 0,01%
    else
        printprogress = 0;
    
    show_data();
    writeGUI();
    
    return 1;
}

int decodeM4001(char *s)
{
    if(s[0] == 'e')
    {
        printf("Qidi printer reports an error: %s\n",s);
        return 0;
    }
    
    machinetype = getElement_int(s,"T:",0);
    if(machinetype == -9999) return 0;
    
    bedsizeX = getElement_int(s,"T:",1);
    if(bedsizeX == -9999) return 0;
    
    bedsizeY = getElement_int(s,"T:",2);
    if(bedsizeY == -9999) return 0;
    
    machinesizeZ = getElement_int(s,"T:",3);
    if(machinesizeZ == -9999) return 0;
    
    nozzlenumber = getElement_int(s,"T:",4);
    if(nozzlenumber == -9999) return 0;
    
    hotbedenabled = getElement_int(s,"B:",0);
    if(hotbedenabled == -9999) return 0;
    
    return 1;
}

int decodeM4006(char *s)
{
    printf("Decode M4006\n");
    if(s[0] == 'e')
    {
        printf("Qidi printer reports an error: %s\n",s);
        return 0;
    }
    
    char *hp = strchr(s,'\'');
    if(hp)
    {
        hp++;
        char *he = strchr(hp,'\'');
        if(he)
        {
            *he = 0;
            //printf("Printing: <%s>\n",hp);
            // read printing time if available
            FILE *fr = fopen("printting_times.dat","r");
            if(fr)
            {
                while(1)
                {
                    char rs[512];
                    char *res = fgets(rs, 511, fr);
                    if(res)
                    {
                        char *hp;
                        hp = strchr(res,':');
                        if(hp)
                        {
                            *hp = 0;
                            hp++;
                            char *he = strchr(hp,'\n');
                            if(he) *he=0;
                            unsigned long len = atol(hp);
                            if(!strstr(res,hp))
                            {
                                // add 5 minutes (=300s) for warm up
                                len += 300;
                                
                                printf("printing %s with duration %ld s\n",res,len);
                                totalPrinttime = len;
                                break;
                            }
                        }
                    }
                    else
                    {
                        printf("object not found in timing list\n");
                        break;
                    }
                }
                fclose(fr);
            }
        }
        else
            printf("cannot open printting_times.dat\n");
    }

    return 1;
}

#define MAXFILES 250
#define FILENAMELEN 300

char SDfiles[MAXFILES][FILENAMELEN];
int SDidx = 0;
       
int decodeM20(char *s)
{
    if(strstr(s,"Begin file list")) 
    {
        SDidx=0;
        return 1;
    }
    
    if(strstr(s,"End file list")) 
    {
        printf("=== files on SD card ===\n");
        for(int i=0; i<SDidx; i++)
            printf(" ===> %s\n",SDfiles[i]);
        printf("========================\n");
        
        // and write into file
        SDfileUpdate++;
        writeGUI();
        
        printf("Directory List completed\n");
        
        return 2;
    }
    
    // check for valid file name, it must have one SPC
    // and remove CRLF
    int spc=0;
    for(int i=0; i<strlen(s); i++)
    {
        if(s[i] == ' ') spc++;
        if(s[i] == '\n' || s[i] == '\r') s[i] = 0;
    }
    
    if(spc == 1)
    {
        if(s[0] != '.' && SDidx < MAXFILES) // ignore trash entry
            strcpy(SDfiles[SDidx++],s);
    }
    else
        printf("invalid: %s",s);
    return 1;
}

// remove SPCs from the string
char *cleanString(char *sact)
{
static char s[RXBUFLEN];

    //printf("original string:<%s>\n",sact);
    
    // make a copy of the original string, so we do not corrupt the original
    int dst=0;
    for(int src=0; src<strlen(sact); src++)
    {
        if(sact[src] != ' ' && sact[src] != '\n' && sact[src] != '\r')
        {
            s[dst++] = sact[src];
        }
    }
    s[dst] = 0;
    
    //printf("cleaned string:<%s>\n",s);
    return s;
}

// extract substring from a string
// ok. B:-50/0 E1:-52 / 0 E2: 76/0 X:0.000 Y:0.000 Z:0.000 F:0/0 D:0/0/0 T:0
char *getElement_string(char *sact, char *elem, int elemnum)
{
static char *sres;

    char *s = cleanString(sact);
    
    // search string for elem
    char *hps = strstr(s,elem);
    if(hps == NULL) return NULL;
    // hps is now at the elem started
    // go to the start of the parameter, which is 1 char after the ':'
    hps = strchr(hps,':');
    if(hps == NULL) return NULL;
    hps++;
    if(*hps == 0) return NULL;
    // hps is now at the start of the parameter
    
    // search the end of the parameter, which is one char before the next letter or end of string
    char *hpe = hps;
    while(1)
    {
        if((*hpe >= 'A' && *hpe <= 'Z') || *hpe == 0)
        {
            // end found
            *hpe = 0;
            // hpe is now on the last char of the parameter
            break;
        }
        
        hpe++;
    }
    
    // hps is the start of the parameters, delimited by '/'
    sres = strtok(hps,"/");
    if(sres == NULL) return NULL;
    if(elemnum == 0) return sres;
    int e=1;
    while(sres != NULL) 
    {
        sres = strtok(NULL, "/");
        if(sres == NULL) return NULL;
        if(e == elemnum) return sres;
        e++;
    }
    return NULL;
}

long getElement_int(char *s, char *elem, int elemnum)
{
    char *sres = getElement_string(s,elem,elemnum);
    //printf("search %d of <%s> in <%s>, result <%s>\n",elemnum, elem,s,sres);
    if(sres == NULL) return -9999;
    return atol(sres);
}

double getElement_float(char *s, char *elem, int elemnum)
{
    char *sres = getElement_string(s,elem,elemnum);
    //printf("search %d of <%s> in <%s>, result <%s>\n",elemnum, elem,s,sres);
    if(sres == NULL) return -9999;
    return atof(sres);
}

// upload status variables
int perc;
int offset;
int filesize;

// write upload status into a file for the webpage
void showperc(int p, int o, int f)
{
    perc = p;
    offset = o;
    filesize = f;
    
    writeGUI();
}

/* ==== Communication C-Program -> GUI =====
* this C-program write the information into a file  html/phpdir/cgui.dat 
* the Webpage reads this file with Ajax
*/

char *formatTime(int seconds, int mode)
{
static char s[100];

    if(mode == 2)
    {
        // calculate expected end time, add seconds to actual time
        time_t timeNow;
        time(&timeNow);
        if(seconds > 0)
            timeNow += seconds;
        struct tm* time_info = localtime(&timeNow);
        strftime(s, sizeof(s), "%H:%M", time_info);
        return s;
    }

    int hour = seconds / 3600;
    int h_seconds = seconds - hour*3600;
    int minute = h_seconds/60;
    int second = h_seconds - minute*60;
    
    if(hour < 0 || minute < 0 || second < 0)
    {
        sprintf(s,"---");
        return s;
    }
    
    if(mode == 0)
        sprintf(s,"%02d:%02d:%02d",hour,minute,second);
    else
        sprintf(s,"%02d:%02d",hour,minute);
    return s;
}

void insert_bedlist(int temp, int targettemp)
{
static long lastseconds = 0;

    // get seconds
    long seconds = time(NULL);
    
    if(lastseconds == 0)
    {
        // we just started the program, enter the new value at the end of the array
        for(int i=0; i<TEMPARRLEN; i++)
            bedtemparr[i] = 0;
        
        bedtemparr[TEMPARRLEN-1] = temp;
    }
    else
    {
        // how many seconds do we have to enter into the list
        int diffs = (int)(seconds - lastseconds);
        if(diffs > 0)
        {
            // shift the array
            memmove(bedtemparr, &(bedtemparr[diffs]),sizeof(int)*(TEMPARRLEN-diffs));
            // insert the new value
            //printf("-----------> insert %d for %d times\n",temp,diffs);
            for(int i=0; i<diffs; i++)
                bedtemparr[TEMPARRLEN-1-i] = temp;
        }
    }
    
    lastseconds = seconds;
}

/*
 * insert new temperature into array.
 * max. length TEMPARRLEN (500) with 1 seconds for each value
 * so the list length is 500s.
*/
void insert_nozzlelist(int temp, int targettemp)
{
static long lastseconds = 0;

    // get seconds
    long seconds = time(NULL);
    
    if(lastseconds == 0)
    {
        // we just started the program, enter the new value at the end of the array
        for(int i=0; i<TEMPARRLEN; i++)
            noztemparr[i] = 0;
        
        noztemparr[TEMPARRLEN-1] = temp;
    }
    else
    {
        // how many seconds do we have to enter into the list
        int diffs = (int)(seconds - lastseconds);
        if(diffs > 0)
        {
            // shift the array
            memmove(noztemparr, &(noztemparr[diffs]),sizeof(int)*(TEMPARRLEN-diffs));
            // insert the new value
            //printf("-----------> insert %d for %d times\n",temp,diffs);
            for(int i=0; i<diffs; i++)
                noztemparr[TEMPARRLEN-1-i] = temp;
        }
    }
    
    lastseconds = seconds;
}
    
void writeGUI()
{
FILE *fw;
char s[1000];

    snprintf(s,999,"%s/phpdir/cgui.dat",htmldir);
    s[999]=0;
    printf("write to %s\n",s);

    fw = fopen(s,"w");
    if(fw)
    {
        // printer type information from command M4001
        // line 0-5
        fprintf(fw,"%s\n",machinetype?"delta":"cartesian");
        fprintf(fw,"%d\n",bedsizeX);
        fprintf(fw,"%d\n",bedsizeY);
        fprintf(fw,"%d\n",machinesizeZ);
        fprintf(fw,"%d\n",nozzlenumber);
        fprintf(fw,"%s\n",hotbedenabled?"yes":"no");
        
        
        // printer status information from command M4000
        // lines 6-18
        fprintf(fw,"%d\n",bedtemp);
        fprintf(fw,"%d\n",bedtargettemp);
        fprintf(fw,"%d\n",head1temp);
        fprintf(fw,"%d\n",head1targettemp);
        fprintf(fw,"%d\n",head2temp);
        fprintf(fw,"%d\n",head2targettemp);
        fprintf(fw,"%.3f\n",posX);
        fprintf(fw,"%.3f\n",posY);
        fprintf(fw,"%.3f\n",posZ);
        fprintf(fw,"%d\n",fan1rpm);
        fprintf(fw,"%d\n",fan2rpm);
        fprintf(fw,"%s\n",formatTime(printstat,0));
        fprintf(fw,"%.2f\n",(double)printprogress/100);
        
       
        // remaining_time = (printstat[seconds printing time] - 180s[average warm up time])*100 / printprogress
        // lines 19..21
        if(printstat <= 300 || (printprogress/100) < 5)
        {
            fprintf(fw,"wait for 5%%\n");
            fprintf(fw,"...\n");
            fprintf(fw,"...\n");
        }
        else
        {
            // expected print time
            if(totalPrinttime > 0)
            {
                // use precise calculation
                printf("totalPrinttime:%ld\n",totalPrinttime);
                fprintf(fw,"%s\n",formatTime(totalPrinttime,1));
                
                int remaining_time = totalPrinttime - printstat;
                fprintf(fw,"%s\n",formatTime(remaining_time,1));
                fprintf(fw,"%s\n",formatTime(remaining_time,2));
            }
            else
            {
                // duration unknown, use only calc depending of file position
                // expected print time
                // give 300s = 5min for warm up
                int total_printtime = ((printstat - 300) * 10000)/printprogress;  
                int remaining_time = total_printtime - printstat;
                fprintf(fw,"%s\n",formatTime(total_printtime,1));
                fprintf(fw,"%s\n",formatTime(remaining_time,1));
                fprintf(fw,"%s\n",formatTime(remaining_time,2));
            }
        }
        
        
        // upload status
        // line 22-24
        fprintf(fw,"%d\n",perc);
        fprintf(fw,"%d\n",offset);
        fprintf(fw,"%d\n",filesize);
        
        // SD file list has been updated
        // line 25 (incrementing number)
        fprintf(fw,"%d\n",SDfileUpdate);
        
        // delete command finished
        // line 26 (incrementing number)
        fprintf(fw,"%d\n",delete_finished);
        
        // line 27: 3d printer 0=offline, 1=found but no response 2=active
        fprintf(fw,"%d\n",printer_online);
        
        // line 28: Array of nozzle temp values, space separated
        for(int i=0; i<TEMPARRLEN; i++)
            fprintf(fw,"%d ",noztemparr[i]);
        fprintf(fw,"\n");
        
        
        // line 29: Array of bed temp values, space separated
        for(int i=0; i<TEMPARRLEN; i++)
            fprintf(fw,"%d ",bedtemparr[i]);
        fprintf(fw,"\n");
        
        
        // SD card file list
        // line 30 until end of file
        for(int i=0; i<SDidx; i++)
            fprintf(fw,"%s\n",SDfiles[i]);
        
        
        fclose(fw);
    }
    else
        printf("cannot open %s for writing\n",s);
    
}
