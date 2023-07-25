void setup() {
    //Configure SPIFFS
    SPIFFS.begin(true);
    if (digitalRead(CLR_PIN) == HIGH) {
        clearPersistentStorage();
    } else {
        initializePersistentStorage();
    }

    //Configure IO
    pinMode(DHT_PIN, INPUT);
    pinMode(LDR_PIN, INPUT);
    pinMode(POT_PIN, INPUT);
    pinMode(CLR_PIN, INPUT);

    //Configure DHT11
    dht.begin();

    //Create and write a new record
    Record newRecord = generateRecord();
    writeRecord(newRecord);

    //Configure WiFi
    unsigned char wiFiConnectTimeLeft = 20;
    WiFi.begin(SSID, PSK);

    //Wait until wifi is connected or timeout
    while (
        WiFi.status() != WL_CONNECTED &&
        wiFiConnectTimeLeft > 0
    ) {
        wiFiConnectTimeLeft--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        //Configure system time if not configured
        if (!timeConfigured || time(NULL) < 1663485411) {
            configTime(19800, 0, "pool.ntp.org");
        }

        //Empty the remaining files by sending all data while WiFi is connected
        File recordsDir = SPIFFS.open("/records");
        File nextRecordFile = recordsDir.openNextFile();
        while (
            WiFi.status() == WL_CONNECTED &&
            nextRecordFile
        ) {
            int httpCode = uploadFile(nextRecordFile);
            if (httpCode == 200) {
                SPIFFS.remove(nextRecordFile);
            }

            nextRecordFile = recordsDir.openNextFile();
        }
    }

    //Initate deep sleep
    deepSleep(INTERVAL_SECS);
}