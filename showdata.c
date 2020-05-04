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
    printf("bedtemp.......... %d -> %d\n",bedtemp,bedtargettemp);
    printf("head1temp........ %d -> %d\n",head1temp,head1targettemp);
    //printf("head2temp.......... %d -> %d\n",head2temp,head2targettemp); // for dual extruder machines only
    printf("posX............. %.1f\n",posX);
    printf("posY............. %.1f\n",posY);
    printf("posZ............. %.1f\n",posZ);
    printf("fan1rpm.......... %d %%\n",(fan1rpm*100)/255);
    //printf("fan2rpm.......... %d %%\n",(fan2rpm*100)/255);
    
    printf("machine type .... %s\n",machinetype?"cartesian":"delta");
    printf("bed size X ...... %d mm\n",bedsizeX);
    printf("bed size Y ...... %d mm\n",bedsizeY);
    printf("size Z .......... %d mm\n",machinesizeZ);
    printf("number of nozzles %d\n",nozzlenumber);
    printf("hot bed enable... %s\n",hotbedenabled?"yes":"no");
}
