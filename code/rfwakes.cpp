/* rfwakes
 * g++ rfwakes.cpp rfm69bios.o -o rfwakes -lwiringPi
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
#include <signal.h>
#include <iostream>
#include <fstream>
#include <vector>
// TIME HEADER
#include <chrono>
#include <ctime>


#ifndef RFM69BIOS_H
#include "rfm69bios.h"
#endif

#define LogDIR "/home/pi/Desktop/log/"
#define roundDIR "/home/pi/Desktop/log/round"
#define startPATH "/home/pi/Desktop/log/startWakeTime.csv"
struct Target {
	char rem[30];	
	unsigned char remrfid[IDSIZE];
};

void readConfig(char const *fileName, char clist[2][30])
{
	FILE *file = fopen(fileName, "r");
	int c;

	if (file == NULL) {
		fprintf(stderr, "Config file cannot find in /home/pi/myConfig.");
		fprintf(stderr, "Usage: rfwake [calledRFID] opt.[localRFID] [GPIO]\n");
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
	fclose(file);
}

std::vector <Target> readTargets(char* targetFName){
	FILE *file = fopen(targetFName, "r");
	std::vector <Target> tlist;
	char item[30] = {};
	int c;
	size_t n = 0;
	while ((c = fgetc(file)) != EOF)
	{
		if (c == '\n') {
			item[n] = '\0';
			//fprintf(stdout, "%s\n", item);
			Target t;
			strcpy(t.rem, item);
			tlist.push_back(t);
			memset(&item[0], 0, sizeof(item));
			n = 0;
		}
		else
			item[n++] = (char) c;
	}
	fclose(file);
	return tlist;
	//fprintf(stdout, "%s\n", tt);
}

void printrfid(unsigned char rfid[]) {
	for (int i = 0; i < IDSIZE; i++) {
		if(i != 0) fprintf(stdout,":");
		fprintf(stdout, "%02x", rfid[i]);
	}
}

char* toTime(std::chrono::system_clock::time_point target) {
	time_t temp = std::chrono::system_clock::to_time_t(target);
	char* result = ctime(&temp);
	for (char *ptr = result; *ptr != '\0'; ptr++)
	{
		if (*ptr == '\n' || *ptr == '\r')
			*ptr = '\0';
	}
	return result;
}

int intReg = 0;
void myInterrupt0(void) {if (!intReg) intReg = 1;}
int fd;

void intHandler(int signo)
{
	fprintf(stdout, "Receive CTRL+C\n");
	close(fd);
}

int main(int argc, char* argv[]) {
	signal(SIGINT, intHandler);
	std::vector <Target> Targetlist;
	int mode;

	char targetFName[256];
	if (argc == 3)
	{  		
		fprintf(stdout, "Now Usage: rfwakes fName round\n");
		strcpy(targetFName, "/home/pi/targetlist");
	}
	else if (argc == 4)
	{  		
		fprintf(stdout, "Now Usage: rfwakes fName round targetFile\n");
		strcpy(targetFName, "/home/pi/");
		strcat(targetFName, argv[3]);

	}
	else
	{  		
		fprintf(stdout, "[ERROR] Usage: rfwakes fName round [targetFile]\n");
		exit(EXIT_FAILURE);
	}

	// **************
	// *** Config ***
	// **************
	char config[2][30];
	readConfig("/home/pi/myConfig", config); 
	Targetlist = readTargets(targetFName);
	int gpio;
	unsigned char locrfid[IDSIZE], recrfid[IDSIZE];

	for (int i = 0, char* ap = config[0]; i < IDSIZE; i++, ap++) {
		locrfid[i] = strtoul(ap,&ap,16);
	}
	gpio = atoi(config[1]);

	fprintf(stdout, "Local RFID:%s, GPIO:%d\n",config[0] , gpio); 
	int round = atoi(argv[2]);
	std::cout << "Start rfwakes at: " <<  round << ", " << toTime(std::chrono::system_clock::now()) << std::endl;


	int counter = 0;
	for(auto it = Targetlist.begin(); it != Targetlist.end(); it++) {
		for (int i = 0, char* ap = it->rem; i < IDSIZE; i++, ap++) {
			it->remrfid[i] = strtoul(ap,&ap,16);
		}
		fprintf(stdout, "Wake Remote RFID[%d]: ", ++counter);
		printrfid(it->remrfid);
		//----------------------------------------------------------------------
		fprintf(stdout, "\n");
	}

	// *********************
	// *** Initial Setup ***
	// *********************
	if (wiringPiSetupSys() < 0) {
		fprintf(stderr, "Failed to setup wiringPi\n");
		exit(EXIT_FAILURE);
	}
	fd = wiringPiSPISetup(SPI_DEVICE, SPI_SPEED);
	if (fd < 0) {
		fprintf(stderr, "Failed to open SPI device\n");
		exit(EXIT_FAILURE);
	}
	if (wiringPiISR(gpio, INT_EDGE_RISING, &myInterrupt0) < 0)
	{
		fprintf(stdout, "Failed to set gpio to wiringPiISR\n");
		exit(EXIT_FAILURE);
	}

	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

	int nbr = 1, gotyou = 0;
	do {
		for (auto it = Targetlist.begin(); it != Targetlist.end();) {
			unsigned char remrfid[IDSIZE];
			memcpy(&remrfid, &it->remrfid, sizeof it->remrfid);
			// ********************
			// *** Transmission ***
			// ********************
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
			} while (mode & 0x2000) == 0);

			// write Tx data
			unsigned char payload[11];
			for (int j = 0; j < 11; j++)
				payload[j] = remrfid[IDSIZE-1];
			rfm69txdata(payload, 11);
			// Wait for TX_Sent, takes approx. 853.3 micro-second
			do {
				usleep(1000);
				mode = rfm69getState();
				if (mode < 0) {
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
			} while ((mode & 0x08) == 0);
			fprintf(stdout, "%d. Wake-Telegram sent to %s.\n", nbr++, it->rem);

			// switch back to STDBY Mode
			if (rfm69STDBYMode()) {
				fprintf(stderr, "Failed to enter STDBY Mode\n");
				exit(EXIT_FAILURE);
			}
			// *** Reception ***
			// prepare for RX
			intReg = 0;
			if (rfm69startRxMode(locrfid)) {
				fprintf(stderr, "Failed to enter RX Mode\n");
				exit(EXIT_FAILURE);
			}

			// wait for HW interrupt(s) and check for CRC_Ok state
			usleep(84000);
			if (intReg == 1) { // in case of reception ...
				int mode = rfm69getState();
				if (mode < 0) {
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
				if ((mode & 0x02) == 0x02) { // Check CrcOk
					// read remote RF ID from FIFO
					memset(payload, 0, 11);
					rfm69rxdata(payload, 11); 
					fprintf(stdout, "Received payload:");
					for (int i = 0; i < 11; i++) {
						if(i != 0) fprintf(stdout,":");
						fprintf(stdout, "%02x", payload[i]);
					}
					fprintf(stdout, "\n");

					// Compare remote RFID with targetList
					for(std::vector <Target>::iterator it2 = Targetlist.begin(); it2 != Targetlist.end(); it2++)
					{					
						gotyou = 1;
						unsigned char temp = it2->remrfid[IDSIZE - 1];
						for (int j = 1; j < 11; j++)
							if (temp != payload[j]) 
								gotyou = 0;
						if (gotyou == 1)
						{
							it = it2;
							break;
						}
						
					}
				}
			}
			// switch back to STDBY Mode
			if (rfm69STDBYMode()) {
				fprintf(stderr, "Failed to enter STDBY Mode\n");
				exit(EXIT_FAILURE);
			}

			// if not receive ACK
			if (gotyou == 0) {
				++it;
			}
			else {
				// output of remote RF ID
				fprintf(stdout, "ACK received from called Station RF ID ");
				printrfid(it->remrfid);
				fprintf(stdout,"\n");
				
				// write into log file
				std::fstream flog;
				std::string name = std::string("ack_") + argv[1];
				flog.open (LogDIR + name + ".csv", std::fstream::in | std::fstream::out | std::fstream::app);	    	

				auto now = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = now-startTime;

				flog << "\"" << it->rem << "\"," << round << ",\"" << toTime(now) << "\",\"" 
				<< toTime(startTime) << "\"," << elapsed_seconds.count() <<" \r\n";
				flog.close();
				// recover `gotyou` switch
				gotyou = 0;
				it = Targetlist.erase(it);
			}
		}
	} while(!Targetlist.empty());


	close(fd);

	exit(EXIT_SUCCESS);
}
