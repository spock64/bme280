// *** BUTTON ***

#include <Arduino.h>
#include "jButton.h"

// Button handling ...

#define LONGPRESS 2000
#define SETTLE 500
#define DEBOUNCE 20
#define MAXCLICKS 6

int btnLast;
unsigned long btnLastchange;

typedef enum
      {
        bUP,
        bDOWN,
        bLONG,
        bIDLE
      } buttonState;

typedef void (*bFunc)();

typedef struct {
  const char * name;
  bFunc func;
} bFunc_t;

bFunc_t btnHandler[MAXCLICKS];
bFunc_t btnLongHandler;

buttonState btn = bIDLE;

unsigned long tDown, tUp;

unsigned int btnClicks = 0;
int bPin = -1;

void btnSetHandler(
  int clicks,
  const char * name,
  bFunc f
)
{
  Serial.printf("Func %s on %d clicks\n", name, clicks);

  if(clicks == -1)
  {
    Serial.printf("%s called on long click\n", name);

    btnLongHandler.name = name;
    btnLongHandler.func = f;
  }

  if(clicks > MAXCLICKS)
  {
    Serial.printf("%d exceeds mac %d\n", clicks, MAXCLICKS);
    return;
  }

  // array index from 0
  btnHandler[clicks-1].name = name;
  btnHandler[clicks-1].func = f;
}

void handleButton(int clicks)
{
  Serial.printf("Button clicked %d times\n", clicks);

  if(clicks > MAXCLICKS)
  {
    Serial.printf("Too many clicks %d\n", clicks);
  }
  else
    // Array is indexed from 0 ...
    if(
      (btnHandler[clicks-1].name == NULL)
      ||
      (btnHandler[clicks-1].func == NULL)
    )
      Serial.printf("No handler for %d clicks\n", clicks);
    else
      (btnHandler[clicks-1].func)();
}

void handleLongpress()
{
  Serial.printf("Long press\n");

  if(
    (btnLongHandler.name == NULL)
    ||
    (btnLongHandler.func == NULL)
  )
    Serial.printf("No long press handler\n");
  else
    (btnLongHandler.func)();
}

void doButton()
{
  int pin;
  if(millis() < (btnLastchange + DEBOUNCE))
  {
    // don't bother - this could be a bounce ...
    return;
  }

  pin = digitalRead(bPin);

  // remember when the pin changed for debounce
  if(btnLast != pin)
  {
    btnLast = pin;
    btnLastchange = millis();
  }

  if (pin == LOW)
  {
    // Button is down ...
    switch(btn)
    {
      case bIDLE:
        btnClicks = 0;
        // deliberate fall-through
      case bUP:
        btnClicks++;
        btn = bDOWN;
        tDown = millis();
        break;

      case bDOWN:
        if(millis() > (tDown + LONGPRESS))
          btn = bLONG;
        break;
    }
  }
  else
  {
    // Button is up ...
    switch(btn)
    {
      case bDOWN:
        tUp = millis();
        btn = bUP;
        break;

      case bUP:
        // Wait a while after the up just in case there's another click
        if(millis() > (tUp + SETTLE))
        {
          handleButton(btnClicks);
          btn = bIDLE;
        }
        break;

      case bLONG:
        // It's long press
        handleLongpress();
        btn = bIDLE;
        break;

      case bIDLE:
        break;
    }
  }
}

void setupButton(int io_pin)
{
  int i;

  bPin = io_pin;
  pinMode(bPin, INPUT_PULLUP);

  btnLastchange = millis();
  if ((btnLast = digitalRead(bPin)) == LOW)
    btn = bDOWN;

  for(i=0;i<MAXCLICKS;i++)
  {
    btnHandler[i].name = NULL;
    btnHandler[i].func = NULL;
  }
  btnLongHandler.name = NULL;
  btnLongHandler.func = NULL;
}

bool buttonDown()
{
  return (btn == bDOWN);
}

// *** END BUTTON ***
