#define MY_RADIO_NRF24
#define MY_RF24_CE_PIN 49
#define MY_RF24_CS_PIN 53
//#define MY_DEBUG

#include <Adafruit_GFX.h> // Hardware-specific library
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <MySensors.h>
MCUFRIEND_kbv tft;

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
// most mcufriend shields use these pins and Portrait mode:
uint8_t YP = A1;  // must be an analog pin, use "An" notation!
uint8_t XM = A2;  // must be an analog pin, use "An" notation!
uint8_t YM = 7;   // can be a digital pin
uint8_t XP = 6;   // can be a digital pin
uint8_t SwapXY = 0;

uint16_t TS_LEFT = 171;
uint16_t TS_RT = 906;
uint16_t TS_TOP = 166;
uint16_t TS_BOT = 899;
char *name = "Unknown controller";

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

#define MINPRESSURE 15
#define MAXPRESSURE 1000

#define SWAP(a, b) {uint16_t tmp = a; a = b; b = tmp;}

#define NUMBER_OF_SENSORS 4
struct Button {
	int xPos;
	int yPos;
	int width;
	int height;
	bool state;
	bool initValueReceived;
	String description;
  String name;
};
Button buttons[NUMBER_OF_SENSORS] = { {10,10,105, 145,false, false, "Light1", "Button1"},{125,10,105, 145,false, false, "Light2", "Button2"},{10,165,105, 145,false, false, "Light3", "Button3"},{125,165,105, 145,false, false, "Light4", "Button4"} };

#define VARIABLE_TYPE V_STATUS
#define SENSOR_TYPE S_BINARY
#define INTERRUPT_PIN  21
#define INTERRUPT_MODE  RISING // LOW, CHANGE, RISING, FALLING
const int awakeTime = 15;//in seconds
const bool ENABLE_SLEEP_MODE = true;//true;
const char Sketch_Info[] = "TFT_Switch";
const char Sketch_Version[] = "1.0";
//#define BATTERY_POWERED // For more info look here https://www.mysensors.org/build/battery
const int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point

MyMessage msg;
int currentWakeTime = 0;
int oldBatteryLevel = 100;

void before() {
 // msg.clear();
	// Initialize external hardware etc here...
	currentWakeTime = millis() / 1000;
	for (int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++) {
		bool state = loadState(sensor);
    bool strSent = loadState(10+sensor);
		buttons[sensor].state = state;
    if(strSent == true){
      Serial.print("fu ");
      Serial.println(sensor);
      buttons[sensor].name = "null";      
    }
	}
	if (ENABLE_SLEEP_MODE)
	{
		pinMode(INTERRUPT_PIN, INPUT_PULLUP);
		//attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), test, INTERRUPT_MODE);    
	}

#if defined(BATTERY_POWERED)
	// use the 1.1 V internal reference
#if defined(__AVR_ATmega2560__)
	analogReference(INTERNAL1V1);
#else
	analogReference(INTERNAL);
#endif
#endif

	tft.reset();
	uint16_t id = tft.readID();
	tft.begin(id);
	tft.setRotation(1);     //Portrait
	tft.fillScreen(BLACK);
	tft.setTextColor(WHITE, BLACK);
	tft.setTextSize(2);     // System font is 8 pixels.  ht = 8*2=16
	tft.setCursor(35, 100);
  Serial.print("Initializing...");
	tft.print("Initializing...");
}

void presentation()
{
	sendSketchInfo(Sketch_Info, Sketch_Version);
	for (int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++) {
    String description = buttons[sensor].description;
    int stringLength = description.length() + 1;
    char descrBuffer[stringLength];
    description.toCharArray(descrBuffer, stringLength);
    
		Serial.print("Presenting Sensor ");
		Serial.print(sensor);   
    Serial.print(" -> ");    
    Serial.println(descrBuffer);
    int strSensor = 10+sensor;
    present(strSensor, S_INFO, descrBuffer);
    
		present(sensor, SENSOR_TYPE, descrBuffer);
	}
 tft.fillScreen(BLACK);
 tft.fillRect(0,0, 20,  20, CYAN);
 tft.fillRect(300,0, 20,  20, MAGENTA);
 tft.fillRect(0,220, 20,  20, YELLOW);
 tft.fillRect(300,220, 20,  20, WHITE);
}


void loop() {
  //Serial.println("loop");
  uint16_t xpos, ypos;  //screen coordinates
  tp = ts.getPoint();   //tp.x, tp.y are ADC values
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(XP, OUTPUT);
  pinMode(YM, OUTPUT);
  //SWAP(tp.x, tp.y)
  xpos = map(tp.x, TS_LEFT, TS_RT, 0, tft.width());    
  ypos = 220-map(tp.y, TS_TOP, TS_BOT, 0, tft.height());
    /*if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE) {
      Serial.print("x : ");
      Serial.println(xpos);
      Serial.print("y : ");
      Serial.println(ypos);
      wait(1000);
    }*/
	for (int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++) {
		//if not initialValue send
		if (buttons[sensor].initValueReceived==false)
		{
			
			Serial.print("Requesting initialValue for Sensor: ");
			Serial.println(sensor);
      msg.setType(VARIABLE_TYPE);
      msg.setSensor(sensor);
      msg.set(buttons[sensor].state);
			send(msg);   
      request(sensor, VARIABLE_TYPE);
      wait(2000, C_SET, VARIABLE_TYPE);

      int strSensor = 10+sensor;
      if(buttons[sensor].name != "null"){
        Serial.print("Requesting initialValue for Sensor: ");
        Serial.println(strSensor);      
        msg.setType(V_TEXT);
        msg.setSensor(strSensor);
        msg.set(buttons[sensor].name);
        send(msg);        
      }
      request(strSensor, V_TEXT);
      wait(2000, C_SET, V_TEXT);
		}		

		if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE) {
      
        
			int btnX = buttons[sensor].xPos;
			int btnY = buttons[sensor].yPos;
			int btnX2 = btnX + buttons[sensor].width;
			int btnY2 = btnY + buttons[sensor].height;
			//if (xpos >= btnX && xpos <= btnX2 && ypos >= btnY && ypos <= btnY2)
      if (ypos >= btnX && ypos  <= btnX2 && xpos >= btnY && xpos <= btnY2)
			{
				//change button state
				buttons[sensor].state = !buttons[sensor].state;
				// Store state in eeprom
				saveState(sensor, buttons[sensor].state);
        msg.setType(VARIABLE_TYPE);
				send(msg.setSensor(sensor).set(buttons[sensor].state));
				Serial.print("Sending message '");
				Serial.print(buttons[sensor].state);
				Serial.print("' for Sensor: ");
				Serial.println(sensor);        
				drawButton(buttons[sensor]);
				wait(1000);
			}
		}
	}

#if defined(BATTERY_POWERED)  
	// calc battery and send
	int batterySensorValue = analogRead(BATTERY_SENSE_PIN);
	int batteryLevel = batterySensorValue / 10;
	if (oldBatteryLevel != batteryLevel)
	{
		Serial.print("Battery sensor value: ");
		Serial.println(batterySensorValue);
		Serial.print("Sending batteryLevel: ");
		Serial.println(batteryLevel);
		sendBatteryLevel(batteryLevel);
		oldBatteryLevel = batteryLevel;
	}
#endif

	if (ENABLE_SLEEP_MODE)
	{
		int secondsUntilStart = millis() / 1000;
		/*Serial.print("SecondsUntilStart: ");
		Serial.println(secondsUntilStart);
		Serial.print("CurrentWakeTime: ");
		Serial.println(currentWakeTime);
		Serial.print("AwakeTime: ");
		Serial.println(currentWakeTime + awakeTime);*/
		if ((currentWakeTime + awakeTime) <= secondsUntilStart)
		{
    //check all initvalues before sleep
		
      bool stayAwake = false;
      for (int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++) {
        //if not initialValue send
        if (buttons[sensor].initValueReceived==false)
        {
          stayAwake = true;
        }
      }
      if(stayAwake == false){       
  			fallAsleep();     
        wait(100);
        sleep(digitalPinToInterrupt(INTERRUPT_PIN), INTERRUPT_MODE, 0);
  			//smartSleep(digitalPinToInterrupt(INTERRUPT_PIN), INTERRUPT_MODE, 0);
  			wakeMeUp();
		  }
	  }
	}
 
 
}


void drawButton(Button button)
{
	Serial.println("Drawing Button");
	char *color = RED;
	if (button.state == true)
		color = GREEN;
  tft.setRotation(1); 
	//tft.fillRect(button.xPos, button.yPos, button.width, button.height, color);
  tft.fillRect(button.yPos,button.xPos, button.height,  button.width, color);
  //tft.setRotation(1); 
  tft.setTextColor(WHITE, color);
  tft.setTextSize(2);     // System font is 8 pixels.  ht = 8*2=16
  tft.setCursor( button.yPos+5,button.xPos+(button.width/2));
  tft.print(button.name);      
}

void fallAsleep() {
	Serial.println("fallAsleep");
	tft.fillScreen(BLACK);
	tft.setCursor(35, 150);
	tft.print("Sleeping...");
	//send data before sleep, turn of hardware etc...  
	// example tft.displayOff()

}

void wakeMeUp() {
	Serial.println("wakeMeUp");
	//do your wakeup stuff here  
	tft.fillScreen(BLACK);
  for (int sensor = 0; sensor < NUMBER_OF_SENSORS; sensor++) {
    drawButton(buttons[sensor]);
    //request switch state
    request(sensor, VARIABLE_TYPE);
    wait(2000, C_SET, VARIABLE_TYPE);
  }
	currentWakeTime = millis() / 1000;
	//do your wakeup stuff here
	// example tft.displayOn()
}

void receive(const MyMessage &message) {
	Serial.println("receive");
  Serial.println(message.type);
  int sensor = message.sensor;
	if (message.type == VARIABLE_TYPE) {
		if (buttons[sensor].initValueReceived==false)
		{
			buttons[sensor].initValueReceived = true;
			Serial.print("InitialValueReceived for Sensor: ");
			Serial.println(sensor);
		}
		int value = message.getBool();
		// Store state in eeprom
		saveState(sensor, value);
		// sendback state
    msg.setType(VARIABLE_TYPE);
		send(msg.setSensor(sensor).set(value));
		buttons[sensor].state = value;
		Serial.print("Received message '");
		Serial.print(value);
		Serial.print("' for Sensor ");
		Serial.println(sensor);
    drawButton(buttons[sensor]);
	}
   else if(message.type == V_TEXT) {
              
      saveState(sensor, true);
      
      Serial.print("String for Sensor: ");
      Serial.print(sensor);
      String value = message.getString();
      Serial.print(" :");
      Serial.println(value);
      buttons[sensor-10].name = value;

      int valueLength = value.length() + 1;
      char valueBuffer[valueLength];
      value.toCharArray(valueBuffer, valueLength);
      
      msg.setType(V_TEXT);
      msg.setSensor(sensor).set(valueBuffer);
      send(msg);
      drawButton(buttons[sensor-10]);
   }
 
}

