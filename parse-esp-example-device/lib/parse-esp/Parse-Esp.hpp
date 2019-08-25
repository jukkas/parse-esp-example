#ifndef PARSE_ESP_H_
#define PARSE_ESP_H_

#include <WiFiClientSecure.h>
#include "fwsc.h"

class ParseEsp {
    public:
        ParseEsp(const char *host, const char *parsePath = "/parse/");
        void setApplicationId(const char *appId);
        void setSessionToken(const char *token);
        void setRestApiKey(const char *key);
        void setJavascriptApiKey(const char *key);
        int connectStream(const char *subscr, int (*subscrCb)(const char *data), 
                          const char *server = nullptr, const char *path = nullptr);
        void disconnectStream();
        int set(const char *object, const char *data);
        char* get(const char *object, const char *query=nullptr);
        char* post(const char *className, const char *objectJson=nullptr);
        void loop();
        const char* getSessionToken();
        const char* login(const char *username, const char *password);

    private:
        const char *host;
        const char *parsePath;
        WiFiClientSecure client;
        Fwsc ws;
};

/* Very limited JSON parsing routines */
extern bool parseBool(const char* json, const char *key);
extern char* parseText(const char* json, const char *key);
extern int parseInt(const char* json, const char *key);
extern bool hasErrorResp(const char* json, int code);
#endif /* PARSE_ESP_H_ */
