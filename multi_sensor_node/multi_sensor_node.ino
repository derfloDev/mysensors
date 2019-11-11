#include <SPI.h>
#include <MySensors.h>

#define VARIABLE_TYPE V_STATUS
#define SENSOR_TYPE S_BINARY
#define NUMBER_OF_SENSORS 4
//Uno, Nano, Mini, other 328-based	2, 3
//Mega, Mega2560, MegaADK	2, 3, 18, 19, 20, 21
#define INTERRUPT_PIN  21
#define INTERRUPT_MODE  CHANGE // LOW, CHANGE, RISING, FALLING
const int awakeTime = 10;//in seconds
const bool ENABLE_SLEEP_MODE = false;
const String Sketch_Info = "Info";
const String Sketch_Version = "1.0";
const bool logDebug = true;

MyMessage msg(0, VARIABLE_TYPE);
int initialValueReceived[NUMBER_OF_SENSORS];
int values[NUMBER_OF_SENSORS];
int currentWakeTime = 0;
int oldBatteryLevel = 100;

void before(){
  // Initialize external hardware etc here...
  currentWakeTime = millis()/1000;
  for( int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++ ) {
    initialValueReceived[sensor] = false;
    values[sensor] = 0;
  }
  if(ENABLE_SLEEP_MODE)
  {
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  }  
  if(ENABLE_SLEEP_MODE)
  {
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), wakeUp, INTERRUPT_MODE);
  }
}

void presentation()  
{ 
  sendSketchInfo(Sketch_Info, Sketch_Version);
  
  for( int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++ ) {
    LogDebug("presenting Sensor '" + String(sensor) + "' with type '" + String(SENSOR_TYPE) + "'");
    present(sensor, SENSOR_TYPE);
  }
}


void loop(){
  
  for( int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++ ) {  
    //if not initialValue received
    if(!initialValueReceived[sensor])
    {
      LogDebug("requesting initialValue for Sensor '" + String(sensor) + "'");
      request(sensor, S_BINARY);
    }
    
    msg.setSensor(sensor);    
    //Get value from external hardware/sensor etc..
    values[sensor] = rand() % 101 + 0;
    send(msg.set(values[sensor]));
    LogDebug("sending message '" + String(values[sensor]) + "' for Sensor '" + String(sensor) + "'");
  }

  // calc battery and send
  int batteryLevel =  rand() % 101 + 0;
  if(oldBatteryLevel != batteryLevel)
  {
    LogDebug("sending batteryLebel '" + String(batteryLevel));
    sendBatteryLevel(batteryLevel);
  }
  
  if(ENABLE_SLEEP_MODE)
  {
    secondsUntilStart = millis()/1000;
    LogDebug("SecondsUntiStart: " + String(secondsUntilStart));
    if((currentWakeTime + awakeTime) >= secondsUntilStart)
    {
      LogDebug("currentWakeTime: " + String(currentWakeTime));
      fallAsleep();
    }
  }
}

void fallAsleep(){
  LogDebug("fallAsleep");
  //send data before sleep, turn of hardware etc...  
  // example tft.displayOff()
  
}

void wakeUp(){
  LogDebug("wakeUp");
  //do your wakeup stuff here
  currentWakeTime = millis()/1000;
  // example tft.displayOn()
}

void receive(const MyMessage &message){
  if (message.type == VARIABLE_TYPE) {    
    int sensor = message.sensor;
    if(!initialValueReceived[sensor])
    {      
      initialValueReceived[sensor] = true;
      LogDebug("initialValueReceived for Sensor '" + String(sensor) + "'");
    }
    int value = message.getInt();//getString();getByte();getFloat();getLong();getULong();getBool();
    values[sensor] = value
    LogDebug("received message '" + String(value) + "' for Sensor '" + String(sensor) + "'");
  }  
}

void LogDebug(String message)
{
  if(logDebug==true)
  {    
    Serial.print("Debug :");
    Serial.println(message);
  }
}
