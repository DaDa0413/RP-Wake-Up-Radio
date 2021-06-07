#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "rfm69bios.h"

#ifndef SETCONFIG_H
#define SETCONFIG_H

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

std::vector<Target> readTargetsByID(char *targetFName)
{
	std::vector<Target> tlist;
	char item[30] = {};
	char buf[10];
	char *temp = strtok(targetFName, "_");

	strcpy(item, "11:11:11:11:11:11:11:");
	sprintf(buf, "%2d", atoi(temp));
	strcat(item, buf);
	Target t;
	strcpy(t.rem, item);
	tlist.push_back(t);
	memset(&item[0], 0, sizeof(item));
	while ((temp = strtok(NULL, "_")) != NULL)
	{
		strcpy(item, "11:11:11:11:11:11:11:");
		sprintf(buf, "%2d", atoi(temp));
		strcat(item, buf);
		Target t;
		strcpy(t.rem, item);
		tlist.push_back(t);
		memset(&item[0], 0, sizeof(item));
	}
	return tlist;
}
#endif
