#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include <EEPROM.h>

#include "settings.h" // Set your Parse keys and hostnames here! (+WiFi credentials)

#include "Parse-Esp.hpp"
ParseEsp parse(parseHost, parsePath);

#ifdef LED_BUILTIN
const int OutputPin = LED_BUILTIN; // ESP8266 D1 Mini
#else
const int OutputPin = 32; // ESP32 dev board without build in LED, connect LED+R b/w 3.3v and PIN32
#endif
static bool outputState = HIGH; // Light/LED is off when GPIO high 

volatile bool shouldSetOnline = false; // In main loop write /online=true to DB

// Address in EEPROM where session token is saved
#define STADDR 400
static char sessionTokenBuf[35] = {0};

// Set GPIO high/low, for LED_BUILTIN low means light off
static void turnLightOn(bool isOn) {
    outputState = isOn ? LOW : HIGH;
    digitalWrite(OutputPin, outputState);
}

static void clearTokenFromEeprom() {
        EEPROM.begin(512);
        EEPROM.write(STADDR, 0); 
        EEPROM.end();
}

/**
    Callback called when we receive changes from LiveQuery.
    Do not do here anything that blocks main loop for a long time.

    @params: data JSON document containing changed Object 
*/
static int liveQueryCb(const char *data) {

    turnLightOn(parseBool(data, "light")); // Set GPIO based in value of key "light"

    if (!parseBool(data, "online"))  // System/UI is checking if we are alive
        shouldSetOnline = true; // Execute actual API call in main loop, not in this callback

    if (parseBool(data, "debug")) { // This was used during development
        clearTokenFromEeprom();     
    }

    return 1;
}

static char objectPath[8+11+5] = {0}; // Full Object path, that will be queried in setup()

// Set Object key "online" to true 
static void setOnline() {
    if (objectPath[0]) {
        Serial.print("Setting device online. objectpath:");
        Serial.println(objectPath);
        parse.set(objectPath,"{\"online\":true}");
    }
    // --- or if you wnt to hard code object path, 
    //parse.set("Devices/dzEli8kFvF","{\"online\":true}");

}

// Forward declaration
void connectStream();

void setup() {
    Serial.begin(115200);
    pinMode(OutputPin, OUTPUT);
    digitalWrite(OutputPin, outputState);

    // Connect to a WiFi network (ssid and pass from settings.h)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

    //Serial.println(ESP.getChipId());  // TODO: Could use this ID as device name in server

    parse.setApplicationId(applicationId); // Required
    parse.setRestApiKey(restApiKey); // Only needed with Back4app or old versions (<3) of Parse-server // For set and get
    parse.setJavascriptApiKey(javascriptApiKey); // Only needed with Back4app or old versions (<3) of Parse-server // For stream

    const char *resp;
    
    /* 
       Session token is stored in EEPROM.
       Read if EEPROM contains the session token, if not then try logging in using username and password
       ("device1" and "demo")
    */
#if 1
    EEPROM.begin(512);
    if (EEPROM.read(STADDR) == 0xA1 && 
            EEPROM.read(STADDR+1) == 0xA3 && 
            EEPROM.read(STADDR+2) == 0xA2) {
        for (int i=0; i < 34; i++) { // FIXME, use DEFINE
            sessionTokenBuf[i] = EEPROM.read(STADDR+3+i);
        }
        Serial.print("Read Session Token from EEPROM:");
        Serial.println(sessionTokenBuf);
        parse.setSessionToken(sessionTokenBuf);
    } else {
        resp = parse.login("device1", "demo");
        const char *sessionToken = parseText(resp, "sessionToken");
        Serial.println(sessionToken ? sessionToken : "????");
        if (!sessionToken) { // Login failed
            EEPROM.write(STADDR, 0);
        } else { // Login success. Write Session Token to EEPROM
            EEPROM.write(STADDR, 0xA1); 
            EEPROM.write(STADDR+1, 0xA3); 
            EEPROM.write(STADDR+2, 0xA2);           
            for (int i=0; i < 34; i++) {
                sessionTokenBuf[i] = sessionToken[i]; 
                EEPROM.write(STADDR+3+i, sessionToken[i]);
            }
            Serial.println(sessionTokenBuf);
            parse.setSessionToken(sessionTokenBuf);
        }
    }
    EEPROM.end();
    // ... or just use hardcoded session token: 
    // parse.setSessionToken("r:8e7b1900856910ccb02b2b08ac89ea75");
#endif

    // Query server for "Devices" Object with name "device1".
    // parse.get parameters: Class/object path and query (as defined in REST API)
    resp = parse.get("Devices", "where={\"name\":\"device1\"}");  
    // ... or you can hard code the Object path (get from Dashboard/UI):
    //resp = parse.get("Devices/dzEli8kFvF");

    // Some error handling...
    if (hasErrorResp(resp, 403)) {  // "unauthorized"
        Serial.println("parse.get Devices failed: unauthorized");
        // TODO: reboot?
    }
    if (hasErrorResp(resp, 209)) {  // "Invalid session token"
        Serial.print("Invalid session token");
        clearTokenFromEeprom();
        // TODO: reboot?
    }

    // Initial light setting
    turnLightOn(parseBool(resp, "light"));

    // If you used Where-query, save objectId.
    // Set "online"
    const char* id = parseText(resp,"objectId");
    if (id) {
        strcpy_P(objectPath, PSTR("Devices/"));
        strcat(objectPath, id);
        setOnline();
    }

    connectStream();
}

void connectStream() {
    // LiveQuery stream query setup
    // See https://github.com/parse-community/parse-server/wiki/Parse-LiveQuery-Protocol-Specification
    parse.connectStream("{\"className\":\"Devices\",\"where\":{\"name\":\"device1\"}}", liveQueryCb,
                        streamServer, streamPath);
}

void loop() {
    parse.loop(); // For live query

    // In case control system/UI turned "online" false, lets turn it back true
    if (shouldSetOnline) {
        shouldSetOnline = false;
#ifdef ESP8266
        /*
            ESP8266 does not seem to like have more than one connection
            (crashes if attempted). So do this ugly disconnect/connect stream botch.
            We may lose changes happening to object of interest while this happens. Sad.
        */
        parse.disconnectStream();
#endif
        setOnline();
#ifdef ESP8266
        connectStream();
#endif
    }
}
