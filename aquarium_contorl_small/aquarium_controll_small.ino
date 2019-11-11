//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_NODE_ID 34
#define MY_BAUD_RATE 9600
#include <MyConfig.h>
#include <MySensors.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
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

#define FAN1_PIN 3 //pwm pin 3
#define FAN2_PIN 5 //pwm pin 5

#define RELAY_LIGHT_PIN 6 //digital pin 6
#define RELAY_AIRPUMP_PIN 7 //digital pin 7
#define RELAY_ON LOW
#define RELAY_OFF HIGH

#define WATER_LVL_PIN  A0 // analog 1 ??
#define WATER_LVL_POWER_PIN  8 // digital pin  ??
unsigned long DISPLAY_CHANGE_TIME = 3000; // Sleep time between reads (in milliseconds)

#define NUMBER_OF_SENSORS 11 //8
//Uno, Nano, Mini, other 328-based  2, 3
#define INTERRUPT_PIN 2
const int awakeTime = 20; //in seconds
const bool ENABLE_SLEEP_MODE = true;
const char Sketch_Info[] = "SmallAquariumControll";
const char Sketch_Version[] = "1.0";

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
    "HVAC", //maxTemp "watercoolerCoolPoint" min temp
    false,
    "",
    false,
    0,
    0.0
  },
  {
    0,
    S_HVAC,
    V_TEMP, // acutal water temperature
    "HVAC",
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
    "HVAC", //minTemp "watercoolerHeatPoint" max temp
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
    "waterlevel", // waterlevel
    false,
    "",
    false,
    0,
    0.0
  },
  {
    2,
    S_BINARY,
    V_STATUS,
    "Licht", // light state
    false,
    "",
    false,
    0,
    0.0
  },
  {
    3,
    S_BINARY,
    V_STATUS,
    "Membranpumpe", //air pump state
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
    5,
    S_INFO,
    V_TEXT,
    "MembranpumpeInterval",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    6,
    S_INFO,
    V_TEXT,
    "MembranpumpeOnTime",
    false,
    "",
    false,
    0,
    0.0
  },

};

int currentWakeTime = 0;
TimerObject* displayValuesTimer = new TimerObject(DISPLAY_CHANGE_TIME);
TimerObject* airPumpInterval = new TimerObject(1000);
TimerObject* airPumpTime = new TimerObject(1000);
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
  airPumpInterval->setOnTimer(&airPumpOn);
  airPumpTime->setOnTimer(&airPumpOff);
  airPumpInterval->setEnabled(false);
  airPumpTime->setEnabled(false);
  displayValuesTimer->setOnTimer(&printScreen);
  displayValuesTimer->Start();

  //init relay
  pinMode(RELAY_LIGHT_PIN, OUTPUT);
  pinMode(RELAY_AIRPUMP_PIN, OUTPUT);
  digitalWrite(RELAY_LIGHT_PIN, RELAY_OFF);
  digitalWrite(RELAY_AIRPUMP_PIN, RELAY_OFF);

  // PWM
  pinMode(FAN1_PIN, OUTPUT);
  pinMode(FAN2_PIN, OUTPUT);
  digitalWrite(FAN1_PIN, LOW);
  digitalWrite(FAN2_PIN, LOW);
  analogWrite(FAN1_PIN, 0);
  analogWrite(FAN2_PIN, 0);

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
      String description = sensor.description;
      int stringLength = description.length() + 1;
      char descrBuffer[stringLength];
      description.toCharArray(descrBuffer, stringLength);
      present(id, typee, descrBuffer);
      wait(500);
    }
  }
  Serial.println("presentation end");
}

float lastWaterLvlUpdate = 0;
const float waterLvlUpdateInterval = 60000; // 3 hours
int wakeStatus = 0;
bool sleeping = false;
int pressedLoops = 0;
bool isCooling = false;

void loop()
{
  Serial.println("loop");
  wait(1000);
  if (ENABLE_SLEEP_MODE) {
    wakeStatus = digitalRead(INTERRUPT_PIN);
    if (wakeStatus == HIGH) {
      wakeMeUp();
      pressedLoops ++;
    } else {
      pressedLoops = 0;
    }
  }
  bool allreceived = true;
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    Sensor sensor = sensors[i];
    int variableType = sensor.variableType;
    int id = sensor.childId;
    msg.setType(variableType).setSensor(id);
    //if not initialValue received
    if (sensor.initValueReceived == false) {
      //LogDebug("requesting initialValue for Sensor '" + String(i) + "' ");
      allreceived = false;
      Serial.print(i);
      Serial.print(" sending init id ");
      Serial.print(id);
      Serial.print(" : ");
      //Serial.print(loadState(id));
      if (id == 0 && variableType == V_HVAC_SPEED) {
        int state = loadState(45);
        sensors[i].stringValue = "Auto";
        msg.set("Auto");
        if (state == 9) {
          sensors[i].stringValue = "Auto";
          msg.set("Auto");
        }
        else if (state == 1) {
          sensors[i].stringValue = "Min";
          msg.set("Min");
        }
        else if (state == 2) {
          sensors[i].stringValue = "Normal";
          msg.set("Normal");
        }
        else if (state == 3) {
          sensors[i].stringValue = "Max";
          msg.set("Max");
        }
        Serial.print("Speed: ");
        Serial.println(state);
      }
      else if (id == 0 && variableType == V_HVAC_SETPOINT_COOL) {
        float value = loadState(43);
        if (value > 40) {
          value = 22.0;
        }
        Serial.println(value);
        Serial.println("V_HVAC_SETPOINT_COOL");
        msg.set(value, 2);
        //msg.setType(variableType).setSensor(id).set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 0 && variableType == V_HVAC_SETPOINT_HEAT) {
        float value = loadState(44);
        if (value > 40) {
          value = 23.0;
        }
        Serial.println(value);
        Serial.println("V_HVAC_SETPOINT_HEAT");
        msg.set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 0 && variableType == V_TEMP) {
        float value = getWaterTemperature(0);
        if (value > 40 || value < 0) {
          value = 26;
        }
        Serial.println(value);
        msg.set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 0 && variableType == V_HVAC_FLOW_STATE) {
        int state = loadState(42);
        sensors[i].intValue = "AutoChangeOver";
        msg.set("Off");
        if (state == 2) {
          //Serial.println("CoolOn");
          int state = 2;
          msg.set("CoolOn");
        }
        else if (state == 0) {
          //Serial.println("Off");
          int state = 0;
          msg.set("Off");
        }
        else if (state == 3) {
          //Serial.println("HeatOn");
          int state = 3;
          msg.set("HeatOn");
        }
        else if (state == 1) {
          //Serial.println("AutoChangeOver");
          int state = 1;
          msg.set("AutoChangeOver");
        }
        Serial.println(state);
        sensors[i].intValue = state;
      }
      else if (id == 2 || id == 3 || id == 4) {
        bool val = loadState(id);
        Serial.println(val);
        msg.set(val);
        sensors[i].intValue = val;
      }
      else {
        int val = loadState(id);
        if (id == 5) {
          setAirPumpInterval(val);
        }
        else if (id == 6) {
          setAirPumpTime(val);
        }
        else {
          if (val > 100) {
            val = 100;
          }
        }
        Serial.println(val);
        msg.set(val);
        sensors[i].intValue = val;
      }

      send(msg);
      request(id, variableType);
      wait(2000, C_SET, variableType);
    }
    else {
      switch (i) {
        case 0: //"waterflowState"
          {
            /*if (pressedLoops >= 3) {
              int coolingstate = sensors[0].intValue;
              if (coolingstate == 1 || coolingstate == 2) {
                sensors[0].intValue = 0;
                coolingstate = 0;
                msg.set("Off");
              } else {
                sensors[0].intValue = 2;
                coolingstate = 2;
                msg.set("CoolOn");
              }
              send(msg);
              saveState(42, coolingstate);
            }*/
          } break;
        case 1: //"watercoolerSpeed"
          {
          } break;
        case 2: // maxTemp "watercoolerCoolPoint"
          {
          } break;
        case 3: // "hvac water temp"
          {
            float watertemperature = getWaterTemperature(0);
            float oldWatertemperature = sensors[3].floatValue;
            if (watertemperature != oldWatertemperature) {
              //ASerial.println("sending message '" + String(watertemperature) + "' for Sensor '" + String(i) + "'");
              msg.set(watertemperature, 2);
              send(msg);
              saveState(id, watertemperature);
              sensors[i].floatValue = watertemperature;
            }
          } break;
        case 4: // minTemp water heat point
          {

          } break;
        case 5: //"waterlevel"
          {
            float compare = millis() - waterLvlUpdateInterval;
            //update every x hours or when user pressed the button
            if (lastWaterLvlUpdate == 0 || lastWaterLvlUpdate < compare || wakeStatus == HIGH) {
              float waterlevel = getWaterLevel();
              float oldWaterLevel = sensor.floatValue;
              if (waterlevel != oldWaterLevel) {
                msg.set(waterlevel, 2);
                send(msg);
                lastWaterLvlUpdate = millis();
                saveState(id, waterlevel);
                sensors[i].floatValue = waterlevel;
              }
            }

          } break;
        case 6: //"light state"
          //case 7: //"membranpumpe state"
          {
            bool state = sensors[i].boolValue;
            if (pressedLoops >= 3){
              if(state == true){
                state = false;
              }else{
                state = true;
              }
              sensors[i].boolValue = state;
              msg.set(state);
              send(msg);
              saveState(id, state);
            }
            int pin = 0;
            if (i == 6) {
              pin = RELAY_LIGHT_PIN;
            } else if (i == 7) {
              pin = RELAY_AIRPUMP_PIN;
            }
            if (state == true) {
              digitalWrite(pin, RELAY_ON);
            } else {
              digitalWrite(pin, RELAY_OFF);
            }

          } break;
        case 8: //"coolingstate"
          {
            float oldstate = sensors[8].boolValue;
            if (isCooling != oldstate) {
              msg.set(isCooling);
              send(msg);
              saveState(id, isCooling);
              sensors[i].boolValue = isCooling;
            }

          } break;
        case 9: //"airpumpinterval"
          {
            if (airPumpInterval->isEnabled()) {
              airPumpInterval->Update();
            }
          } break;
        case 10: //"airpumptime"
          {
            if (airPumpTime->isEnabled()) {
              airPumpTime->Update();
            }
          } break;
      }
    }
  }
  displayValuesTimer->Update();
  if ( allreceived == true) {
    float watertemperature = sensors[3].floatValue;
    float maxTemp = sensors[2].floatValue;
    float minTemp = sensors[4].floatValue;
    int coolingstate = sensors[0].intValue;
    String speedVal = sensors[1].stringValue;
    coolWater(coolingstate, watertemperature, maxTemp, minTemp, speedVal);

    if (ENABLE_SLEEP_MODE) {
      int secondsUntilStart = millis() / 1000;
      //Serial.println("SecondsUntilStart: " + String(secondsUntilStart));
      // Serial.println("CurrentWakeTime: " + String(currentWakeTime));
      //Serial.println("AwakeTime: " + String(currentWakeTime + awakeTime));
      //Avoid sleep when peltier is turning on or off
      if ((currentWakeTime + awakeTime) <= secondsUntilStart && sleeping == false) {
        fallAsleep();
      }
    }
  }
}

void coolWater(int state, float watertemperature, float maxTemp, float minTemp, String speedVal) {
  int pwm = 0;
  bool secondFan = false;
  if (speedVal ==  "Min")
  {
    pwm = 140;
  } else if (speedVal ==  "Normal")
  {
    pwm = 180;
    secondFan = true;
  } else if (speedVal ==  "Max")
  {
    pwm = 255;
    secondFan = true;
  } else if (speedVal == "Auto")
  {
    pwm = map(watertemperature, minTemp - 1, maxTemp + 1, 140, 255);
    if (pwm < 0) {
      pwm = 0;
    }
    if (pwm > 255) {
      pwm = 255;
    }
    if (pwm > 140)
    {
      secondFan = true;
    }
  }


  if (state == 2) {
    //on
    analogWrite(FAN1_PIN, pwm);
    if (secondFan == true) {
      analogWrite(FAN2_PIN, pwm);
    }
    isCooling = true;
  }
  else if (state == 0) {
    //off
    pwm = 0;
    isCooling = false;
  }
  else if (state == 3) {
    //heat
    pwm = 0;
    isCooling = false;
  }
  else if (state == 1) {
    //auto
    //Serial.println(watertemperature);
    //Serial.println(minTemp);
    //Serial.println(maxTemp);
    if (watertemperature >= maxTemp && isCooling == false) {
      isCooling = true;
      /*analogWrite(FAN1_PIN, pwm);
        if (secondFan == true) {
        analogWrite(FAN2_PIN, pwm);
        }*/
    }
    else if (watertemperature < minTemp) {
      pwm = 0;
      isCooling = false;
    }/*else{
      pwm = 0;
    }*/
  }
  if (isCooling == false) {
    pwm = 0;
  }
  analogWrite(FAN1_PIN, pwm);
  if (secondFan == true) {
    analogWrite(FAN2_PIN, pwm);
  } else {
    analogWrite(FAN2_PIN, 0);
  }
  //Serial.println(pwm);
}

void fallAsleep()
{
  Serial.println("fallAsleep");
  lcd.clear();
  lcd.setBacklight(LOW);
  lcd.noDisplay();
  displayValuesTimer->Pause();
  sleeping = true;
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
  //Serial.print("getWaterTemperature: ");
  dallasSensors.requestTemperatures();
  float sensorValueWaterTemp = dallasSensors.getTempCByIndex(index);
  //Serial.println(sensorValueWaterTemp);
  if (sensorValueWaterTemp <= 0 ) {
    sensorValueWaterTemp = 26;
  }
  return sensorValueWaterTemp; //(float)
}

float getWaterLevel()
{
  //Serial.print("getWaterLevel: ");
  digitalWrite(WATER_LVL_POWER_PIN, HIGH);
  wait(100);
  int sensorValueWaterLevel = analogRead(WATER_LVL_PIN);
  digitalWrite(WATER_LVL_POWER_PIN, LOW);
  int waterlevel = sensorValueWaterLevel / 1000.0 * 100.0;

  //Serial.println(waterlevel);
  return waterlevel;
}

void airPumpOn() {
  Serial.println("airPumpOn");
  airPumpInterval->Stop();
  airPumpInterval->setEnabled(false);
  digitalWrite(RELAY_AIRPUMP_PIN, RELAY_ON);
  if (sensors[10].intValue == 0) {
    //default interval time
    airPumpTime->setInterval(1000 * 60 * 5);
  }
  airPumpTime->Start();
  airPumpTime->setEnabled(true);
  msg.setType(V_STATUS);
  msg.setSensor(3);
  sensors[7].boolValue = true;
  send(msg.set(true));
}

void airPumpOff() {
  Serial.println("airPumpOff");
  airPumpTime->Stop();
  airPumpTime->setEnabled(false);
  digitalWrite(RELAY_AIRPUMP_PIN, RELAY_OFF);
  airPumpInterval->Start();
  airPumpInterval->setEnabled(true);
  msg.setType(V_STATUS);
  msg.setSensor(3);
  sensors[7].boolValue = false;
  send(msg.set(false));
}

void setAirPumpInterval(unsigned long value) {
  Serial.print("setAirPumpInterval");
  Serial.println(value);
  if (value != sensors[9].intValue && value > 0) {
    airPumpInterval->Stop();
    unsigned long timerVal = value * 1000 * 60;
    airPumpInterval->setInterval(timerVal);
    airPumpInterval->setEnabled(true);
    airPumpInterval->Start();
  } else if (value == 0) {
    airPumpInterval->Stop();
    airPumpInterval->setEnabled(false);
  }
}

void setAirPumpTime(unsigned long value) {
  Serial.print("setAirPumpTime");
  Serial.println(value);
  if (value != sensors[10].intValue && value > 0) {
    unsigned long timerVal = value * 1000 * 60;
    airPumpTime->setInterval(timerVal);
  }
}

void printScreen()
{
  // Reset the display
  lcd.clear();
  lcd.backlight();
  char lineOne[16] = "";
  char lineTwo[16] = "";

  if (printPage == 0) {
    float waterlevel = sensors[5].floatValue;
    strncpy(lineOne, "Waterlvl ", 16);
    char waterlvlChar[7];
    dtostrf(waterlevel, 6, 1, waterlvlChar);
    strcat(lineOne, waterlvlChar);

    float waterTemp = sensors[3].floatValue;
    strncpy(lineTwo, "Watertemp ", 16);
    char waterTempChar[6];
    dtostrf(waterTemp, 5, 1, waterTempChar);
    strcat(lineTwo, waterTempChar);

    dtostrf(waterTemp, 4, 1, &lineTwo[11]);
    printPage++;
  }
  else if (printPage == 1) {


    float minTemp = sensors[4].floatValue;
    strncpy(lineOne, "Mintemp ", 16);
    char minTempChar[6];
    dtostrf(minTemp, 5, 1, minTempChar);
    strcat(lineOne, minTempChar);

    float maxTemp = sensors[2].floatValue;
    strncpy(lineTwo, "Maxtemp ", 16);
    char maxTempChar[6];
    dtostrf(maxTemp, 5, 1, maxTempChar);
    strcat(lineTwo, maxTempChar);
    printPage++;

    //printPage = 0;
  }
  else if (printPage == 2) {
    int state = sensors[0].intValue;
    if (state == 2) {
      //on
      strncpy(lineOne,  "State: CoolOn", 16);
    }
    else if (state == 0) {
      //off
      strncpy(lineOne,  "State: Off", 16);
    }
    else if (state == 3) {
      //heat
      strncpy(lineOne,  "State: HeatOn", 16);
    }
    else if (state == 1) {
      //auto
      strncpy(lineOne,  "State: Auto", 16);
    }
    String speedVal = sensors[1].stringValue;
    if (speedVal == "Auto") {
      strncpy(lineTwo, "Speed: Auto", 16);
    }
    else if (speedVal == "Min") {
      strncpy(lineTwo, "Speed: Min", 16);
    } else if (speedVal == "Normal") {
      strncpy(lineTwo, "Speed: Normal", 16);
    } else if (speedVal == "Max") {
      strncpy(lineTwo, "Speed: Max", 16);
    }
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
          Serial.println(id);
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
            } break;
          case V_HVAC_SETPOINT_COOL: {
              float fvalue = message.getFloat();
              saveState(43, fvalue);
              sensors[i].floatValue = fvalue;
              send(msg.set(fvalue, 2));
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
              if (id == 3) {
                if (bvalue == true) {
                  airPumpInterval->Stop();
                  airPumpInterval->setEnabled(false);
                  digitalWrite(RELAY_AIRPUMP_PIN, RELAY_ON);
                  airPumpTime->Stop();
                  airPumpTime->setEnabled(false);
                } else {
                  airPumpOff();
                }
              }
              saveState(id, bvalue);
              sensors[i].boolValue = bvalue;
              send(msg.set(bvalue));

            } break;
          case V_HVAC_FLOW_STATE: {
              const char* recvData = message.getString();
              int state = 0;
              if (strcmp(recvData, "CoolOn") == 0) {
                state = 2;
              }
              else if (strcmp(recvData, "Off") == 0) {
                state = 0;
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
            } break;
          case V_HVAC_SPEED: {
              const char* recvData = message.getString();
              sensors[i].stringValue = recvData;
              int state = 0;
              if (strcmp(recvData, "Auto") == 0) {
                state = 0;
              }
              else if (strcmp(recvData, "Min") == 0) {
                state = 1;
              }
              else if (strcmp(recvData, "Normal") == 0) {
                state = 2;
              }
              else if (strcmp(recvData, "Max") == 0) {
                state = 3;
              }
              Serial.println(recvData);
              Serial.println(state);
              saveState(45, state);
            } break;
          case V_TEXT: {
              String svalue = message.getString();
              sensors[i].stringValue = svalue;
              if (id == 5 || id == 6) {
                //Serial.println(id);
                //Serial.println(svalue);
                int intValue = svalue.toInt();
                saveState(id, intValue);
                if (id == 5) {
                  setAirPumpInterval(intValue);
                }
                if (id == 6) {
                  setAirPumpTime(intValue);
                }
                sensors[i].intValue = intValue;
                send(msg.set(intValue));

              }
            } break;
        }
      }
    }
  }
}

