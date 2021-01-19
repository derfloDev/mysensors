/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

/*
   Settling time (number of samples) and data filtering can be adjusted in the config.h file
   For calibration and storing the calibration value in eeprom, see example file "Calibration.ino"

   The update() function checks for new data and starts the next conversion. In order to acheive maximum effective
   sample rate, update() should be called at least as often as the HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS.
   If you have other time consuming code running (i.e. a graphical LCD), consider calling update() from an interrupt routine,
   see example file "Read_1x_load_cell_interrupt_driven.ino".

   This is an example sketch on how to use this library
*/

#include <HX711_ADC.h>
#include <EEPROM.h>
//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_NODE_ID 5
//#include <MyConfig.h>
#include <MySensors.h>

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

const char Sketch_Info[] = "BedSensor";
const char Sketch_Version[] = "1.0";
#define NUMBER_OF_SENSORS 7
Sensor sensors[NUMBER_OF_SENSORS] = {
    {0,
     S_WEIGHT,
     V_WEIGHT,
     "Gewicht links",
     false,
     "",
     false,
     0,
     0.0},
    {1,
     S_MOTION,
     V_TRIPPED,
     "Bewegung links",
     false,
     "",
     false,
     0,
     0.0},
    {2,
     S_BINARY,
     V_STATUS,
     "Belegt links",
     false,
     "",
     false,
     0,
     0.0},
    {3,
     S_WEIGHT,
     V_WEIGHT,
     "Gewicht rechts",
     false,
     "",
     false,
     0,
     0.0},
    {4,
     S_MOTION,
     V_TRIPPED,
     "Bewegung rechts",
     false,
     "",
     false,
     0,
     0.0},
    {5,
     S_BINARY,
     V_STATUS,
     "Belegt rechts",
     false,
     "",
     false,
     0,
     0.0},
    {6,
     S_BINARY,
     V_STATUS,
     "Tare",
     false,
     "",
     false,
     0,
     0.0},
};

MyMessage msg;

//pins:
const int HX711_dout = 3; //mcu > HX711 dout pin
const int HX711_sck = 2;  //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;
volatile boolean newDataReady;

void before()
{
  Serial.begin(57600);
  delay(10);
  Serial.println("Starting...");

  LoadCell.begin();
  float calibrationValue;   // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                 //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  }
  else
  {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
}

void presentation()
{
  Serial.println("presentation start");
  sendSketchInfo(Sketch_Info, Sketch_Version);
  for (int i = 0; i < NUMBER_OF_SENSORS; i++)
  {
    Sensor sensor = sensors[i];
    int childId = sensor.childId;
    int sensorType = sensor.sensorType;
    String description = sensor.description;
    int stringLength = description.length() + 1;
    char descrBuffer[stringLength];
    description.toCharArray(descrBuffer, stringLength);
    present(childId, sensorType, descrBuffer);
    wait(500);
  }
  Serial.println("presentation end");
}

//interrupt routine:
void dataReadyISR()
{
  if (LoadCell.update())
  {
    newDataReady = 1;
  }
}

void loop()
{
  wait(1000);

  for (int i = 0; i < NUMBER_OF_SENSORS; i++)
  {
    Sensor sensor = sensors[i];
    int variableType = sensor.variableType;
    int id = sensor.childId;
    if (sensor.initValueReceived == false)
    {
      Serial.print(i);
      Serial.println(" sending init: ");
      if (id == 0 && variableType == V_WEIGHT)
      {
        float value = 0.0;
        msg.setType(variableType).setSensor(id).set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 3 && variableType == V_WEIGHT)
      {
        float value = 0.0;
        msg.setType(variableType).setSensor(id).set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (variableType == V_TRIPPED || variableType == V_STATUS)
      {
        bool value = false;
        msg.setType(variableType).setSensor(id).set(value);
        sensors[i].boolValue = value;
      }
      else
      {
        Serial.println("other");
        int val = loadState(id);
        msg.setType(variableType).setSensor(id).set(val);
        sensors[i].intValue = val;
      }

      send(msg);
      request(id, variableType);
      wait(2000, C_SET, variableType);
    }
    else
    {
      switch (i)
      {
      case 4: //"waterflow"
      {
        /* float waterflow = getWaterFlow();
        float oldWaterflow = sensor.floatValue;
        if (waterflow >= 0 && waterflow != oldWaterflow)
        {
          //ASerial.println("sending message '" + String(waterflow) + "' for Sensor '" + String(i) + "'");
          msg.setType(variableType).setSensor(id).set(waterflow, 2);
          send(msg);
          saveState(id, waterflow);
          sensors[i].floatValue = waterflow;
        } */
      }
      break;
      case 7: //"cooling state"
      {
       /*  float oldstate = sensors[7].boolValue;
        if (isCooling != oldstate)
        {
          //ASerial.println("sending message '" + String(isCooling) + "' for Sensor '" + String(i) + "'");
          msg.setType(variableType).setSensor(id).set(isCooling);
          send(msg);
          saveState(id, isCooling);
          sensors[i].boolValue = isCooling;
        } */
      }
      break;
      }
    }
  }

  const int serialPrintInterval = 1000; //increase value to slow down serial print activity

  // get smoothed value from the dataset:
  if (newDataReady)
  {
    if (millis() > t + serialPrintInterval)
    {
      float i = LoadCell.getData();
      newDataReady = 0;
      Serial.print("Load_cell output val: ");
      Serial.print(i);
      //Serial.print("  ");
      //Serial.println(millis() - t);
      t = millis();
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0)
  {
    char inByte = Serial.read();
    if (inByte == 't')
      LoadCell.tareNoDelay();
  }

  //check if last tare operation is complete
  if (LoadCell.getTareStatus() == true)
  {
    Serial.println("Tare complete");
  }
}
void receive(const MyMessage &message)
{
  Serial.println("receive ");
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }

  for (int i = 0; i < NUMBER_OF_SENSORS; i++)
  {
    Sensor sensor = sensors[i];
    int id = sensor.childId;
    int sensorType = sensor.sensorType;
    int variableType = sensor.variableType;
    msg.setType(variableType);
    msg.setSensor(id);
    if (id == message.sensor)
    {
      if (variableType == message.type)
      {
        if (sensor.initValueReceived == false)
        {
          sensors[i].initValueReceived = true;
          Serial.print("initialValueReceived for Sensor ");
          //Serial.println(String(id));
        }
        switch (variableType)
        {
        case V_WEIGHT:
        {
          float floatValue = message.getFloat();
          saveState(id, floatValue);
          sensors[i].floatValue = floatValue;
          send(msg.set(floatValue, 2));
          //Serial.print("V_VOLTAGE ");
          //ASerial.println("received message '" + String(floatValue) + "' for Sensor '" + String(i) + "'");
        }
        break;
        case V_STATUS:
        case V_TRIPPED:
        {
          bool boolValue = message.getBool();
          saveState(id, boolValue);
          sensors[i].boolValue = boolValue;
          send(msg.set(boolValue));
          //Serial.print("V_TRIPPED ");
          //ASerial.println("received message '" + String(boolValue) + "' for Sensor '" + String(i) + "'");
        }
        break;
        }
      }
    }
  }
}