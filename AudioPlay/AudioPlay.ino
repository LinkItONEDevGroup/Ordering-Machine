#include <LAudio.h>

void setup() {
  LAudio.begin();
  Serial.begin(115200);
}

void loop() {
   AudioStatus status;
   LAudio.setVolume(6);
   LAudio.playFile(storageFlash,(char*)"batcave.mp3");
   Serial.println("playing");
   do{
     status = LAudio.getStatus();
     Serial.println(status);
     /*if(status == AudioEndOfFile){
       break;
     }*/
   }while(status != AudioEndOfFile);
   Serial.println("getting out");
   delay(1000);
}

