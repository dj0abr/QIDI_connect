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
 * gettime.c
 * ---------
 * calculates and stores the expected printing time
 */

#include "qidi_connect.h"

void store_value(char *fn, unsigned long seconds);

void get_processing_time(char *fileandpathname, char *filename)
{
char parameter[512];
unsigned long seconds = 0;

    // call gcodestat and read print time of this file
    snprintf(parameter,511,"gcodestat-master/gcodestat -g %s",fileandpathname);
    FILE *fp = popen(parameter,"r");
    if(fp)
    {
        while (fgets(parameter, sizeof(parameter)-1, fp) != NULL) 
        {
            printf("gcodestat reports: %s s\n", parameter);
            seconds = atol(parameter);
            store_value(filename,seconds);
        }
        pclose(fp);
    }
    else
        printf("cannot execute gcodestat\n");
}

/*
 * store all printing times in a list for later usage
 * this of course works only if the upload was done with this software
*/
void store_value(char *fn, unsigned long seconds)
{
FILE *fw;

    fw = fopen("printting_times.dat","a");
    if(fw)
    {
        fprintf(fw,"%s:%ld\n",fn,seconds);
        fclose(fw);
    }
    else
    {
        printf("cannot open printting_times.dat\n");
    }
}
