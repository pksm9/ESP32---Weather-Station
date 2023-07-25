#include <Arduino.h>
#include <time.h>

#include <FS.h>
#include <SPIFFS.h>

#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <HTTPClient.h>

#include <DHT.h>
#include <Adafruit_BMP085.h>

const char* SSID = "clphy7";
const char* PSK = "cl@phy2020";
const char* DEPLOYMENT_ID = "AKfycbyuCZrqyH1itvJy_ZNg9S5OFxPnavfay4ye6Cuwkr7-nUFOou1mwG0wb4s238J1ZX3I";
const unsigned char INTERVAL_SECS = 60;
const unsigned char SOIL_PIN = 33;
const unsigned char BMP_SCL = 22;
const unsigned char BMP_SDA = 21;
const unsigned char LDR_PIN = 32;
const unsigned char RAIN_PIN = 35;
const unsigned char DHT_PIN = 18;
const unsigned char WATER_LEVEL_PIN = 26;
const unsigned char WIND_PIN = 32; //Must change
const unsigned char CLR_PIN = 19;

struct Record {
    unsigned int uuid;
    unsigned long sentTimestamp;
    double tempDHT;
    double humidity;
    double tempBMP;
    double pressure;
    double altitude;
    double seaLevelPressure;
    unsigned short luminosity;
    unsigned short windDir;
};

HTTPClient http;
DHT dht(DHT_PIN, DHT11);
Adafruit_BMP085 bmp;

String readFile(String path) {
    File file = SPIFFS.open(path);
    String content = file.readString();
    content.trim();
    file.close();
    return content;
}

void writeFile(String path, String content) {
    File file = SPIFFS.open(path, FILE_WRITE);
    file.println(content);
    file.close();
}

void writeRecord(Record record) {
    String content = "";
    content += "?1=";
    content += record.uuid;
    content += "&2=";
    content += record.sentTimestamp;
    content += "&3=";
    content += record.tempDHT;
    content += "&4=";
    content += record.humidity;
    content += "&5=";
    content += record.tempBMP;
    content += "&6=";
    content += record.pressure;
    content += "&7=";
    content += record.seaLevelPressure;
    content += "&8=";
    content += record.luminosity;
    content += "&9=";
    content += record.windDir;

    String filePath = "/records/" + String(record.uuid) + ".txt";
    writeFile(filePath, content);
    Serial.println("WRITTEN: " + content + " TO: " + filePath);
}

int sendRequest(String url) {
    Serial.println("\nSENDING: " + url);

    http.begin(url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    Serial.println("HTTP CODE: " + String(httpCode));

    if (httpCode > 0) {
        Serial.println("RECEIVED: " +  http.getString());
    }
    http.end();

    return httpCode;
}

void sleepDeep(int secs) {
    Serial.println("\nINITIATING: Deep sleep FOR: " + String(secs) + "s" );
    esp_sleep_enable_timer_wakeup(secs * 1000000);
    Serial.println("==================================================================");
    esp_deep_sleep_start();
}

RTC_DATA_ATTR bool timeConfigured = false;

Record generateRecord() {
    //Create a new record for current iteration
    Record record;

    unsigned int uuid;
    unsigned long sentTimestamp;
    double tempDHT;
    double humidity;
    double tempBMP;
    double pressure;
    double altitude;
    double seaLevelPressure;
    unsigned short luminosity;
    unsigned short windDir;

    //Populate the record with data
    record.tempDHT = dht.readTemperature();
    record.humidity = dht.readHumidity();
    record.tempBMP = bmp.readTemperature();
    record.pressure = bmp.readPressure();
    record.altitude = bmp.readAltitude();
    record.seaLevelPressure = bmp.readSealevelPressure();
    record.luminosity = analogRead(LDR_PIN);
    record.windDir = analogRead(WIND_PIN);

    //Assign sentTimestamp
    long lastTimestamp;
    if (timeConfigured) {
        lastTimestamp = time(NULL);
    } else {
        lastTimestamp = readFile("/timestamp_last.txt").toInt() + INTERVAL_SECS;
    }
    record.sentTimestamp = lastTimestamp;
    writeFile("/timestamp_last.txt", String(lastTimestamp));

    //Assign UUID
    int nextUuid = readFile("/uuid_next.txt").toInt();
    record.uuid = nextUuid;
    writeFile("/uuid_next.txt", String(nextUuid+1));

    return record;
}

void setup() {
    //Serial communication
    Serial.begin(115200);
    Serial.println("\n==================================================================");
    Serial.println("CONFIGURED: Serial communication AT: 115200");

    //SPIFFS
    SPIFFS.begin(true);
    if (digitalRead(CLR_PIN) == HIGH) {
        Serial.println("INITIATING: Memory wipe BECAUSE: Memory wipe pin is high");

        if (SPIFFS.remove("/uuid_next.txt")) {
            Serial.println("REMOVED: /uuid_next.txt");
        }
        if (SPIFFS.remove("/timestamp_last.txt")) {
            Serial.println("REMOVED: /timestamp_last.txt");
        }

        File recordsDir = SPIFFS.open("/records");
        File nextRecordFile;
        if (recordsDir) {
            while (nextRecordFile = recordsDir.openNextFile()) {
                String nextRecordFilePath = "/records/" + String(nextRecordFile.name());
                if (SPIFFS.remove(nextRecordFilePath)) {
                    Serial.println("REMOVED: " + nextRecordFilePath);
                }
            }
        }
        Serial.println("CONFIGURED: Storage DO: Reset chip");
        while(true);
    } else {
        if (!SPIFFS.exists("/uuid_next.txt")) {
            writeFile("/uuid_next.txt", "0");
            Serial.println("WRITTEN: /uuid_next.txt CONTENT: 0 BECAUSE: File not found");
        }
        if (!SPIFFS.exists("/timestamp_last.txt")) {
            writeFile("/timestamp_last.txt", "0");
            Serial.println("WRITTEN: /timestamp_last.txt CONTENT: 0 BECAUSE: File not found");
        }
        if (!SPIFFS.open("/records")) {
            Serial.println("CREATED: /records BECAUSE: Directory not found");
        }
        Serial.println("CONFIGURED: Storage");
    }

    //IO
    pinMode(SOIL_PIN, INPUT);
    pinMode(LDR_PIN, INPUT);
    pinMode(RAIN_PIN, INPUT);
    pinMode(DHT_PIN, INPUT);
    pinMode(WATER_LEVEL_PIN, INPUT);
    pinMode(WIND_PIN, INPUT);
    pinMode(CLR_PIN, INPUT);
    Serial.println("CONFIGURED: IO");

    //DHT11
    dht.begin();
    Serial.println("CONFIGURED: DHT11");

    //BMP180
    if (bmp.begin()) {
        Serial.println("CONFIGURED: BMP11");
    } else {
        Serial.println("FAILED: BMP11");
    }

    //WARNING: WiFi uses ADC2. So, record must be taken before WiFi is configured
    //Create and write a new record
    writeRecord(generateRecord());

    //WiFi
    Serial.print("CONFIGURING: WiFi WITH: " + String(SSID) + " ");
    unsigned char wiFiConnectTimeLeft = 20;
    WiFi.begin(SSID, PSK);
    while (WiFi.status() != WL_CONNECTED && wiFiConnectTimeLeft > 0) {
        Serial.print(".");
        wiFiConnectTimeLeft--;
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nCONFIGURED: WiFi WITH: " + String(SSID));

        if (!timeConfigured || time(NULL) < 1663485411) {
            //CASE: System time is not configured
            configTime(19800, 0, "pool.ntp.org");
            timeConfigured = true;
            Serial.println("CONFIGURED: System time WITH: pool.ntp.org BECAUSE: Time not configured to time is wrong");
        }

        //Empty the remaining files by sending all data while WiFi is connected
        File recordsDir = SPIFFS.open("/records");
        File recordFile;
        while (WiFi.status() == WL_CONNECTED && (recordFile = recordsDir.openNextFile())) {
            if(!recordFile.isDirectory()){
                String recordFilePath = "/records/" + String(recordFile.name());
                String url = "https://script.google.com/macros/s/" + String(DEPLOYMENT_ID) + "/exec" + readFile(recordFilePath);
                int httpCode = sendRequest(url);
                if (httpCode == 200) {
                    SPIFFS.remove(recordFilePath);
                }
            }
        }
    } else {
        Serial.println("\nERR: Configuring WiFi WITH: " + String(SSID) + " BECAUSE: Timed out");
    }

    //Initate deep sleep
    sleepDeep(INTERVAL_SECS);
}

void loop() {
    
}