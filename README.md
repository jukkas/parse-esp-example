# Example of an ESP8266/ESP32 device updating, and being controlled by a Parse-server

This repository contains code for ESP8266 and ESP32 microcontollers (+ some server demo routines),
so that the devices can directly read and update Objects in a [Parse-server](https://parseplatform.org/)
based system, as well as be controlled in real time by changes in an Object.

This code is a thin wrapper for [REST-API](https://docs.parseplatform.org/rest/guide/) and 
[LiveQuery](https://github.com/parse-community/parse-server/wiki/Parse-LiveQuery-Protocol-Specification)
(using websocket) interface.

In this example, ESP-device:
1. Connects to a Parse-server
2. Logins if needed, 
3. Searches class "Devices" and reads an Object where key "name" equals its name ("device1")
4. Turns on or off a "Lamp" (i.e. turns on/off a GPIO pin) based on the value of key "light"
5. Calls Parse server to set key "online" to value "true"
6. Creates websocket connection to LiveQuery server and starts listening to changes in "device1" Object
7. Turns "Lamp" (some GPIO, e.g. LED_BUILTIN) on/off based on values in received change notification
8. In case value of "online" changes to false, calls Parse server to set key "online" back to value "true"

You can toggle "light" value (and "online" value) in Parse-server by using the "Parse Dashboard" provided 
by Parse, or by editing and running simple web-interface located in web-ui folder. 


## Setup of the example

### Parse server setup

We assume that you have access to a Parse server, either self hosted or use a hosted service 
(e.g. https://www.back4app.com/).

To setup this example, you need following information:
- App Id
- Parse API Address (server and path)
- Live Query server address (if different from API address)
- REST API key (only for older than version 3 of Parse server)
- JavaScript API key (only for older than version 3 of Parse server)
- Master key, if you use Nodejs script

Use Parse-server [Dashboard](https://github.com/parse-community/parse-dashboard) to create following Class, 
Object and Users:

- Class "Devices"
  - Object in that: `{ "name":"device1", "online":false, "light":false }`

  Users:
  - For ESP device: username: 'device1', password: 'demo', email: whatever, e.g. 'device1@example.com'
  - For web-interface: username', 'ui', password', 'demo', email', 'demo@example.com'

  Permissions:
  - Edit Class level permissions so that user 'device1' has Get,Find,Update permissions
    and 'ui' all of them.
  - Remove Public access, except Get and Find.

**OR** Edit and use Nodejs script in node-scripts folder (FIXME: does not set Class level permissions,
must use Dashboard for that)

### Compile ESP8266 or ESP32 firmware and flash

ESP code is located in `parse-esp-example-device` folder. It uses Arduino framework.

Code is structured to compile in [PlatformIO](https://platformio.org/) environment. To compile in Arduino-IDE
you probably need to modify something, at least manually download Websocket library from https://github.com/jukkas/FeebleESPWSClient.

Kind-of-library /parse-esp/ routines are located in `lib/parse-esp` folder, and example `main.cpp` in `src/`
folder.

For LiveQuery feature, this project uses library from https://github.com/jukkas/FeebleESPWSClient

Server address and Parse keys are located in src/settings.h. You must change them to match your server
settings. (and WiFi credentials)

### Demo web ui

Simple web user interface, using Parse JavaScript-API and Vue.js is located in web-io folder.
Login with "ui"/"demo".

Toggle "light" on and off by pressing the lamp button. You can toggle "online" value by clicking the text.

Edit `index.html` to use your server and API keys, and run it in your browser.

# Notes and limitations

parse-esp library that this example provides, is quite limited and you must study and apply [Parse REST
API documentation](https://docs.parseplatform.org/rest/guide/) and 
[LiveQuery spec](https://github.com/parse-community/parse-server/wiki/Parse-LiveQuery-Protocol-Specification)
when designing your own application.
This library helps only with retrieving and updating Objects, simple login using username, and basic 
live query monitoring changes in a Object. It also contains some very limited and basic JSON parsing
routines.

With all other cases, you are on your own (or try to use unmaintained
[Parse-SDK-Arduino library](https://github.com/parse-community/Parse-SDK-Arduino)).

Finicky with memory usage, when using ESP8266, so e.g. query length in Parse::set is very limited.
(256 bytes after urlencode()). If you experince crashes when getting/setting, try reducing buffer
sizes in Parse-Esp.cpp.
