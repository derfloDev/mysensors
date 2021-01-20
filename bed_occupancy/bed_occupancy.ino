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
#define OCCUPANCY_TRESHOLD 20
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
     S_LOCK,
     V_LOCK_STATUS,
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
     S_LOCK,
     V_LOCK_STATUS,
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
const int HX711_dout_left = 4;  //mcu > HX711 dout pin
const int HX711_sck_left = 5;   //mcu > HX711 sck pin
const int HX711_dout_right = 6; //mcu > HX711 dout pin
const int HX711_sck_right = 7;  //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCellLeft(HX711_dout_left, HX711_sck_left);
HX711_ADC LoadCellRight(HX711_dout_right, HX711_sck_right);

const int calVal_eepromAdress = 0;
unsigned long tLeft = 0;
unsigned long tRight = 0;
volatile boolean newDataReadyLeft;
volatile boolean newDataReadyRight;
float oldWeightLoadLeft = 0.0;
float oldWeightLoadRight = 0.0;
float weightLoadLeft = 0.0;
float weightLoadRight = 0.0;

void before()
{
  /* Serial.begin(115200); */
  delay(10);
  Serial.println("Starting...");

  LoadCellLeft.begin();
  LoadCellRight.begin();

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                 //set this to false if you don't want tare to be performed in the next step
  LoadCellLeft.start(stabilizingtime, _tare);
  LoadCellRight.start(stabilizingtime, _tare);
  checkLoadCellTareTimeout(LoadCellLeft);
  checkLoadCellTareTimeout(LoadCellRight);
  attachInterrupt(digitalPinToInterrupt(HX711_dout_left), dataReadyIsrLeft, FALLING);
  // attachInterrupt(digitalPinToInterrupt(HX711_dout_right), dataReadyIsrRight, FALLING);
}

void checkLoadCellTareTimeout(HX711_ADC loadCell)
{
  float calibrationValue;   // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  if (loadCell.getTareTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  }
  else
  {
    loadCell.setCalFactor(calibrationValue); // set calibration value (float)
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
void dataReadyIsrLeft()
{
  if (LoadCellLeft.update())
  {
    newDataReadyLeft = 1;
  }
}
void dataReadyIsrRight()
{
  if (LoadCellRight.update())
  {
    newDataReadyRight = 1;
  }
}

void loop()
{
  LoadCellLeft.update();
  LoadCellRight.update();
  Serial.println(LoadCellLeft.getData());
  Serial.println(LoadCellRight.getData());
  Serial.println("Loop");
  wait(1000);
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // get smoothed value from the dataset:
  if (newDataReadyLeft)
  {
    Serial.print("test");
    if (millis() > tLeft + serialPrintInterval)
    {
      weightLoadLeft = LoadCellLeft.getData();
      newDataReadyLeft = 0;
      Serial.print("LoadCellLeft output val: ");
      Serial.print(weightLoadLeft);
      tLeft = millis();
    }
  }
  if (newDataReadyRight)
  {
    if (millis() > tRight + serialPrintInterval)
    {
      weightLoadRight = LoadCellRight.getData();
      newDataReadyRight = 0;
      Serial.print("LoadCellRight output val: ");
      Serial.print(weightLoadRight);
      tRight = millis();
    }
  }

  for (int i = 0; i < NUMBER_OF_SENSORS; i++)
  {
    Sensor sensor = sensors[i];
    int variableType = sensor.variableType;
    int id = sensor.childId;
    msg.setType(variableType).setSensor(id);

    if (sensor.initValueReceived == false)
    {
      Serial.print(i);
      Serial.println(" sending init: ");
      if (id == 0 && variableType == V_WEIGHT)
      {
        float value = weightLoadLeft;
        msg.set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (id == 3 && variableType == V_WEIGHT)
      {
        float value = weightLoadRight;
        msg.set(value, 2);
        sensors[i].floatValue = value;
      }
      else if (variableType == V_TRIPPED || variableType == V_STATUS || variableType == V_LOCK_STATUS)
      {
        bool value = false;
        msg.set(value);
        sensors[i].boolValue = value;
      }
      else
      {
        Serial.println("other");
        int val = loadState(id);
        msg.set(val);
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
      case 0: // weight left
      {
        float oldValue = sensor.floatValue;
        float newValue = weightLoadLeft;
        if (newValue != oldValue)
        {
          msg.set(newValue, 2);
          send(msg);
          sensors[i].floatValue = newValue;
        }
      }
      break;
      case 1: // motion left
      {
        bool oldValue = sensor.boolValue;
        bool newValue = oldWeightLoadLeft != weightLoadLeft;
        if (newValue != oldValue)
        {
          msg.set(newValue);
          send(msg);
          sensors[i].boolValue = newValue;
        }
      }
      break;
      case 2: // occupancy left
      {
        bool oldValue = sensor.boolValue;
        bool newValue = weightLoadLeft > OCCUPANCY_TRESHOLD;
        if (newValue != oldValue)
        {
          msg.set(newValue);
          send(msg);
          sensors[i].boolValue = newValue;
        }
      }
      break;
      case 3: // weight right
      {
        float oldValue = sensor.floatValue;
        float newValue = weightLoadRight;
        if (newValue != oldValue)
        {
          msg.set(newValue, 2);
          send(msg);
          sensors[i].floatValue = newValue;
        }
      }
      break;
      case 4: // motion left
      {
        bool oldValue = sensor.boolValue;
        bool newValue = oldWeightLoadRight != weightLoadRight;
        if (newValue != oldValue)
        {
          msg.set(newValue);
          send(msg);
          sensors[i].boolValue = newValue;
        }
      }
      break;
      case 5: // occupancy left
      {
        bool oldValue = sensor.boolValue;
        bool newValue = weightLoadRight > OCCUPANCY_TRESHOLD;
        if (newValue != oldValue)
        {
          msg.set(newValue);
          send(msg);
          sensors[i].boolValue = newValue;
        }
      }
      break;
      case 6: // tare
      {
        bool value = sensor.boolValue;
        if (value == true)
        {
          LoadCellLeft.tare();
          LoadCellRight.tare();

          //check if last tare operation is complete
          if (LoadCellLeft.getTareStatus() == true && LoadCellRight.getTareStatus() == true)
          {
            bool value = false;
            msg.set(value);
            sensors[i].boolValue = value;
            send(msg);
            Serial.println("Tare complete");
          }
        }
      }
      break;
      }
    }
  }
  oldWeightLoadLeft = weightLoadLeft;
  oldWeightLoadRight = weightLoadRight;
}

void receive(const MyMessage &message)
{
  Serial.println("receive ");
  /*  if (message.isAck())
  {
    Serial.println("This is an ack from gateway");
  } */

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
          Serial.println(String(id));
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
        case V_LOCK_STATUS:
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
