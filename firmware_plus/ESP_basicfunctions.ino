
/*
//****************************************************************************************
// Funkcija za iskanje željene besede, ki je vhod v funkcijo, znotraj globalnega stringa input
boolean search_buffer(char beseda[], int dolzina){
  //---------------------------------------------------
  // buf is an array of char, input is a String class 
           memset(buf,0,sizeof(buf)); // Napolni polje z ničlami  
          input.toCharArray(buf,200);  // Kopiraj vsebino Stringa input v buffer
  
          char local_bufer[dolzina-1]; //Pomožni lokalni buffer
          memset(local_bufer,0,sizeof(local_bufer)); //Napolni polje z ničlami

        
          int i; //Števec za zanke
          int length=sizeof(buf); // Dolžina lokalnega bufferja
          
 //------------------------------------------------------------



 if(sizeof(beseda)>length) { //Če je dolžina besede večja kot velikost lokalnega bufferja javi napako
  offset=0;
  return false;
 }

//------------------------------------------------------------------------------------
 //Napolni pomožni buffer z vsebino bufferja
 for(i=0;i<sizeof(local_bufer) ;i++){
  local_bufer[i]=buf[i];
 }
//------------------------------------------------------------------------------------
 // Search for the keyword inside within the buffer using the equal_array function
 while(1){ 
     // You found the keyword
     if(  equal_array(local_bufer, beseda, dolzina) == true )
     {  
              return true;
     }
     // You have reached the end of the global buffer therefor you didn't found the keyword
     if(      offset>= (length-sizeof(local_bufer) )     ) {
             offset=0; //reset offset   
             return false; // če si prišel do konca bufferja          
     }  
     // Keep going further
     offset++; 
     // Shift for one charachter further
     for(     i=0    ;   i   <  sizeof(local_bufer)  ;    i++){ //Napolni pomožni buffer z naslednjim odsekom bufferja
          local_bufer[i]=buf[i+offset];
          }   
 }
}
//*******************************************************************************
// Funkcija, ki primerja dva polja in vrne TRUE v primeru, da sta enaka in FALSE, če sta si različna
boolean equal_array(char char1[],char char2[], int dolzina){

//Preveri, če sta polji sploh enako dolgi
 if(sizeof(char1)  !=    sizeof(char2) ){
  
  return false;
 }



 
 for(int i=0;i< (dolzina-1)  ;i++){
        
        if(char1[i]!= char2[i]){ //Če prideš do znaka, ki je različen od znaka v drugem polju, vrni FALSE
   
          return false;
        }

 }
 
 return true;
}

*/
// ************ DELAY FUNCTION WITHOUT USING A TIMER ****************** //
void delay_custom(long micros){
    // Corresponds to a frequency of 16MHz
    const int freq = 16;
    unsigned long steps =(micros);
    unsigned long i;
    for (i = 0; i<steps; i++){
       asm("NOP");
    }
}
//***********************************************************************************
void fade_lights(int* current_red, int* current_green, int* current_blue){
      int inc = 1;   
// Determine which colors are active **************************
      // red rising, green falling
      if (*current_blue == 0){
       
        *current_red+=inc;
        *current_green-=inc;

      // check for limits
         if (*current_red > MAX_VALUE){
              *current_red = MAX_VALUE;
              
         }
         if (*current_green <= 0){
              *current_green = 0;
              *current_blue = 1;
         }

       // blue falling, green rising
      }
      else if (*current_red == 0){
      

        *current_green+=inc;
        *current_blue-=inc;
           // check for limits
         if (*current_green > MAX_VALUE){
              *current_green = MAX_VALUE;
              
         }
        
         if (*current_blue <= 0){
              *current_blue = 0;
              *current_red = 1;
         }
        
        // blue rising, red falling 
      }
      else{
       

        *current_blue+=inc;
        *current_red-=inc;
           // check for limits
         if (*current_blue > MAX_VALUE){
              *current_blue = MAX_VALUE;
              
         }
       
         if (*current_red <= 0){
              *current_red = 0;
              *current_green = 1;
         }
      }

}
/*
float exponent(float input){
    float out = 1 + input + (input*input)/2 + (input*input*input)/6 + (input*input*input*input)/24;
    return out;  
}
*/
