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
#include <sys/wait.h>
#include <sys/time.h>

// TIME HEADER
#include <chrono>
#include <ctime>


// C++ Lib
#include <fstream>
#include <vector>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <string>
#include <iostream>

#include "rfm69bios.h"
#include "mytime.h"
#include "setConfig.h"

#define LogDIR "/home/pi/Desktop/log/"

int self_pipe_fd[2];
const char* digit = "1234567890";
pid_t fork_pid = 0;
int intReg = 1;
int fd;

void printrfid(unsigned char rfid[]) {
	for (int i = 0; i < IDSIZE; i++) {
		if(i != 0) fprintf(stdout,":");
		fprintf(stdout, "%02x", rfid[i]);
	}
}

void myInterrupt0(void) 
{
	if (!intReg) 
	{
		intReg = 1;
		write(self_pipe_fd[1], "x", 1);	
	}
}

void intHandler(int signo)
{
	fprintf(stdout, "Receive CTRL+C\n");
	// kill(fork_pid, SIGINT);
	close(fd);
	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
	signal(SIGINT, intHandler);
	if (system("pkill -9 -f server.py") < 0)
		fprintf(stdout, "[ERROR] Could not kill IoTServer\n");

	std::vector <Target> Targetlist;
	int mode;

	char targetFName[256];
	if (argc == 4)
	{  		
		fprintf(stdout, "[DEBUG] Now Usage: rfwakes fName round distance\n");
		strcpy(targetFName, "/home/pi/target/targetlist");
	}
	else if (argc == 5)
	{
		fprintf(stdout, "[DEBUG] Now Usage: rfwakes fName round distance targetFile\n");
		strcpy(targetFName, "/home/pi/target/");
		strcat(targetFName, argv[4]);

	}
	else
	{  		
		fprintf(stdout, "[ERROR] Usage: rfwakes fName round distance [targetFile]\n");
		exit(EXIT_FAILURE);
	}
	// Check wrong argv
	if (std::string(argv[2]).find_first_not_of(digit) != std::string::npos ||
		std::string(argv[3]).find_first_not_of(digit) != std::string::npos)
	{
		fprintf(stdout, "[DEBUG] Round or distance is not number\n");
		exit(EXIT_FAILURE);
	}

	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();

	// *****************
	// *** IoTServer ***
	// *****************
	fork_pid = 0;
	while ((fork_pid = fork()) < 0)
	{
		int status;
		pid_t pid = waitpid(-1, &status, 0);
	}
	if (!fork_pid) // This is child
	{
		struct timeval te;
	    	gettimeofday(&te, NULL); // get current time
 	       	long long seconds = te.tv_sec + te.tv_usec / 1000000; // calculate milliseconds

		char tmp[100];
		strcpy(tmp, "python /home/pi/Desktop/RP-Wake-Up-Radio/server.py ");
		strcat(tmp, argv[1]);
		strcat(tmp, " ");
		strcat(tmp, argv[2]);
		strcat(tmp, " ");
		strcat(tmp, std::to_string(seconds).c_str());
		strcat(tmp, "\n");
	
		char *tmpArgv[5];
		tmpArgv[0] = strtok(tmp, " ");
		tmpArgv[1] = strtok(NULL, " ");
		tmpArgv[2] = strtok(NULL, " ");
		tmpArgv[3] = strtok(NULL, " ");
		tmpArgv[4] = strtok(NULL, "\n");
		tmpArgv[5] = NULL;

		close(0);
		// close(1);
		// close(2);
		execvp("python", tmpArgv);
	}

	// **************
	// *** Config ***
	// **************
	char config[2][30];
	readConfig("/home/pi/myConfig", config); 
	Targetlist = readTargets(targetFName);
	int gpio;
	unsigned char locrfid[IDSIZE], recrfid[IDSIZE];

	// Read local rfid and gpio
	{
		int i;
		char *ap;
		for (i = 0, ap = config[0]; i < IDSIZE; i++, ap++) {
			locrfid[i] = strtoul(ap,&ap,16);
		}
	}
	gpio = atoi(config[1]);
	fprintf(stdout, "[INFO] Local RFID:%s, GPIO:%d\n", config[0], gpio);

	int round = atoi(argv[2]);
	int distance = atoi(argv[3]);
	std::cout << "[DEBUG] Ouput Format: remote_RFID, round, distance, ACK_time, wake_time, elapsed_time" << std::endl;
	std::cout << "[INFO] Start rfwakes at: Round " << round << ", " << toTime(std::chrono::system_clock::now()) << std::endl;


	// Ouput wake-up time and wake-up target to file
	{
		std::fstream flog;
		std::string name = std::string("wake_") + argv[1];
		flog.open(LogDIR + name + ".csv", std::fstream::in | std::fstream::out | std::fstream::app);

		flog << round << "," << distance << ",\"" << (argc == 5 ? argv[4] : "targetlist")
			<< "\",\"" << toTime(startTime) << "\"\r\n";
		flog.close();
	}

	int counter = 0;
	for(auto it = Targetlist.begin(); it != Targetlist.end(); it++) {
		int i;
		char* ap;
		for (i = 0, ap = it->rem; i < IDSIZE; i++, ap++) {
			it->remrfid[i] = strtoul(ap,&ap,16);
		}
		fprintf(stdout, "[DEBUG] Wake Remote RFID[%d]: ", ++counter);
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

	int nbr = 1, gotyou = 0;
	do {
		// **********************
		// *** Wake Up target ***
		// **********************
		for (auto it = Targetlist.begin(); it != Targetlist.end(); it++) {
			unsigned char remrfid[IDSIZE];
			memcpy(&remrfid, &it->remrfid, sizeof it->remrfid);
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
			unsigned char payload[PAYLOADLENGTH];
			for (int j = 0; j < IDLENGTH; j++)
				payload[j] = remrfid[IDSIZE-1];

			memcpy((void *)(&(payload[IDLENGTH])), timePointToChar(startTime), PAYLOADLENGTH - IDLENGTH);

			rfm69txdata(payload, PAYLOADLENGTH);
			// Wait for TX_Sent, takes approx. 853.3 micro-second
			do {
				usleep(1000);
				mode = rfm69getState();
				if (mode < 0) {
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
			} while ((mode & 0x08) == 0);
			// fprintf(stdout, "%d. Wake-Telegram sent to %s.\n", nbr++, it->rem);

			// switch back to STDBY Mode
			if (rfm69STDBYMode(locrfid))
			{
				fprintf(stderr, "Failed to enter STDBY Mode\n");
				exit(EXIT_FAILURE);
			}
		
			struct timeval delay;
			delay.tv_sec = 0;
			delay.tv_usec = 85;		// air time 
			select(0, NULL,NULL, NULL, &delay);
		}

		int listening_time = 5;
		while (listening_time--)
		{
			// *** Reception ***
			// prepare for RX
			if (pipe(self_pipe_fd) < 0)
			{
				fprintf(stderr, "Fail ot open pipe\n");
				exit(EXIT_FAILURE);
			}
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(self_pipe_fd[0], &rfds);
			intReg = 0;
			if (rfm69startRxMode(locrfid)) {
				fprintf(stderr, "Failed to enter RX Mode\n");
				exit(EXIT_FAILURE);
			}

			// When it is interrupted by rfm, write something to self_pipe to stop sleeping
			struct timeval delay;
			delay.tv_sec = 0;
			delay.tv_usec = 136000; // 136 ms = 100 * 1.36 ms (time for a packet)
			select(self_pipe_fd[0] + 1, &rfds,NULL, NULL, &delay);
			close(self_pipe_fd[0]);
			close(self_pipe_fd[1]);

			std::vector<Target>::iterator it2;
			if (intReg == 1) { // in case of reception ...
				int mode = rfm69getState();
				if (mode < 0) {
					fprintf(stderr, "Failed to read RFM69 Status\n");
					exit(EXIT_FAILURE);
				}
				if ((mode & 0x02) == 0x02) { // Check CrcOk
					// read remote RF ID from FIFO
					unsigned char payload[PAYLOADLENGTH];
					memset(payload, 0, PAYLOADLENGTH);
					rfm69rxdata(payload, PAYLOADLENGTH); 
					fprintf(stdout, "Received payload:");
					for (int i = 0; i < IDLENGTH; i++) {
						if(i != 0) fprintf(stdout,":");
						fprintf(stdout, "%02x", payload[i]);
					}
					fprintf(stdout, "\n");

					// Compare remote RFID with targetList
					for(it2 = Targetlist.begin(); it2 != Targetlist.end(); it2++)
					{					
						gotyou = 1;
						unsigned char temp = it2->remrfid[IDSIZE - 1];
						for (int j = 1; j < IDLENGTH; j++)
							if (temp != payload[j]) 
								gotyou = 0;
						if (gotyou == 1)
							break;					
					}
				}
			}
			// switch back to STDBY Mode
			if (rfm69STDBYMode(locrfid))
			{
				fprintf(stderr, "Failed to enter STDBY Mode\n");
				exit(EXIT_FAILURE);
			}

			// if receiving ACK
			if (gotyou) {
				// output of remote RF ID
				fprintf(stdout, "ACK received from called Station RF ID ");
				printrfid(it2->remrfid);
				fprintf(stdout,"\n");
				
				// write into log file
				std::fstream flog;
				std::string name = std::string("ack_") + argv[1];
				flog.open (LogDIR + name + ".csv", std::fstream::in | std::fstream::out | std::fstream::app);	    	

				auto now = std::chrono::system_clock::now();
				std::chrono::duration<double> elapsed_seconds = now-startTime;

				flog << "\"" << it2->rem << "\"," << round << ","<< distance << ",\"" << toTime(now) << "\",\"" 
				<< toTime(startTime) << "\"," << elapsed_seconds.count() <<" \r\n";
				flog.close();
				// recover `gotyou` switch
				gotyou = 0;
				Targetlist.erase(it2);
			}
		}
		
	} while(!Targetlist.empty());


	close(fd);
	// kill(fork_pid, SIGINT);
	exit(EXIT_SUCCESS);
}
