#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
/* #else   -- or #ifdef ESP32
   #include <Wifi.h> 
?? */
#include "fwsc.h"
#include <WiFiClientSecure.h>

#include "Parse-Esp.hpp"

static char _buffer[1024] = {0};
static int (*_subscrCb)(const char *data) = nullptr;
static char _streamQuery[128] = {0};
static const char *_applicationId = nullptr;
static const char *_sessionToken = nullptr;
static const char *_restApiKey = nullptr;
static const char *_javascriptApiKey = nullptr;


ParseEsp::ParseEsp(const char *host_i, const char *parsePath_i) {
    host = host_i;
    parsePath = parsePath_i;
}

void ParseEsp::setApplicationId(const char *appId) {
    _applicationId = appId;
}

void ParseEsp::setSessionToken(const char *token) {
    _sessionToken = token;
}

const char* ParseEsp::getSessionToken() {
    return _sessionToken;
}


void ParseEsp::setRestApiKey(const char *key) {
    _restApiKey = key;
}

void ParseEsp::setJavascriptApiKey(const char *key) {
    _javascriptApiKey = key;
}

void ParseEsp::loop(void) {
    ws.loop();
}

static int transaction(const char *hostname, const char *request) {
    WiFiClientSecure client;
#ifdef wificlientbearssl_h
    client.setInsecure();
#endif
    if (!client.connect(hostname, 443)) {
        Serial.print("https connection failed:");
        Serial.println(hostname);
        return -1;
    }
    client.print(_buffer);
    int contentLen = 0;
    int statusCode = 0;
    while (client.connected()) {
        int len = client.readBytesUntil('\n', _buffer, 1023);
        _buffer[len] = '\0';
        if (!strncmp_P(_buffer, PSTR("HTTP/1.1"), 8)) {
            statusCode = atoi(_buffer+9);
            Serial.print("< Response status: ");
            Serial.println(statusCode);
        }
        if (!strncmp_P(_buffer, PSTR("Content-Length: "), 16)) {
            contentLen = atoi(_buffer+16);
            // Serial.print("< Content length:");
            // Serial.println(contentLen);
        }
        //Serial.print("<");
        //Serial.println(_buffer);
        if (!strcmp(_buffer, "\r")) {
            if (!contentLen) break;
            len = client.readBytes(_buffer, contentLen);
            _buffer[len] = '\0';
            Serial.print("< Body: ");
            Serial.println(_buffer);

            break;
        }
    }
    client.stop();
    return statusCode;
}
static char *urlencode(const char *url) {
    static char buf[256] = {0};
    char *p=buf;
    int urllen = strlen(url);
    for (int i=0; i < urllen; i++) {
        char c = url[i];
        char code0;
        char code1;
        if (isalnum(c) || (c == '=')) {
            *p++ = c;
        } else {
            code1=(c & 0xf)+'0';
            if ((c & 0xf) >9){
                code1=(c & 0xf) - 10 + 'A';
            }
            c=(c>>4)&0xf;
            code0=c+'0';
            if (c > 9){
                code0=c - 10 + 'A';
            }
            *p++ = '%';
            *p++ = code0;
            *p++ = code1;
        }
    }
    *p=0;
    return buf;
}

static char* createHttpRequest(const char *host, const char *method, const char *parsePath,
                              const char *object, const char *body, 
                              const char *query=nullptr, const char *query2=nullptr) {
    int dataLen = strlen(body);
    strcpy(_buffer, method);
    strcat(_buffer, " ");
    strcat(_buffer, parsePath);
    if (strcmp(object, "login"))     // Ugly hack: use "classes/" in url except in login case
        strcat_P(_buffer, PSTR("classes/"));
    strcat(_buffer, object);
    if (query && strlen(query)>0) {
        strcat(_buffer, "?");
        strcat(_buffer, urlencode(query));
        if (query2 && strlen(query)>0) {
            strcat(_buffer, "&");
            strcat(_buffer, urlencode(query2));
        }
    }
    strcat_P(_buffer,  PSTR(" HTTP/1.1\r\nHost: "));
    strcat(_buffer, host);
    strcat_P(_buffer,  PSTR("\r\nUser-Agent: ESP8266\r\nX-Parse-Application-Id: "));
    strcat_P(_buffer, _applicationId);
    strcat(_buffer,  "\r\n");
    if (_restApiKey) {
        strcat_P(_buffer,  PSTR("X-Parse-REST-API-Key: "));
        strcat_P(_buffer,  _restApiKey);
        strcat(_buffer,  "\r\n");
    }
    //if (!strcmp(object, "login")) {
    //    strcat_P(_buffer,  PSTR("X-Parse-Revocable-Session: 1\r\n"));
    //}
    if (_sessionToken) {
        strcat_P(_buffer,  PSTR("X-Parse-Session-Token: "));
        strcat(_buffer,  _sessionToken);
        strcat(_buffer,  "\r\n");
    }
    strcat_P(_buffer,  PSTR("Content-Type: application/json\r\nConnection: close\r\nContent-Length: "));
    char lenStr[5];
    itoa(dataLen, lenStr, 10);  
    strcat(_buffer,  lenStr);
    strcat(_buffer,  "\r\n\r\n");
    strcat(_buffer, body);
    return _buffer;
}

int ParseEsp::set(const char *object, const char *data) {
    Serial.println("ParseEsp::set");
    char *req = createHttpRequest(host, "PUT", parsePath, object, data);
    //Serial.println(req);
    return transaction(host, req);
}

char* ParseEsp::get(const char *object, const char *query) {
    Serial.println("ParseEsp::get");
    char *req = createHttpRequest(host, "GET", parsePath, object, "", query);
    //Serial.println(req);
    transaction(host, req);
    return _buffer;
}

char* ParseEsp::post(const char *className, const char *objectJson) {
    Serial.println("ParseEsp::post");
    char *req = createHttpRequest(host, "POST", parsePath, className, objectJson);
    //Serial.println(req);
    transaction(host, req);
    return _buffer;
}


const char* ParseEsp::login(const char *username, const char *password) {
    Serial.println("ParseEsp::login");
    char usernameParam[40];
    char passwordParam[40];
    strcpy_P(usernameParam, PSTR("username="));
    strcat(usernameParam, username);
    strcpy_P(passwordParam, PSTR("password="));
    strcat(passwordParam, password);

    char *req = createHttpRequest(host, "GET", parsePath, "login", "", 
                                  usernameParam, passwordParam);
    transaction(host, req);
    return _buffer;
}


int ParseEsp::connectStream(const char *subscr, int (*subscrCb)(const char *data),
                            const char *server, const char *path) {
    _subscrCb = subscrCb;
    strncpy(_streamQuery, subscr, sizeof(_streamQuery)-1); // Save live query

    auto cb = [&](WSEvent type, uint8_t * payload) {
        switch (type)
        {
        case WSEvent::error:
            Serial.println("Websocket error");
            break;
        case WSEvent::disconnected:
            Serial.println("Websocket disconnected");
            break;
        case WSEvent::connected:
            {
                Serial.println("Websocket connected");
                // Send "connect" command
                strcpy_P(_buffer, PSTR("{\"op\": \"connect\",\"applicationId\": \""));
                strcat_P(_buffer, _applicationId);
                strcat(_buffer, "\"");
                if (_sessionToken) {
                    strcat_P(_buffer, PSTR(",\"sessionToken\": \""));
                    strcat(_buffer, _sessionToken);
                    strcat(_buffer, "\"");
                }
                if (_javascriptApiKey) {
                    strcat_P(_buffer, PSTR(",\"javascriptKey\": \""));
                    strcat_P(_buffer, _javascriptApiKey);
                    strcat(_buffer, "\"");
                }
                strcat(_buffer, "}");
                ws.sendtxt(_buffer);
            }
            break;
        case WSEvent::text:
            Serial.print("< From Websocket: ");
            Serial.println((const char *)payload);
            if (!strncmp_P((const char *)payload, PSTR("{\"op\":\"connected\""),17)) {
                strcpy_P(_buffer, PSTR("{\"op\":\"subscribe\",\"requestId\":1,\"query\":"));
                strcat(_buffer, _streamQuery);
                strcat(_buffer, "}");
                ws.sendtxt(_buffer);
            } else if (!strncmp_P((const char *)payload, PSTR("{\"op\":\"update\""),14)) {
                if (_subscrCb) _subscrCb((const char *)payload);
            }
            break;
        default:
            Serial.printf_P(PSTR("WS Got unimplemented\n"));
            break;
        }
    };
    ws.setCallback(cb);
    ws.connect(server ? server:host, 443, path ? path:parsePath);
    return 1;
}

void ParseEsp::disconnectStream() {
    ws.disconnect();
}

/************* JSON parsing routines ******************/
static const char* findKey(const char* json, const char *key) {
    int keyLen = strlen(key);
    const char *p = json;
    do {
        p = strstr(p, key);
        if (p && (*(p-1) != '"' || *(p+keyLen) != '"')) { 
            p++;
        } else {
            break;
        }
    } while (p);
    return p;
}

bool parseBool(const char* json, const char *key) {
    int keyLen = strlen(key);
    const char *p = findKey(json, key);

    if (p) {
        return (*(p+keyLen+2) == 't');
    }
    //Serial.println("parseBool: key not found");
    return false;
}

char* parseText(const char* json, const char *key) {
    static char buf[41]={0};
    int keyLen = strlen(key);
    const char *p = findKey(json, key);
    if (!p) return NULL;

    const char *end = strchr(p+keyLen+3, '"');
    if (!end) return NULL;
    strncpy(buf, p+keyLen+3, end-(p+keyLen+3));
    buf[40]='\0';

    return buf;
}

int parseInt(const char* json, const char *key) {
    static char buf[11]={0};
    int keyLen = strlen(key);
    const char *p = findKey(json, key);
    if (!p) return -1;
    char *b = buf;
    p += keyLen + 2;
    while(isdigit(*p)) {
        *b = *p;
        p++;
        b++;
    }
    *b = '\0';
    return atoi(buf);
}

/*************** Error routines ****************/
bool hasErrorResp(const char* json, int code) {
    const char *p = findKey(json, "error");
    if (!p) return false;

    return (parseInt(json,"code") == code);
}