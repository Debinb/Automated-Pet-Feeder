#include <inttypes.h>
#include <float.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "clock.h"
#include "eeprom.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "sortEvent.h"
#include "getInput.h"
#include "initModules.h"

void AlarmTime()
{
    uint8_t a = 0;
    uint32_t H = 0;
    uint32_t M = 0;
    uint32_t newSeconds = 0;

    H = readEeprom((16*a)+3);
    M = readEeprom((16*a)+4);

    newSeconds = ((3600 * H) + (M * 60));

    uint32_t RealTimeSeconds = 0;
    while(!(HIB_CTL_R & HIB_CTL_WRC));
    RealTimeSeconds = HIB_RTCC_R;           //Gets the value of the RTCC

    uint32_t Difference = 0;
    Difference = newSeconds - RealTimeSeconds;

    while(!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_RTCM0_R = RealTimeSeconds + Difference;

    uint32_t MatchRead;

    while(!(HIB_CTL_R & HIB_CTL_WRC));
    MatchRead = HIB_RTCM0_R;

    char MRead[40];
    uint32_t MatchHH = MatchRead / 3600;
    uint32_t MatchMM = (MatchRead % 3600) / 60;

    if((H == 0xFFFFFFFF) && (M == 0xFFFFFFFF))
    {
        putsUart0("No alarm scheduled.\n");
    }
    else
    {
        snprintf(MRead, sizeof(MRead), "Alarm time is %02d:%02d\n", MatchHH, MatchMM);
        putsUart0(MRead);
    }
}

