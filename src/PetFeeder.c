/*
 *  Created on: September 10, 2023
 *  Author: Debin Babykutty
/-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------
 * AUGER:   Port B7 - M0PWM1
 * CO-:     Port C7
 * SPEAKER: Port D0
 * TRIGGER: Port D6
 * PUMP:    Port F0 - M1PWM4
 * RED_LED: Port F1
 * SENSOR:  Port A2
*/

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
#include "AlarmTime.h"

// BIT-BANDING:
#define SENSOR      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 2*4)))     //PA2
#define AUGER       (*((volatile uint32_t *)(0x42000000 + (0x400053FC-0x40000000)*32 + 7*4)))     //PB7
#define CO_NEG      (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 7*4)))     //PC7
#define SPEAKER     (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 0*4)))     //PD0
#define TRIGGER     (*((volatile uint32_t *)(0x42000000 + (0x400073FC-0x40000000)*32 + 6*4)))     //PD6
#define PUMP        (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 0*4)))     //PF0

// MASKING:
#define PUMP_MASK 1         // 2^0    -   PORT F0
#define SPEAKER_MASK 1      // 2^0    -   PORT D0
#define SENSOR_MASK 4       // 2^2    -   PORT A2
#define TRIGGER_MASK 64     // 2^6    -   PORT D6 (WT5CCP0)
#define AUGER_MASK 128      // 2^7    -   PORT B7
#define CO_NEG_MASK 128     // 2^7    -   PORT C7 (CO- : Analog Comparator Negative Input)

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    //Initialize Uart clock and set the baud rate
    initUart0();
    setUart0BaudRate(115200, 40e6);

    //SYSTEM CONTROL MODULES
  //--------------------------------------------
     // Enable GPIO clocks for Register 1 [PORT_B], 2 [PORT_C], 3 [PORT_D] and 5 [PORT_F]
     SYSCTL_RCGCGPIO_R |=  SYSCTL_RCGCGPIO_R0 | SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R2 | SYSCTL_RCGCGPIO_R3 | SYSCTL_RCGCGPIO_R5;

     // Enable Timer Clock for Timer 1, Timer 2 and Timer 3
     SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1 | SYSCTL_RCGCTIMER_R2 | SYSCTL_RCGCTIMER_R3 | SYSCTL_RCGCTIMER_R4;

     SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R1 | SYSCTL_RCGCWTIMER_R4;     //Enable Wide Timer Clock
     SYSCTL_RCGCACMP_R |=  0x00000001;                                       //Enable Analog Comparator Clock
     SYSCTL_RCGCHIB_R |= SYSCTL_RCGCHIB_R0;                                  //Enable Hibernation Clock
     SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;                                  //Enable PWM Clocking
     _delay_cycles(3);
  //---------------------------------------------

     //TIMER CONFIGURATIONS
   //---------------------------------------------
     //Configuring Timer X to fire every 10 seconds - 9.96 sec
     TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                     // turn-off timer before reconfiguring
     TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;               // configure as 32-bit timer (A+B)
     TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;              // configure for periodic mode (count down)
     TIMER1_TAILR_R = 400000000;                          // Timer Ticks # = Seconds x Clock Freq.
     TIMER1_IMR_R = TIMER_IMR_TATOIM;                     // turn-on interrupts for timeout in timer module
     TIMER1_CTL_R |= TIMER_CTL_TAEN;                      // turn-on timer
     NVIC_EN0_R = 1 << (INT_TIMER1A-16);                  // turn-on interrupt 37 (TIMER1A)
   //---------------------------------------------

     //WIDE TIMER CONFIGURATION - Count Up Timer
   //---------------------------------------------
     WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;                            // turn-off counter before reconfiguring
     WTIMER1_TAMR_R = TIMER_TAMR_TAMR_1_SHOT | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
   //---------------------------------------------

     //TIMER2 CONFIGURATION - Count Up Timer
   //---------------------------------------------
     TIMER2_CTL_R &= ~TIMER_CTL_TAEN;                            // turn-off counter before reconfiguring
     TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;                      // Configured timer to be a 32 bit Timer 2
     TIMER2_TAMR_R = TIMER_TAMR_TAMR_1_SHOT | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
   //---------------------------------------------

     //TIMER3 CONFIGURATION - One Shot Timer
     //---------------------------------------------
     TIMER3_CTL_R &= ~TIMER_CTL_TAEN;                            // turn-off counter before reconfiguring
     TIMER3_CFG_R = TIMER_CFG_32_BIT_TIMER;                      // Configured timer to be a 32 bit Timer 2
     TIMER3_TAMR_R = TIMER_TAMR_TAMR_1_SHOT | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
     TIMER3_IMR_R = TIMER_IMR_TATOIM;                            // turn-on interrupts for timeout in timer module
     //---------------------------------------------

     //WIDE TIMER CONFIGURATION - PERIOIDIC TIMER
     //---------------------------------------------
       WTIMER4_CTL_R &= ~TIMER_CTL_TAEN;                            // turn-off counter before reconfiguring
       WTIMER4_TAMR_R = TIMER_TAMR_TAMR_PERIOD| TIMER_TAMR_TACDIR;  // configure for edge count mode, count up
       WTIMER4_TAILR_R = 80000000;                                  // configure interrupts to be fired every 2 seconds. 40M * 2 = 80M
       WTIMER4_IMR_R |= TIMER_IMR_TATOIM;                           // turn on interrupts for timeout in timer module
       NVIC_EN3_R = 1 << (INT_WTIMER4A-16-96);                      // turn-on interrupt 37 (TIMER1A)
     //---------------------------------------------

      //TIMER3 CONFIGURATION - One Shot Timer
      //---------------------------------------------
       TIMER4_CTL_R &= ~TIMER_CTL_TAEN;                            // turn-off counter before reconfiguring
       TIMER4_CFG_R = TIMER_CFG_32_BIT_TIMER;                      //Configured timer to be a 32 bit Timer 2
       TIMER4_TAMR_R = TIMER_TAMR_TAMR_1_SHOT | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
       TIMER4_TAILR_R = 100000000;                                 // Timer Ticks # = Seconds x Clock Freq. 10M = 5seconds
       TIMER4_IMR_R = TIMER_IMR_TATOIM;                            // turn-on interrupts for timeout in timer module
       NVIC_EN2_R = 1 << (INT_TIMER4A-16-64);                      // turn-on interrupt 37 (TIMER1A)
      //---------------------------------------------

    //GPIO CONFIGURATIONS
  //---------------------------------------------
     //PORT F0 UNLOCKING
     GPIO_PORTF_LOCK_R |= 0x4C4F434B;    //Using this to unlock PORTF[0] (PUMP). The HEX is the value used to unlock the GPIO Commit register so that write access is granted.
     GPIO_PORTF_CR_R |= PUMP_MASK;       //Unlocks the commit register so that PUMP can be used.

     // Configure Direction and Enabling - SENSOR, CO_NEG = GPI.........PUMP, TRIGGER, AUGER & SPEAKER = GPO
     GPIO_PORTA_DIR_R &= ~(SENSOR_MASK);                //PORT A Input Configuration
     GPIO_PORTC_DIR_R &= ~(CO_NEG_MASK);                //PORT C Input Configurations
     GPIO_PORTD_DIR_R |= SPEAKER_MASK | TRIGGER_MASK;   //PORT D Output Configuration
     GPIO_PORTF_DIR_R |= PUMP_MASK;                     //PORT F Output Configuration

     GPIO_PORTA_DEN_R |= SENSOR_MASK;
     GPIO_PORTB_DEN_R |= AUGER_MASK;                   //PORT B Digital Enable
     GPIO_PORTC_DEN_R |= ~CO_NEG_MASK;                 //PORT C Digital Disable
     GPIO_PORTD_DEN_R |= SPEAKER_MASK | TRIGGER_MASK;  //PORT D Digital Enable
     GPIO_PORTF_DEN_R |= PUMP_MASK;                    //PORT F Digital Enable

     // Configure CO- Input
     GPIO_PORTC_AFSEL_R &= ~CO_NEG_MASK;                  // Disable Alternate Function
     GPIO_PORTC_AMSEL_R |= CO_NEG_MASK;                   // Enable Analog for CO_NEG_MASK

     //Configure Port B7 for the PWM
     GPIO_PORTB_AFSEL_R |= AUGER_MASK;                    // Enabling alternate function for AUGER
     GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB7_M0PWM1;           // Port control for the AUGER
  //---------------------------------------------

    //ANALOG COMPARATOR CONFIGURATIONS
  //---------------------------------------------
    COMP_ACREFCTL_R |= COMP_ACREFCTL_EN;                 // Enable the reference module
    COMP_ACREFCTL_R &= ~COMP_ACREFCTL_RNG;               // Setting internal reference to high range
    COMP_ACREFCTL_R |= 0xF;                              // Use internal voltage reference of 2.469
    COMP_ACCTL0_R |= COMP_ACCTL0_CINV | COMP_ACCTL0_ISEN_RISE | COMP_ACCTL0_ASRCP_REF;  //CINV - invert, ISEN_RISE - RIsing edge, ASRCP_REF - Internal VREF
  //---------------------------------------------
}


void timer1Isr()
{
    TRIGGER = 1;                                 //De-integrate the GPO pin
    waitMicrosecond(10);
    TRIGGER = 0;

    WTIMER1_CTL_R |= TIMER_CTL_TAEN;             //turn-ON One Shot Timer
    WTIMER1_TAV_R = 0;                           //Set the One Shot Timer to zero

    COMP_ACINTEN_R |= COMP_ACINTEN_IN0;          //Enabling Interrupts for Comparator
    NVIC_EN0_R = 1 << (INT_COMP0-16);            //turn-on interrupt 37 (COMP0)
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;           //clear interrupt flag
}

void analogISR()
{
    uint32_t time = 0;
    float level = 0.0;
    time = WTIMER1_TAV_R;                       //Give 'time' the value of the Wide Timer 1 (One Shot Timer)
    WTIMER1_CTL_R &= ~TIMER_CTL_TAEN;           //turn-off timer before reconfiguring
    COMP_ACMIS_R = 0x1;                         //Clear comparator interrupt

    if(time >= 2000 && time <= 2100)
    {
        level = 0;
    }
    else if(time >= 2700 && time <= 2775)
    {
        level = 50;
    }
    else if(time >= 2800 && time <= 2900)
    {
        level = 100;
    }
    else if(time >= 2901 && time <= 3030)
    {
        level = 200;
    }
    else if(time >= 3075 && time <= 3150)
    {
        level = 300;
    }
    else if(time >= 3200 && time <= 3275)
    {
        level = 400;
    }
    else if(time >= 3276 && time <= 3375)
    {
        level = 500;
    }
    else if(time >= 3376 && time <= 3499)
    {
        level = 600;
    }
    else
    {
        //level = (time - 2709)/(1.24);             //Linear regression formula for detecting unknown ticks
        level = 0;
    }
//    char why[90];                                 //Print out ticks and water level to the interface
//    snprintf(why, sizeof(why), "Period: %7"PRIu32" (us)\t WaterLevel: %.2f (mL)\n", time, level);
//    putsUart0(why);

    uint8_t mode = 0;                               //Reading the mode and water level setting from the EEPROM
    mode = readEeprom((16*0)+7);
    uint16_t volume = 0;
    volume = readEeprom((16*0)+6);

    if(mode == 1)                                   //AUTO mode turns on the pump when the water level in
    {                                               //the bowl is lower than the water level set by the user
        if(level < volume)  
        {
           PUMP = 1;
           TIMER3_CTL_R |= TIMER_CTL_TAEN;         //Timer 3 enabled
           NVIC_EN1_R = 1 << (INT_TIMER3A-16-32);  // turn-on interrupt 37 (TIMER1A)
        }
        else if(level == volume)                   //Do nothing if in AUTO mode and level is same as water setting
        {
            PUMP = 0;
        }
    }
    else if(mode == 2)                             //MOTION mode turns on Wide Timer 4 to trigger the 2 second interrupt 
    {
        WTIMER4_CTL_R |= TIMER_CTL_TAEN;
    }

//    if(level < volume)                            //Speaker/Alarm goes off in AUTO mode
//    {
//        uint8_t i;
//        for(i = 0; i < 50; i++)
//        {
//            SPEAKER = 1;
//            waitMicroseconds(5);
//            SPEAKER = 0;
//            waitMicroseconds(5);
//        }
//    }
}

void alarmISR()                             // Hibernate ISR 
{
    float speed = 0;
    uint16_t pwm = 0;
    uint16_t dur = 0;
    uint16_t block = 0;
    dur = readEeprom((16*block)+1);          // Access the duration field of block 0 from the EEPROM.
    pwm = readEeprom((16*block)+2);          // Access the PWM field of block 0 from the EEPROM.

    TIMER2_TAILR_R = dur * 40000000;         // Duration x System Clock to get the ticks needed to be run.
    TIMER2_CTL_R |= TIMER_CTL_TAEN;          // Timer 2 is enabled and starts counting until the required ticks.
    TIMER2_IMR_R = TIMER_IMR_TATOIM;         // Turn-on interrupts for timeout in timer module.
    NVIC_EN0_R = 1 << (INT_TIMER2A-16);      // Storing the Timer 2 interrupt.

    speed = (pwm/100.0)*1023;                // Stores the duty cycle of the motor.
    PWM0_0_CMPB_R = speed;                   // The compare register uses the duty cycle value to control the speed of the motor.

    while(!(HIB_CTL_R & HIB_CTL_WRC));       // Wait until the write cycle of the HIB module is complete.
    HIB_IC_R |= HIB_RIS_RTCALT0;             // Clear the hibernate module interrupt for the RTC alarm.
    putsUart0("Matched. \n");
}

void timer2ISR()                             // Timer 2 ISR
{
    uint8_t i = 0;
    putsUart0("Triggered. \n");

    PWM0_0_CMPB_R = 0;
    TIMER2_ICR_R |= TIMER_ICR_TATOCINT;      // Clear the Timer 2 interrupt.
    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;         // Disables the timer.

    writeEeprom((16*0)+5, 0x0);              // Active flag is set to 0 and block 0 is cleared.
    for(i = 0; i < 5; i++)
    {
        writeEeprom((16*0)+i, 0xFFFFFFFF);
    }

    sortEvent();                            // Sorts the event and now the block 1 becomes block 0.
    AlarmTime();                            // Puts the next alarm into the Match Register, aka, reseeding.
}

void timer3ISR()
{
    PUMP = 0;
    TIMER3_ICR_R |= TIMER_ICR_TATOCINT;     // Clear the Timer 2 interrupt
}

void Wide4ISR()                             // WIDE TIMER 4 ISR runs every two seconds in MOTION mode
{
    uint8_t mode = 0;
    mode = readEeprom((16*0)+7);            // Reads the fill mode

    uint8_t MotionDetected = 0;
    MotionDetected = SENSOR;                // MotionDetected stores the PIR Sensor values.

    if((mode == 2))                         // MOTION Mode turns the water pump on when motion is detected
    {
        if((MotionDetected == 1) )
        {
            PUMP = 1;
            TIMER4_CTL_R |= TIMER_CTL_TAEN;
        }
    }
    WTIMER4_ICR_R |= TIMER_ICR_TATOCINT;      //Clear the Timer 2 interrupt

}

void Timer4ISR()                            
{
    PUMP = 0;
    TIMER4_ICR_R |= TIMER_ICR_TATOCINT;      //Clear the Timer 2 interrupt
}
//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    USER_DATA data;
    initHw();
    initEeprom();
    initHIB();
    initPWM();

    uint16_t event = 0;
    uint16_t duration = 0;
    uint16_t PWM = 0;
    uint16_t hour = 0;
    uint16_t mins = 0;
    int32_t RTCtime = 0;
    char str[60];
    int32_t HH = 0;
    int32_t MM = 0;
    putsUart0("Enter instructions:\n");

    while(true)
    {
        bool valid = false;
        uint8_t EventActive = 1;
        getsUart0(&data);
        putcUart0('\n');
        parseFields(&data);

        if(isCommand(&data, "time", 2))             // Extracts the time values from user input on the interface using UART
        {
            valid = true;
            int32_t HOURS = getFieldInteger(&data, 1);
            int32_t MINS =  getFieldInteger(&data, 2);

            if(((HOURS < 24) && (MINS <= 59)) || ((HOURS == 0) && (MINS <= 59)))    // Valid time range is loaded into the RTCLD register 
            {                                                                       // for the clock to start counting up
                while(!(HIB_CTL_R & HIB_CTL_WRC));
                HIB_RTCLD_R = ((3600 * HOURS) + (MINS * 60));
            }
            else
            {
                putsUart0("Invalid time value has been input.\n");
            }
            sortEvent();
        }

        else if(isCommand(&data, "time", 0))            // Displays the time when "time" is entered on the interface
        {
            valid = true;
            while(!(HIB_CTL_R & HIB_CTL_WRC));
            RTCtime = HIB_RTCC_R;                       // Stores the value from the RTCC

            HH = RTCtime / 3600;                        // Converts the seconds into hours
            MM = (RTCtime % 3600) / 60;                 // Converts the remaining seconds into minutes
            HH = HH % 48;                               // Sets a 2 day limit for the RTCC (to account for the next day events)
            MM = MM % 60;

            snprintf(str, sizeof(str), "Real Time is %02d:%02d\n", HH, MM);
            putsUart0(str);  //displayed in seconds
        }

        else if(isCommand(&data, "feed", 5))            // Lets the user add feeding schedules
        {                                               // Usage: feed 'index' 'duration to run motor (secs)' 'motor speed' 'time to run'
            valid = true;                               // Example: "feed 0 10 99 5:10"
            event = getFieldInteger(&data, 1);
            duration = getFieldInteger(&data, 2);
            PWM = getFieldInteger(&data, 3);
            hour = getFieldInteger(&data, 4);
            mins = getFieldInteger(&data, 5);

            if((event < 10) && (hour < 24) && (mins < 60))            // If user enters event index and hours/mins in a valid range then execute
            {
                uint32_t secondsCompare = (hour * 3600) + (mins * 60); // For the case if user enters time lesser than the current time.
                if(secondsCompare < HIB_RTCC_R)                        // Sets that specific feeding schedule to the next day.
                {
                    hour += 24;
                }

                writeEeprom((16 * event), event);        // index field
                writeEeprom((16 * event) + 1, duration); // duration: Amount of time to run in seconds
                writeEeprom((16 * event) + 2, PWM);      // pwm: Motor speed (50-100 duty cycle)
                writeEeprom((16 * event) + 3, hour);     // hours
                writeEeprom((16 * event) + 4, mins);     // minutes
                writeEeprom((16 * event) + 5, EventActive); //Event Activated == Active Flag is set to 1, i.e, the event is active (background)
                putsUart0("The event has been scheduled.\n");

                sortEvent();
                AlarmTime();
            }
            else if(event > 10)
            {
                putsUart0("Event specified is out of range. Enter event between 0-9.\n");
            }
            else if(hour > 24 || mins > 60)
            {
                putsUart0("Enter time between 0:01 and 23:59.\n");
            }
        }

        else if(isCommand(&data, "feed", 2))                // To delete the entered feeding schedule using event index
        {                                                   // Since EEPROM is sorted, delete feeding schedule after viewing sorted schedule
            uint32_t deleteEvent = getFieldInteger(&data, 1);
            char* deleteText = getFieldString(&data, 2);

            if(deleteText != NULL && cmpStr(deleteText, "delete") == 0) // Compares if the second argument is "delete"
            {
                if(deleteEvent < 10)
                {
                    valid = true;
                    putsUart0("Event has been deleted.\n");
                    EventActive = 0;
                    uint8_t i = 0;
                    for(i = 0; i < 5; i++)
                    {
                        char strr[40];
                        writeEeprom((16*deleteEvent)+i, 0xFFFFFFFF);                            // Writes '0' for the feeding schedule
                        writeEeprom((16*deleteEvent)+5, 0x0);                                   // Sets the feeding schedule to inactive
                        snprintf(strr, sizeof(strr), "%d\t", readEeprom((16*deleteEvent)+i));   
                        putsUart0(strr);
                    }
                    sortEvent();
                    putsUart0("\n");
                    AlarmTime();
                }

                else
                {
                    putsUart0("Enter a valid event range.\n");
                }
            }

            else
            {
                putsUart0("Feeding Event has not been deleted.\nUsage: 'feed' [event #] 'delete'\n");
            }
        }

        else if(isCommand(&data, "schedule", 0))                // Displays the feeding schedules
        {
            valid = true;
            sortEvent();
            putsUart0("Event \t Duration      PWM \t HH:MM\n");

            char inputData[40];
            int eNum = 0;
            for(eNum = 0; eNum < 10; eNum++)
            {
                if((readEeprom(16*eNum) == 0xFFFFFFFF) && (readEeprom(16*eNum+5) == 0)) // Displays message if all the blocks have the value of 0
                {
                    putsUart0("\nNo events scheduled.\n");
                }
                else if(EventActive == readEeprom(16*eNum+5)) // Displays the feeding schedules if there is an active schedule  
                {
                    uint16_t NewHours = 0;
                    if(readEeprom(16*eNum+3) > 24)            // Converts the next day event (23:59+)in a range of 0-24
                    {
                        NewHours = readEeprom(16*eNum+3) % 24;
                    }
                    else if(readEeprom(16*eNum+3) < 24)       // Reads the exact value if inside 24 hours
                    {
                        NewHours = readEeprom(16*eNum+3);
                    }

                    snprintf(inputData, sizeof(inputData), "  %d\t    %02d \t\t%02d\t %02d:%02d\n", readEeprom(16*eNum), readEeprom(16*eNum+1), readEeprom(16*eNum+2), NewHours, readEeprom(16*eNum+4)); //event
                    putsUart0(inputData);
                }
            }
            putsUart0("\n");
            AlarmTime();
        }

        else if(isCommand(&data, "water", 1))           // Sets the water level and writes the level into the EEPROM
        {
            valid = true;
            uint16_t volume = 0;
            volume = getFieldInteger(&data, 1);

            if(volume > 0)
            {
                writeEeprom((16*0)+6, volume);
                char waterlevel[70];
                snprintf(waterlevel, sizeof(waterlevel), "Water level regulation has been set to %d\n", volume);
                putsUart0(waterlevel);
            }
            else
            {
                writeEeprom((16*0)+6, 0x0);
            }
        }

        else if(isCommand(&data, "fill", 1))              // Sets the mode to be either AUTO or MOTION for the water to be filled.
        {
            valid = true;
            uint8_t modeFlag = 0;
            char* mode = getFieldString(&data, 1);

            if(mode != NULL && cmpStr(mode, "auto") == 0)
            {
                modeFlag = 1;
                putsUart0("Fill mode has been set to AUTO.\n");
            }
            else if(mode != NULL && cmpStr(mode, "motion") == 0)
            {
               modeFlag = 2;
               putsUart0("Fill mode has been set to MOTION.\n");
            }
            else
            {
                putsUart0("Please choose the mode as 'auto' or 'motion'\n");
            }

            writeEeprom((16*0)+ 7, modeFlag);
        }

        else if(isCommand(&data, "alert", 1))               // Sets the Alert mode to alert pet owner about low water alarm
        {
            valid = true;
            uint8_t lowWaterAlarm = 0;
            char* alertmode = getFieldString(&data, 1);

            if((alertmode != NULL && cmpStr(alertmode, "ON") == 0) || (alertmode != NULL && cmpStr(alertmode, "on") == 0))
            {
                lowWaterAlarm = 1;
                putsUart0("Alert mode has been turned ON\n");
            }

            if((alertmode != NULL && cmpStr(alertmode, "OFF") == 0) || (alertmode != NULL && cmpStr(alertmode, "off") == 0))
            {
                lowWaterAlarm = 0;
                putsUart0("Alert mode has been turned OFF\n");
            }
            writeEeprom((16*0)+ 8, lowWaterAlarm);
        }

        else if(isCommand(&data, "setting", 0))             // Displays the set water level, fill mode and alert mode
        {
            valid = true;
            uint16_t watervolume = readEeprom((16*0)+6);
            uint16_t fillmode = readEeprom((16*0)+7);
            uint16_t alertmode = readEeprom((16*0)+8);

            char lol[80];
            snprintf(lol, sizeof(lol), "Volume = %d ml\nFill Mode is %d\nAlert mode is %d\n", watervolume, fillmode, alertmode);
            putsUart0(lol);
        }

        if(!valid)                                          // Displayed if the user enters invalid commands.
        {
            putsUart0("Invalid Command. Please try again.\n");
        }

    }
}
