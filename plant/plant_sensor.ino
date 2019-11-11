//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_NODE_ID 6
#define MY_BAUD_RATE 9600
#include <MyConfig.h>
#include <MySensors.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h> // F Malpartida's NewLiquidCrystal library
#include <TimerObject.h>
#include <DHT.h>
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

//#define DISPLAY_SUPPORT
//Pin A4, A5
#if defined(DISPLAY_SUPPORT)
LiquidCrystal_I2C lcd(0x27, 16, 2);
#endif

#define DHTPIN 3     // digital pin
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

unsigned long DISPLAY_CHANGE_TIME = 2000; // Sleep time between reads (in milliseconds)

// Max is 4 due to 4 free analog pins
#define NUMBER_OF_PLANTS 4

#define INTERRUPT_PIN 2    //Uno, Nano, Mini, other 328-based  2, 3
#define INTERRUPT_MODE CHANGE // LOW, CHANGE, RISING, FALLING
const int awakeTime = (DISPLAY_CHANGE_TIME / 1000) + NUMBER_OF_PLANTS * (DISPLAY_CHANGE_TIME * 2 / 1000); //in seconds
const unsigned long sleepTime = 3600000 * 3;//in milliseconds
const bool ENABLE_SLEEP_MODE = true;
const char Sketch_Info[] = "Plant_Sensor";
const char Sketch_Version[] = "1.0";
//#define BATTERY_POWERED // For more info look here https://www.mysensors.org/build/battery

#define LED_PIN 4
#if defined(BATTERY_POWERED)
#define BRIGHTNESS  15
#else
#define BRIGHTNESS  20
#endif

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUMBER_OF_PLANTS];
#define UPDATES_PER_SECOND 100
CRGBPalette16 currentPalette;
TBlendType    currentBlending;

//extern CRGBPalette16 myRedWhiteBluePalette;
//extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;


const int BATTERY_SENSE_PIN = A7;  // select the input pin for the battery sense point
int oldBatteryLevel = 100;
int oldBattery = 1023;
int printPage = -1;

struct Plant
{
  String name;
  int Id;
  float moisture;
  String minMoisture;
  bool initValueReceived;
  bool initMinValueReceived;
  bool nameRequested;
  bool nameReceived;
  int sensorPin;//analog pin
  int sensorPowerPin;
};
// free analog pins => A0, A1, A2, A3,
Plant plants[NUMBER_OF_PLANTS] = {
  { "Pflanze 1", 11, 0.0, "0.0", false, false,false,false, A0, 5},
  { "Pflanze 2", 12, 0.0, "0.0", false, false,false,false, A1, 6},
  { "Pflanze 3", 13, 0.0, "0.0", false, false,false,false, A2, 7},
  { "Pflanze 4", 14, 0.0, "0.0", false, false,false,false, A3, 8}
};

int currentWakeTime = 0;
#if defined(DISPLAY_SUPPORT)
TimerObject *displayValuesTimer = new TimerObject(DISPLAY_CHANGE_TIME);
#endif

MyMessage msg;

float oldTemp = 0.0;
float oldHum = 0.0;
float oldLight = 0.0;

void before() {

  Serial.println("start before");
  // Initialize external hardware etc here...
  if (ENABLE_SLEEP_MODE) {
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  }

  dht.begin();


#if defined(DISPLAY_SUPPORT)
  lcd.begin();  // initialize the lcd
  // Switch on the backlight
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  //timer
  displayValuesTimer->setOnTimer(&printScreen);
  displayValuesTimer->Start();
#endif

  oldTemp = loadState(0);
  oldHum = loadState(1);
  oldLight = loadState(2);
  for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
    Plant loopPlant = plants[i];
    float moistureValue = loadState(loopPlant.Id);
    plants[i].moisture = moistureValue;
    int id = loopPlant.Id + 10;
    float minMoistureValue = loadState(id);
    Serial.print("Restore :");
    Serial.println(minMoistureValue);
    plants[i].minMoisture = String(minMoistureValue);

    pinMode(loopPlant.sensorPowerPin, OUTPUT);
    digitalWrite(loopPlant.sensorPowerPin, LOW);
  }

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUMBER_OF_PLANTS, 0);
  //.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  /*for (int gHue = 0; gHue < 400; gHue++) {
    fill_rainbow( leds, NUMBER_OF_PLANTS, gHue, 0);
    //FastLED.show();
    delay(10);
  }
  FastLED.clear();
  FastLED.show();*/
}

void presentation() {
  Serial.println("Presentation");
  sendSketchInfo(Sketch_Info, Sketch_Version);

  present(0, S_TEMP, "Temperature");
  present(1, S_HUM, "Humidity");
  present(2, S_LIGHT_LEVEL, "Light Level");

  for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
    Plant loopPlant = plants[i];

    digitalWrite(loopPlant.sensorPowerPin, HIGH);
    int sensorId = loopPlant.Id;
    String description = loopPlant.name;
    int stringLength = description.length() + 1;
    char descrBuffer[stringLength];
    description.toCharArray(descrBuffer, stringLength);
    present(sensorId, S_MOISTURE, descrBuffer);
    Serial.print("Presenting Sensor ");
    Serial.print(i);
    Serial.print(" -> ");
    Serial.println(description);

    String minDescription = loopPlant.name + " minimum moisture";
    int minStringLength = minDescription.length() + 1;
    char mindDescrBuffer[minStringLength];
    minDescription.toCharArray(mindDescrBuffer, minStringLength);
    int minSensorId = sensorId + 10;
    present(minSensorId, S_INFO, mindDescrBuffer);
    Serial.print("Presenting Sensor ");
    Serial.print(minSensorId);
    Serial.print(" -> ");
    Serial.println(minDescription);

    String minDescriptionN = loopPlant.name + " name";
    int minNStringLength = minDescriptionN.length() + 1;
    char mindDescrNBuffer[minNStringLength];
    minDescriptionN.toCharArray(mindDescrNBuffer, minNStringLength);
    int nameId = sensorId + 20;
    present(nameId, S_INFO, mindDescrNBuffer);
    Serial.print("Presenting Sensor ");
    Serial.print(nameId);
    Serial.print(" -> ");
    Serial.println(mindDescrNBuffer);
  }
  currentWakeTime = (millis() / 1000) + 1;
}

bool initTempReceived = false;
bool initHumReceived = false;
bool initLightReceived = false;
bool wakeupFromUser = true;

void loop() {
  wait(1000);
#if defined(BATTERY_POWERED)
  // calc battery and send
  int readings = 5;
  int batterySensorValue = 0;
  for (int reading = 0; reading < readings; reading++) {
    batterySensorValue += analogRead(BATTERY_SENSE_PIN);
  }
  batterySensorValue = batterySensorValue / readings;
  int batteryLevel = 100;
  batteryLevel = map(batterySensorValue, 460, 700, 0, 100);
  if (oldBatteryLevel != batteryLevel)
  {
    Serial.print("Battery sensor value: ");
    Serial.println(batterySensorValue);
    Serial.print("batteryLevel: ");
    Serial.println(batteryLevel);
    oldBatteryLevel = batteryLevel;
    oldBattery = batterySensorValue;
  }
#endif

  if (wakeupFromUser == true) {
#if defined(DISPLAY_SUPPORT)
    displayValuesTimer->Update();
#endif
  }

  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();
  float newLight = getBrightness(A6);// 0.0; //TODO get light level

  if (initTempReceived == false || (newTemp != oldTemp))
  {
    saveState(0, newTemp);
    msg.setType(V_TEMP).setSensor(0).set(newTemp, 2);
    send(msg);
    if (initTempReceived == false)
    {
      request(0, V_TEMP);
      wait(2000, C_SET, V_TEMP);
    }
  }

  if (initHumReceived == false || (newHum != oldHum))
  {
    saveState(1, newHum);
    msg.setType(V_HUM).setSensor(1).set(newHum, 2);
    send(msg);
    if (initHumReceived == false)
    {
      request(1, V_HUM);
      wait(2000, C_SET, V_HUM);
    }
  }

  if (initLightReceived == false || (newLight != oldLight))
  {
    saveState(2, newLight);
    msg.setType(V_LIGHT_LEVEL).setSensor(2).set(newLight, 2);
    send(msg);
    if (initLightReceived == false)
    {
      request(2, V_LIGHT_LEVEL);
      wait(2000, C_SET, V_LIGHT_LEVEL);
    }
  }

  for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
    Plant loopPlant = plants[i];
    int variableType = V_LEVEL;
    int sensorId = loopPlant.Id;
    int minSensorId = sensorId + 10;
    float oldMoisture = loopPlant.moisture;
    float moisture = getMoisture(loopPlant);
    if (loopPlant.initValueReceived == false) {
      Serial.print("Sending init for ");
      Serial.print(loopPlant.name);
      Serial.print(" with value ");
      Serial.println(oldMoisture);

      msg.setType(variableType).setSensor(sensorId).set(moisture, 2);
      send(msg);
      request(sensorId, variableType);
      wait(2000, C_SET, variableType);
    }
    else {
      if (moisture != oldMoisture) {
        Serial.println("sending message '" + String(moisture) + "' for " + loopPlant.name);
        msg.setType(variableType).setSensor(sensorId).set(moisture, 2);
        send(msg);
        saveState(sensorId, moisture);
        plants[i].moisture = moisture;
      }
    }

    int nameId = sensorId + 20;
    if(loopPlant.nameRequested == false){
      request(nameId, V_TEXT);
      wait(2000, C_SET, V_TEXT);
      plants[i].nameRequested = true;
    }else if(loopPlant.nameReceived == false){
      String name = loopPlant.name;
      int nameLength = name.length() + 1;
      char nameBuffer[nameLength];
      name.toCharArray(nameBuffer, nameLength);
      msg.setType(V_TEXT);
      msg.setSensor(nameId);
      msg.set(nameBuffer);
      send(msg);
      request(minSensorId, V_TEXT);
      wait(2000, C_SET, V_TEXT);
    }

    if (loopPlant.initMinValueReceived == false) {
      Serial.print("Sending init for ");
      Serial.print(loopPlant.name);
      Serial.print(" ");
      Serial.print(minSensorId);
      Serial.print(" with min ");
      Serial.println(loopPlant.minMoisture);

      /// only working with this shit and idk why?????
      String minMoi = String(loopPlant.minMoisture);
      int minMoiLength = minMoi.length() + 1;
      char minMoiBuffer[minMoiLength];
      minMoi.toCharArray(minMoiBuffer, minMoiLength);
      Serial.println(minMoiBuffer);

      msg.setType(V_TEXT);
      msg.setSensor(minSensorId);
      msg.set(minMoiBuffer);
      send(msg);
      request(minSensorId, V_TEXT);
      wait(2000, C_SET, V_TEXT);
    }
    float floatMinMoisture = loopPlant.minMoisture.toFloat();
    float orangePercent = floatMinMoisture + ((100 - floatMinMoisture) * 0.1);
    float yellowPercent = floatMinMoisture + ((100 - floatMinMoisture) * 0.2);
    Serial.print("orangePercent: ");
    Serial.println(orangePercent);
    Serial.print("yellowPercent: ");
    Serial.println(yellowPercent);

    if (loopPlant.moisture < floatMinMoisture) {
      leds[i] = CRGB::DarkRed;
      Serial.println("LED red");
    } else if (loopPlant.moisture < orangePercent) {
      leds[i] = CRGB::DarkOrange ;
      Serial.println("LED orange");
    } else if (loopPlant.moisture < yellowPercent) {
      leds[i] = CRGB::Yellow;
      Serial.println("LED yellow");
    }
    else {
      leds[i] = CRGB::Black;
      Serial.println("LED off");
    }
  }
  if (wakeupFromUser == true) {
    FastLED.show();
    wait(100);
    Serial.println("show LEDs");
  }

  if (ENABLE_SLEEP_MODE) {
    int secondsUntilStart = millis() / 1000;
    /*Serial.print("SecondsUntilStart: ");
      Serial.println(secondsUntilStart);
      Serial.print("CurrentWakeTime: ");
      Serial.println(currentWakeTime);
      Serial.print("AwakeTime: ");
      Serial.println(currentWakeTime + awakeTime);*/
    if ((currentWakeTime + awakeTime) <= secondsUntilStart ||	wakeupFromUser == false)
    {
#if defined(BATTERY_POWERED)
      Serial.print("Sending batteryLevel: ");
      Serial.println(batteryLevel);
      sendBatteryLevel(batteryLevel);
#endif
      bool stayAwake = false;
      for (int sensor = 0; sensor < NUMBER_OF_PLANTS; sensor++) {
        //if not initialValue send
        if (plants[sensor].initValueReceived == false)
        {
          stayAwake = true;
        }
        if (plants[sensor].initMinValueReceived == false)
        {
          stayAwake = true;
        }
      }
      if (stayAwake == false) {
        fallAsleep();
        wait(100);
        //sleep(60000);
        int wake = sleep(digitalPinToInterrupt(INTERRUPT_PIN), INTERRUPT_MODE, sleepTime);
        //smartSleep(digitalPinToInterrupt(INTERRUPT_PIN), INTERRUPT_MODE, sleepTime);
        Serial.print("wake: ");
        Serial.println(wake);
        if (wake >= 0)
        {
          wakeupFromUser = true;
          wakeUpDisplay();
        }
        wakeMeUp();

      }


    }
  }
}

void fallAsleep() {

  Serial.println("fallAsleep");
  //send data before sleep, turn of hardware etc...
  FastLED.clear();
  FastLED.show();
  for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
    Plant loopPlant = plants[i];
    digitalWrite(loopPlant.sensorPowerPin, LOW);
  }
#if defined(DISPLAY_SUPPORT)
  lcd.clear();
  lcd.setBacklight(LOW);
  lcd.noDisplay();
#endif
  wakeupFromUser = false;
  
}

void wakeUpDisplay() {
  Serial.println("wakeUpDisplay");
  printPage = -1;
#if defined(DISPLAY_SUPPORT)
  lcd.setBacklight(HIGH);
  lcd.display();
#endif
}

void wakeMeUp() {

  Serial.println("wakeMeUp");
  //do your wakeup stuff here
  for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
    Plant loopPlant = plants[i];
    int minSensorId = loopPlant.Id + 10;
    request(minSensorId, V_TEXT);
    wait(500, C_SET, V_TEXT);
    digitalWrite(loopPlant.sensorPowerPin, HIGH);
    int nameId = loopPlant.Id + 20;
    request(nameId, V_TEXT);
    wait(500, C_SET, V_TEXT);
  }
  currentWakeTime = millis() / 1000;
}

float getBrightness(int sensorPin)
{
  float retVal = 0.0;
  //Serial.println("getBrightness");
  retVal = 1023 - analogRead(sensorPin);
  // calculate percentage
  retVal = map(retVal, 0, 1023, 0, 100);
  //retVal = map(retVal, 550, 0, 0, 100);
  return retVal;
}

float getMoisture(Plant plant)
{
  float retVal = 0.0;
  //Serial.println("getMoisture");
  //turn sensor on and off to prevent corrosion
  //digitalWrite(plant.sensorPowerPin, HIGH);
  wait(100);
  retVal = analogRead(plant.sensorPin);
  //digitalWrite(plant.sensorPowerPin, LOW);
  /*Serial.print("Sensor: ");
    Serial.print(plant.Id);
    Serial.print(", Analog: ");
    Serial.print(retVal);*/
  // calculate percentage
  if (retVal < 300) {
    retVal = 100;
  } else {
    retVal = map(retVal, 1023, 300, 0, 100);
  }
  /*Serial.print(", Percent: ");
    Serial.println(retVal);*/
  return retVal;
}


void printScreen() {
#if defined(DISPLAY_SUPPORT)
  // Reset the display
  lcd.clear();
  lcd.backlight();
  if (printPage == -1) {
    lcd.setCursor(0, 0);
    lcd.print("T: ");
    lcd.print(oldTemp);
    lcd.print(" H: ");
    lcd.print(oldHum);

    lcd.setCursor(0, 1);
    lcd.print("Bat: ");
    lcd.print(oldBatteryLevel);
    lcd.print("|");
    lcd.print(oldBattery);

  }
  else {
    Plant currentPlant = plants[printPage];

    lcd.setCursor(0, 0);
    lcd.print(currentPlant.name);

    lcd.setCursor(0, 1);
    lcd.print(currentPlant.moisture);
    lcd.print(" / ");
    lcd.print(currentPlant.minMoisture);
    if (currentPlant.moisture < currentPlant.minMoisture.toFloat())
      lcd.print(" !!");
  }


  if (printPage >= NUMBER_OF_PLANTS - 1)
  {
    printPage = -1;
  }
  else
  {
    printPage++;
  }
#endif
}

void receive(const MyMessage & message) {
  //Serial.println("receive ");
  /*if (message.isAck()) {
  	Serial.println("This is an ack from gateway");
  	return;
    }*/


  float value = message.getFloat();
  int sensorId = message.sensor;
  for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
    Plant loopPlant = plants[i];

    if (sensorId == loopPlant.Id)
    {
      if (loopPlant.initValueReceived == false) {
        plants[i].initValueReceived = true;
        Serial.print("Received init for ");
        Serial.print(sensorId);
        Serial.print(" ");
        Serial.print(loopPlant.name);
        Serial.print(" with value ");
        Serial.println(value);
      }

      msg.setType(V_LEVEL);
      msg.setSensor(sensorId);
      msg.set(value, 2);
    }

    int minMoistureId = loopPlant.Id + 10;
    if (sensorId == minMoistureId) {
      String strvalue = message.getString();
      float floatValue = strvalue.toFloat();
      Serial.print("Received minMoisture for ");
      if (loopPlant.initMinValueReceived == false) {
        plants[i].initMinValueReceived = true;
        Serial.print("init ");
      }
      Serial.print(sensorId);
      Serial.print(" ");
      Serial.print(loopPlant.name);
      Serial.print(" with value ");
      Serial.println(floatValue);

      plants[i].minMoisture = strvalue;
      saveState(minMoistureId, floatValue);
      msg.setType(V_TEXT);
      msg.setSensor(minMoistureId);
      msg.set(message.getString());
      send(msg);
    }
    int nameId = loopPlant.Id + 20;
    if (sensorId == nameId) {
      String strvalue = message.getString();
      Serial.print("Received name for ");
      if (loopPlant.nameReceived == false) {
        plants[i].nameReceived = true;
        Serial.print("init ");
      }
      Serial.print(sensorId);
      Serial.print(" ");
      Serial.print(loopPlant.name);
      Serial.print(" with value ");
      Serial.println(strvalue);

      plants[i].name = strvalue;
      msg.setType(V_TEXT);
      msg.setSensor(nameId);
      msg.set(message.getString());
      send(msg);
    }
  }
  if ((sensorId == 0) && initTempReceived == false)
  {
    Serial.println("Received init for temperature");
    msg.setType(V_TEMP).setSensor(sensorId).set(value, 2);
    initTempReceived = true;
  }
  if (sensorId == 1 && initHumReceived == false)
  {
    Serial.println("Received init for humidity");
    msg.setType(V_HUM).setSensor(sensorId).set(value, 2);
    initHumReceived = true;
  }
  if (sensorId == 2 && initLightReceived == false)
  {
    Serial.println("Received init for lightlevel");
    msg.setType(V_LIGHT_LEVEL).setSensor(sensorId).set(value, 2);
    initLightReceived = true;
  }
}

