/* rfwait
Copyright (C) 2020  Ray, Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifndef RFM69BIOS_H
#include "rfm69bios.h"
#endif
void readConfig(char const *fileName, char clist[2][30])
{
    FILE *file = fopen(fileName, "r");
    int c;

    if (file == NULL) {
			fprintf(stderr, "Config file cannot find in /home/pi/myConfig.");
			fprintf(stderr, "Usage: rfwait opt.[localRFID] [GPIO]\n");
			exit(EXIT_FAILURE);
	} //could not open file

    char item[30] = {};
	
	size_t n = 0;
    while ((c = fgetc(file)) != EOF)
    {
		if (c == ' ') {
			item[n] = '\0';
			strcpy(clist[0], item);
			memset(&item[0], 0, sizeof(item));
			n = 0;
		}
		else
			item[n++] = (char) c;
    }

    // terminate with the null character
    item[n] = '\0';
    strcpy(clist[1], item); 
}

int main(int argc, char* argv[]) {
   int fd, gpio, i;
   char *ap;
   unsigned char rfid[IDSIZE];

   // ****************************************************
   // Read config frome "/home/pi/myConfig" and argv
   // ****************************************************
   char config[2][30];
   if (argc != 3) readConfig("/home/pi/myConfig", config); 
	else {
		strcpy(config[0], argv[1]);
		strcpy(config[1], argv[2]);
	}
	
   // ****************************************************
   // Stroe RDID into rfid[] (a character array)
   // ****************************************************
	ap = config[0];
	for (i = 0, ap = config[0]; i < IDSIZE; i++,ap++) {
		rfid[i] = strtoul(ap,&ap,16);
	}
	gpio = atoi(config[1]);
	fprintf(stdout, "RFID:%s, GPIO:%d\n",config[0] , gpio); 

   // ****************************************************
   // Initiate the RPi and SPI
   // ****************************************************
   if (wiringPiSetupSys() < 0) {
      fprintf(stderr, "Failed to setup wiringPi\n");
      exit(EXIT_FAILURE);
   }
   fd = wiringPiSPISetup(SPI_DEVICE, SPI_SPEED);
   if (fd < 0) {
      fprintf(stderr, "Failed to open SPI device\n");
      exit(EXIT_FAILURE);
   }

   unsigned char spibuffer[2];
   spibuffer[0] = 0x01 | 0x80; // Address + write cmd bit
	spibuffer[1] = 0x10; // RX Mode
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0) exit(2);

   close(fd);
   exit(EXIT_SUCCESS);
}
