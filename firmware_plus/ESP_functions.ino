
void ESP_AP(){
  Serial.print("AT+CWMODE_DEF=2\r\n");
 
//  Timer1.restart();
//  Timer1.attachInterrupt(timer_AP,dva); BOŠ UPORABIL V KONČNI VERZIJI!
delay_custom(DELAY); 
//Serial.println("Končal sem s timerjem");
}

void ESP_reset(){
  Serial.print("AT+RST\r\n"); //pošlji ukaz reset
   //Serial.println("Resetiram modul ...");
//   Timer1.restart();
//   Timer1.attachInterrupt(timer_RST,stiri);  
delay_custom(100000);
/*Serial.end();
delay(500);
Serial.begin(115200);
                           
Serial.print("AT+CIOBAUD=9600\r\n");
                            delay(500);
                           Serial.end();
                           delay(500);
                            Serial.begin(9600);
*/
//Serial.println("Končal sem s timerjem");
}

void ESP_SSID(){
  
 Serial.print("AT+CWSAP_CUR=\"LED_ANJA\",\"\",1,0\r\n");
  
//  Timer1.restart();
//  Timer1.attachInterrupt(timer_SSID,dva);
delay_custom(DELAY);
//Serial.println("Končal sem s timerjem");
}

void ESP_MUX(){
 
Serial.print("AT+CIPMUX=1\r\n");
delay_custom(DELAY);;
}

void ESP_WEB(){
Serial.print("AT+CIPSERVER=1,80\r\n");

delay_custom(DELAY);
}


boolean ESP_WAIT(){
  if(search_buffer(http, sizeof(http) ) == true ) return true; //če je v dogovoru +IPD, potem true
  else return false;


}


void ESP_READ(){
  
           offset=0;
          
          search_buffer(http,sizeof(http) ) ;  //Preberi sporočilo. Pri tem nastaviš kazalec offset, ki ga uporabiš kasneje.
            stevec++;
                if(stevec %2 ){
                    digitalWrite(13,HIGH);
                }
                else{
                    digitalWrite(13,LOW);
                }
          channel=buf[offset+sizeof(http)]-'0';     
          M=(buf[offset+sizeof(http)+15]-'0')*10+(buf[offset+sizeof(http)+16]-'0');
          Z=(buf[offset+sizeof(http)+19]-'0')*10+(buf[offset+sizeof(http)+20]-'0');
          R=(buf[offset+sizeof(http)+22]-'0')*10+(buf[offset+sizeof(http)+23]-'0');
          stikalo=(buf[offset+sizeof(http)+26]-'0');
          program = 0;
          //return true;
           //}
/*
           else{
          
           return false; 
  
            }*/
}
/*
void ESP_ANSWER(){ //Info message when you access the IP address over a browser

  
String string1="AT+CIPSEND=";
String string2=",80\r\n";
String ukaz;
ukaz=string1+channel+string2;
Serial.print(ukaz);
delay(DELAY);
Serial.print("<!DOCTYPE html><html><body><h1>To je stran LED_ANJA!</h1><p>Dobrodosel!</p></body></html>");
delay(DELAY);
Serial.flush();


}

*/

void ESP_STATUS(){
Serial.print("AT+CIPSTATUS\r\n");
delay_custom(DELAY);
}



boolean ESP_READSTATUS(){
 
return search_buffer(stat, sizeof(stat));

}

/*
void ESP_CLOSEWEB(){

 Serial.print("AT+CIPSERVER=0\r\n");
  delay(DELAY);
}
*/

void ESP_CLOSE(){
 
  String ukaz="AT+CIPCLOSE=";
  String konec="\r\n";
  ukaz+=channel+konec;
  Serial.print(ukaz);
delay_custom(DELAY);
}
