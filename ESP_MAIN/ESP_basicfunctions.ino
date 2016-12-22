///////////////////FUNKCIJA ZA BRANJE ODGOVORA/////////////////////////////////////////////////////////




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
