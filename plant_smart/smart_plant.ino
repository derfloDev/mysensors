//#define MY_DEBUG
#define MY_RADIO_NRF24
#include <SPI.h>
#include <MySensors.h>
//#include <Sensor.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h> // F Malpartida's NewLiquidCrystal library
#include <Wire.h>
#include <DallasTemperature.h>
#include <Timer.h>
#include <analogmuxdemux.h>


#define I2C_ADDR 0x27 // Define I2C Address for LCD controller
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7
#define BACKLIGHT 3
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

#define ONE_WIRE_BUS 4
OneWire  ds(ONE_WIRE_BUS);
DallasTemperature dallasSensors(&ds);

int waterlevelSensor = 0;
unsigned long DISPLAY_CHANGE_TIME = 5000; // Sleep time between reads (in milliseconds)


#define NUMBER_OF_PLANTS 3
//Uno, Nano, Mini, other 328-based  2, 3
#define INTERRUPT_PIN 2
#define INTERRUPT_MODE CHANGE // LOW, CHANGE, RISING, FALLING
const int awakeTime = 10; //in seconds
const float minWaterLevel = 15;
const bool ENABLE_SLEEP_MODE = false;
const char Sketch_Info[] = "SmartPlant";
const char Sketch_Version[] = "1.0";

#define MUXREADPIN 0 // What analog input on the arduino do you want?
// master selects
#define M_S0 4
#define M_S1 3
#define M_S2 2
// slave selects
#define S_S0 7
#define S_S1 6
#define S_S2 5
AnalogMux amx(M_S0,M_S1,M_S2, S_S0,S_S1,S_S2, MUXREADPIN);

typedef struct
{
	int childId;
	int sensorType;
	int variableType;
	String description;
	float floatValue;
	bool initValueSent;
	int sensorPin;
}  Sensor;

typedef struct
{
	String name;
	int Id;
	Sensor moisture;
	Sensor temperature;
	Sensor brightness;
	Sensor minMoisture;
	Sensor waterLevel;
	bool watering;
	DeviceAddress tempDeviceAddress;
}  Plant;

plants Plant[NUMBER_OF_PLANTS] = [
	{ "Pflanze 1", 10, {}, {}, {}, {}, {}, false,{ 0x28, 0x8A, 0xB1, 0x40, 0x04, 0x00, 0x00, 0xC7 } },
	{ "Pflanze 2", 20, {}, {}, {}, {}, {}, false,{ 0x28, 0x8A, 0xB1, 0x40, 0x04, 0x00, 0x00, 0xC7 } },
	{ "Pflanze 3", 30, {}, {}, {}, {}, {}, false,{ 0x28, 0x8A, 0xB1, 0x40, 0x04, 0x00, 0x00, 0xC7 } }
];

int currentWakeTime = 0;
Timer *displayValuesTimer = new Timer(DISPLAY_CHANGE_TIME);

MyMessage msg;

void before() {
	Serial.println("before start");
	// Initialize external hardware etc here...
	currentWakeTime = millis() / 1000;
	if (ENABLE_SLEEP_MODE) {
		pinMode(INTERRUPT_PIN, INPUT_PULLUP);
	}
	lcd.begin(16, 2);  // initialize the lcd
	// Switch on the backlight
	lcd.setBacklightPin(BACKLIGHT, POSITIVE);
	lcd.setBacklight(HIGH);
	dallasSensors.begin();

	//timer
	displayValuesTimer->setOnTimer(&printScreen);
	displayValuesTimer->Start();

	//initialize plants and its sensors and load init values
	for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
		Plant loopPlant = plants[i];
		int plantId = loopPlant.Id;

		int sensorId = plantId + 0;
		float moistureValue = loadState(sensorId);
		int sensorPin = calcSensorPin();
		loopPlant.moisture = { sensorId,S_MOISTURE,V_LEVEL,"Soil moisture", moistureValue,false,sensorPin};

		sensorId = plantId + 1;
		float temperatureValue = loadState(sensorId);
		loopPlant.temperature = { sensorId,S_TEMP,V_TEMP,"Temperature", temperatureValue,false };

		sensorId = plantId + 2;
		float brightnessValue = loadState(sensorId);
		int sensorPin = calcSensorPin();
		loopPlant.brightness = { sensorId,S_LIGHT_LEVEL,V_LIGHT_LEVEL,"Brightness", brightnessValue,false,sensorPin };

		sensorId = plantId + 3;
		float minMoistureValue = loadState(sensorId);
		loopPlant.minMoisture = { sensorId,S_CUSTOM,V_VAR1,"Min moisture", minMoistureValue,false };

		sensorId = plantId + 4;
		float waterLevelValue = loadState(sensorId);
		int sensorPin = calcSensorPin();
		loopPlant.waterLevel = { sensorId,S_WATER,V_VOLUME,"Water level", waterLevelValue,false,sensorPin };

	}
	Serial.println("before end");
}

int sensorPin = 0;
int calcSensorPin()
{
	int retVal = sensorPin;
	sensorPin++;
	return retVal
}

void presentation() {
	Serial.println("Presentation");
	sendSketchInfo(Sketch_Info, Sketch_Version);
	bool hvacPresent = false;
	for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
		Plant loopPlant = plants[i];

		int sensorId = loopPlant.moisture.childId;
		int type = loopPlant.moisture.sensorType;
		String description = loopPlant.name + " " + loopPlant.moisture.description
			int stringLength = description.length() + 1;
		char descrBuffer[stringLength];
		description.toCharArray(descrBuffer, stringLength);
		present(sensorId, type, descrBuffer);

		sensorId = loopPlant.temperature.childId;
		type = loopPlant.temperature.sensorType;
		description = loopPlant.name + " " + loopPlant.temperature.description
			stringLength = description.length() + 1;
		descrBuffer[stringLength];
		description.toCharArray(descrBuffer, stringLength);
		present(sensorId, type, descrBuffer);

		sensorId = loopPlant.brightness.childId;
		type = loopPlant.brightness.sensorType;
		description = loopPlant.name + " " + loopPlant.brightness.description
			stringLength = description.length() + 1;
		descrBuffer[stringLength];
		description.toCharArray(descrBuffer, stringLength);
		present(sensorId, type, descrBuffer);

		sensorId = loopPlant.minMoisture.childId;
		type = loopPlant.minMoisture.sensorType;
		description = loopPlant.name + " " + loopPlant.minMoisture.description
			stringLength = description.length() + 1;
		descrBuffer[stringLength];
		description.toCharArray(descrBuffer, stringLength);
		present(sensorId, type, descrBuffer);


		sensorId = loopPlant.waterLevel.childId;
		type = loopPlant.waterLevel.sensorType;
		description = loopPlant.name + " " + loopPlant.waterLevel.description
			stringLength = description.length() + 1;
		descrBuffer[stringLength];
		description.toCharArray(descrBuffer, stringLength);
		present(sensorId, type, descrBuffer);
	}
}
void loop() {
	wait(1000);
	displayValuesTimer->Update();
	for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
		Plant loopPlant = plants[i];
		int variableType = loopPlant.moisture.variableType;
		int sensorId = loopPlant.moisture.childId;
		float oldMoisture = loopPlant.moisture.floatValue;
		if (loopPlant.moisture.initValueSent == false) {
			Serial.print("Sending init for ");
			Serial.print(loopPlant.name + " " + loopPlant.moisture.description);
			Serial.print(" with value ");
			Serial.println(loopPlant.moisture.floatValue);

			msg.setType(variableType).setSensor(sensorId).set(oldMoisture);
			send(msg);
			request(sensorId, variableType);
			wait(2000, C_SET, variableType);
		}
		else {
			float moisture = getMoisture(loopPlant.moisture);
			float minMoisture = loopPlant.minMoisture.floatValue;
			// water plant if too dry and not already watering
			if (moisture < minMoisture && loopPlant.watering == false)
			{
				waterPlant(loopPlant.Id, true);
			}
			// stop watering plant if wet enough and still watering
			else if (moisture > minMoisture && loopPlant.watering == true)
			{
				waterPlant(loopPlant.Id, false);
			}
			if (moisture != oldMoisture) {
				Serial.println("sending message '" + String(moisture) + "' for " + loopPlant.name + " " + loopPlant.moisture.description);
				msg.setType(variableType).setSensor(sensorId).set(moisture, 2);
				send(msg);
				saveState(sensorId, moisture);
				plants[i].moisture.floatValue = moisture;
			}
		}

		// temperature
		int variableType = loopPlant.temperature.variableType;
		int sensorId = loopPlant.temperature.childId;
		float oldTemperature = loopPlant.temperature.floatValue;
		if (loopPlant.temperature.initValueSent == false) {
			Serial.print("Sending init for ");
			Serial.print(loopPlant.name + " " + loopPlant.temperature.description);
			Serial.print(" with value ");
			Serial.println(loopPlant.temperature.floatValue);

			msg.setType(variableType).setSensor(sensorId).set(oldTemperature);
			send(msg);
			request(sensorId, variableType);
			wait(2000, C_SET, variableType);
		}
		else {
			float temperature = getTemperature(loopPlant.temperature);
			//float oldTemperature = loopPlant.temperature;
			if (temperature != oldTemperature) {
				Serial.println("sending message '" + String(temperature) + "' for " + loopPlant.name + " " + loopPlant.temperature.description);
				msg.setType(variableType).setSensor(sensorId).set(temperature, 2);
				send(msg);
				saveState(sensorId, temperature);
				plants[i].temperature.floatValue = temperature;
			}
		}

		// brightness
		int variableType = loopPlant.brightness.variableType;
		int sensorId = loopPlant.brightness.childId;
		float oldBrightness = loopPlant.brightness.floatValue;
		if (loopPlant.brightness.initValueSent == false) {
			Serial.print("Sending init for ");
			Serial.print(loopPlant.name + " " + loopPlant.brightness.description);
			Serial.print(" with value ");
			Serial.println(loopPlant.brightness.floatValue);

			msg.setType(variableType).setSensor(sensorId).set(oldBrightness);
			send(msg);
			request(sensorId, variableType);
			wait(2000, C_SET, variableType);
		}
		else {
			float brightness = getBrightness(loopPlant.brightness);
			float oldBrightness = loopPlant.brightness;
			if (brightness != oldBrightness) {
				Serial.println("sending message '" + String(brightness) + "' for " + loopPlant.name + " " + loopPlant.brightness.description);
				msg.setType(variableType).setSensor(sensorId).set(brightness, 2);
				send(msg);
				saveState(sensorId, brightness);
				plants[i].brightness.floatValue = brightness;
			}
		}

		// min moisture
		int variableType = loopPlant.minMoisture.variableType;
		int sensorId = loopPlant.minMoisture.childId;
		float value = loopPlant.minMoisture.floatValue;
		if (loopPlant.minMoisture.initValueSent == false) {
			Serial.print("Sending init for ");
			Serial.print(loopPlant.name + " " + loopPlant.minMoisture.description);
			Serial.print(" with value ");
			Serial.println(loopPlant.minMoisture.floatValue);

			msg.setType(variableType).setSensor(sensorId).set(value);
			send(msg);
			request(sensorId, variableType);
			wait(2000, C_SET, variableType);
		}

		// water level
		int variableType = loopPlant.waterLevel.variableType;
		int sensorId = loopPlant.waterLevel.childId;
		float oldWaterLevel = loopPlant.waterLevel.floatValue;
		if (loopPlant.waterLevel.initValueSent == false) {
			Serial.print("Sending init for ");
			Serial.print(loopPlant.name + " " + loopPlant.waterLevel.description);
			Serial.print(" with value ");
			Serial.println(loopPlant.waterLevel.floatValue);

			msg.setType(variableType).setSensor(sensorId).set(oldWaterLevel);
			send(msg);
			request(sensorId, variableType);
			wait(2000, C_SET, variableType);
		}
		else {
			float waterLevel = getWaterLevel(loopPlant.waterLevel);
			if (waterlevel < minWaterLevel) {
				waterPlant(loopPlant.Id, false);				
				Serial.println("Refill water tank");
			}
			if (waterLevel != oldWaterLevel) {
				Serial.println("sending message '" + String(waterLevel) + "' for " + loopPlant.name + " " + loopPlant.waterLevel.description);
				msg.setType(variableType).setSensor(sensorId).set(waterLevel, 2);
				send(msg);
				saveState(sensorId, waterLevel);
				plants[i].waterLevel.floatValue = waterLevel;
			}
		}
	}


	if (ENABLE_SLEEP_MODE) {
		int secondsUntilStart = millis() / 1000;
		Serial.println("SecondsUntilStart: " + String(secondsUntilStart));
		Serial.println("CurrentWakeTime: " + String(currentWakeTime));
		Serial.println("AwakeTime: " + String(currentWakeTime + awakeTime));

		if ((currentWakeTime + awakeTime) <= secondsUntilStart)
		{
			fallAsleep();
			sleep(digitalPinToInterrupt(INTERRUPT_PIN), INTERRUPT_MODE, 0);
			wakeUp();
		}
	}
}

void fallAsleep() {
	Serial.println("fallAsleep");
	tft.fillScreen(BLACK);
	tft.setCursor(35, 150);
	tft.print("Sleeping...");
	//send data before sleep, turn of hardware etc...  
	// example tft.displayOff()

}

void wakeUp() {
	Serial.println("wakeUp");
	//do your wakeup stuff here  
	tft.fillScreen(BLACK);
	currentWakeTime = millis() / 1000;
	//do your wakeup stuff here
	// example tft.displayOn()
}

float getSensorValue(sensor sensor)
{
	return amx.AnalogRead(sensor.sensorPin);
}

float getBrightness(sensor sensor)
{
	float retVal = 0.0;
	Serial.println("getBrightness");
	retVal = getSensorValue(sensor.sensorPin);
	// calculate percentage
	retVal = map(retVal,1023,0,0,100);
	return retVal;
}

float getMoisture(sensor sensor)
{
	float retVal = 0.0;
	Serial.println("getMoisture");
	retVal = getSensorValue(sensor.sensorPin);
	// calculate percentage
	retVal = map(retVal,550,0,0,100);
	return retVal;
}

float getWaterLevel(sensor sensor)
{
	float retVal = 0.0;
	Serial.println("getMoisture");
	retVal = getSensorValue(sensor.sensorPin);
	// calculate percentage
	retVal =  map(retVal,710,0,0,100);
	return retVal;
}

float getTemperature(Plant plant)
{
	float retVal = 0.0;
	Serial.println("getTemperature");
	if (currentPlant != null)
	{
		dallasSensors.requestTemperaturesByAddress(plant.tempDeviceAddress);
		retVal = dallasSensors.getTempC(plant.tempDeviceAddress);
	}
	return retVal;
}

void waterPlant(Plant plant, bool turnOn)
{
	Serial.println("waterPlant");
	if (plant != null)
	{
		if (turnOn == true) {
			if (plant.waterLevel.floatValue > minWaterLevel)
			{
				//TODO
				//Turn waterpump on
				Serial.println("water pump on");
			}
			else{
				Serial.println("Can't water. Refill water tank");
			}
		}
		else
		{
			//TODO			
			//Turn waterpump off
			Serial.println("water pump off");
		}
	}
	else
	{
		Serial.println("Uups");
	}

}

int printPage = 0;
void printScreen() {
	// Reset the display 
	lcd.clear();
	lcd.backlight();
	Plant currentPlant = plants[printPage];

	// EXAMPLE
	//##################
	//#PLANT 1|M 100|40#
	//#B 100 T 26   OFF#
	//##################
	lcd.setCursor(0, 0);
	lcd.print(currentPlant.name);
	lcd.setCursor(7, 0);
	lcd.print("|M");
	lcd.setCursor(10, 0);
	lcd.print(currentPlant.moisture.floatValue);
	lcd.setCursor(13, 0);
	lcd.print("|");
	lcd.print(currentPlant.minMoisture.floatValue);

	lcd.setCursor(0, 1);
	lcd.print("B");
	lcd.setCursor(2, 1);
	lcd.print(currentPlant.brightness.floatValue);
	lcd.setCursor(6, 1);
	lcd.print("T");
	lcd.setCursor(8, 1);
	lcd.print(currentPlant.temperature.floatValue);
	if (currentPlant.watering == true)
	{
		lcd.setCursor(14, 1);
		lcd.print("ON");
	}
	else
	{
		lcd.setCursor(13, 1);
		lcd.print("OFF");
	}

	if (printPage >= NUMER_OF_PLANTS)
	{
		printPage = 0;
	}
	else
	{
		printPage++;
	}
}

void receive(const MyMessage & message) {
	Serial.print("receive ");
	if (message.isAck()) {
		Serial.println("This is an ack from gateway");
		return;
	}

	for (int i = 0; i < NUMBER_OF_PLANTS; i++) {
		Plant loopPlant = plants[i];

		float value = message.getFloat();
		int plantId = message.sensor / 10;

		if (plantId == loopPlant.Id)
		{
			Sensor currentSensor;
			if (loopPlant.moisture.childId == message.sensor)
			{
				currentSensor = loopPlant.moisture;
			}
			else if (loopPlant.temperature.childId == message.sensor)
			{
				currentSensor = loopPlant.temperature;
			}
			else if (loopPlant.brightness.childId == message.sensor)
			{
				currentSensor = loopPlant.brightness;
			}
			else if (loopPlant.minMoisture.childId == message.sensor)
			{
				currentSensor = loopPlant.minMoisture;
			}

			if (currentSensor != null)
			{
				if (currentSensor.initValueSent == false) {
					currentSensor.initValueSent = true;
					Serial.print("Received init for ");
				}
				else
				{
					Serial.print("Received message for ");
				}
				Serial.print(loopPlant.name + " " + currentSensor.description);
				Serial.print(" with value ");
				Serial.println(value);

				saveState(currentSensor.childId, value);
				currentSensor.floatValue = value;
				msg.setType(currentSensor.variableType);
				msg.setSensor(currentSensor.childId);
				msg.set(value);
				send(msg);
			}
			else
			{
				Serial.println("Uuups");
			}
			return;
		}
	}
}



