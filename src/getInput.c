//Functions to extract and parse user input from Terminal

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "clock.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"

//STRUCT:
#define MAX_CHARS 80
#define MAX_FIELDS 5
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;



void getsUart0(USER_DATA* data)
{
    uint8_t count = 0;
    while(true)
    {
        char c = getcUart0();                     //getting serial data
        if((c == 8 || c == 127) && (count > 0))   //Checking if the character is a backspace (ASCII code 8 or 127) and count > 0
        {
            count--;                              //decrement the last character
        }

        else if(c == 13)                          //Adding a null terminator at the end of string after carriage return (ASCII code 13
        {
            data -> buffer[count] = '\0';
            return;
        }

        else if (c >= 32)                         //Incrementing count when space (ASCII code 32) is pressed
        {
            data->buffer[count] = c;
            count++;

            if (count == MAX_CHARS)               //MAX chars [80] typed results in returning the whole string in the next line.
            {
                putcUart0('\n');
                data->buffer[count] = '\0';
                return;
            }
        }
    }
}

//Parsing the strings input by the user.
void parseFields(USER_DATA* data)
{
    data->fieldCount = 0;
    uint8_t i = 0;
    int prevCh = 0;
    uint8_t indexcount= 0;

    while(data->buffer[i]) // != '\0' && data->fieldCount < MAX_FIELDS)   When data buffer is not empty, the while loop is true
    {
        if (((data->buffer[i] >= 'a') && (data->buffer[i] <= 'z')) || ((data->buffer[i] >= 'A') && (data->buffer[i] <= 'Z'))) //checking if the char given is alphanumeric
        {
            if (prevCh == 0) //Transition from delimiter to alpha char
            {
                data->fieldType[indexcount] = 'a';      //Assigns 'a' if the string is alphanumeric
                data->fieldPosition[indexcount] = i;    //The char position of the string is assigned to 'i'.
                data->fieldCount++;
                prevCh = 1;
                indexcount++;

            }
        }

        else if (((data->buffer[i] >= '0') && (data->buffer[i] <= '9')) || (data->buffer[i] == '-') || (data->buffer[i] == '.')) //printing 'n' if numeric
        {
           if(prevCh == 0) //Transition from delimiter to numeric char
           {
               data->fieldType[indexcount] = 'n';
               data->fieldPosition[indexcount] = i;
               data->fieldCount++;
               prevCh = 1;
               indexcount++;

            }
        }
        else                              //If its not a alphanumeric or numeric
        {
            data->buffer[i] = '\0';       //the end is given a NULL
            prevCh = 0;
        }
        i++;
    }
}

//Gets the string typed
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    if(fieldNumber <= MAX_FIELDS)
    {
        return &(data->buffer[data->fieldPosition[fieldNumber]]);
    }
    else
    {
        return 0;
    }
}

int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    int32_t i = 0;
    int32_t fieldint = 0;
    char *strtoi;

    if((fieldNumber <= data -> fieldCount) && (data->fieldType[fieldNumber] == 'n'))
    {
        strtoi = &(data->buffer[data->fieldPosition[fieldNumber]]);
        while(strtoi[i])
        {
            fieldint = fieldint * 10 + (strtoi[i] - '0');
            i++;
        }
        return fieldint;
    }
    return 0;
}

//Function to compare the
bool isCommand (USER_DATA * data, const char strCommand[], uint8_t minArguments)
{
    uint8_t argCount = (data->fieldCount)-1;  // number of arguments (excluding the command field)
    uint8_t i;
    uint8_t index = 0;

    int strSize = 0;
    while (strCommand[strSize] != '\0')
    {
        ++strSize;
    }

    while (data ->buffer[index] != '\0')
    {
        for(i = 0; i < strSize ; i++)
        {
            if (data->buffer[index + i] != strCommand[i])
            {
                break;
            }
        }

        if (i == strSize)
        {
            if (argCount >= minArguments)
            {
                return 1;  // Command found and has enough arguments
            }
            else
            {
                return 0;  // Command found but not enough arguments
            }
        }
        index++;
    }
    return 0;
}

//Function to compare the strings (Custom strcmp)
int cmpStr(const char* string1, const char* string2)
{
    while(*string1 && *string2 && (*string1 == *string2))
    {
        ++string1;
        ++string2;
    }
    return (*string1 - *string2);
}
