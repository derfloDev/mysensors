/**
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * http://www.mysensors.org/build/light
 */

#define MY_DEBUG
#define MY_RADIO_NRF24

#define MY_RF24_CE_PIN 49
#define MY_RF24_CS_PIN 53
 
#include <MySensors.h>

#define SN "LightSwitch"
#define SV "1.0"
#define CHILD_ID 1

MyMessage msg(CHILD_ID, V_TRIPPED);

void setup()
{
  sendSketchInfo(SN, SV);
  present(CHILD_ID, S_DOOR);    
}



bool state = false;

  
void loop()
{
  /*sleep(10000);
  state = !state;
  Serial.print("State: ");
  Serial.println(state);
  send(msg.set(state));
  //if (debouncer.update()) {
    // Get the update value.
    //int value = debouncer.read();
    // Send in the new value.
    //gw.send(msg.set(value == LOW ? 1 : 0));
  //}
  */
}
