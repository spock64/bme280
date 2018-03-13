// jLED.cpp
// Simple library to flash and blink LEDs ...

#include <Arduino.h>
#include "jLED.h"


typedef enum {
  OFF,
  ON,
  BLINKING, // on/off 0.1s each
  FLASHING, // on/off 0.25s each
  WINKING // on/off0.51s each
} LED_state_t;


long last = 0;

int led_pin = -1;

LED_state_t LED_state = OFF;

void jLEDinit(int pin)
{
  pinMode(pin, OUTPUT);

  led_pin = pin;
}

void jLEDon()
{
  LED_state = ON;
  digitalWrite(led_pin, 0);     // Active LOW
}

void jLEDoff()
{
  LED_state = OFF;
  digitalWrite(led_pin, 1);     // Active LOW
}

int _jLEDflip()
{
  int state = digitalRead(led_pin);  // get the current state of BUILTIN_LED pin
  digitalWrite(led_pin, !state);     // set pin to the opposite state
}

void jLEDdo()
{
  if(led_pin == -1)
    return;

  switch(LED_state)
  {
    case OFF:
    case ON:
      break;

    case BLINKING:
      if(millis() > (last + 100))
      {
        _jLEDflip();
        last = millis();
      }
      break;
    case FLASHING:
    if(millis() > (last + 250))
    {
      _jLEDflip();
      last = millis();
    }
      break;
    case WINKING:
    if(millis() > (last + 500))
    {
      _jLEDflip();
      last = millis();
    }
      break;
  }
}

void jLEDblink()
{
  LED_state = BLINKING;
}

void jLEDflash()
{
  LED_state = FLASHING;
}

void jLEDwink()
{
  LED_state = WINKING;
}
