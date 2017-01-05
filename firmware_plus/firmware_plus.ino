/*            FIRMWARE NA ARUDINO NANO PRI PROJEKTU KRMILNIK LED RAZSVETLJAVE

Avtor: Nejc Deželak
Verzija: 1.0, 30.10.2015
      Prva verzija vključuje naslednje funkcionalnosti:
                                              -Konfiguracijo ATMEGA mikrokrmilnika, ki se nahaja na Arduino NANO
                                              -Konfiguracijo ESP WI-FI modula (Konfiguracija Access Point-a)
                                              -Zmožnost čakanja na HTTP request, njegovo tolmačenje in zaprtje vzpostavljene povezave
                                              -Zmožnost detektiranja napake in prehod v stanje reset.

     Datoteka se deli na tri dele:
                ESP_MAIN-> Tu je zgrajen končni avtomat, ki je pomemben za komunikacijo z ESP modulom. Prav tako je tu implementirano krmiljenje LED traku s PWM.
                ESP_functions-> Tu so implementirane vse funkcije, ki direktno pošiljajo AT ukaze preko UART na ESP modul
                ESP_basicfunctions-> Tu so implementirane funkcije za branje bufferja in primerjanje dveh polj.

Verzija 1.1, 23.12.2015
      -Odpravljena napaka ob hardware RESETU
       -Zmanjšano čakanje ob operacijah
       -Nov koncept s pomočjo hard interrupt-a na serijskem vmesniku. 
       -Prav tako asinhron prehod v stanja FSM s pomočjo newMsg in newHTTP
       -PWM na izhodih se konfigurira zgolj ob spremembi nastavitev.
Verzija 1.2, 28.12.2015
      -Dodal linearizacijo svetlosti LED traku

Verzija 1.2.1, 22.12.2016
      - Higher Baudrate used
      - Some AT commands have been updated to the new version
Version 1.2.2 27.12.2016
      - Low level control over the PWM outputs
      - Custom delay function (uses  empty processor cycles instead)
*/
//#include <TimerOne.h>
//#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
//spremenljivke za konfiguracijo časovnika in njegove prekinitve
#define pol     500000
#define ena     1000000
#define dva     2000000
#define stiri   4000000
#define DELAY   5000 //Delay in ms for all the functions.
#define INT uint32_t
//SoftwareSerial ESP(2, 4); // RX, TX


//---------- GLOBALNE SPREMENLJIVKE --------------------------------------//
String input; //buffer za komunikacijo
char ok[3]="OK";
char http[5]="+IPD";
char stat[12]="+CIPSTATUS:";
char closed[8]=",CLOSED";
char unlink[7]="UNLINK";
int count=0; //števec pri poizkusih konfiguracije
int channel=5;
int M=0;
int R=0;
int Z=0;
boolean stikalo=false;
int offset=0;
char buf[200];
int stevec=0;
boolean newHTTP=false;
boolean newMsg=false;
boolean attempt=false;
boolean program = true;
INT interrupt_count = 0;



//----------- ZASTAVICE KONČNEGA AVTOMATA ---------------------------------------------//
boolean AP_flag=false; //dovoljenje za konfiguracijo AP
boolean RST_flag=true; //dovoljenje za reset
boolean SSID_flag=false;
boolean MUX_flag=false;
boolean WEB_flag=false;
boolean WAIT_flag=false;
boolean READ_flag=false;
boolean ANSWER_flag=false;
boolean STATUS_flag=false;
boolean CLOSEWEB_flag=false;
boolean CLOSE_flag=false;

boolean CLOSE_answer=false;
boolean STATUS_answer=false;
boolean MUX_answer=false;
boolean WEB_answer=false;
boolean SSID_answer=false;
boolean AP_answer=false; //odgovor ESP na konfiguracijo AP
boolean RST_answer=false;  // odgovor ESP na reset

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
 // Initialize string for saving the server response   
        input="";
        input.reserve(2000);
 // Initialize IOs
        pinMode(13,OUTPUT);
        pinMode(3,OUTPUT);
        pinMode(6,OUTPUT);
        pinMode(10,OUTPUT);
        pinMode(9,OUTPUT);
        pinMode(11,OUTPUT);
        pinMode(5,OUTPUT);
        
// Initialize timers
        TCCR0A = (1<<COM0A1) | (1<<WGM00);//*| (1<<WGM01)*/ | 
        TCCR0B = (1<<CS00);
        TCCR1A = (1<<COM1B1) | (1<<WGM10);
        TCCR1B = (1<<CS10);// | (1<<WGM12);
        TCCR2A = (1<<COM2B1) | (1<<WGM20); //*| (1<<WGM21) */
        TCCR2B = (1<<CS20);

        // enable timer1 overflow interrupt. Flag is set at BOTTOM in PWM phase correct mode.
        TIMSK1 = (1 << TOIE1);


// Initialize UART
        Serial.begin(115200);
    
        delay_custom(1000000); //Počakaj, da se vzpostavi tudi ESP 
        // Set Server Timeout time to minimum (as connections are actually omnidirectional)
        Serial.print("AT+CIPSTO=1\r\n");
         /*TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
         TCCR2B = _BV(WGM22) | _BV(CS22);
         OCR2A = 180;
         OCR2B = 50;*/
// enable interrupts
        sei();
// start colors
      M = 50;
      R = 50;

}
//***********************************************************************************************************************************************************************************
void loop() { // run over and over


      //******** RESET MODULA *********
                 if(RST_flag){ //Ko si na vrsti za reset
                    //Serial.println(F("Si v funkciji RESET"));
                          ESP_reset();                        
                          RST_flag=false;
                          AP_flag=true;
                          
                  }

                 
      //***** SOFT AP KONFIGURACIJA ***********************
            if(AP_flag){ //če si na vrsti za konfiguracijo softAP
              // Serial.println("AP_FLAG=1");
                ESP_AP();
                AP_flag=false;
                AP_answer=true;
            }
  
            if(AP_answer && (!Serial.available())&& newMsg){ // ko dobiš odgovor
              newMsg=false;
             // Serial.println(F("DOBIL SI ODGOVOR PRI AP"));
                    if(search_buffer(ok, sizeof(ok))==true){                  
                        SSID_flag=true;
                        count=0;
               }
                    else { //If answer OK was not found inside the buffer
                
                        if(count<3){            
                            count++;
                            AP_flag=true;
                        } 

                        else{ // Pojdi v napako, če si že več kot trikrat poizkusil nastaviti ESP v soft Access point
                             error();
             
                         }

              }  
              //Izprazni buffer in izbriši zastavico
              input="";
              AP_answer=false; 
          
  }
    //*****************

    //******** SSID konfiguracija **************
    if(SSID_flag){
      //Serial.println(F("SSID_flag=1"));
      ESP_SSID();
      SSID_flag=false;
      SSID_answer=true;
    }

    if(SSID_answer && (!Serial.available()) && newMsg ){
      newMsg=false;
     // Serial.println(F("Dobil si odgovor pri SSID"));
                    if(search_buffer(ok, sizeof(ok) )==true){
                     
                        count=0;
                        MUX_flag=true;
                   }
                   
                    else {
                
                          if(count<3){
                           
                            count++;
                            SSID_flag=true;
                          }

                        else{
                          error();
             
                        }

                    }  

                    
              input="";
              SSID_answer=false; 
      
    }
      //*********CIP MUX********************************

          if(MUX_flag){
          // Serial.println(F("Si v CIPMUX"));
          ESP_MUX();
          MUX_flag=false;
          MUX_answer=true;  
          }

        if(MUX_answer && (!Serial.available())  && newMsg ){
          newMsg=false;
          //Serial.println(F("Dobil si odgovor na CIPMUX"));
                    if(search_buffer(ok, sizeof(ok))==true){
                       
                        count=0;
                        WEB_flag=true;
                     }
                    else {
                
                          if(count<3){
                             
                              count++;
                              MUX_flag=true;
                          }

                          else{
                              error();
             
                          }

                        }  
              input="";
              MUX_answer=false;        
        }
      //************************************************




      //****** VZPOSTAVITEV WEBSERVERJA *****************
      if(WEB_flag){
        //Serial.println(F("Si v WEB"));
        ESP_WEB();
        WEB_flag=false;
        WEB_answer=true;  
      }

        if(WEB_answer && (!Serial.available()) && newMsg ){
          newMsg=false;
          //Serial.println(F("Dobil si odgovor na WEBSERVER"));
                    if(search_buffer(ok, sizeof(ok))==true){
                      
                        count=0;
                        WAIT_flag=true;
                     }
                    else {
                
                          if(count<3){
                            
                              count++;
                              WEB_flag=true;
                          }

                          else{
                              error();
             
                          }

                        }  
              input="";
              WEB_answer=false;        
        }
      //**************************************************************







      //****** WAIT ******************************************
            if(  WAIT_flag   &&   newMsg && (!Serial.available())  ){ //Čakanje na HTTP request 
            //Serial.println(F("Si v WAIT_flag"));
            newMsg=false;
            if( newHTTP){ //Če si dobil HTTP request (se vidi iz +IPD v sporočilu)
               ESP_READ();
               CLOSE_flag=true;
               WAIT_flag=false;
               newHTTP=false; 
               LED_drive(); //Spremeni duty cycle PWM izhodov
            }
            
              input="";
            
            
            }
       



          if(READ_flag){
            
           ESP_READ();            
                CLOSE_flag=true;
           READ_flag=false;
           input="";
          }

          if(CLOSE_flag){
  
              ESP_CLOSE();
              CLOSE_flag=false;
              CLOSE_answer=true;
              //input="";
            
          }

        if(CLOSE_answer && (!Serial.available()) && newMsg   ){
          newMsg=false;
             if(newHTTP){ //Če vmes dobiš nov HTTP request, zapusti zapiranje in raje preberi nov request
                CLOSE_flag=false;
                CLOSE_answer=false;
                STATUS_flag=false;
                WAIT_flag=true;
             }
             else{ //Če si dobil navaden odgovor
                             if(search_buffer(closed,sizeof(closed))==true ||  search_buffer(unlink,sizeof(unlink))==true ){ //Preveri uspesnost.
                                    CLOSE_answer=false;
                                    STATUS_flag=true;
             
                              }

                              else { //Če nisi bil uspešen pri zapiranju
                                    CLOSE_flag=true;
                                    CLOSE_answer=false;
                                    count++;
                                                        if(count>3){
                                                                    count=0;
                                                                    CLOSE_flag=false;
                                                                    WAIT_flag=true; 
                                                                               
                                                       }
             
                            }
                   
                  input="";   //Izprazni buffer
                             
            }
     
         


        }






      

          if(STATUS_flag){
             ESP_STATUS();
             STATUS_flag=false;
             STATUS_answer=true;
          }





          if(        STATUS_answer==true && (!Serial.available() )  && newMsg  ){
            newMsg=false;
            
             if(newHTTP){
                            WAIT_flag=true;
                            STATUS_answer=false;
             }
             else{
                        if( !ESP_READSTATUS() ){ // Check if status is DISCONNECTED
                                        WAIT_flag=true; 
                
                        }
                        else { //If still connected, try another channel
                
                                     
                                         CLOSE_flag=true; //zapri kanal, ki je še vedno odprt.
                                         channel++; //pošlji na naslednji kanal
                                                      if(channel>=5) {
                                                        channel=0;
                                                        attempt=true;
                                                      }
                                                      if(channel>=5 && attempt==true){
                                                        attempt=false;
                                                        WAIT_flag=true;
                                                      }
                              }

     
               //Še enkrat preveri, če buffer nima slučajno novo HTTP sporočilo
             input=""; //Izprazni buffer
              
              STATUS_answer=false;
            }

   }
           





 // LED_drive(); //Spremeni nastavitve
 //myserialEvent();



}//LOOP
//**********************************************************************************************************************************************************************



//************ UART INTERRUPT************************************
void serialEvent(){
  // As long as new data is available in the UART RX BUFFER
  while(Serial.available()){ 
      input+=(char)Serial.read();
      delay_custom(2000);
  }
  // Check if the received message is a HTTP response
   if(search_buffer(http,sizeof(http)) ){ 
         newHTTP=true;
   }
   // Global flag that a new message has been received
    newMsg=true;    
   
}
//******************************************************************

//**************LED DRIVER FUNCTION*******************************
void LED_drive(){ 
   if(stikalo==1 || program == 1){
/*
//Koeficienti parabole, ki izhaja iz linearizacije tokovne karakteristike LED diode.
    //Rdeca barva
    float A_r=111.1;
    float B_r=-172.2;

    //Koeficient parabole tokovne karakteristike
    float a_r=7.3326;
    float b_r=-8.352;
    
    //Modra in zelena barva
    float A_b=52.7;
    float B_b=-149.75;

    //Koeficienti parabole tokovne karakteristike
    float a_b=4.743;
    float b_b=-12.775;

  

//Rešitev kvadratne enačbe za napetost na diodi
    float u_rdeca=(-B_r+sqrt(B_r*B_r+4.0*A_r*R) )  /    (2.0*A_r);
    float u_modra=(-B_b+sqrt(B_b*B_b+4.0*A_b*M) )  /    (2.0*A_b);
    float u_zelena=(-B_b+sqrt(B_b*B_b+4.0*A_b*Z) )  /    (2.0*A_b);

//Izračun potrebne napajalne napetosto posamezne barve
   float rdeca=a_r*u_rdeca*u_rdeca+b_r*u_rdeca;
    float modra=a_b*u_modra*u_modra+b_b*u_modra;
    float zelena=a_b*u_zelena*u_zelena+b_b*u_zelena;

//Izračun duty cycle za PWM
     rdeca=(rdeca)*(255.0/12.0);
     modra=(modra)*(255.0/12.0);
     zelena=(zelena)*(255.0/12.0); 
*/
// S - curve model for more linear driving of the LEDs
      float duty_red = (float(R)/100) * 255;
      float duty_blue = (float(M)/100) * 255;
      float duty_green = (float(Z)/100) * 255;
    /* 
     float rdeca= 255 * (1 / (1 + exponent(-1 * ( (duty_red/21) - 6)    ) ) );
     float modra= 255 * (1 / (1 + exponent(-1 * ( (duty_blue/21) - 6)    ) ) );
     float zelena= 255 * (1 / (1 + exponent(-1 * ( (duty_green/21) - 6)    ) ) );
   */
     int rdeca = lookup_table[int(duty_red)];
     int modra = lookup_table[int(duty_blue)];
     int zelena = lookup_table[int(duty_green)];

     
     /*Serial.print("RED:");
     Serial.println(rdeca);
     Serial.print("GREEN:");
     Serial.println(zelena);
     */
     if(M<=0)modra=0;
     if(R<=0)rdeca=0;
     if(Z<=0)zelena=0;
     
     if(modra>255)modra=255;
     if(rdeca>255)rdeca=255;
     if(zelena>255)zelena=255;
   
    /*
    float modra=2.55*M;
    float zelena=2.55*Z;
    float rdeca=2.55*R;
*/

    //analogWrite(3,(int)zelena);//zelena
    // D3 corresponds to pin 0C2B
    OCR2B=(int)zelena;
    
    //analogWrite(6,(int)rdeca); //rdeca
    // D6 corresponds to pin OC0A
    OCR0A = (int)rdeca;

    
    //analogWrite(10,(int)modra); //modra
    // D10 corresponds to OC1B
    OCR1BL = int(modra);

    
   }
   else{

    /*analogWrite(3,0);
    analogWrite(6,0);
    analogWrite(10,0);
*/
  OCR2B = 0;
  OCR0A=0;
  OCR1BL=0;

   }
}

//******************UTRIPANJE LED OB NAPAKI******************************+++
void error(){
  Serial.println("ERROR! RESET MODUL!");
  while(1){
  digitalWrite(13,HIGH);
  delay_custom(500000);
  digitalWrite(13,LOW);
  delay_custom(500000);
  }
}
//**********************************************************************
// This interrupt is triggered every 512 timer clock_cylcles
// Currently a clock_cycle is 62,5 ns. 
ISR(TIMER1_OVF_vect){
  interrupt_count++;
 
  if (interrupt_count > 2000){
      digitalWrite(13,HIGH);
      interrupt_count = 0;
      if(program){
        fade_lights(&R,&Z,&M);
        LED_drive();
      }
  }
  
}



////*******************************INTERRUPT TIMER ROUTINES***********************************************************************
//void timer_AP(){
//  Timer1.detachInterrupt();
//  ESP.flush();
//  AP_answer=true;
//}
//
//void timer_RST(){
//  Timer1.detachInterrupt();
//  ESP.flush();
//  RST_answer=true;
//}
//
//void timer_SSID(){
// Timer1.detachInterrupt();
// ESP.flush();
// SSID_answer=true;
//}
//
////*************************************************************************************************************************
