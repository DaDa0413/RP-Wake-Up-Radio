/* rfrespond
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
#include <time.h>

#ifndef RFM69BIOS_H
#include "rfm69bios.h"
#endif
#define REMOTE_RFID 0x99

void readConfig(char const *fileName, char clist[2][30])
{
    FILE *file = fopen(fileName, "r");
    int c;

    if (file == NULL) {
			fprintf(stderr, "Config file cannot find in /home/pi/myConfig.");
			fprintf(stderr, "Usage: rfrespond opt.[localRFID] [GPIO]\n");
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

int intReg1 = 0;
void myInterrupt0(void) {}
void myInterrupt1(void) { intReg1 = 1; }
void myInterrupt2(void) {}

int main(int argc, char* argv[]) {
	pid_t pid, sid;
	int fdspi, gpio, i, mode, nbr=1, random_coef;
	char *ap;
	unsigned char locrfid[IDSIZE], remrfid[IDSIZE];

	// *** Protect ***
	if(geteuid() != 0) {
		fprintf(stderr, "Hint: call me as root!\n");
		exit(EXIT_FAILURE);
	}

	// *** Config ***
	char config[2][30];
	
	readConfig("/home/pi/myConfig", config); 
	if (argc == 2)
	{
		random_coef = atoi(argv[1]);
	}
	else
	{
		random_coef = 1;
	}
	
	
	// Set locrfid and remoterfid
	ap = config[0];
	for (i = 0, ap = config[0]; i < IDSIZE; i++,ap++) {
		locrfid[i] = strtoul(ap,&ap,16);
	}
	for (i = 0; i < IDSIZE - 1; i++)	// Last byte will be filled in after receiving WuR
	{
		remrfid[i] = 0x11;
	}
	remrfid[IDSIZE - 1] = REMOTE_RFID;
	gpio = atoi(config[1]);
	fprintf(stdout, "[INFO] rfrespond start\n");

	// *** Setup ***
	if (wiringPiSetupSys() < 0) {
		fprintf(stderr, "Failed to setup wiringPi\n");
		exit(EXIT_FAILURE);
	}
	fdspi = wiringPiSPISetup(SPI_DEVICE, SPI_SPEED);
	if (fdspi < 0) {
		fprintf(stderr, "Failed to open SPI device\n");
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "Listening on RF ID: ");
	for (i = 0; i < IDSIZE; i++) {
		if(i != 0) {
			fprintf(stdout,":");
		}
		fprintf(stdout, "%02x", locrfid[i]);
	}
	fprintf(stdout,"\n");
	fflush(stdout);

	// *** Check RFM69 Status ***
	// get mode
	mode = rfm69getState();
	if (mode < 0) {
		fprintf(stderr, "Failed to read RFM69 Status\n");
		exit(EXIT_FAILURE);
	}

	if ((mode & 0x02) == 0x02) {
		fprintf(stdout, "Received WuR while packet was sleeping\n");

		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stderr, "Failed to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		delay(20);

		// read remote RF ID from FIFO
		unsigned char payload[11];
		rfm69rxdata(payload, 11); // skip last byte of called RF ID
		fprintf(stdout, "Received payload:");
		for (int i = 0; i < 11; i++) {
			if(i != 0) fprintf(stdout,":");
			fprintf(stdout, "%02x", payload[i]);
		}
		fprintf(stdout, "\n");
		int same = 1;
		for (int i = 0; i < 11; i++) {
			if (payload[i] != locrfid[IDSIZE - 1])
			{
				same = 0;
				break;
			}
		}
		if (same)
		{
			fprintf(stdout,"ACKed %d. Call from Station: ",nbr++);
			for (i = 0; i < IDSIZE; i++) {
				if(i != 0) {
					fprintf(stdout,":");
				}
				fprintf(stdout, "%02x", remrfid[i]);
			}
			fprintf(stdout,"\n");
			fflush(stdout);

			// *** Send ACK ***
			// prepare for TX
			if (rfm69startTxMode(remrfid)) {
				fprintf(stderr, "Failed to enter TX Mode\n");
				exit(EXIT_FAILURE);
			}
			do
			{
				mode = rfm69getState();
				if (mode < 0)
				{
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
			} while ((mode & 0x2000) == 0);
			// write Tx data
			payload[0] = REMOTE_RFID;
			for (int j = 1; j < 11; j++)
				payload[j] = locrfid[IDSIZE - 1];
			srand(time(NULL) + locrfid[IDSIZE - 1]);
			for (int k = 0; k < 5; k++)
			{
				struct timeval delayTime;
				int temp = 1000 * (rand() % 1000) * random_coef;
				delayTime.tv_sec = temp / 1000000;
				delayTime.tv_usec = temp % 1000000; // 10 ms ~ 1s
				select(0, NULL,NULL, NULL, &delayTime);
				
				rfm69txdata(payload, 11); // write complete local RF ID
				do {
					usleep(1000);
					mode = rfm69getState();
					if (mode < 0) {
						fprintf(stderr, "Failed to read RFM69 Status\n");
						exit(EXIT_FAILURE);
					}
				} while ((mode & 0x08) == 0);
				// Clear RegIrqFlags2 
				unsigned char spibuffer[2];
				spibuffer[0] = 0x28 | 0x80;  // address + write command
				spibuffer[1] = 0x00;
				if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0)
				{
					fprintf(stderr, "Fail to clear RegIrqFlags\n");
					exit(1);
				}
			}
		}
	}

	while(1) {
		// switch back to STDBY Mode
		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stderr, "Failed to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		// *** Reception ***
		// prepare for RX
		if (rfm69startRxMode(locrfid)) {
			fprintf(stderr, "Failed to enter RX Mode\n");
			exit(EXIT_FAILURE);
		}
		// Check for CRC_Ok state
		do {
			rfm69restartRx();
			usleep(86000);
			mode = rfm69getState();
			if (mode < 0) {
				fprintf(stderr, "Failed to read RFM69 Status\n");
				exit(EXIT_FAILURE);
			}
		} while ((mode & 0x02) == 0);
		// switch back to STDBY Mode
		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stderr, "Failed to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		usleep(20);

		// read remote RF ID from FIFO
		unsigned char payload[11];
		rfm69rxdata(payload, 11); // skip last byte of called RF ID
		fprintf(stdout, "Received payload:");
		for (int i = 0; i < 11; i++) {
			if(i != 0) fprintf(stdout,":");
			fprintf(stdout, "%02x", payload[i]);
		}
		fprintf(stdout, "\n");
		int same = 1;
		for (int i = 0; i < 11; i++) {
			if (payload[i] != locrfid[IDSIZE - 1])
			{
				same = 0;
				break;
			}
		}
		if (!same)
			continue;
		fprintf(stdout,"ACKed %d. Call from Station: ",nbr++);
		for (i = 0; i < IDSIZE; i++) {
			if(i != 0) {
				fprintf(stdout,":");
			}
			fprintf(stdout, "%02x", remrfid[i]);
		}
		fprintf(stdout,"\n");
		fflush(stdout);
		
		usleep(1000000);
		// *** Send ACK ***
		// prepare for TX
		if (rfm69startTxMode(remrfid)) {
			fprintf(stderr, "Failed to enter TX Mode\n");
			exit(EXIT_FAILURE);
		}
		do
		{
			mode = rfm69getState();
			if (mode < 0)
			{
				fprintf(stderr, "Failed to read RFM69 Status\n");
				exit(EXIT_FAILURE);
			}
		} while ((mode & 0x2000) == 0);
		// write Tx data
		payload[0] = REMOTE_RFID;
		for (int j = 1; j < 11; j++)
			payload[j] = locrfid[IDSIZE - 1];
		srand(time(NULL) + locrfid[IDSIZE - 1]);
		for (int k = 0; k < 5; k++)
		{
			struct timeval delayTime;
			int temp = 1000 * (rand() % 1000)  * random_coef;
			delayTime.tv_sec = temp / 1000000;
			delayTime.tv_usec = temp % 1000000; // 10 ms ~ 1s
			select(0, NULL,NULL, NULL, &delayTime);
			
			rfm69txdata(payload, 11); // write complete local RF ID
			do {
				usleep(1000);
				mode = rfm69getState();
				if (mode < 0) {
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
			} while ((mode & 0x08) == 0);
			fprintf(stdout,"ACK sent\n");
			fflush(stdout);
			// Clear RegIrqFlags2 
			unsigned char spibuffer[2];
			spibuffer[0] = 0x28 | 0x80;  // address + write command
			spibuffer[1] = 0x00;
			if (wiringPiSPIDataRW(SPI_DEVICE, spibuffer, 2) < 0)
			{
				fprintf(stderr, "Fail to clear RegIrqFlags\n");
				exit(1);
			}
		}
	}
	// guarantee < 1% air time
	usleep(85);
	close(fdspi);
	fclose(stdout);
	fclose(stderr);
	exit(EXIT_SUCCESS);
}
