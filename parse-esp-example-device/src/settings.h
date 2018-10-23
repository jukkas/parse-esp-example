const char ssid[] = "YOUR-WIFI-SSID";
const char pass[] = "YOUR-WIFI-PASSWORD";

const char*  parseHost = "parseapi.back4app.com"; // Your Parse server hostname
const char* parsePath = "/"; // Parse path in your server. Default is /parse/
const char* streamServer = "YOURWSSERVER.back4app.io";  // LiveQuery websocket streaming server. Default parsePath
const char* streamPath = "/"; // default is parsePath

static const char applicationId[] PROGMEM = "YOUR-PARSE-APPLICATION-ID";
static const char restApiKey[] PROGMEM = "YOUR-PARSE-REST-API-KEY"; // Only needed if Parse version < 3. For set and get
static const char javascriptApiKey[] PROGMEM = "YOUR-PARSE-JS-API-KEY"; // Only needed if Parse version < 3. For LiveQuery stream
