// PJR - test deep sleep, wifi manager & mqtt
// Also test BM280 sensor ...
// Now at github ...
/*

States:

[WIFI|MQTT]_DISCONNECTED
[WIFI|MQTT]_CONNECTING
[WIFI|MQTT]_CONNECTED

States used in sequence?

Assume proper initialisation when waking from deep sleep - but no context

*/

#define DEBUG_PRINT(x) Serial.println(x)

#include <Arduino.h>

#include <BME280I2C.h>
#include <Wire.h>

// This should not be needed - some problem with BME280 library?
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <jWiFiManager.h>
#include <ArduinoJson.h>

#include <FS.h>
#include "SPIFFSServer.h"

// Wifi Support
//ESP8266WebServer server(80);
SPIFFSReadServer server(80);
DNSServer dnsServer;
jWiFiManager jWM(server, dnsServer);

#include <jButton.h>

BME280I2C bme;

#define BUTTON D1
#define REPORT_PERIOD 20000


// Update these with values suitable for your network.

// const char* ssid = "ROGERS";
// const char* password = "*jaylm123456!";
const char* mqtt_server = "192.168.16.3";
//
// // Used to accelerate connection ...
// IPAddress ip= IPAddress(192,168,17,200);
// IPAddress gw= IPAddress(192,168,16,1);
// IPAddress subnet= IPAddress(255,255,254,0);
// IPAddress dns= IPAddress(192,168,16,1);

WiFiClient espClient;

PubSubClient client(espClient);

long lastMsg = 0;
char msg[100];
int value = 0;
const char* topic = "xTemp";

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP BME280 Btn")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      //***********************************************
      // Not battery friendly ...
      // **********************************************
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// void configModeCallback (WiFiManager *myWiFiManager) {
//   Serial.println("Entered WiFi config mode");
//   Serial.println(WiFi.softAPIP());
//
//   Serial.println(myWiFiManager->getConfigPortalSSID());
// }
//
void one_click()
{
  Serial.printf("one click\n");
}

void two_click()
{
  Serial.printf("two click\n");
}

void three_click()
{
  Serial.printf("three click\n");
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




  // if(wifiManager.startConfigPortal(AP))
  // {
  //   // Connected
  //   printf("Connected.\n");
  // }
  // else
  // {
  //   // Not Connected ... set state ?
  //   printf("Not Connected !\n");
  // }

  // Read parameters back out - depending on whether it worked ord not ...

}

int x;
String y;

void doStartWiFi()
{
  DEBUG_PRINT("* Starting wifi");
  //make connecting/disconnecting non-blocking
  jWM.setConnectNonBlock(true);

  //in non-blocking mode, program will continue past this point without waiting
  jWM.begin();

  //handles commands from webpage, sends live data in JSON format
  server.on("/api", []() {
    DEBUG_PRINT("server.on /api");
    if (server.hasArg("x")) {
      x = server.arg("x").toInt();
      DEBUG_PRINT(String("x: ") + x);
    } //if
    if (server.hasArg("y")) {
      y = server.arg("y");
      DEBUG_PRINT("y: " + y);
    } //if

    //build json object of program data
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["x"] = x;
    json["y"] = y;

    char jsonchar[200];
    json.printTo(jsonchar); //print to char array, takes more memory but sends in one piece
    server.send(200, "application/json", jsonchar);

  }); //server.on api


  server.begin();
}


void setup() {

  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

  setupButton(D6);
  btnSetHandler(1, "hOne", one_click);
  btnSetHandler(2, "hTwo", two_click);
  btnSetHandler(3, "hThree", three_click);
  btnSetHandler(4, "hFour", four_click);
  btnSetHandler(-1, "hLong", doConfigPortal);


  Serial.begin(115200);             // Does this use power?
  delay(10);

  Serial.println("\nWelcome - BME280/Button tester");

  SPIFFS.begin();

  // WiFiManager wifiManager;
  jWM.onConnect([]() {
    DEBUG_PRINT("wifi connected");
    DEBUG_PRINT(WiFi.localIP());
    //EasySSDP::begin(server);
  });
  //...or AP mode is started
  jWM.onAp([](){
    DEBUG_PRINT("AP MODE");
    //DEBUG_PRINT(persWM.getApSsid());
  });

  doStartWiFi();


  // If the button is down, start the configuration portal ...

  if(buttonDown())
  {
    printf("Button pressed on startup\n");
    doConfigPortal();
  }
  else
  {
    printf("Button up, continuing as normal\n");
  }

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
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");

       // What do we do now ?
  }

  // *** This does not make sense here ? Does it ?


  // wifiManager.setSTAStaticIPConfig(ip, gw, dns);
  // wifiManager.setAPCallback(configModeCallback);
  //
  // //*********************************
  // wifiManager.setDebugOutput(true);
  // //*********************************
  // WiFi.mode(WIFI_STA);
  //
  // if(!wifiManager.autoConnect())
  // {
  //   printf("*** ERROR - WIFI DID NOT CONNECT - SLEEPING 20s");
  //   ESP.deepSleep(20e6); // 20e6 is 20 microseconds
  // }

  //printf("To wifi connected %d ms\n", millis());

  // following from loop ...
  client.setServer(mqtt_server, 1883);

  // Receives subscribed-to messages ?
  client.setCallback(callback);
}


void loop()
{

  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  doButton();

  jWM.handleWiFi();

  dnsServer.processNextRequest();
  server.handleClient();

  // connects to MQTT if not connected already
  if (!client.loop())
  {
    Serial.printf("Mqtt disconnected - state %d Wifi %d\n", client.state(), WiFi.status());
    // what if we hit this again ? Does the lib protect against it?
    reconnect();
    client.loop(); // Is this one neccessary ?
  }

  long now = millis();
  if ((now - lastMsg) > REPORT_PERIOD)
  {
    lastMsg = now;
    ++value;

    bme.read(pres, temp, hum, tempUnit, presUnit);

    // No %f in 2.3.0 Arduino ...
    // snprintf (msg, 75, "Weather @ #%ld (%ld)ms T: %f° H: %f%% P: %f",
    //                         value, millis(), temp, hum, pres /100);

    int t, h, p;

    // two digits of precision
    // so t is in hundredths of degrees, and h is hundredths of millibars
    t = (int) (temp * 100.0);
    h = (int) (hum * 100.0);
    p = (int) pres;

    // snprintf (msg, 75, "Weather @ #%ld (%ld)ms T: %d.%02d° H: %d.%02d%% P: %d.%02d",
    //                         value, millis(),
    //                         t / 100, t % 100,
    //                         h / 100, h % 100,
    //                         p / 100, p %100);
    //
    snprintf (msg, 75, "{ \"T\" : \"%d.%02d\", \"H\" : \"%d.%02d\", \"P\" : \"%d.%02d\" }",
                            t / 100, t % 100,
                            h / 100, h % 100,
                            p / 100, p %100);



    Serial.print("Publish message: "); Serial.println(msg);

    // publish
    client.publish(topic, msg);

    // // Not needed - happens next time around the loop
    // if(!client.loop())
    //   reconnect();
  }



}

// // ----------------------------------------------------------------------------
//
// #ifdef PJR9NZ
//
// // state driven loop
//   switch(state)
//   {
//     case WIFI_DISCONNECTED:
//       // connect wifi
//       break;
//
//     case WIFI_CONNECTING:
//       // read temp, if not read already
//       break;
//
//     case WIFI_CONNECTED:
//       // connect to mqtt
//       break;
//
//     case MQTT_CONNECTING:
//       // nothing ?
//       break;
//
//     case MQTT_CONNECTED:
//       // Send in the message
//       // we should send in metrics - e.g. how long connections took?
//       // do we get message sent callbacks
//       break;
//
//   }
//
// // Example "fast" startup sequence ...
//
// // but should read values from flash?
// // so will take a little longer ...
// // can then use button press to force wifi manager and AP mode etc.
//
// start = millis();
// WiFi.config(ip,gw,subnet, dns);
// WiFi.begin("SSID","password");
// while (WiFi.status() != WL_CONNECTED) {
//       delay(1);
//   }
// now = millis();
// codeTime = now - start;
// wifi_station_get_config(&conf);
// //conf.bssid[0]=conf.bssid[0]+1;
// //WiFi.begin("SSID","password",6,conf.bssid,1);
// Serial.begin(115200);
//
//
//
// #endif
