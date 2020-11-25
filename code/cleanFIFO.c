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
   int fd;

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
 
   if (rfm69cleanFIFO()) {
   	fprintf(stderr, "Failed to clean FIFO\n");
	   exit(EXIT_FAILURE);
   }
   close(fd);
   exit(EXIT_SUCCESS);
}
