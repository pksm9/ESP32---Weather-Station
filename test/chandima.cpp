#include "WiFi.h"
#include <HTTPClient.h>
#include "time.h"
#include "DHT.h" // include the DHT library

#include <Wire.h> // library for BMP180 Sensor
#include <Adafruit_BMP085.h> // library for BMP180 Sensor

#define DHTPIN 4
#define DHT_MOD_TYPE DHT11 
#define LDRPIN 36 // define LDR pin is GPIO36
#define rainAnalogPin 35
#define rainDigitalPin 34

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;

// WiFi credentials
const char* ssid = "Redmi Note 8";         // change SSID
const char* password = "12345678";    // change password


// Google script ID and required credentials
String GOOGLE_SCRIPT_ID = "AKfycby87NBaa55-8paHMnfRA_YWFyuLzbf4VDs8OP_9cso9bg-WaD07GC3nxFgDHu3xUkV2nw";    // change Gscript ID

int count = 0;

DHT dht11(DHTPIN,DHT_MOD_TYPE);

Adafruit_BMP085 bmp; //BMP180 sensor

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  // connect to WiFi
 
  dht11.begin();
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  //for BMP if not connected
  if(!bmp.begin()){
    Serial.println("Could not find the BMP180 sensor, check connections!!!");
    while(1){}
    }

   //Rain Sensor
   pinMode(rainDigitalPin, INPUT);

  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

     //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  

}

void loop() {
   delay(2000);
    float dhtHumidity = dht11.readHumidity();
    float dhtTemperature = dht11.readTemperature();
    float bmpPressure = bmp.readPressure();

   //check the any reads fail to read, then try again
   if(isnan(dhtHumidity) || isnan(dhtTemperature)){
    Serial.println(F("Failed read DHT11 sensor!!!"));
    return;
    }

   Serial.print(F("Humidity: "));
   Serial.println(dhtHumidity);
   Serial.print(F("Temperature: "));
   Serial.print(dhtTemperature);
   Serial.println(F("°C "));

    //print serial monitor the pressure values
    Serial.print("Pressure = ");
    Serial.print(bmpPressure);
    Serial.println(" Pa");

    //LDR sensor data
    int analogValueLDR = analogRead(LDRPIN);
    Serial.print("Analog value of LDR = ");
    Serial.println(analogValueLDR);
    //====================================================
    //=======================================================
    //must calibrate LDR============+++++++++++++++++++

    //Rain Sensor data
    int rainAnalogVal = analogRead(rainAnalogPin);
    int rainDigitalVal = digitalRead(rainDigitalPin);

    
    if(rainAnalogVal<=4000){
      Serial.println("Light Rain");
      if (WiFi.status() == WL_CONNECTED) {
        static bool flag = false;
        struct tm timeinfo;
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
        String urlFinal = "https://script.google.com/macros/s/"+ GOOGLE_SCRIPT_ID +"/exec?"+"Temperature=" + String(dhtTemperature) + "&Humidity=" + String(dhtHumidity) +"&LDR=" + String(analogValueLDR) + "&Pressure=" + String(bmpPressure) + "&Rain=Light_Rain";
        Serial.print("POST data to spreadsheet:");
        Serial.println(urlFinal);
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET(); 
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);
        
        //---------------------------------------------------------------------
        //getting response from google sheet
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
            Serial.println("Payload: "+payload);    
        }
        
          //---------------------------------------------------------------------
          http.end();
    }
   }
    
    
    else if(rainAnalogVal<=3500){
      Serial.println("Midium Rain");
      if (WiFi.status() == WL_CONNECTED) {
        static bool flag = false;
        struct tm timeinfo;
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
        String urlFinal = "https://script.google.com/macros/s/"+ GOOGLE_SCRIPT_ID +"/exec?"+"Temperature=" + String(dhtTemperature) + "&Humidity=" + String(dhtHumidity) +"&LDR=" + String(analogValueLDR) + "&Pressure=" + String(bmpPressure) + "&Rain=Midium_Rain";
        Serial.print("POST data to spreadsheet:");
        Serial.println(urlFinal);
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET(); 
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);
        
        //---------------------------------------------------------------------
        //getting response from google sheet
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
            Serial.println("Payload: "+payload);    
        }
        
          //---------------------------------------------------------------------
          http.end();
      }
    }
    
    
    else if(rainAnalogVal<=2000){
      Serial.println("Heavy Rain");
      if (WiFi.status() == WL_CONNECTED) {
        static bool flag = false;
        struct tm timeinfo;
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
        String urlFinal = "https://script.google.com/macros/s/"+ GOOGLE_SCRIPT_ID +"/exec?"+"Temperature=" + String(dhtTemperature) + "&Humidity=" + String(dhtHumidity) +"&LDR=" + String(analogValueLDR) + "&Pressure=" + String(bmpPressure) + "&Rain=Heavy_Rain";
        Serial.print("POST data to spreadsheet:");
        Serial.println(urlFinal);
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET(); 
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);
        
        //---------------------------------------------------------------------
        //getting response from google sheet
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
            Serial.println("Payload: "+payload);    
        }
        
          //---------------------------------------------------------------------
          http.end();
      }
    }
    
    
    else{
      Serial.println("No Rain");
      if (WiFi.status() == WL_CONNECTED) {
        static bool flag = false;
        struct tm timeinfo;
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
        String urlFinal = "https://script.google.com/macros/s/"+ GOOGLE_SCRIPT_ID +"/exec?"+"Temperature=" + String(dhtTemperature) + "&Humidity=" + String(dhtHumidity) +"&LDR=" + String(analogValueLDR) + "&Pressure=" + String(bmpPressure) + "&Rain=No_Rain";
        Serial.print("POST data to spreadsheet:");
        Serial.println(urlFinal);
        HTTPClient http;
        http.begin(urlFinal.c_str());
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET(); 
        Serial.print("HTTP Status Code: ");
        Serial.println(httpCode);
        
        //---------------------------------------------------------------------
        //getting response from google sheet
        String payload;
        if (httpCode > 0) {
            payload = http.getString();
            Serial.println("Payload: "+payload);    
        }
        
          //---------------------------------------------------------------------
          http.end();
      }
      
    }
      count++;
      WiFi.disconnect();
      
      Serial.println("WiFi Disconnectiong....");
      delay(1000);

     Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}