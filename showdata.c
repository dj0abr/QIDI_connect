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
 * showdata.c
 * -----
 * displays data received from the 3d printer
 */

#include "qidi_connect.h"

void show_data()
{
    printf("\n3D Printer Information\n");
    printf("----------------------\n");
    printf("machine type .... %s\n",machinetype?"delta":"cartesian");
    printf("bed size X ...... %d mm\n",bedsizeX);
    printf("bed size Y ...... %d mm\n",bedsizeY);
    printf("size Z .......... %d mm\n",machinesizeZ);
    printf("number of nozzles %d\n",nozzlenumber);
    printf("hot bed enable... %s\n",hotbedenabled?"yes":"no");

    printf("\n3D Printer Status\n");
    printf("-----------------\n");
    printf("bedtemp..........% 4d (setpoint % 4d)\n",bedtemp,bedtargettemp);
    printf("head1temp........% 4d (setpoint % 4d)\n",head1temp,head1targettemp);
    //printf("head2temp.......... %d -> %d\n",head2temp,head2targettemp); // for dual extruder machines only
    printf("posX............. %7.3f mm\n",posX);
    printf("posY............. %7.3f mm\n",posY);
    printf("posZ............. %7.3f mm\n",posZ);
    printf("fan1rpm.......... %d %%\n",fan1rpm);
    //printf("fan2rpm.......... %d %%\n",fan2rpm);
    if(printstat)
    {
        printf("Print Time....... %02d:%02d min\n",printstat/60,printstat - (printstat/60)*60);
    }
    else
        printf("Print Time....... not printing\n");
    printf("Progress......... %d %%\n",printprogress);
    
    // write data into a file in the Apache HTML directory
    char s[1000];
    sprintf(s,"%s/qidi.dat",htmldir);
    FILE *fw = fopen(s,"w");
    if(fw)
    {
        fprintf(fw,"%s\n",machinetype?"delta":"cartesian");
        fprintf(fw,"%d\n",bedsizeX);
        fprintf(fw,"%d\n",bedsizeY);
        fprintf(fw,"%d\n",machinesizeZ);
        fprintf(fw,"%d\n",nozzlenumber);
        fprintf(fw,"%s\n",hotbedenabled?"yes":"no");
        
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
        fprintf(fw,"%02d:%02d\n",printstat/60,printstat - (printstat/60)*60);
        fprintf(fw,"%d\n",printprogress);
        
        fclose(fw);
    }
    else
    {
        printf("ERROR: cannot write to HTML folder. This program must run as root\n");
    }
}
