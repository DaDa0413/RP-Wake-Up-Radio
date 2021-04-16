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
#include <sys/time.h>
// C++ Lib
#include <sstream>
#include <ctime>
#include <iomanip>
#include <iostream>

#include "rfm69bios.h"
#include "setConfig.h"

// #define REMOTE_RFID 0x06
// #define PAYLOADLENGTH 20
void readConfig(char const *fileName, char clist[2][30]);
int checkReceivedPayload();
void printAckMsg();
void rxAndResetLoop();
void txAndModeReady();
void sendACK();

int fdspi, gpio, mode, nbr = 1, random_coef, ack_times;
unsigned char locrfid[IDSIZE], remrfid[IDSIZE];
time_t lastWuPTime = 0;

int main(int argc, char *argv[])
{
	fprintf(stdout, "[DEBUG] rfrespond start\n");

	// *******
	// Check root
	// ******
	if (geteuid() != 0)
	{
		fprintf(stdout, "Hint: call me as root!\n");
		exit(EXIT_FAILURE);
	}

	// *** Config ***
	char config[2][30];

	readConfig("/home/pi/myConfig", config);
	if (argc == 3)
	{
		random_coef = atoi(argv[1]);
		ack_times = atoi(argv[2]);
		fprintf(stdout, "[Debug] Now Usage: rfrespond random_coef ack_times\n");
	}
	else
	{
		fprintf(stdout, "[Debug] Now Usage: rfrespond random_coef=1 ack_times=5\n");
		random_coef = 1;
		ack_times = 5;
	}

	// Set locrfid and remoterfid
	{
		char *ap;
		int i;
		for (i = 0, ap = config[0]; i < IDSIZE; i++, ap++)
		{
			locrfid[i] = strtoul(ap, &ap, 16);
		}
		for (i = 0; i < IDSIZE - 1; i++) // Last byte will be filled in after receiving WuR
		{
			remrfid[i] = 0x11;
		}

		remrfid[IDSIZE - 1] = REMOTE_RFID;
		gpio = atoi(config[1]);
	}

	// *** Setup ***
	if (wiringPiSetupSys() < 0)
	{
		fprintf(stdout, "[ERROR] Fail to setup wiringPi\n");
		exit(EXIT_FAILURE);
	}
	fdspi = wiringPiSPISetup(SPI_DEVICE, SPI_SPEED);
	if (fdspi < 0)
	{
		fprintf(stdout, "[ERROR] Fail to open SPI device\n");
		exit(EXIT_FAILURE);
	}

	// fprintf(stdout, "Listening with RFID: ");
	// for (int i = 0; i < IDSIZE; i++) {
	// 	if(i != 0) {
	// 		fprintf(stdout,":");
	// 	}
	// 	fprintf(stdout, "%02x", locrfid[i]);
	// }
	// fprintf(stdout,"\n");
	// fflush(stdout);

	// *** Check RFM69 Status ***
	// get mode
	mode = rfm69getState();
	if (mode < 0)
	{
		fprintf(stdout, "[ERROR] Fail to read RFM69 Status\n");
		exit(EXIT_FAILURE);
	}

	if ((mode & 0x02) == 0x02)
	{
		fprintf(stdout, "[DEBUG] Received WuR while RPi was sleeping\n");

		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stdout, "[ERROR] Fail to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		usleep(20);

		// read remote RF ID from FIFO
		if (checkReceivedPayload())
		{
			printAckMsg();
			// *** Send ACK ***
			sendACK();
		}
	}

	while (1)
	{
		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stdout, "[ERROR] Fail to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		usleep(20);

		// Wait for Wake Up Packet
		rxAndResetLoop();

		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stdout, "[ERROR] Fail to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		usleep(20);

		// read remote RF ID from FIFO
		if (checkReceivedPayload())
		{
			printAckMsg();
			// *** Send ACK ***
			sendACK();
		}
	}
	// guarantee < 1% air time
	usleep(85);
	close(fdspi);
	fclose(stdout);
	exit(EXIT_SUCCESS);
}

int checkReceivedPayload()
{
	// read remote RF ID from FIFO
	unsigned char payload[PAYLOADLENGTH];
	memset(payload, 0, PAYLOADLENGTH);
	rfm69rxdata(payload, PAYLOADLENGTH); // skip last byte of called RF ID
	fprintf(stdout, "[INFO] Received payload:");
	for (int j = 0; j < IDLENGTH; j++)
		printf("%02x:", payload[j]);
	for (int j = IDLENGTH; j < PAYLOADLENGTH; j++)
		printf("%c", payload[j]);
	printf(".\n");

	fprintf(stdout, "\n");
	for (int i = 1; i < IDLENGTH; i++)
	{
		if (payload[i] != locrfid[IDSIZE - 1])
			return 0;
	}

	// **************************************
	// Retrieve time string & set system time
	// **************************************
	char temp[PAYLOADLENGTH - IDLENGTH + 1];
	memcpy(temp, &(payload[IDLENGTH]), PAYLOADLENGTH - IDLENGTH);
	temp[PAYLOADLENGTH - IDLENGTH] = '\0';
	std::tm tm = {};
	if (!strptime(temp, "%a %b %d %H:%M:%S %Y", &tm))
	{
		printf("[ERROR] Parse Error\n");
		exit(EXIT_FAILURE);
	}
	time_t t;
	if ((t = mktime(&tm)) == -1)
	{
		printf("[ERROR] Mktime can not translate tm.");
		exit(EXIT_FAILURE);
	}
	if (t > lastWuPTime)
	{
		struct timeval tv = {t, 0};
		if (settimeofday(&tv, (struct timezone *)0) < 0)
		{
			printf("[ERROR] Set system datetime error!\n");
			exit(EXIT_FAILURE);
		}

		FILE *fptr = fopen("/home/pi/wakedRecord.csv", "a");
		fprintf(fptr, "%s\n", temp);
		fclose(fptr);
	}
	return 1;
}

void printAckMsg()
{
	fprintf(stdout, "[INFO] ACKed %d. Call from Station: ", nbr++);
	for (int i = 0; i < IDSIZE; i++)
	{
		if (i != 0)
		{
			fprintf(stdout, ":");
		}
		fprintf(stdout, "%02x", remrfid[i]);
	}
	fprintf(stdout, "\n");
	fflush(stdout);
}

void rxAndResetLoop()
{
	// prepare for RX
	if (rfm69startRxMode(locrfid))
	{
		fprintf(stdout, "[ERROR] Fail to enter RX Mode\n");
		exit(EXIT_FAILURE);
	}
	// Check for CRC_Ok state
	do
	{
		rfm69restartRx();
		usleep(86000);
		mode = rfm69getState();
		if (mode < 0)
		{
			fprintf(stdout, "[ERROR] Fail to read RFM69 Status\n");
			exit(EXIT_FAILURE);
		}
	} while ((mode & 0x02) == 0);
}

void txAndModeReady()
{
	if (rfm69startTxMode(remrfid))
	{
		fprintf(stdout, "[ERROR] Fail to enter TX Mode\n");
		exit(EXIT_FAILURE);
	}
	do
	{
		mode = rfm69getState();
		if (mode < 0)
		{
			fprintf(stdout, "[ERROR] Fail to read RFM69 Status\n");
			exit(EXIT_FAILURE);
		}
	} while ((mode & 0x2000) == 0);
}

void sendACK()
{
	unsigned char payload[PAYLOADLENGTH];
	srand(time(NULL) + locrfid[IDSIZE - 1]);
	for (int k = 0; k < ack_times; k++)
	{
		txAndModeReady();
		// Prepare Tx data
		payload[0] = REMOTE_RFID;
		for (int j = 1; j < PAYLOADLENGTH; j++)
			payload[j] = locrfid[IDSIZE - 1];
		// Random delay
		struct timeval delayTime;
		int temp = 1000 * (rand() % 1000) * random_coef;
		delayTime.tv_sec = temp / 1000000;
		delayTime.tv_usec = temp % 1000000; // 1 ms ~ 1s
		select(0, NULL, NULL, NULL, &delayTime);

		// Write payload to FIFO
		rfm69txdata(payload, PAYLOADLENGTH); // write complete local RF ID
		do
		{
			usleep(1000);
			mode = rfm69getState();
			if (mode < 0)
			{
				fprintf(stdout, "[ERROR] Fail to read RFM69 Status\n");
				exit(EXIT_FAILURE);
			}
		} while ((mode & 0x08) == 0);
		fprintf(stdout, "[Debug] ACK sent\n");
		fflush(stdout);

		// *******
		// Switch back to receive mode, to check whether ACK is received by drone
		// ******
		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stdout, "[ERROR] Fail to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		if (rfm69startRxMode(locrfid))
		{
			fprintf(stdout, "[ERROR] Fail to enter RX Mode\n");
			exit(EXIT_FAILURE);
		}
		// Check for CRC_Ok state
		int count = 10;
		do
		{
			rfm69restartRx();
			usleep(86000);
			mode = rfm69getState();
			if (mode < 0)
			{
				fprintf(stdout, "[ERROR] Fail to read RFM69 Status\n");
				exit(EXIT_FAILURE);
			}
		} while (count-- && (mode & 0x02) == 0);

		if (rfm69STDBYMode(locrfid))
		{
			fprintf(stdout, "[ERROR] Fail to enter STDBY Mode\n");
			exit(EXIT_FAILURE);
		}
		usleep(20);

		// TimeOut, receiver stop sending WuP
		if ((mode & 0x02) == 0)
			break;
	}
}
