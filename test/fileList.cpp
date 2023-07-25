#include "SPIFFS.h"
 
void listAllFiles(){
 
  File root = SPIFFS.open("/");
 
  File file = root.openNextFile();
 
  while(file){
 
      Serial.print("FILE: ");
      Serial.println(file.name());
 
      file = root.openNextFile();
  }
 
}
 
void setup() {
 
  Serial.begin(115200);
 
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
 
  
 
  
 
  Serial.println("\n\n---BEFORE REMOVING---");
  listAllFiles();
 
  
  File file = SPIFFS.open("/28.txt",FILE_WRITE);
  file.println("ri,ui,ui,");

    file = SPIFFS.open("/29.txt",FILE_WRITE);
  file.println("ri,ui,ui,");

   file = SPIFFS.open("/30.txt",FILE_WRITE);
  file.println("ri,ui,ui,");
 
  Serial.println("\n\n---AFTER REMOVING---");
  listAllFiles();
 
}
 
void loop() {}