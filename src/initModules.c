#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "clock.h"
#include "eeprom.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"

//Hibernation Module
void initHIB()
{
    while(!(HIB_CTL_R & HIB_CTL_WRC));          //Wait for the write to be complete
    HIB_CTL_R |= HIB_CTL_CLK32EN;               //Enable 32.768kHz Hibernation oscillator

    while(!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_CTL_R |= 0x41;                          //Enables the Real Time Clock Timer [RTCEN]

    while(!(HIB_CTL_R & HIB_CTL_WRC));
    HIB_IM_R |= HIB_IM_RTCALT0;                 //Turns on interrupts for RTC Alarm 0

    while(!(HIB_CTL_R & HIB_CTL_WRC));
    NVIC_EN1_R = 1 << (INT_HIBERNATE-16-32);    //Enables the Hibernate interrupt and ISR to be triggered
}

// PWM SETUP FOR M0PWM1
void initPWM()
{
    SYSCTL_SRPWM_R = SYSCTL_SRPWM_R0;                // reset PWM0 module
    SYSCTL_SRPWM_R = 0;                              // leave reset state
    PWM0_0_CTL_R = 0;                                // turn-off PWM0 generator 1 (drives outs 0, 1)

    PWM0_0_GENB_R = PWM_0_GENB_ACTCMPBD_ONE | PWM_0_GENB_ACTLOAD_ZERO;
                                                     // output 5 on PWM1, gen 0b, cmpb
    PWM0_0_LOAD_R = 1024;                            // set frequency to 40 MHz sys clock / 2 / 1024 = 19.53125 kHz

    PWM0_0_CMPB_R = 0;                               //sets the Compare register to 0 therefore no bogus value.

    PWM0_0_CTL_R = PWM_0_CTL_ENABLE;                 // turn-on PWM1 generator 0
    PWM0_ENABLE_R = PWM_ENABLE_PWM1EN;               // enable outputs
}
