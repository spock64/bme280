// PJR - test deep sleep, wifi manager & mqtt
// Also test BM280 sensor ...
/*

States:

[WIFI|MQTT]_DISCONNECTED
[WIFI|MQTT]_CONNECTING
[WIFI|MQTT]_CONNECTED

States used in sequence?

Assume proper initialisation when waking from deep sleep - but no context



*** Below for reference only ***
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

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
#include <WiFiManager.h>

BME280I2C bme;

#define BUTTON D1


// Update these with values suitable for your network.

const char* ssid = "ROGERS";
const char* password = "*jaylm123456!";
const char* mqtt_server = "192.168.16.3";

// Used to accelerate connection ...
IPAddress ip= IPAddress(192,168,17,200);
IPAddress gw= IPAddress(192,168,16,1);
IPAddress subnet= IPAddress(255,255,254,0);
IPAddress dns= IPAddress(192,168,16,1);

WiFiClient espClient;

PubSubClient client(espClient);
long lastMsg = 0;
char msg[100];
int value = 0;

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
    if (client.connect("ESP8266Client")) {
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

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered WiFi config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {

  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(BUTTON, INPUT_PULLUP);

  Serial.begin(115200);             // Does this use power?
  delay(10);

  Serial.println("Welcome - BME280 tester");

  printf("\n\nSDK version: %s\n", system_get_sdk_version());


  // Could / should detect a pin being held up or down so we can enter a
  // "config mode"
  // in that mode we can e.g. check for a new u-code version or new config
  // also we could think about a longer press meaning that the wifi needs
  // be set up again ??

  WiFiManager wifiManager;

  // If the button is down, start the configuration portal ...
  // Active LOW

  // Temp frig ...
  //if(!digitalRead(BUTTON))
  if(0)
  {
    char AP[20];
    byte mac[6];
    printf("Button pressed, entering config portal\n");

    WiFi.macAddress(mac);
    sprintf(AP,"PJR9N_%02X%02X%02X",mac[3],mac[4],mac[5]);
    printf("AP on '%s'\n",AP);

    wifiManager.startConfigPortal(AP);
    //printf("Connected %s %s\n", WiFi.SSID(), WiFi.localIP());
  }
  else
  {
    printf("Button up, continuing as normal\n");
  }

  // For BME280 ...
  Wire.begin(D3,D4); // esp pins 0,2 c/w  10k pullups ...

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


  wifiManager.setSTAStaticIPConfig(ip, gw, dns);
  wifiManager.setAPCallback(configModeCallback);

  //*********************************
  wifiManager.setDebugOutput(false);
  //*********************************

  if(!wifiManager.autoConnect())
  {
    printf("*** ERROR - DID NOT CONNECT - SLEEPING 20s");
    ESP.deepSleep(20e6); // 20e6 is 20 microseconds
  }

  printf("To wifi connected %d ms\n", millis());

  // following from loop ...
  client.setServer(mqtt_server, 1883);

  // Receives subscribed-to messages ?
  client.setCallback(callback);
}


void loop() {

  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);


  // connects to MQTT if not connected already
  if (!client.connected())
  {
    reconnect();
  }

  // allows MQTT to connect properly ...
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000)
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
    client.publish("outTopic", msg);
  }




  // // Active LOW
  // if(1)
  // //if(!digitalRead(BUTTON))
  // {
  //   delay(1000);
  // }
  // else
  {
    Serial.println("Going into deep sleep for 20 seconds");
    printf("Been awake %d\n\n", millis());
    ESP.deepSleep(20e6); // 20e6 is 20 seconds
  }

}

// ----------------------------------------------------------------------------

#ifdef PJR9NZ

// state driven loop
  switch(state)
  {
    case WIFI_DISCONNECTED:
      // connect wifi
      break;

    case WIFI_CONNECTING:
      // read temp, if not read already
      break;

    case WIFI_CONNECTED:
      // connect to mqtt
      break;

    case MQTT_CONNECTING:
      // nothing ?
      break;

    case MQTT_CONNECTED:
      // Send in the message
      // we should send in metrics - e.g. how long connections took?
      // do we get message sent callbacks
      break;

  }

// Example "fast" startup sequence ...

// but should read values from flash?
// so will take a little longer ...
// can then use button press to force wifi manager and AP mode etc.

start = millis();
WiFi.config(ip,gw,subnet, dns);
WiFi.begin("SSID","password");
while (WiFi.status() != WL_CONNECTED) {
      delay(1);
  }
now = millis();
codeTime = now - start;
wifi_station_get_config(&conf);
//conf.bssid[0]=conf.bssid[0]+1;
//WiFi.begin("SSID","password",6,conf.bssid,1);
Serial.begin(115200);



#endif
