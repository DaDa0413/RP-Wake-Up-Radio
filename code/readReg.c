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

int main(int argc, char* argv[]) {
   int fd, gpio, i;
   char *ap;
   unsigned char rfid[IDSIZE];

   // ****************************************************
   // Read config frome "/home/pi/myConfig" and argv
   // ****************************************************
   char config[2][30];
   if (argc != 2)
   {
      printf("readReg regNum\n");
      exit(1);
   } 
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
   spibuffer[0] = strtoul(argv[1], NULL, 0) & 0x7f; // Address + read cmd bit
	if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0) exit(2);
   printf("%02X %02X\n", spibuffer[0], spibuffer[1]);

   close(fd);
   exit(EXIT_SUCCESS);
}
