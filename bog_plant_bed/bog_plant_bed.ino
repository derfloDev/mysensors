//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_NODE_ID 35
#define MY_BAUD_RATE 9600
#include <MyConfig.h>
#include <MySensors.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h> // F Malpartida's NewLiquidCrystal library
#include <Wire.h>
#include <DallasTemperature.h>
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


const bool ENABLE_SLEEP_MODE = true;
const char Sketch_Info[] = "BogPlantBed";
const char Sketch_Version[] = "1.0";

#define ONE_WIRE_BUS 8
OneWire ds(ONE_WIRE_BUS);
DallasTemperature dallasSensors(&ds);

#define RELAY_HEATCABLE_PIN 2
#define RELAY_WATERPUMP_PIN 3
#define RELAY_ON LOW
#define RELAY_OFF HIGH

#define WATER_LVL_LOW_PIN  A0 // analog 1 ??
#define WATER_LVL_LOW_POWER_PIN  5 // digital pin 
#define WATER_LVL_HIGH_PIN  A1 // analog 1 ??
#define WATER_LVL_HIGH_POWER_PIN  6 // digital pin
#define LIGHT_LVL_PIN  A2


#define MOISTURE_PIN  A3
#define MOISTURE_POWER_PIN  7


#define NUMBER_OF_SENSORS 11

Sensor sensors[NUMBER_OF_SENSORS] = {
  {
    0,
    S_WATER,
    V_VOLUME,
    "Waterlevel low",
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
    "Waterlevel high",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    2,
    S_WATER_QUALITY,
    V_TEMP,
    "Water temperature",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    3,
    S_TEMP,
    V_TEMP,
    "Soil temperature",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    4,
    S_LIGHT_LEVEL,
    V_LIGHT_LEVEL,
    "Light level",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    5,
    S_BINARY,
    V_STATUS,
    "Pumpe",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    6,
    S_MOISTURE,
    V_LEVEL ,
    "Soil moisture",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    7,
    S_BINARY,
    V_STATUS,
    "Heat cable",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    8,
    S_INFO,
    V_TEXT,
    "Heat cable on temp",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    9,
    S_INFO,
    V_TEXT,
    "Heat cable off temp",
    false,
    "",
    false,
    0,
    0.0
  },
  {
    10,
    S_INFO,
    V_TEXT,
    "Min water level",
    false,
    "",
    false,
    0,
    0.0
  }

};
MyMessage msg;

void before()
{
  Serial.println("before");
  dallasSensors.begin();

  //init relay
  pinMode(RELAY_HEATCABLE_PIN, OUTPUT);
  pinMode(RELAY_WATERPUMP_PIN, OUTPUT);
  digitalWrite(RELAY_HEATCABLE_PIN, RELAY_OFF);
  digitalWrite(RELAY_WATERPUMP_PIN, RELAY_OFF);

  //waterlevel pins
  pinMode(WATER_LVL_LOW_POWER_PIN, OUTPUT);
  digitalWrite(WATER_LVL_LOW_POWER_PIN, LOW);
  pinMode(WATER_LVL_HIGH_POWER_PIN, OUTPUT);
  digitalWrite(WATER_LVL_HIGH_POWER_PIN, LOW);
  Serial.println("beforeend");
}

void presentation()
{
  Serial.println("presentation start");
  sendSketchInfo(Sketch_Info, Sketch_Version);
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    Sensor sensor = sensors[i];
    int id = sensor.childId;
    int typee = sensor.sensorType;
    String description = sensor.description;
    int stringLength = description.length() + 1;
    char descrBuffer[stringLength];
    description.toCharArray(descrBuffer, stringLength);
    present(id, typee, descrBuffer);
    wait(500);
  }
  Serial.println("presentation end");
}

int loops = 0;

void loop()
{
  Serial.println("loop");
  //float b = getWaterLevel(1);
  //float a = getMoisture();
  wait(1000);
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    Sensor sensor = sensors[i];
    bool sendMessage = false;
    int variableType = sensor.variableType;
    int id = sensor.childId;
    msg.setType(variableType).setSensor(id);
    //if not initialValue received
    if (sensor.initValueReceived == false) {
      Serial.print(i);
      Serial.print(" sending init id ");
      Serial.print(id);
      Serial.print(" : ");
      if (id == 0 || id == 1 || id == 2 || id == 3 || id == 4 || id == 6) {
        float value = 0.0;
        if (id == 0 || id == 1) {
          value = getWaterLevel(id);
        } else if (id == 2) {
          value = getWaterTemperature(0);
        } else if (id == 3) {
          value = getWaterTemperature(1);
        } else if (id == 4) {
          value = getLightLevel();
        } else if (id == 6) {
          value = getMoisture();
        }
        if (value > 100) {
          value = 100;
        }
        Serial.println(value);
        msg.set(value, 2);
        sensors[i].floatValue = value;
      } else if (id == 5 || id == 7) {
        bool value = loadState(id);
        Serial.println(value);
        msg.set(value);
        sensors[i].boolValue = value;
      } else if (id == 8 || id == 9 || id == 10) {
        float value = loadState(id);
        if (id != 10)
        {
          bool negative = loadState(id + 10);
          if (negative) {
            value = value - 256;
          }
        }
        /*if (value > 100) {
          value = 100;
          }*/
        /// only working with this shit and idk why?????
        String stringVal = String(value);
        int stringValLength = stringVal.length() + 1;
        char stringValBuffer[stringValLength];
        stringVal.toCharArray(stringValBuffer, stringValLength);
        msg.set(stringValBuffer);
        Serial.println(value);
        Serial.println(stringValBuffer);
        //msg.set(value, 2);
        sensors[i].floatValue = value;
      }
      sendMessage = true;
    }
    else {
      switch (id) {
        case 0: //"water lvl low"
        case 1: //"water lvl high"
        case 2:  //water temp
        case 3: //soil temp
        case 4: //light lvl
        case 6: //soil moisture
          {
            if (loops >= 10) {
              float value = 0.0;
              float oldValue = sensor.floatValue;
              if (id == 0 || id == 1) {
                value = getWaterLevel(id);
              } else if (id == 2) {
                value = getWaterTemperature(0);
              } else if (id == 3) {
                value = getWaterTemperature(1);
              } else if (id == 4) {
                value = getLightLevel();
              } else if (id == 6) {
                value = getMoisture();
              }
              if (value != oldValue) {
                sensors[i].floatValue = value;
                saveState(id, value);
                msg.set(value, 2);
                sendMessage = true;
              }
            }
          } break;
        case 5:  //water pump
        case 7: //heat cable
          {
            int pin = 0;
            bool state = RELAY_OFF;
            //Serial.println(sensor.boolValue);
            if (id == 5) {
              //Serial.println("pump");
              pin = RELAY_WATERPUMP_PIN;
              float watertemp = sensors[2].floatValue;
              float soiltemp = sensors[3].floatValue;
              float waterLvlLow = sensors[0].floatValue;
              // dont turn on water pump when water is too cold
              float minWaterLvl = sensors[10].floatValue;
              if (watertemp <= 1.0 || soiltemp <= 1.0 || waterLvlLow <= minWaterLvl) {
                sensor.boolValue = false;
                sensors[i].boolValue = false;
                msg.set(false);
                sendMessage = true;
              }
            } else if (id == 7) {
              //Serial.println("cable");
              pin = RELAY_HEATCABLE_PIN;
            }
            if (sensor.boolValue == true) {
              state = RELAY_ON;
            } else {
              state = RELAY_OFF;
            }
            //Serial.println(sensor.boolValue);
            //Serial.println(state);
            //Serial.println(pin);
            digitalWrite(pin, state);
            //wait(5000);
          } break;
        case 8: // heatcable on temp
        case 9: // heatcable off temp
          {
            float value = sensor.floatValue;
            float watertemp = sensors[2].floatValue;
            float soiltemp = sensors[3].floatValue;
            if (id == 8) {
              if (watertemp <= value || soiltemp <= value) {
                sensors[7].boolValue = true;
                msg.setSensor(7);
                msg.setType(V_STATUS);
                sendMessage = true;
                digitalWrite(RELAY_HEATCABLE_PIN, RELAY_ON);
              }
            } else if (id == 9) {
              if (watertemp >= value || soiltemp >= value) {
                sensors[7].boolValue = false;
                msg.setSensor(7);
                msg.setType(V_STATUS);
                msg.set(false);
                sendMessage = true;
                digitalWrite(RELAY_HEATCABLE_PIN, RELAY_OFF);
              }
            }
          } break;
      }
    }
    if (sendMessage == true) {
      send(msg);
      if (sensor.initValueReceived == false) {
        request(id, variableType);
        wait(2000, C_SET, variableType);
      }
    }
  }
  loops++;
  if (loops > 10) {
    loops = 0;
  }
}

float getMoisture()
{
  //Serial.print("getMoisture ");
  float retVal = 0.0;
  //turn sensor on and off to prevent corrosion
  digitalWrite(MOISTURE_POWER_PIN, HIGH);
  wait(100);
  retVal = analogRead(MOISTURE_PIN);
  digitalWrite(MOISTURE_POWER_PIN, LOW);
  //Serial.println(retVal);
  // calculate percentage
  if (retVal < 88) {
    retVal = 100;
  } else if (retVal > 136) {
    retVal = 0;
  }  else {
    retVal = map(retVal, 136, 88, 0, 100);
  }
  return retVal;
}

float getLightLevel()
{
  float retVal = 0.0;
  retVal = 1023 - analogRead(LIGHT_LVL_PIN);
  // calculate percentage
  retVal = map(retVal, 0, 1023, 0, 100);
  return retVal;
}

float getWaterTemperature(int index)
{
  dallasSensors.requestTemperatures();
  float sensorValueWaterTemp = dallasSensors.getTempCByIndex(index);
  return sensorValueWaterTemp; //(float)
}

float getWaterLevel(int id)
{
  //Serial.print("getWaterLevel: ");
  //Serial.print(id);
  int pin = 0;
  int pwrPin = 0;
  if (id == 0) {
    pin = WATER_LVL_LOW_PIN;
    pwrPin = WATER_LVL_LOW_POWER_PIN;
  } else {
    pin = WATER_LVL_HIGH_PIN;
    pwrPin = WATER_LVL_HIGH_POWER_PIN;
  }
  digitalWrite(pwrPin, HIGH);
  wait(100);
  int sensorValueWaterLevel = analogRead(pin);
  digitalWrite(pwrPin, LOW);
  int waterlevel = sensorValueWaterLevel / 1000.0 * 100.0;
  if (sensorValueWaterLevel > 205) {
    waterlevel = 100;
  }  else {
    waterlevel = map(sensorValueWaterLevel, 0, 205, 0, 100);
  }

  /*Serial.print(sensorValueWaterLevel);
  Serial.print("  ");
  Serial.println(waterlevel);*/
  return waterlevel;
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
        bool sendBackValue = false;
        switch (id) {
          case 0: //"water lvl low"
          case 1: //"water lvl high"
          case 2: //water temp
          case 3: //soil temp
          case 4: //light lvl
          case 6: //soil moisture
            {
              float value = message.getFloat();
              sensors[i].floatValue = value;
              saveState(id, value);
              msg.set(value, 2);
              sendBackValue = true;
            } break;
          case 5:  //water pump
          case 7: //heat cable
            {
              bool value = message.getBool();
              sensors[i].boolValue = value;
              saveState(id, value);
              msg.set(value);
              sendBackValue = true;
            } break;
          case 8:
          case 9:
          case 10:
            {
              String strvalue = message.getString();
              float floatValue = strvalue.toFloat();
              sensors[i].floatValue = floatValue;
              /*Serial.println(i);
              Serial.println(id);
              Serial.println(floatValue);
              Serial.println(strvalue);*/
              saveState(id, floatValue);
              if (id != 10)
              {
                bool negative = false;
                if (floatValue < 0) {
                  negative = true;
                }
                saveState(id + 10, negative);
              }
              msg.set(floatValue, 2);
              sendBackValue = true;
            } break;
        }
        if (sendBackValue == true) {
          //Serial.println("sendBackValue");
          msg.setType(variableType);
          msg.setSensor(id);
          send(msg);
        }
      }
    }
  }
}

