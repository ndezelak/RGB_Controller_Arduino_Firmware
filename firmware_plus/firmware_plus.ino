/*            FIRMWARE SIMPLER VERSION (NO ESP INTERFACE)

Author: Nejc Deželak
Initial version date: 06.01.2017

Changelog:

06/01/17: 
          -Cleaned up the code for the "kitchen" version.   
          - Added two programs that are time dependant 
*/
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

// **** MODULE SETTINGS **** //
#define DELAY   5000 //Delay in ms for all the functions.
#define INT uint32_t
#define LONG uint64_t
#define CLOCK_PERIOD 0.0000000625
#define NUMBER_CLOCK_CYCLES 512
#define SMALL_TIME_STEP  1.0  // In seconds  
#define LARGE_TIME_STEP  10.0 * 60.0 // In seconds
#define FADE_PERIOD 10.0*60.0 // In seconds

// Global variables 
int M=0;
int R=0;
int Z=0;
boolean program = false;
boolean normal_light = true;
boolean led_drive_flag = true;
INT interrupt_count = 0;
LONG interrupt_count_large = 0;
int MAX_VALUE = 100;

int lookup_table [256] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,\
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,\
0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05,\
0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0B, 0x0B,\
0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0x10, 0x11, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,\
0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x20, 0x21, 0x23, 0x24, 0x26, 0x27, 0x29, 0x2B, 0x2C,\
0x2E, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E, 0x40, 0x43, 0x45, 0x47, 0x4A, 0x4C, 0x4F,\
0x51, 0x54, 0x57, 0x59, 0x5C, 0x5F, 0x62, 0x64, 0x67, 0x6A, 0x6D, 0x70, 0x73, 0x76, 0x79, 0x7C,\
0x7F, 0x82, 0x85, 0x88, 0x8B, 0x8E, 0x91, 0x94, 0x97, 0x9A, 0x9C, 0x9F, 0xA2, 0xA5, 0xA7, 0xAA,\
0xAD, 0xAF, 0xB2, 0xB4, 0xB7, 0xB9, 0xBB, 0xBE, 0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,\
0xD0, 0xD2, 0xD3, 0xD5, 0xD7, 0xD8, 0xDA, 0xDB, 0xDD, 0xDE, 0xDF, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5,\
0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xED, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 0xF1, 0xF2,\
0xF2, 0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8,\
0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFC,\
0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD,\
0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF};
//****************************************************************
void setup() {
 // Initialize IOs for RGB 
        pinMode(13,OUTPUT);
        pinMode(3,OUTPUT);
        pinMode(6,OUTPUT);
        pinMode(10,OUTPUT);
        pinMode(9,OUTPUT);
        pinMode(11,OUTPUT);
        pinMode(5,OUTPUT);
        
// Initialize timers for PWM
        TCCR0A = (1<<COM0A1) | (1<<WGM00);//*| (1<<WGM01)*/ | 
        TCCR0B = (1<<CS00);
        TCCR1A = (1<<COM1B1) | (1<<WGM10);
        TCCR1B = (1<<CS10);// | (1<<WGM12);
        TCCR2A = (1<<COM2B1) | (1<<WGM20); //*| (1<<WGM21) */
        TCCR2B = (1<<CS20);

// Enable timer1 overflow interrupt. Flag is set at BOTTOM in PWM phase correct mode.
        TIMSK1 = (1 << TOIE1);
        
// Enable global interrupts
        sei();
// Start colors
        M=100;
        R=100;
        Z=100;

}

void loop() {
        // Activate the LED drive function if a change of PWM settings has been requested
        if(led_drive_flag){
          led_drive_flag = false;
          LED_drive();
        }
}


//**************LED DRIVER FUNCTION*******************************
void LED_drive(){ 

// Get lookup table inputs
      float duty_red = (float(R)/100) * 255;
      float duty_blue = (float(M)/100) * 255;
      float duty_green = (float(Z)/100) * 255;
// Read the lookup table
     int rdeca = lookup_table[int(duty_red)];
     int modra = lookup_table[int(duty_blue)];
     int zelena = lookup_table[int(duty_green)];
// Limit checks
     if(M<=0)modra=0;
     if(R<=0)rdeca=0;
     if(Z<=0)zelena=0;  
     if(modra>255)modra=255;
     if(rdeca>255)rdeca=255;
     if(zelena>255)zelena=255;

// Set the output compare registers of all timers
      // D3 corresponds to pin 0C2B
      OCR2B=(int)zelena;

      // D6 corresponds to pin OC0A
       OCR0A = (int)rdeca;
      // D10 corresponds to OC1B
      OCR1BL = int(modra);
}

//**************      TIMER INTERRUPT  ******************
// This interrupt is triggered every 512 timer clock_cycles
// Currently a clock_cycle is 62,5 ns. 
ISR(TIMER1_OVF_vect){
  interrupt_count++; 
  // Smaller cycle happens here
  if (interrupt_count > (int)(SMALL_TIME_STEP/(CLOCK_PERIOD*NUMBER_CLOCK_CYCLES)) ){
      interrupt_count_large++;
      interrupt_count = 0;
      if(program){
        // Lower the maximum value so that the lights fade out
        if (interrupt_count_large > (int) (100/MAX_VALUE)*( (FADE_PERIOD/(CLOCK_PERIOD*NUMBER_CLOCK_CYCLES))\
                                            /(SMALL_TIME_STEP/(CLOCK_PERIOD*NUMBER_CLOCK_CYCLES)) )   ) 
         {
             MAX_VALUE-=1;
             if (MAX_VALUE < 0) MAX_VALUE=0;                                                             
             interrupt_count_large = 0;                               
         }
                                            
        fade_lights(&R,&Z,&M);
        led_drive_flag = true;
      }
      else if (normal_light){
        M=100;
        R=100;
        Z=100;
        led_drive_flag = true;
        // Change to RGB program after the large cycle has ended
        if (interrupt_count_large > (int) ( (LARGE_TIME_STEP/(CLOCK_PERIOD*NUMBER_CLOCK_CYCLES))\
                                            /(SMALL_TIME_STEP/(CLOCK_PERIOD*NUMBER_CLOCK_CYCLES)) )                                                       ) {
                                              
               normal_light = false;
               program = true;
               M=50;
               R=50;
               Z=0;
               interrupt_count_large = 0;
        }
      }
  }
  
}

