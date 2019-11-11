//#define MY_DEBUG
#define MY_RADIO_NRF24
#include <SPI.h>
#include <MySensors.h>

#define SENSOR_ID 1
#define SENSOR_ID_TEMP 2
#define Sketch_Info = "Temperature Light";
#define Sketch_Version = "1.0";
#define RED_PIN 3 
#define WHITE_PIN 9
#define GREEN_PIN 5
#define BLUE_PIN 6
#define NUM_CHANNELS 3 // how many channels, RGBW=4 RGB=3...

byte channels[4] = { RED_PIN, GREEN_PIN, BLUE_PIN, WHITE_PIN };
byte values[4] = { 0, 0, 0, 255 };
byte target_values[4] = { 0, 0, 0, 255 };

#define STEP 1
#define INTERVAL 10
const int pwmIntervals = 255;
float R; // equation for dimming curve

// tracks if the strip should be on of off
boolean isOn = true;

MyMessage rgb_msg(SENSOR_ID, V_RGBW);
MyMessage status_msg(SENSOR_ID, V_STATUS);
MyMessage dimmer_msg(SENSOR_ID, V_DIMMER);
MyMessage temperature_msg(SENSOR_ID_TEMP, V_TEXT);

// dimmer, status, rgb, text(temperature)
boolean first_message_sent[3] = [false,false,false,false];


void before() {
	// Set all channels to output (pin number, type)
	for (int i = 0; i < NUM_CHANNELS; i++) {
		pinMode(channels[i], OUTPUT);
	}

	// set up dimming
	R = (pwmIntervals * log10(2)) / (log10(255));
}

void presentation() {
	Serial.println("Presentation");

	// Present sketch (name, version)
	sendSketchInfo(Sketch_Info, Sketch_Version);

	// Register sensors (id, type, description, ack back)
	present(SENSOR_ID, S_RGBW_LIGHT, Sketch_Info, true);
	present(SENSOR_ID_TEMP, S_INFO, "Temperature");

	// get old values if this is just a restart
	request(SENSOR_ID, V_RGBW);
	request(SENSOR_ID, V_STATUS);
	request(SENSOR_ID, V_DIMMER);
	request(SENSOR_ID_TEMP, V_TEXT);
}

void loop() {
	wait(1000);
	// Send msg on startup
	for(var i = 0; i<=3;i++)
	{
		if (first_message_sent[i] == false) {
			Serial.println("Sending initial state...");
			//send all start values if only one value is not already known by the controller
			send_startup();
			i = 3;			
		}
	}
	// init lights
	updateLights();
}

void updateLights() {

	// for each color
	for (int v = 0; v < NUM_CHANNELS; v++) {

		if (values[v] < target_values[v]) {
			values[v] += STEP;
			if (values[v] > target_values[v]) {
				values[v] = target_values[v];
			}
		}

		if (values[v] > target_values[v]) {
			values[v] -= STEP;
			if (values[v] < target_values[v]) {
				values[v] = target_values[v];
			}
		}
	}

	// dimming
	if (dimming < target_dimming) {
		dimming += STEP;
		if (dimming > target_dimming) {
			dimming = target_dimming;
		}
	}
	if (dimming > target_dimming) {
		dimming -= STEP;
		if (dimming < target_dimming) {
			dimming = target_dimming;
		}
	}

	/*
	// debug - new values
	Serial.println(greenval);
	Serial.println(redval);
	Serial.println(blueval);
	Serial.println(whiteval);
	Serial.println(target_greenval);
	Serial.println(target_redval);
	Serial.println(target_blueval);
	Serial.println(target_whiteval);
	Serial.println("+++++++++++++++");
	*/

	// set actual pin values
	for (int i = 0; i < NUM_CHANNELS; i++) {
		if (isOn) {
			// normal fading
			//analogWrite(channels[i], dimming / 100.0 * values[i]);

			// non linear fading, idea from https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms/
			analogWrite(channels[i], pow(2, (dimming / R)) - 1);
		}
		else {
			analogWrite(channels[i], 0);
		}
	}
}

void send_startup() {
	send(rgb_msg.set("ff0000ff"));
	send(dimmer_msg.set(0));
	send(status_msg.set(0));
	send(temperature_msg.set("20"));
}

void fallAsleep() {
	Serial.println("fallAsleep");
	//send data before sleep, turn of hardware etc...  
	// example tft.displayOff()

}

void wakeUp() {
	Serial.println("wakeUp");
	currentWakeTime = millis() / 1000;
	//do your wakeup stuff here  
}

// callback function for incoming messages
void receive(const MyMessage &message) {

	// DEBUG
	Serial.print("Got a message - ");
	Serial.print("Messagetype is: ");
	Serial.println(message.type);
	// END DEBUG

	

	// Acknoledgment
	if (message.isAck()) {
		Serial.println("Got ack from gateway");
	}
	else if (message.type == V_DIMMER) {
		if (first_message_sent[0] == false)
		{
			first_message_sent[0] = true;
		}
		Serial.println("Dimming to ");
		int dimmerVal = message.getInt();
		Serial.println(dimmerVal);
		send(dimmer_msg.set(dimmerVal));

		// a new dimmer value also means on, no seperate signal gets send (by domoticz)
		isOn = true;

	}
	else if (message.type == V_STATUS) {
		if (first_message_sent[1] == false)
		{
			first_message_sent[1] = true;
		}
		Serial.print("Turning light ");
		int lightVal = message.getInt();
		Serial.println(lightVal);
		send(status_msg.set(lightVal));
		isOn = lightVal;
		// Do On/Off stuff here

	}
	else if (message.type == V_RGB) {
		if (first_message_sent[2] == false)
		{
			first_message_sent[2] = true;
		}
		Serial.println("RGBW!");
		const char * rgbvalues = message.getString();
		Serial.println(rgbvalues);
		send(rgb_msg.set(rgbvalues));
		inputToRGBW(rgbvalues);
		isOn = true;
		// Set color here

	}
	else if (message.type == V_TEXT) {
		if (first_message_sent[3] == false)
		{
			first_message_sent[3] = true;
		}
		const char * stringTemp = message.getString();
		Serial.println(stringTemp);
		Serial.print("Temperature ");
		send(temperature_msg.set(stringTemp));

		float temp = stringTemp.toFloat();
		const char * rgbvalues = "ffffff";
		if (temp > 40) {
			rgbvalues = "F51003";
		}
		else if (temp > 35.0) {
			rgbvalues = "CA5B13";
		}
		else if (temp > 30.0) {
			rgbvalues = "A49D22";
		}
		else if (temp > 25.0) {
			rgbvalues = "84D42E";
		}
		else if (temp > 20.0) {
			rgbvalues = "67FF3C";
		}
		else if (temp > 15.0) {
			rgbvalues = "55FF56";
		}
		else if (temp > 10.0) {
			rgbvalues = "46FF6D";
		}
		else if (temp > 5.0) {
			rgbvalues = "33FF88";
		}
		else if (temp > 0.0) {
			rgbvalues = "22FFA0";
		}
		else if (temp > -5.0) {
			rgbvalues = "28E5B6";
		}
		else if (temp > -10.0) {
			rgbvalues = "31C8CA";
		}
		else if (temp > -15.0) {
			rgbvalues = "3AABDF";
		}
		else if (temp > -20.0) {
			rgbvalues = "438DF5";
		}
		inputToRGBW(rgbvalues);
		isOn = true;
	}
}

// converts incoming color string to actual(int) values
// ATTENTION this currently does nearly no checks, so the format needs to be exactly like domoticz sends the strings
void inputToRGBW(const char * input) {
	Serial.print("Got color value of length: ");
	Serial.println(strlen(input));

	if (strlen(input) == 6) {
		Serial.println("new rgb value");
		target_values[0] = fromhex(&input[0]);
		target_values[1] = fromhex(&input[2]);
		target_values[2] = fromhex(&input[4]);
		target_values[3] = 0;
	}
	else if (strlen(input) == 9) {
		Serial.println("new rgbw value");
		target_values[0] = fromhex(&input[1]); // ignore # as first sign
		target_values[1] = fromhex(&input[3]);
		target_values[2] = fromhex(&input[5]);
		target_values[3] = fromhex(&input[7]);
	}
	else {
		Serial.println("Wrong length of input");
	}


	Serial.print("New color values: ");
	Serial.println(input);

	for (int i = 0; i < NUM_CHANNELS; i++) {
		Serial.print(target_values[i]);
		Serial.print(", ");
	}

	Serial.println("");
	Serial.print("Dimming: ");
	Serial.println(dimming);
}

// converts hex char to byte
byte fromhex(const char * str)
{
	char c = str[0] - '0';
	if (c > 9)
		c -= 7;
	int result = c;
	c = str[1] - '0';
	if (c > 9)
		c -= 7;
	return (result << 4) | c;
}
