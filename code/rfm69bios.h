

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define SPI_SPEED 10000000
#define SPI_DEVICE 0
#define IDSIZE 8

#define PAYLOADLENGTH 27
#define IDLENGTH 3
#define REMOTE_RFID 0x99

#ifndef RFM69BIOS_H
#define RFM69BIOS_H

int rfm69getState(void);
int rfm69getAllState(void);
int rfm69txdata(const unsigned char*, unsigned int);
int rfm69rxdata(unsigned char*, unsigned int);
int rfm69STDBYMode(const unsigned char *);
int rfm69startTxMode(const unsigned char *);
int rfm69startRxMode(const unsigned char*);
int rfm69restartRx(void);
int rfm69ListenMode(const unsigned char*);
int rfm69cancelAddrFilter(void);
int rfm69setAutoModes(void);
int rfm69cancelAutoModes(void);
int rfm69cleanFIFO(void);
#endif