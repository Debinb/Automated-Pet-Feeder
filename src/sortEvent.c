//File created in order to sort the events using the hours and minutes values and updating the index.
#include <stdio.h>
#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "eeprom.h"

void sortEvent()
{
    uint16_t TOTAL_BLOCKS = 10;
    uint8_t fo;
    uint8_t block = 0;
    for (block = 0; block < TOTAL_BLOCKS; block++)
    {
        int ActiveFlag = readEeprom(16 * block + 5);

        if (ActiveFlag == 1)
        {
            uint8_t i = 0;
            for (i = 0; i < TOTAL_BLOCKS - 1; i++)
            {
                uint8_t j = 0;
                for (j = 0; j < TOTAL_BLOCKS - i -1; j++)
                {
                    uint32_t currentHours = (readEeprom(16 * j + 3));
                    uint32_t currentMinutes = (readEeprom(16 * j + 4));

                    uint32_t nextHours = (readEeprom(16 * (j + 1) + 3));
                    uint32_t nextMinutes = (readEeprom(16 * (j + 1) + 4));

                    if ((currentHours > nextHours) || (currentHours == nextHours && currentMinutes > nextMinutes))
                    {
                        uint8_t k = 0;
                        for (k = 0; k < 16; k++)    //Swap the entire block
                        {
                            uint32_t temp = readEeprom(16 * j + k);
                            writeEeprom(16 * j + k, readEeprom(16 * (j + 1) + k));
                            writeEeprom(16 * (j + 1) + k, temp);
                        }
                    }
                }
            }
            // Update the event field on the eeprom

            for (fo = 0; fo < 10; fo++)
            {
                uint8_t sorted = 16* fo;
                writeEeprom(sorted, fo);  //Event address
            }
        }
    }
}

