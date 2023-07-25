//Include required libraries
#include "WiFi.h"
#include <HTTPClient.h>
#include "time.h"

#include <Wire.h>
#include <Adafruit_BMP085.h>
#define seaLevelPressure_hPa 1013.25

Adafruit_BMP085 bmp;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

// WiFi credentials
const char* ssid = "Dialog 4G";         // change SSID PORT-ROUT
const char* password = "73F0D7D3";    // change password
// Google script ID and required credentials
String GOOGLE_SCRIPT_ID = "AKfycbzl7YcuTZig1-CiAYZiglAQIVa80r07l3qvpf2bJL_usc37mgwl4nzq59oh5PNMqCjbIA";    // change Gscript ID

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds) */

#include "SPIFFS.h"
#include <vector>
using namespace std;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void httprequest(String temperature, String humidity, String GOOGLE_SCRIPT_ID) {// this method create http reqests
  String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + "&sensor=" + temperature + "&humidity=" + humidity;
  //Serial.print("POST data to spreadsheet:");
  Serial.println(urlFinal);
  HTTPClient http;
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();

  String payload;
  if (httpCode > 0) {
    payload = http.getString();
   
  }
  //---------------------------------------------------------------------
  http.end();
}
void listAllFiles() {// this method remove file from SPIFF
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}
void setup() {
  delay(1000);
  Serial.begin(115200);
  
  if (!bmp.begin()) {
  Serial.println("BMP180 Not Found. CHECK CIRCUIT!");
  while (1) {}
  }
  
  SPIFFS.begin(true);
  char k = 0;
  char m = 0;
  char n = 0;
 // char p = 0;
  
  char t=0;
  char StartupEmptypoint;
  //char n = 80; //offline recorded array length
  char WifiConnectWaitingTime=25;
  
  String presarray[n];
  String seaLevelPresarray[n];
//  String altitudearray[n];
  
  delay(1000);

  // connect to WiFi
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.begin(ssid, password);


  float pres = bmp.readPressure();
  float seaLevelPres = bmp.readSealevelPressure();
 // float altitude = bmp.readAltitude();
 // float realAltitude = bmp.readAltitude(seaLevelPressure_hPa * 100);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    k++;
    if (k > WifiConnectWaitingTime) { // if waiting time is over, then start offline data logging
      Serial.println("\ndata recording Offline...");

      File presfile = SPIFFS.open("/pres.txt", FILE_WRITE);
      File seaLevelPressfile = SPIFFS.open("/seaLevelPres.txt", FILE_WRITE);
     // File altitudefile = SPIFFS.open("/altitude.txt", FILE_WRITE);
  
      presfile.println(String(pres));
      seaLevelPressfile.println(String(seaLevelPres));
     // altitudefile.println(String(altitude));
      
      presfile.close();
      seaLevelPressfile.close();
     // altitudefile.close();
      
      k = 0;
      break;// when process is done leave while() loop
    }

  }

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  print_wakeup_reason();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP)+" Seconds");

  if ( isnan(isnan(pres) || isnan(seaLevelPres))) {// check whether the reading is successful or not
    Serial.println("Failed to read from BPM sensor!");
  }

  if (WiFi.status() == WL_CONNECTED) {// when wifi is connected
    static bool flag = false;
    struct tm timeinfo;
//if there exists spiff files they should upload first to the google sheet
    if ((SPIFFS.exists("/pres.txt")) && (SPIFFS.exists("/seaLevelPres.txt"))) { 
   
      File presfile1 = SPIFFS.open("/pres.txt", FILE_WRITE);
      File seaLevelPressfile1 = SPIFFS.open("/seaLevelPres.txt", FILE_WRITE);
      //File altitudefile1 = SPIFFS.open("/altitude.txt", FILE_WRITE);
      
  
      vector<String> v3;
      vector<String> v4;
      vector<String> v5;

    

      while (presfile1.available()) {
        v3.push_back(presfile1.readStringUntil('\n'));
      }

      while (seaLevelPressfile1.available()) {
        v4.push_back(seaLevelPressfile1.readStringUntil('\n'));
      }

//      while (altitudefile1.available()) {
//        v5.push_back(altitudefile1.readStringUntil('\n'));
//      }

     
      presfile1.close();
      seaLevelPressfile1.close();
     // altitudefile1.close();
      
    

      for (String s3 : v3) {
        presarray[m] = s3;//retrieve temperature log data to array 
        m++;
      }

      for (String s4 : v4) {
        seaLevelPresarray[n] = s4;//retrieve humidity log data to array
        n++;
      }
//      for (String s5 : v5) {
//        altitudearray[p] = s5;//retrieve temperature log data to array 
//        p++;
//      }
      
      for (t = 0; t <= n; t++) {
        if(presarray[t]==0 && seaLevelPresarray[t]==0){//prevent 0 values uploading to the google sheets when arrays not completly write
            Serial.println("\nArrays are empty..");
            StartupEmptypoint=t;
            break;
          }else{
            Serial.println("offline data uploading... ");
            httprequest(presarray[t], seaLevelPresarray[t], GOOGLE_SCRIPT_ID);
          }  
      }
      if (t == n || t==StartupEmptypoint) {//remove two files after uploading log data to sheet
          Serial.println("\n\n---BEFORE REMOVING---");
          listAllFiles();
          
          SPIFFS.remove("/pres.txt");
          SPIFFS.remove("/seaLevelPres.txt");
         // SPIFFS.remove("/altitude.txt");

          Serial.println("\n\n---AFTER REMOVING---");
          listAllFiles();
        
          m = 0;
          n = 0;
          //p = 0;
        }
    }

    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }
    char timeStringBuff[50]; //50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    String asString(timeStringBuff);
    asString.replace(" ", "-");
    Serial.print("Time:");
    Serial.println(asString);

    httprequest( String(pres), String(seaLevelPres), GOOGLE_SCRIPT_ID);// upload current data to sheet

  }
  //count++;
  delay(1000);
  Serial.println("Going to sleep now");
  Serial.flush();
  esp_deep_sleep_start();// start deep sleep
}
void loop() {}