/*
 * getInput.h
 *
 *  Created on: Dec 1, 2023
 *      Author: debin
 */

#ifndef GETINPUT_H_
#define GETINPUT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_CHARS 80
#define MAX_FIELDS 5
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;


void getsUart0(USER_DATA* data);
void parseFields(USER_DATA* data);
char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
bool isCommand (USER_DATA * data, const char strCommand[], uint8_t minArguments);
int cmpStr(const char* string1, const char* string2);

#endif /* GETINPUT_H_ */
