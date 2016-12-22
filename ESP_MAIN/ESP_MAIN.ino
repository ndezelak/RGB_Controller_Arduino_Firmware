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

*/







//#include <TimerOne.h>
//#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

//spremenljivke za konfiguracijo časovnika in njegove prekinitve
#define pol     500000
#define ena     1000000
#define dva     2000000
#define stiri   4000000
#define DELAY   5 //Delay in ms for all the functions.
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
boolean attempt=false; //spremenljivka, ki signalizira ali poizkušaš že drugič zapreti isti kanal.



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
        
        //Timer1.initialize(stiri);
// Initialize UART
        Serial.begin(115200);
        
        delay(100); //Počakaj, da se vzpostavi tudi ESP 
       
         /*TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
         TCCR2B = _BV(WGM22) | _BV(CS22);
         OCR2A = 180;
         OCR2B = 50;*/

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
        //***************************************************



        //************READ CHANNEL*********************************
        /* V to stanje prideš lahko zgolj iz WAIT funkcije. 
          if(READ_flag){
            
           ESP_READ();            
                CLOSE_flag=true;
           READ_flag=false;
           input="";
          }

        //**********ANSWER TO THE CHANNEL***************************
          if(ANSWER_flag){

          ESP_ANSWER();
          ANSWER_flag=false;
          CLOSE_flag=true;
          
          }



        */  
          
/*
          //*******CLOSE WEB***************************
            if(CLOSEWEB_flag &&   (!Serial.available()) ){
                  ESP_CLOSEWEB();
                  CLOSEWEB_flag=false;
                  WEB_flag=true;


                  input="";
            }
*/
//***************CLOSE TCP CONNECTION********************************************
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






      
        //************CHECK CIPSTATUS******************************
          if(STATUS_flag){
             ESP_STATUS();
             STATUS_flag=false;
             STATUS_answer=true;
          }





        //********STATUS ANSWER**********************************
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
           
//*********************************************************************************************




 // LED_drive(); //Spremeni nastavitve
 //myserialEvent();



}//LOOP
//**********************************************************************************************************************************************************************



//************ UART INTERRUPT************************************
void serialEvent(){
  // As long as new data is available in the UART RX BUFFER
  while(Serial.available()){ 
      input+=(char)Serial.read();
      delay(2);
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
   if(stikalo==1){


    
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

     if(M==0)modra=0;
     if(R==0)rdeca=0;
     if(Z==0)zelena=0;
     if(modra>255)modra=255;
     if(rdeca>255)rdeca=255;
     if(zelena>255)zelena=255;
     
    
    /*
    float modra=2.55*M;
    float zelena=2.55*Z;
    float rdeca=2.55*R;
*/
    
    analogWrite(3,(int)zelena);//zelena
    analogWrite(6,(int)rdeca); //rdeca
    analogWrite(10,(int)modra); //modra
   }
   else{
    analogWrite(3,0);
    analogWrite(6,0);
    analogWrite(10,0);
   }
}

//******************UTRIPANJE LED OB NAPAKI******************************+++
void error(){
  Serial.println("ERROR! RESET MODUL!");
  while(1){
  digitalWrite(13,HIGH);
  delay(500);
  digitalWrite(13,LOW);
  delay(500);
  }
}
//**********************************************************************




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
