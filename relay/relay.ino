
//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE
#define MY_NODE_ID 1
#include <SPI.h>
#include <MySensors.h>
#include <Bounce2.h>


#define RELAY_1 3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 6 // Total number of attached relays
#define RELAY_ON 0
#define RELAY_OFF 1
#define CHILD_ID 1

Bounce debouncer = Bounce();
bool states [NUMBER_OF_RELAYS];// = false;
bool initialValueSent[NUMBER_OF_RELAYS];// = false;

MyMessage msg(CHILD_ID, V_STATUS);
MyMessage msg2(2, V_STATUS);
MyMessage msgs [NUMBER_OF_RELAYS];


void before()
{
    for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS; sensor++, pin++) {
        msgs[sensor-1] = MyMessage(sensor,V_STATUS);
        initialValueSent[sensor-1]= false;
        // Then set relay pins in output mode
        pinMode(pin, OUTPUT);
        // Set relay to last known state (using eeprom storage)
        Serial.print("Load state for sensor :");
        Serial.print(sensor);
        Serial.print("; State :");
        Serial.println(loadState(sensor));
        bool state = loadState(sensor);
        digitalWrite(pin, state?RELAY_ON:RELAY_OFF);
        states[sensor-1] = state;
    }
}

void setup()
{
  debouncer.interval(10);
}

void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Relay", "1.0");

    for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS; sensor++, pin++) {
        // Register all sensors to gw (they will be created as child devices)
        present(sensor, S_BINARY);
    }
}


void loop()
{
  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS; sensor++, pin++) {
    if (!initialValueSent[sensor-1]) {
      msg.setSensor(sensor);
      
      Serial.print("Sensor: ");
      Serial.print(sensor);
      Serial.print("initval: ");
      Serial.println(states[sensor-1]);
      send(msg.set(states[sensor-1])); 
      Serial.print("Requesting initial value for sensor: ");
      Serial.println(sensor);
      request(sensor, V_STATUS);
      wait(2000, C_SET, V_STATUS);
      /*Serial.print("Sending initial value for sensor: ");
      Serial.print(sensor);
      Serial.print("; Value:");
      Serial.println(states[sensor-1]);
      send(msgs[sensor-1].set(states[sensor-1]?RELAY_OFF:RELAY_ON));
      Serial.println("Requesting initial value from controller");
      request(sensor-1, V_STATUS);
      wait(2000, C_SET, V_STATUS);
    }
    if (debouncer.update()) {
    if (debouncer.read()==LOW) {
      states[sensor-1] = !states[sensor-1];
      // Send new state and request ack back
      send(msgs[sensor-1].set(states[sensor-1]?RELAY_OFF:RELAY_ON), true);
    }*/
  }
   }
   
}

void receive(const MyMessage &message)
{
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }
    // We only expect one type of message from controller. But we better check anyway.
    if (message.type==V_STATUS) {
        if (!initialValueSent[message.sensor-1]) {
          Serial.print("Receiving initial value from controller for sensor:");
          Serial.println(message.sensor);      
          initialValueSent[message.sensor-1] = true;
        }
        // Change relay state
        digitalWrite(message.sensor-1+RELAY_1, message.getBool()?RELAY_ON:RELAY_OFF);
        // Store state in eeprom
        saveState(message.sensor, message.getBool());
        // sendback state 
        send(msgs[message.sensor-1].set(message.getBool()?RELAY_OFF:RELAY_ON));
        // Write some debug info
        Serial.print("Incoming change for sensor:");
        Serial.print(message.sensor);
        Serial.print(", New status: ");
        Serial.println(message.getBool());
    }
}
