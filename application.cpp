// This #include statement was automatically added by the Spark IDE.
#include "LEDEffect.h"
#include "clickButton.h"
//#include <LEDEffect.h>

// General Garage Controller Config
int version = 12;        // Current version number of this code
int internalState = 0;  // 0 = Normal Operation

// LED Config
const int ledPin = D0;

LEDEffect led1(ledPin);
int ledState = 0;

// Relay Config
const int rlyPin = D1;
char rlyState = 0;
unsigned long rlyLastTime = 0;
const int rlyLockoutTime = 3000; // Seconds
const int rlyTime = 1000;
const int rlyDelayTime = 15000;

// Door Switch / Reed Switch
const int doorSwitchPin = D4;
int doorSwitchState;
int lastDoorSwitchState;
unsigned long lastDoorDebounceTime;
unsigned long debounceDelay = 50;       // 50 ms
int doorState = -1;  // -1 = Unknown, 0 = Closed, 1 = Open
#define DOOR_UNKNOWN -1
#define DOOR_CLOSED 0
#define DOOR_OPEN 1

// Button Config
const int buttonPin1 = D3;
ClickButton button1(buttonPin1, LOW, CLICKBTN_PULLUP);

// Cloud Functions
int get(String command);
int set(String command);
int go(String command);

void setup()
{
    // Pin Mode Config
    pinMode(ledPin, OUTPUT);
    pinMode(rlyPin, OUTPUT);
    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(doorSwitchPin, INPUT_PULLUP);
    
    // Clickbutton Setup
    button1.debounceTime   = 20;   // Debounce timer in ms
    button1.multiclickTime = 250;  // Time limit for multi clicks
    button1.longClickTime  = 1000; // time until "held-down clicks" register

    // Door Switch / Reed Switch Config/Setup
    doorSwitchState = digitalRead(doorSwitchPin);
    lastDoorDebounceTime = millis();
    
    led1.on();
    ledState = 1;
    
    led1.breath(30);
    
    digitalWrite(rlyPin, LOW);
    
    // Expose a few functions
    Spark.function("get", get);
    Spark.function("set", set);
    Spark.function("go", go);
    
    // Expose a few variables
    Spark.variable("version", &version, INT);
    Spark.variable("intState", &internalState, INT);
    Spark.variable("doorState", &doorState, INT);
    
    RGB.control(true);
}

int go(String command)
{
    if(command == "relay")
    {
        Spark.publish("GARAGE-ACTION", "REMOTE-ACTUATION");
        return goRelay();
    }
    else if(command == "relay-delay")
    {
        Spark.publish("GARAGE-ACTION", "REMOTE-ACTUATION-DELAY");
        goRelayWithDelay();        
    }
    else
        return -1;
}

int set(String command)
{
    if(command == "lock")
    {
        return -2;
    }
    else if(command == "unlock")
    {
        return -3;
    }
    else
    {
        return -1;
    }
}

int get(String command)
{
  // look for the matching argument "version"
  if(command == "version")
  {
    // do something here
    return version;
  }
  else if(command == "rlyState")
  {
      return (int)rlyState;
  }
  else if(command == "ledState")
  {
      return ledState;
  }
  else return -1;
}

int goRelay()
{
    if(rlyState == 0)   // Relay is "normal"
    {
        rlyState = 1;
        rlyLastTime = millis();
        return 1;
    }
    else if(rlyState == 3)  // Relay is in delayed respose mode. Cancel the response
    {
        Spark.publish("GARAGE-ACTION", "DELAY-CANCELED");
        rlyState = 0;
    }
    return 0;
}

int goRelayWithDelay()
{
    if(rlyState == 0)   // Relay is "normal"
    {
        rlyState = 3;
        rlyLastTime = millis();
        return 1;
    }
    return 0;   
}

void relayUpdate()
{
    // Relay is "ON" and should stay on for rlyTime ms
    if(rlyState == 1)
    {
        led1.blink(125);
        digitalWrite(rlyPin, HIGH);
        if((millis() - rlyLastTime) > rlyTime)
        {
            rlyState = 2;   // Lock the relay
        }
    }
    // Relay is "locked out" and should stay that way for rlyLockoutTime ms
    else if(rlyState == 2)
    {
        led1.dim(50);
        digitalWrite(rlyPin, LOW);
        if((millis() - rlyLastTime) > rlyLockoutTime)
        {
            rlyState = 0;   // Lock the relay
        }
    }
    // Relay should fire in rlyDelayTime ms
    else if(rlyState == 3)
    {
        led1.blink(500);
        if((millis() - rlyLastTime) > rlyDelayTime)
        {
            rlyLastTime = millis();
            rlyState = 1;   // Lock the relay
        }
    }
    else
    {
        rlyState = 0;
        digitalWrite(rlyPin, LOW);
    }
}

void stateLEDUpdate()
{
    if(rlyState == 0)   // Normal Relay Operation
    {
        if(internalState == 0)  // Normal Operation
        {
            // Door is CLOSED
            if(doorState == DOOR_CLOSED)
                led1.on();
            // Door is OPEN
            else if(doorState == DOOR_OPEN)
                led1.breath(25);
        } 
    }
    
}

/*
const int doorSwitchPin = D4;
int doorSwitchState;
int lastDoorSwitchState;
unsigned long lastDoorDebounceTime;
unsigned long debounceDelay = 50;       // 50 ms
*/

void doorSwitchUpdate()
{
    int currentSwitchState = digitalRead(doorSwitchPin);
    
    if(currentSwitchState != lastDoorSwitchState)
    {
        lastDoorDebounceTime = millis();
    }
    
    if((millis() - lastDoorDebounceTime) > debounceDelay)
    {
        // Door is CLOSED
        if ((lastDoorSwitchState == HIGH) && (doorState != DOOR_CLOSED))
        {
            doorState = DOOR_CLOSED;
            RGB.color(255, 125, 0);     // Orange
            Spark.publish("GARAGE-DOOR", "CLOSED");
        }
        // Door is OPEN
        else if ((lastDoorSwitchState == LOW) && (doorState != DOOR_OPEN))
        {
            doorState = DOOR_OPEN;
            RGB.color(0, 125, 255);     // Blue Green (Teal?)         
            Spark.publish("GARAGE-DOOR", "OPEN");
        }
    }
    
    lastDoorSwitchState = currentSwitchState;
}

void loop()
{
    // Update button state
    stateLEDUpdate();
    
    button1.Update();
    led1.update();
    
    relayUpdate();
    
    doorSwitchUpdate();
    
    if(button1.clicks == 1)
    {
        Spark.publish("GARAGE-ACTION", "LOCAL-ACTUATION");
        goRelay();
    }
    
    if(button1.clicks == 2)
    {
        Spark.publish("GARAGE-ACTION", "LOCAL-ACTUATION-DELAY");
        goRelayWithDelay();
    }
    /*if(ledState == 0)
        {
            led1.on();
            ledState = 1;
        }
        else if(ledState == 1)
        {
            led1.breath(30);
            ledState = 2;
        }
        else if(ledState == 2)
        {
            led1.fadeDown(30);
            ledState = 3;
        }
        else if(ledState == 3)
        {
            led1.fadeUp(10);
            ledState = 4;
        }
        else if(ledState == 4)
        {
            led1.blink(250);
            ledState = 5;
        }
        else
        {
            led1.off();
            ledState = 0;
        }*/
      
}