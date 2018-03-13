// PJR - test deep sleep, wifi manager & mqtt
// Also test BM280 sensor ...
// Now at github ...

#define DEBUG_PRINT(x) Serial.println(x)

#include <stdlib.h>

#include <Arduino.h>

#include <BME280I2C.h>
#include <Wire.h>

// This should not be needed - some problem with BME280 library?
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

#include <jWiFiManager.h>
#include <ArduinoJson.h>

#include <FS.h>
#include "SPIFFSServer.h"

#include <jLED.h>

#define UPDATE_PATH "/firmware"
#define UPDATE_USERNAME "pjr"
#define UPDATE_PASSWORD "pjr9npassword"

// Wifi Support
//ESP8266WebServer server(80);
SPIFFSReadServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

DNSServer dnsServer;
jWiFiManager jWM(server, dnsServer);

#include <jButton.h>

BME280I2C bme;

#define REPORT_PERIOD 20000

// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
// *** CONFIG   *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***

bool sys_configured = false;
#define CONFIG_FILE_VERSION 0

const char* default_config = R"(
    {
      "dhcp_id":"*",
      "mqtt_topic":"*",
      "mqtt_svr":"spock.rogers.lan",
      "mqtt_port":1883,
      "mqtt_user":"",
      "mqtt_pass":"",
      "fw_svr":"",
      "fw_port":1880,
      "debug_level":0,
      "version":0
    }
  )";

char dhcp_id[20];
char mqtt_topic[20];
char mqtt_svr[20];
int mqtt_port = 0;
char mqtt_user[20];
char mqtt_pass[20];
char fw_svr[20];
int fw_port = 0;
int debug_level = 0;
int version = CONFIG_FILE_VERSION;
// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***

bool cfgWrite()
{
  Serial.println("! Writing config file");

  const size_t bufferSize = JSON_OBJECT_SIZE(8);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();

  String buf;

  root["dhcp_id"] = dhcp_id;
  root["mqtt_topic"] = mqtt_topic;
  root["mqtt_svr"] = mqtt_svr;
  root["mqtt_port"] = mqtt_port;
  root["mqtt_user"] = mqtt_user;
  root["mqtt_pass"] = mqtt_pass;
  root["fw_svr"] = fw_svr;
  root["fw_port"] = fw_port;
  root["debug_level"] = debug_level;
  root["version"] = CONFIG_FILE_VERSION;

  root.prettyPrintTo(buf);

  File cfgFile = SPIFFS.open("/config.json", "w");

  if(!cfgFile)
  {
    Serial.println("! Failed to open config file json");
    return false;
  }

  Serial.println("* Opened config file\n");

  cfgFile.print(buf.c_str());


  Serial.printf("Config file written is \n%s\n", buf.c_str());

  cfgFile.close();

  return true;

}

// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
// *** FIRMWARE UPDATE  *** *** *** *** *** *** *** *** *** *** *** *** ***
// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***




// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***


// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
// *** MQTT *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***

WiFiClient espClient;

PubSubClient mq_client(espClient);

long lastMsg = 0;
char msg[100];
int value = 0;
long last_connect = 0;
#define MQTT_RECONN_TIME 5000


void mq_callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

void mq_reconnect()
{
  if(
    ((millis() - last_connect) < MQTT_RECONN_TIME)
    ||
    (strcmp(mqtt_svr,"") == 0)
    )
    return;

  Serial.printf("Mqtt - state %d Wifi %d\n", mq_client.state(), WiFi.status());

  last_connect = millis();

  // Might have changed ...
  mq_client.setServer(mqtt_svr, 1883);


  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
  if (mq_client.connect("ESP BME280 Btn"))
  {
    Serial.println("connected");
    // Once connected, publish an announcement...
    //client.publish("outTopic", "hello world");
    // ... and resubscribe
    //client.subscribe("inTopic");
  }
  else
  {
    //***********************************************
    // Not battery friendly ...
    // **********************************************
    Serial.print("failed, rc=");
    Serial.print(mq_client.state());
  }
}

// void configModeCallback (WiFiManager *myWiFiManager) {
//   Serial.println("Entered WiFi config mode");
//   Serial.println(WiFi.softAPIP());
//
//   Serial.println(myWiFiManager->getConfigPortalSSID());
// }
//


// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
// *** BUTTONS  *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
// **** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***

void one_click()
{
  Serial.printf("one click\n");
  jLEDblink();
}

void two_click()
{
  Serial.printf("two click\n");
  jLEDflash();
}

void three_click()
{
  Serial.printf("three click\n");
  jLEDwink();
}

void four_click()
{
  Serial.printf("four click\n");
}

void long_click()
{
  Serial.printf("long click\n");
}

void doConfigPortal()
{
  char AP[20];
  byte mac[6];
  // WiFiManager wifiManager;

  WiFi.macAddress(mac);

  // // Set up wifi page ...
  // WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "default", 40);
  // wifiManager.addParameter(&custom_mqtt_server);

  sprintf(AP,"PJR9N_%02X%02X%02X",mac[3],mac[4],mac[5]);
  printf("AP:'%s' entering config portal\n", AP);

  jWM.setApCredentials(AP);

}

void doStartWiFi()
{
  DEBUG_PRINT("* Starting wifi");

  // **** PJR - where should this be done?
  // A generated default is needed
  char id[20];
  if(strcmp(dhcp_id, "*") == 0)
  {
    // Auto generate a topic ...
    byte mac[6];

    WiFi.macAddress(mac);

    sprintf(id,"PJR9N_%02X%02X%02X",mac[3],mac[4],mac[5]);
  }
  else
  {
    strcpy(id, dhcp_id);
  }

  Serial.printf("! dhcp id %s\n", id);

  WiFi.hostname(id);

  jWM.onConnect([]() {
    DEBUG_PRINT("wifi connected");
    DEBUG_PRINT(WiFi.localIP());
    jLEDoff();
    //EasySSDP::begin(server);
  });

  jWM.onAp([](){
    DEBUG_PRINT("AP MODE");
    jLEDwink();
    //DEBUG_PRINT(persWM.getApSsid());
  });

  //make connecting/disconnecting non-blocking
  jWM.setConnectNonBlock(true);

  //in non-blocking mode, program will continue past this point without waiting
  jWM.begin();

  #define ifargcpy(_arg, _var) if(server.hasArg(_arg)) {\
                strncpy(_var, server.arg(_arg).c_str(), sizeof(_var));\
                Serial.printf(_arg " %s", _var);\
                }

  #define ifargcpyi(_arg, _var) if(server.hasArg(_arg)) {\
                _var = atoi(server.arg(_arg).c_str());\
                Serial.printf(_arg " %d", _var);\
                }

  //handles commands from webpage, sends live data in JSON format
  server.on("/api", []() {
    DEBUG_PRINT("server.on /api");
    ifargcpy("dhcp_id", dhcp_id);
    ifargcpy("mqtt_topic", mqtt_topic);
    ifargcpy("mqtt_svr", mqtt_svr);
    ifargcpyi("mqtt_port", mqtt_port);
    ifargcpy("mqtt_usr", mqtt_user);
    ifargcpy("mqtt_pw", mqtt_pass);
    ifargcpy("fw_svr", fw_svr);
    ifargcpyi("fw_port", fw_port);
    ifargcpyi("debug_level", debug_level);

    if(cfgWrite())
      Serial.println("** Config file written");

    //build json object of program data
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["j"] = "pjr9n_result=0";

    char jsonchar[200];
    json.printTo(jsonchar); //print to char array, takes more memory but sends in one piece
    server.send(200, "application/json", jsonchar);

  }); //server.on api


  MDNS.begin(id);

  httpUpdater.setup(&server, UPDATE_PATH, UPDATE_USERNAME, UPDATE_PASSWORD);
  server.begin();

  MDNS.addService("http", "tcp", 80);

  Serial.println("* exit setup WiFi");
}

void doInitBME()
{
  Serial.println("* Init BME");

  // For BME280 ...
  Wire.begin(D1,D2);

  while(!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("BME280 sensor");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("BMP280 sensor - No Humidity");
       break;
     default:
       Serial.println("*** Found UNKNOWN sensor!");

       // What do we do now ?
  }

}

void doInitMQTT()
{
  Serial.println("* Init MQTT");


  // Receives subscribed-to messages ?
  mq_client.setCallback(mq_callback);
}

void doInitButton()
{
  setupButton(D6);
  btnSetHandler(1, "hOne", one_click);
  btnSetHandler(2, "hTwo", two_click);
  btnSetHandler(3, "hThree", three_click);
  btnSetHandler(4, "hFour", four_click);
  btnSetHandler(-1, "hLong", doConfigPortal);
}


void doWifi()
{
  jWM.handleWiFi();
  dnsServer.processNextRequest();
  server.handleClient();
}

void doMqtt()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  // connects to MQTT if not connected already
  if (!mq_client.loop())
  {
    mq_reconnect();
    mq_client.loop(); // Does an immediate schedule - which might not be needed
  }
}

void doMeasurement()
{
  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  if (WiFi.status() != WL_CONNECTED)
    return;

  long now = millis();
  if ((now - lastMsg) > REPORT_PERIOD)
  {
    lastMsg = now;
    ++value;

    bme.read(pres, temp, hum, tempUnit, presUnit);

    // No %f in 2.3.0 Arduino printf ...
    int t, h, p;

    // two digits of precision
    // so t is in hundredths of degrees, and h is hundredths of millibars
    t = (int) (temp * 100.0);
    h = (int) (hum * 100.0);
    p = (int) pres;

    snprintf (msg, 75, "{ \"T\" : \"%d.%02d\", \"H\" : \"%d.%02d\", \"P\" : \"%d.%02d\" }",
                            t / 100, t % 100,
                            h / 100, h % 100,
                            p / 100, p %100);

    Serial.print("Publish message: "); Serial.println(msg);

    char topic[20];
    if(strcmp(mqtt_topic, "*") == 0)
    {
      // Auto generate a topic ...
      byte mac[6];

      WiFi.macAddress(mac);

      sprintf(topic,"PJR9N_%02X%02X%02X",mac[3],mac[4],mac[5]);
    }
    else
    {
      strcpy(topic, mqtt_topic);
    }

    mq_client.publish(topic, msg); // assume connected ... needs error handling
    mq_client.loop();              // redundant - done on next loop?
  }

}

bool configFromSPIFFS()
{
  if(!SPIFFS.begin())
  {
    Serial.println("! Failed mount SPIFFS file system");
    // SPIFFS.format();
    return false;
  }

  Serial.println("* File system mounted");

  if(!SPIFFS.exists("/config.json"))
  {
    Serial.println("! Config file not present");
    // *** Should initialise config file ???
    // write out default_config ...
    if(!cfgWrite())
    {
      Serial.println("! Could not creat config file ");
      return false;
    }
  }
  //file exists, reading and loading
  Serial.println("* Reading config file");
  File configFile = SPIFFS.open("/config.json", "r");

  if(!configFile)
  {
    Serial.println("! Failed to open config file json");
    return false;
  }

  Serial.println("* Opened config file\n");
  size_t size = configFile.size();

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if(!json.success())
  {
    Serial.println("! Failed to parse config file");
    return false;
  }

  // Debug ...
  json.printTo(Serial);

  Serial.println("\n* json parse complete");

  if (!json.containsKey("version"))
  {
    // No version key ...
    Serial.println("! No version key found ...");
    return false;
  }

  if(json["version"] != CONFIG_FILE_VERSION)
  {
    Serial.println("\n! Config file is wrong version");
    return false;
  }

  // File is OK,
  Serial.println("* Config file version matches - loading");

#define cpyifpresent(a,b) if(json.containsKey(b)) strncpy(a, json[b], sizeof(a))

  cpyifpresent(dhcp_id, "dhcp_id");
  if(dhcp_id[0] == '\0') strcpy(dhcp_id,"*");
  cpyifpresent(mqtt_topic, "mqtt_topic");
  if(mqtt_topic[0] == '\0') strcpy(mqtt_topic,"*");
  cpyifpresent(mqtt_svr, "mqtt_svr");
  if(json.containsKey("mqtt_port")) mqtt_port = json["mqtt_port"];
  cpyifpresent(mqtt_user, "mqtt_user");
  cpyifpresent(mqtt_pass, "mqtt_pass");
  cpyifpresent(fw_svr, "fw_svr");
  if(json.containsKey("fw_port")) fw_port = json["fw_port"];
  if(json.containsKey("debug_level")) debug_level = json["debug_level"];
  if(json.containsKey("version")) version = json["version"];

  Serial.println("* Config loaded");
  return true;
}

// *************************************
// *   STANDARD ARDUINO SETUP / LOOP   *
// *************************************

void setup()
{
  // Basic startup ...
  jLEDinit(BUILTIN_LED);
  jLEDon();
  Serial.begin(115200);
  delay(10);

  Serial.println("\n*** BME280/Button tester ***");

  doInitButton();

  // If this fails, all is lost - exit setup and fast flash the LED ...
  if(!(sys_configured = configFromSPIFFS()))
  {
    // Config did not work ...
    jLEDblink();
    Serial.println("\n*** Fatal error (config) ***");
    return;
  }

  // If the button is down, start the configuration portal ...
  if(buttonDown())
  {
    Serial.println("Button down ...");
    doConfigPortal();
  }

  doStartWiFi();

  doInitBME();

  doInitMQTT();

  jLEDflash();
  Serial.println("*** setup completed");
}

void loop()
{
  jLEDdo();

  if(!sys_configured)
    // Just let the LED blink ... and that's it
    return;

  doButton();

  doWifi();

  doMqtt();

  doMeasurement();
}
