//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_NODE_ID 2
#include <MyConfig.h>
#include <MySensors.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h> // F Malpartida's NewLiquidCrystal library
#include <Wire.h>
#include <DallasTemperature.h>
#include <TimerObject.h>
struct Sensor
{
  int childId;
  int sensorType;
  int variableType;
  String description;
  bool initValueReceived;
  String stringValue;
  bool boolValue;
  int intValue;
  float floatValue;
};

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define ONE_WIRE_BUS 4 // digital 4 ??
OneWire ds(ONE_WIRE_BUS);
DallasTemperature dallasSensors(&ds);

#define RELAY_PELTIER_PIN 5 //digital pin 5
#define RELAY_COOLER_PIN 6 //digital pin 6
#define RELAY_WATERPUMP_PIN 7 //digital pin 7
#define RELAY_ON LOW
#define RELAY_OFF HIGH

#define WATER_LVL_PIN  A0 // analog 1 ??
#define WATER_LVL_POWER_PIN  8 // digital pin  ??
unsigned long SENSOR_SLEEP_TIME = 3000; // Sleep time between reads (in milliseconds)
unsigned long DISPLAY_CHANGE_TIME = 3000; // Sleep time between reads (in milliseconds)

#define MAX_INTERNAL_WATERTEMP 30

#define NUMBER_OF_SENSORS 9 //8
//Uno, Nano, Mini, other 328-based  2, 3
#define INTERRUPT_PIN 3
const int awakeTime = 20; //in seconds
const bool ENABLE_SLEEP_MODE = true;
const char Sketch_Info[] = "AquariumControll";
const char Sketch_Version[] = "1.6";

Sensor sensors[NUMBER_OF_SENSORS] = {
  {
    0,
    S_HVAC,
    V_HVAC_FLOW_STATE,
    "HVAC", //"watercoolerFlowState"
    false,
    "",
    false,
    0,
    0.0
  },
  {
    0,
    S_HVAC,
    V_HVAC_SPEED,
    "HVAC", //"watercoolerSpeed"
    false,
    "",
    false,
    0,
    0.0
  },
  {
    0,
    S_HVAC,
    V_HVAC_SETPOINT_COOL,
    "HVAC", //"watercoolerCoolPoint" start cooling above
    false,
    "",
    false,
    0,
    0.0
  },
  {
    1,
    S_WATER,
    V_VOLUME,
    "waterlevel",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    2,
    S_WATER,
    V_FLOW,
    "waterflow", //waterflow from sensor
    false,
    "",
    false,
    0,
    0.0
  },
  {
    3,
    S_WATER_QUALITY,
    V_TEMP,
    "internalWaterTemp",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    0,
    S_HVAC,
    V_TEMP,
    "HVAC",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    4,
    S_SPRINKLER,
    V_TRIPPED,
    "Coolingstate",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    0,
    S_HVAC,
    V_HVAC_SETPOINT_HEAT,
    "HVAC", //"watercoolerHeatPoint" stop cooling below
    false,
    "",
    false,
    0,
    0.0
  }
};

int currentWakeTime = 0;
TimerObject* displayValuesTimer = new TimerObject(DISPLAY_CHANGE_TIME);

//flow rate config
#define INTERRUPT_PIN_FLOW_SENSOR 3
float calibrationFactorFlowRate = 4.5;
volatile byte pulseCountFlowRate;
unsigned long totalMilliLitresFlowRate;
unsigned long oldTimeFlowRate;

TimerObject* peltierTimerOn = new TimerObject(10000);
TimerObject* peltierTimerOff = new TimerObject(30000);
bool isCooling = false;
bool peltierOnOffProcessRunning = false;

MyMessage msg;

void before()
{
  Serial.println("before");
  // Initialize external hardware etc here...
  currentWakeTime = millis() / 1000;
  if (ENABLE_SLEEP_MODE) {
    pinMode(INTERRUPT_PIN, INPUT);
  }
  // Switch on the backlight
  lcd.begin();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  dallasSensors.begin();
  Serial.println("beforeend");

  //timer
  displayValuesTimer->setOnTimer(&printScreen);
  displayValuesTimer->Start();
  peltierTimerOn->setOnTimer(&turnOnPeltier);
  peltierTimerOff->setOnTimer(&turnOffPeltier);

  //init relay
  pinMode(RELAY_PELTIER_PIN, OUTPUT);
  pinMode(RELAY_COOLER_PIN, OUTPUT);
  pinMode(RELAY_WATERPUMP_PIN, OUTPUT);
  digitalWrite(RELAY_PELTIER_PIN, RELAY_OFF);
  digitalWrite(RELAY_COOLER_PIN, RELAY_OFF);
  digitalWrite(RELAY_WATERPUMP_PIN, RELAY_OFF);

  //init waterflow sensor
  pinMode(INTERRUPT_PIN_FLOW_SENSOR, INPUT);
  digitalWrite(INTERRUPT_PIN_FLOW_SENSOR, HIGH);
  pulseCountFlowRate = 0;
  totalMilliLitresFlowRate = 0;
  oldTimeFlowRate = 0;
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_FLOW_SENSOR), pulseCounter, FALLING);

  //waterlevel pin
  pinMode(WATER_LVL_POWER_PIN, OUTPUT);
  digitalWrite(WATER_LVL_POWER_PIN, LOW);
}

void presentation()
{
  Serial.println("presentation start");
  sendSketchInfo(Sketch_Info, Sketch_Version);
  bool hvacPresent = false;
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    Sensor sensor = sensors[i];
    int id = sensor.childId;
    if (id != 0 || hvacPresent == false) {
      if (id == 0) {
        hvacPresent = true;
        Serial.println("hvacPresented");
      }
      int typee = sensor.sensorType;
      String description = sensor.description;//getDescription();
      int stringLength = description.length() + 1;
      char descrBuffer[stringLength];
      description.toCharArray(descrBuffer, stringLength);

      /*Serial.print("presenting Sensor '");
        Serial.print(id);
        Serial.print( "' with type '");*/

      present(id, typee, descrBuffer);
      wait(500);
    }
  }
  Serial.println("presentation end");
}

float lastWaterLvlUpdate = 0;
const float waterLvlUpdateInterval = 10800000; // 3 hours
int wakeStatus = 0;
bool sleeping = false;
int pressedLoops = 0;
void loop()
{
  wait(1000);
  if (ENABLE_SLEEP_MODE) {
    wakeStatus = digitalRead(INTERRUPT_PIN);
    if (wakeStatus == HIGH) {
      wakeMeUp();
      pressedLoops ++;
    }else{
      pressedLoops = 0;
    }
  }

  peltierTimerOn->Update();
  peltierTimerOff->Update();
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    Sensor sensor = sensors[i];
    int variableType = sensor.variableType;
    int id = sensor.childId;
    //if not initialValue received
    if (sensor.initValueReceived == false) {
      //LogDebug("requesting initialValue for Sensor '" + String(i) + "' ");

      Serial.print(i);
      Serial.println(" sending init :");
      //Serial.print(loadState(id));
      if (id == 0 && variableType == V_HVAC_SPEED) {
        //Serial.println("Min");
        msg.setType(variableType).setSensor(id).set("Min");
        sensors[i].stringValue = "Min";
      }
      else if (id == 0 && variableType == V_HVAC_SETPOINT_COOL) {
        float value = loadState(43);
        //Serial.println(value);
        msg.setType(variableType).setSensor(id).set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 0 && variableType == V_HVAC_SETPOINT_HEAT) {
        float value = loadState(44);
        //Serial.println(value);
        msg.setType(variableType).setSensor(id).set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 0 && variableType == V_TEMP) {
        //Serial.println("24.0");
        float value = 24.0;
        msg.setType(variableType).setSensor(id).set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 4 && variableType == V_TRIPPED) {
        //Serial.print(false);
        bool value = false;
        msg.setType(variableType).setSensor(id).set(value);
        sensors[i].boolValue = value;
      }
      else if (id == 0 && variableType == V_HVAC_FLOW_STATE) {
        int state = loadState(42);
        //Serial.print(" state ");
        //Serial.println(state);
        sensors[i].intValue = "AutoChangeOver";
        msg.setType(variableType).setSensor(id).set("Off");
        if (state == 2) {
          //Serial.println("CoolOn");
          int state = 2;
          msg.setType(variableType).setSensor(id).set("CoolOn");
        }
        else if (state == 0) {
          //Serial.println("Off");
          int state = 0;
          msg.setType(variableType).setSensor(id).set("Off");
        }
        else if (state == 3) {
          //Serial.println("HeatOn");
          int state = 3;
          msg.setType(variableType).setSensor(id).set("HeatOn");
        }
        else if (state == 1) {
          //Serial.println("AutoChangeOver");
          int state = 1;
          msg.setType(variableType).setSensor(id).set("AutoChangeOver");
        }
        sensors[i].intValue = state;
      }
      else {
        //Serial.println("other");
        int val = loadState(id);
        msg.setType(variableType).setSensor(id).set(val);
        sensors[i].intValue = val;
        //ASerial.println(loadState(id));
      }

      send(msg);
      request(id, variableType);
      wait(2000, C_SET, variableType);
    }
    else {
      switch (i) {
        case 0: //"waterflowState"
          {
            int state = sensor.intValue;
            if(pressedLoops>=3){
              if(isCooling==true){
                sensors[0].intValue = 0;
                state = 0;
                msg.set("Off");
                Serial.println("Touch Off");
              }else{                
                sensors[0].intValue = 2;
                state = 2;
                msg.set("CoolOn");
                Serial.println("Touch On");
              }
              msg.setType(variableType).setSensor(id);
              send(msg);
              saveState(42, state);
            }
            //Serial.print("     state: ");
            //Serial.println(state);
            if (state == 2) {
              coolWater(true);
            }
            else if (state == 0) {
              coolWater(false);
            }
            else if (state == 3) {
              //heat
            }
            else if (state == 1) {
              //auto
            }
          } break;
        case 1: //"watercoolerSpeed"
          {
            /*String coolingSpeed = "Max";//45;//getCoolingSpeed();
              String oldCoolingState = sensor.getString();
              if(coolingSpeed != oldCoolingState)
              {
                LogDebug("sending message '" + String(oldCoolingState) + "' for Sensor '" + String(i) + "'");
                msg.setType(variableType).setSensor(id).set(coolingSpeed);
                send(msg);
                sensors[i].setValue(coolingSpeed);
              }*/
          } break;
        case 3: //"waterlevel"
          {
            float compare = millis() - waterLvlUpdateInterval;
            if (lastWaterLvlUpdate == 0 || lastWaterLvlUpdate < compare || wakeStatus == HIGH) {
              float waterlevel = getWaterLevel();
              float oldWaterLevel = sensor.floatValue;
              if (waterlevel != oldWaterLevel) {
              
                //ASerial.println("sending message '" + String(waterlevel) + "' for Sensor '" + String(i) + "'");
                msg.setType(variableType).setSensor(id).set(waterlevel, 2);
                send(msg);
                lastWaterLvlUpdate = millis();
              }
              saveState(id, waterlevel);
              sensors[i].floatValue = waterlevel;
            }

          } break;
        case 4: //"waterflow"
          {
            float waterflow = getWaterFlow();
            float oldWaterflow = sensor.floatValue;
            if (waterflow >= 0 && waterflow != oldWaterflow) {
              //ASerial.println("sending message '" + String(waterflow) + "' for Sensor '" + String(i) + "'");
              msg.setType(variableType).setSensor(id).set(waterflow, 2);
              send(msg);
              saveState(id, waterflow);
              sensors[i].floatValue = waterflow;
            }
          } break;
        case 5: //"internal water temp"
          {
            float internalWaterTemp = getWaterTemperature(1);
            float oldInternalWaterTemp = sensor.floatValue;
            if (internalWaterTemp != oldInternalWaterTemp) {
              // turn cooler off when peltier getting too hot
              if (internalWaterTemp >= MAX_INTERNAL_WATERTEMP) {
                //Serial.println("Stop Cooling because too hot");
                coolWater(false);
              }
              //ASerial.println("sending message '" + String(internalWaterTemp) + "' for Sensor '" + String(i) + "'");
              msg.setType(variableType).setSensor(id).set(internalWaterTemp, 2);
              send(msg);
              saveState(id, internalWaterTemp);
              sensors[i].floatValue = internalWaterTemp;
            }
          } break;
        case 6: //"hvac water temp"
        case 2: //"watercoolerCoolPoint"
        case 8:
          {
            float watertemperature = getWaterTemperature(0);
            float coolPoint = sensors[2].floatValue;
            float heatPoint = sensors[8].floatValue;
            int coolingstate = sensors[0].intValue;

            if (coolingstate == 1) {
              if (watertemperature <= heatPoint) {
                coolWater(false);
              }
              else if (watertemperature >= coolPoint) {
                coolWater(true);
              }
            }

            if (i == 6) {
              float oldWatertemperature = sensors[6].floatValue;
              if (watertemperature != oldWatertemperature) {
                //ASerial.println("sending message '" + String(watertemperature) + "' for Sensor '" + String(i) + "'");
                msg.setType(variableType).setSensor(id).set(watertemperature, 2);
                send(msg);
                saveState(id, watertemperature);
                sensors[i].floatValue = watertemperature;
              }
            }
          } break;
        case 7: //"cooling state"
          {
            float oldstate = sensors[7].boolValue;
            if (isCooling != oldstate) {
              //ASerial.println("sending message '" + String(isCooling) + "' for Sensor '" + String(i) + "'");
              msg.setType(variableType).setSensor(id).set(isCooling);
              send(msg);
              saveState(id, isCooling);
              sensors[i].boolValue = isCooling;
            }

          } break;
      }
    }
  }
  displayValuesTimer->Update();
  if (ENABLE_SLEEP_MODE) {
    int secondsUntilStart = millis() / 1000;
    //Serial.println("SecondsUntilStart: " + String(secondsUntilStart));
    //Serial.println("CurrentWakeTime: " + String(currentWakeTime));
    //Serial.println("AwakeTime: " + String(currentWakeTime + awakeTime));
    //Avoid sleep when peltier is turning on or off
    if (peltierOnOffProcessRunning == false && (currentWakeTime + awakeTime) <= secondsUntilStart && sleeping == false) {
      fallAsleep();
    }
  }
}

void fallAsleep()
{
  Serial.println("fallAsleep");
  lcd.clear();
  lcd.setBacklight(LOW);
  lcd.noDisplay();
  displayValuesTimer->Pause();
  sleeping == true;
}

int printPage = 0;
void wakeMeUp()
{
  Serial.println("wakeMeUp");
  lcd.setBacklight(HIGH);
  lcd.display();
  //do your wakeup stuff here
  currentWakeTime = millis() / 1000;
  printPage = 0;
  displayValuesTimer->Resume();
  sleeping = false;
}

float getWaterTemperature(int index)
{
  dallasSensors.requestTemperatures();
  float sensorValueWaterTemp = dallasSensors.getTempCByIndex(index);
  //LogDebug("getWaterTemperature "+String(index)+": " + String(sensorValueWaterTemp));
  return sensorValueWaterTemp; //(float)
}

float getWaterLevel()
{
  Serial.print("getWaterLevel: ");
  digitalWrite(WATER_LVL_POWER_PIN, HIGH);
  wait(100);
  int sensorValueWaterLevel = analogRead(WATER_LVL_PIN);
  digitalWrite(WATER_LVL_POWER_PIN, LOW);
  int waterlevel = sensorValueWaterLevel / 1000.0 * 100.0;

  Serial.println(waterlevel);
  return waterlevel;
}

String getCoolingState()
{
  if (isCooling == true) {
    return "CoolOn";
  }
  else {
    return "Off";
  }
}

float getWaterFlow()
{
  Serial.println("getWaterFlow");
  float retVal = -1;
  /*if ((millis() - oldTimeFlowRate) > 1000) // Only process counters once per second
    {
      // Disable the interrupt while calculating flow rate and sending the value to
      // the host
      detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_FLOW_SENSOR));

      float flowRate = ((1000.0 / (millis() - oldTimeFlowRate)) * pulseCountFlowRate) / calibrationFactorFlowRate;

      Serial.print("flowRate ");
      Serial.println(flowRate);
      oldTimeFlowRate = millis();

      int flowMilliLitres = (flowRate / 60) * 1000;

      // Add the millilitres passed in this second to the cumulative total
      totalMilliLitresFlowRate += flowMilliLitres;
      //LogDebug("totalMilliLitresFlowRate: "+String(totalMilliLitresFlowRate));
      //LogDebug("totalMilliLitresFlowRateL: "+String(totalMilliLitresFlowRate/1000));
      // Reset the pulse counter so we can start incrementing again
      pulseCountFlowRate = 0;

      // Enable the interrupt again now that we've finished sending output
      attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN_FLOW_SENSOR), pulseCounter, FALLING);
      retVal = flowRate;
    }*/
  return retVal; //rand() % 101 + 0;
}

void pulseCounter()
{
  // Increment the pulse counter
  pulseCountFlowRate++;
}

float lastPeltierCycle = 0;
float peltierCoolDownTime = 0;//300000;
int coolWater(bool turnOn)
{
  String turnOnStr = turnOn ? "on" : "off";
  //Serial.println("coolWater " + turnOnStr);

  //LogDebug("lastPeltierCycle:" +String(lastPeltierCycle));
  float compare = millis() - peltierCoolDownTime;
  //LogDebug("compare:" +String(compare));
  if (lastPeltierCycle == 0 || lastPeltierCycle < compare) {
    //avoid turn on/off when already turning on/off
    if (peltierOnOffProcessRunning == false) {
      if (turnOn == true && isCooling == false) {
        peltierOnOffProcessRunning = true;
        isCooling = true;
        // Turn water pump on/power supply/cooling fans
        digitalWrite(RELAY_WATERPUMP_PIN, RELAY_ON);
        // wait for some seconds
        peltierTimerOn->Start();
        // Turn off if running for too long e.g. 6 hours
      }
      else if (turnOn == false && isCooling == true) {
        peltierOnOffProcessRunning = true;
        isCooling = false;
        // Turn peltierelement off
        digitalWrite(RELAY_PELTIER_PIN, RELAY_OFF);
        // wait for some seconds
        peltierTimerOff->Start();
      }
    }
  }
}

void turnOnPeltier()
{
  Serial.println("turnOnPeltier");
  // Turn cooler on
  digitalWrite(RELAY_COOLER_PIN, RELAY_ON);
  delay(1000);
  // Turn peltierelement on
  digitalWrite(RELAY_PELTIER_PIN, RELAY_ON);
  peltierOnOffProcessRunning = false;
  peltierTimerOn->Stop();
}

void turnOffPeltier()
{
  Serial.println("turnOffPeltier");
  // Turn cooler off
  digitalWrite(RELAY_COOLER_PIN, RELAY_OFF);
  // Turn water pump off
  digitalWrite(RELAY_WATERPUMP_PIN, RELAY_OFF);
  peltierOnOffProcessRunning = false;
  lastPeltierCycle = millis();
  peltierTimerOff->Stop();
}

void printScreen()
{
  // Reset the display
  lcd.clear();
  lcd.backlight();
  char lineOne[16] = "";
  char lineTwo[16] = "";

  if (printPage == 0) {
    float waterlevel = sensors[3].floatValue;
    strncpy(lineOne, "Waterlvl ", 16);
    char waterlvlChar[7];
    dtostrf(waterlevel, 6, 1, waterlvlChar);
    strcat(lineOne, waterlvlChar);

    float waterTemp = sensors[6].floatValue;
    strncpy(lineTwo, "Watertemp ", 16);
    char waterTempChar[6];
    dtostrf(waterTemp, 5, 1, waterTempChar);
    strcat(lineTwo, waterTempChar);

    dtostrf(waterTemp, 4, 1, &lineTwo[11]);
    printPage++;
  }
  else if (printPage == 1) {
    float coolingPoint = sensors[2].floatValue;
    strncpy(lineOne, "Coolpoint ", 16);
    char coolingPointChar[6];
    dtostrf(coolingPoint, 5, 1, coolingPointChar);
    strcat(lineOne, coolingPointChar);

    float heatPoint = sensors[8].floatValue;
    strncpy(lineTwo, "Heatpoint ", 16);
    char heatPointChar[6];
    dtostrf(heatPoint, 5, 1, heatPointChar);
    strcat(lineTwo, heatPointChar);

    //int state = sensors[0].intValue;
    //sprintf(lineTwo, "State:        %d", state);

    printPage++;
  }
  else if (printPage == 2) {
    float flowrate = sensors[4].floatValue;
    strncpy(lineOne, "Flowrate ", 16);
    char flowrateChar[7];
    dtostrf(flowrate, 6, 1, flowrateChar);
    strcat(lineOne, flowrateChar);

    float total = totalMilliLitresFlowRate / 1000;
    strncpy(lineTwo, "Total ", 16);
    char totalChar[10];
    dtostrf(total, 9, 1, totalChar);
    strcat(lineTwo, totalChar);

    printPage = 0;
  }
  lcd.setCursor(0, 0);
  lcd.print(lineOne);
  lcd.setCursor(0, 1);
  lcd.print(lineTwo);
}

void receive(const MyMessage& message)
{
  Serial.println("receive ");

  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    Sensor sensor = sensors[i];
    int id = sensor.childId;
    int sensorType = sensor.sensorType;
    int variableType = sensor.variableType;
    msg.setType(variableType);
    msg.setSensor(id);
    if (id == message.sensor) {
      if (variableType == message.type) {
        if (sensor.initValueReceived == false) {
          sensors[i].initValueReceived = true;
          Serial.print("initialValueReceived for Sensor ");
          //Serial.println(String(id));
        }
        switch (variableType) {
          case V_PERCENTAGE: {
              int ivalue = message.getInt();
              saveState(id, ivalue);
              sensors[i].intValue = ivalue;
              send(msg.set(ivalue));
              //Serial.print("V_PERCENTAGE ");
              //ASerial.println("received message '" + String(ivalue) + "' for Sensor '" + String(i) + "'");
            } break;
          case V_VOLUME:
          case V_TEMP:
          case V_FLOW:
          case V_HUM:
          case V_PRESSURE:
          case V_LEVEL:
          case V_VOLTAGE: {
              float fvalue = message.getFloat();
              saveState(id, fvalue);
              sensors[i].floatValue = fvalue;
              send(msg.set(fvalue, 2));
              //Serial.print("V_VOLTAGE ");
              //ASerial.println("received message '" + String(fvalue) + "' for Sensor '" + String(i) + "'");
            } break;
          case V_HVAC_SETPOINT_COOL: {
              float fvalue = message.getFloat();
              saveState(43, fvalue);
              sensors[i].floatValue = fvalue;
              send(msg.set(fvalue, 2));
              //Serial.print("V_HVAC_SETPOINT_COOL ");
              //ASerial.println("received message '" + String(fvalue) + "' for Sensor '" + String(i) + "'");
            } break;
          case V_HVAC_SETPOINT_HEAT: {
              float fvalue = message.getFloat();
              saveState(44, fvalue);
              sensors[i].floatValue = fvalue;
              send(msg.set(fvalue, 2));
            } break;
          case V_STATUS:
          case V_TRIPPED: {
              bool bvalue = message.getBool();
              saveState(id, bvalue);
              sensors[i].boolValue = bvalue;
              send(msg.set(bvalue));
              //Serial.print("V_TRIPPED ");
              //ASerial.println("received message '" + String(bvalue) + "' for Sensor '" + String(i) + "'");
            } break;
          case V_HVAC_FLOW_STATE: {
              const char* recvData = message.getString();
              int state = 0;
              if (strcmp(recvData, "CoolOn") == 0) {
                state = 2;
                //Serial.println("Cooling from server");
                //coolWater(true);
              }
              else if (strcmp(recvData, "Off") == 0) {
                state = 0;
                //Serial.println("Stop Cooling from server");
                //coolWater(false);
              }
              else if (strcmp(recvData, "HeatOn") == 0) {
                state = 3;
              }
              else if (strcmp(recvData, "AutoChangeOver") == 0) {
                state = 1;
              }
              else {
              }
              saveState(42, state);
              sensors[i].intValue  = state;
              //Serial.print("V_HVAC_FLOW_STATE ");
              //ASerial.println("received data '" + String(recvData) + "' for Sensor '" + String(i) + "'");
            }
          case V_HVAC_SPEED: {
              //Serial.println(" V_HVAC_SPEED ");
              String recvData = message.data;
              sensors[i].stringValue = recvData;
              //ASerial.println("received data '" + String(recvData) + "' for Sensor '" + String(i) + "'");
              /* if(recvData.equalsIgnoreCase("auto")) fan_speed = 0;
                else if(recvData.equalsIgnoreCase("min")) fan_speed = 1;
                else if(recvData.equalsIgnoreCase("normal")) fan_speed = 2;
                else if(recvData.equalsIgnoreCase("max")) fan_speed = 3;*/
            } break;
          case V_TEXT: {
              String svalue = message.getString();
              //saveState(id,svalue);
              sensors[i].stringValue = svalue;
              //send(msg.set(svalue));
              //Serial.print("V_TEXT ");
              //ASerial.println("received message '" + String(svalue) + "' for Sensor '" + String(i) + "'");
            } break;
        }
      }
    }
  }
}

