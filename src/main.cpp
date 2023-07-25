#include <Arduino.h>            // need to import in VS code
#include "WiFi.h"               // for Wifi
#include <HTTPClient.h>         // for HTTP request
#include "time.h"               // to get time from NTP servers
#include "DHT.h"                // DHT library
#include <Adafruit_BMP085.h>    // BMP library
#include <SPIFFS.h>             // SPIFFS library
#include <SPI.h>                // for SPI communication

#define uS_TO_S_FACTOR 1000000 // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60       // Time ESP32 will go to sleep (in seconds)

#define DHTPIN 18
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;

RTC_DATA_ATTR int bootCount = 0; // saves waken up count in RTC memory

const char *ntpServer = "pool.ntp.org"; // NTP server
const long gmtOffset_sec = 19800;       // GMT time in seconds
const int daylightOffset_sec = 0;

const unsigned char CLR_PIN = 19;

const char *ssid = "BELL4G-3B8C";         // SSID
const char *password = "DET327TMTRM"; // password

String GOOGLE_SCRIPT_ID = "AKfycbx2oF-70_zIydhw740CItaL7XOxk2f-cd9KUCpesEVQ1dg3DX5SBxU2DcPChb4FpPP-"; // Google script ID

void setup()
{
    // initialize serial communication
    delay(1000);
    Serial.begin(115200);
    delay(1000);

    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));    // boot count will incremented and printed in serial monitor

    static bool setTime = false; 

    // deleting saved files if CLR_PIN is high
    if (digitalRead(CLR_PIN) == HIGH)
    {
        Serial.println("Memory wipe initiated.");

        File records = SPIFFS.open("/");                // opening root folder and save it in the records variable
        File nextRecordFile = records.openNextFile();   // from recods file will be iterate in the loop and saved in nextRecord File
        while (nextRecordFile)
        {
            String nextRecordFilePath = "/" + String(nextRecordFile.name());
            if (SPIFFS.remove(nextRecordFilePath))             // removing that file
            {
                Serial.println("REMOVED: " + nextRecordFilePath);
            }
            nextRecordFile = records.openNextFile();          // iterate and next file will be saved in nextRecordFile
        }
        Serial.println("Memory reset completed.");
        while (true);
    }
    else{
    }

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    if (!bmp.begin())
    {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1)
        {
        }
    }

    dht.begin();

    // getting readings
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    float hindex = dht.computeHeatIndex(temperature, humidity, false);
    float pressure = bmp.readPressure();
    float altitude = bmp.readAltitude();
    float slPressure = bmp.readSealevelPressure();

    long sentTime = time(NULL);     // setting time
    
    //saving data to a query string
    String data = ""; data += "?bootCount="; data += bootCount; data += "&sentTime="; data += sentTime; data += "&temperature="; data += temperature; data += "&humidity="; data += humidity; data += "&hindex="; data += hindex; data += "&pressure="; data += pressure; data += "&altitude="; data += altitude; data += "&slPressure="; data += slPressure;

    // writing data to flash memory
    File file = SPIFFS.open("/" + String(bootCount) + ".txt", FILE_WRITE);

    if (!file)
    {
        Serial.println("There was an error opening the file for writing");
        return;
    }
    if (file.print(data))
    {
        Serial.println("File was written with" + data);
    }
    else
    {
        Serial.println("File write failed");
    }

    file.close();

    // connect to WiFi
    Serial.println();
    Serial.print("Connecting to wifi: ");
    Serial.println(ssid);
    Serial.flush();
    unsigned char wifiTimeout = 25;     // for this ammount of time it will wait for a connection
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED && wifiTimeout > 0)
    {
        Serial.print(".");
        wifiTimeout--;
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        if (!setTime)
        {
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // setting up the time
            setTime = true;         // when the time is been set varible will set as true
            Serial.println("System time Configured.");
        }

        File records = SPIFFS.open("/");    // open root file
        File recordFile;

        while (WiFi.status() == WL_CONNECTED && (recordFile = records.openNextFile()))
        {
            if (!recordFile.isDirectory())
            {
                File file = SPIFFS.open("/" + String(recordFile.name()));   
                String data = file.readString();    // file data will be save in 'data' variable
                data.trim();
                file.close();

                String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec" + data;
                Serial.print("Posting data to spreadsheet:");
                Serial.println(url);

                HTTPClient http;

                http.begin(url.c_str());
                http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
                int httpCode = http.GET();
                Serial.print("HTTP Status Code: ");
                Serial.println(httpCode);

                // getting response from the google sheet
                String payload;
                if (httpCode > 0)
                {
                    payload = http.getString();
                    // Serial.println("Payload: "+payload);
                    if (httpCode == 200)
                    {
                        SPIFFS.remove("/" + String(recordFile.name()));     // revoving file from flash memory 
                        Serial.println("File removed from memory after uploading");
                    }
                }
                http.end();
            }
        }
    }else {
        Serial.println("WiFi connecting timeout. Unable to establish a connection.");
    }
    delay(1000);

    
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // define wakeup source
    Serial.println("ESP32 will go to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    Serial.println("ESP going to sleep now");
    delay(1000);
    Serial.flush();
    esp_deep_sleep_start(); // initiating deep sleep
    Serial.println("-------------------------------------------------------------------------------------------------");
}

void loop()
{
}