#define MY_RADIO_NRF24
#define MY_BAUD_RATE 9600
#include <MySensors.h>
#include <MQ135.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>;

LiquidCrystal_I2C lcd(0x27, 16, 2);

int mq135_analogPin = A0;
int mq3_analogPin = A1;
int mq4_analogPin = A2;
int mq9_analogPin = A3;
MQ135 gasSensor = MQ135(mq135_analogPin);

#define DHTPIN 3     // digital pin
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

const char Sketch_Info[] = "Air Quality Sensor";
const char Sketch_Version[] = "1.0";
MyMessage msg;
#define NUMBER_OF_SENSORS 6
#define INTERRUPT_PIN 8
int warmUpTime = 120; //in seconds
int DISPLAY_CHANGE_TIME = 6; // in loops


void before() {
  Serial.println("before");
  lcd.begin();
  dht.begin();
  if (warmUpTime != 0) {
    lcd.setCursor(0, 0);
    lcd.print("Warming up");
    for (int i = warmUpTime; i >= 0; i--) {
      if(i>99){
        lcd.setCursor(11, 0);        
      }else{        
        lcd.setCursor(11, 0);
        lcd.print(" ");
        lcd.setCursor(12, 0);
      }  
      lcd.print(i);
      delay(1000);
    }
  }else{    
      lcd.setCursor(0, 0);
      lcd.print("Initializing...");
  }
  pinMode(INTERRUPT_PIN, INPUT);
}

void presentation() {
  Serial.println("presentation");
  sendSketchInfo(Sketch_Info, Sketch_Version);
  bool hvacPresent = false;
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    int id = i;
    int type = S_AIR_QUALITY;
    String description = "";
    if (id == 0) {
      description = "MQ135 Sensor";
    } else if (id == 1) {
      description = "MQ3 Sensor";
    } else if (id == 2) {
      description = "MQ4 Sensor";
    } else if (id == 3) {
      description = "MQ9 Sensor";
    }else if (id == 4) {
      description = "Temperature Sensor";
    }else if (id == 5) {
      description = "Humidity Sensor";
    }
    int stringLength = description.length() + 1;
    char descrBuffer[stringLength];
    description.toCharArray(descrBuffer, stringLength);
    present(id, type, descrBuffer);
  }
}

bool initial = true;
float rzero135 = 0.0;
const int NUMBER_OF_READINGS = 10;
float values[NUMBER_OF_SENSORS][NUMBER_OF_READINGS];
float mq135_value_now = 0.0;
float mq3_value_now = 0.0;
float mq4_value_now = 0.0;
float mq9_value_now = 0.0;
float hum_value_now = 0.0;
float temp_value_now = 0.0;

int currentReading = 0;
int wakeStatus=0;
bool sleeping = false;
int lastPrintTime = DISPLAY_CHANGE_TIME;
int displayLoop = 0;
int displayLoops = 2;
void loop() { 
  wakeStatus=digitalRead(INTERRUPT_PIN);
  if(wakeStatus==HIGH && sleeping == true){
     wakeMeUp();
  }
  
  
     
  mq3_value_now = analogRead(mq3_analogPin); 
  mq4_value_now = analogRead(mq4_analogPin);
  mq9_value_now = analogRead(mq9_analogPin);
  temp_value_now = dht.readTemperature();
  hum_value_now = dht.readHumidity();
      
  if (initial==true) {
    for (int i = 0; i < 5; i++) {  
      if(isnan(temp_value_now))
      {      
        temp_value_now = dht.readTemperature();
        hum_value_now = dht.readHumidity();
      }
      //Serial.println(temp_value_now);  
      //Serial.println(hum_value_now);  
      wait(1000);
    }
  }
  
  if(!isnan(temp_value_now))
  {
    mq135_value_now = gasSensor.getCorrectedPPM(temp_value_now,hum_value_now);
    rzero135 = gasSensor.getCorrectedRZero( temp_value_now, hum_value_now);
  }else{
    mq135_value_now = gasSensor.getPPM();
    rzero135 = gasSensor.getRZero();
  }
        
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {    
    int variableType = V_LEVEL;
    int id = i;
    msg.setType(variableType).setSensor(id);
    if (id == 0) {
        if(!isnan(mq135_value_now))
        {
          values[i][currentReading] = mq135_value_now;
        }          
        else
          values[i][currentReading] = 0.0;    
      
    } else if (id == 1) {
        values[i][currentReading] = mq3_value_now;
      
    } else if (id == 2) {
        values[i][currentReading] = mq4_value_now;
     
    } else if (id == 3) {
        values[i][currentReading] = mq9_value_now;
      
    } else if (id == 4) {    
        if(!isnan(temp_value_now))
        {
          values[i][currentReading] = temp_value_now;
        }
        else
          values[i][currentReading] = 0.0;
      
    }else if (id == 5) {
        if(!isnan(hum_value_now))
        {
          values[i][currentReading] = hum_value_now;
        }
        else
          values[i][currentReading] = 0.0;
      
    }

   if (initial==true) {
      float value =  values[i][0];
      msg.set(value, 2);
      Serial.print("Send init for ");
      Serial.print(id);
      Serial.print(" with value ");
      Serial.println(value);
      send(msg);
      request(id, variableType);
      wait(2000, C_SET, variableType);
   
   }else{    
      if (currentReading == (NUMBER_OF_READINGS-1)) {
        float valueSend = 0.0;
        for (int reading = 0; reading < NUMBER_OF_READINGS; reading++) {    
          valueSend +=values[i][reading];
          values[i][reading]=0.0;
        }
        valueSend = valueSend / NUMBER_OF_READINGS;
        msg.set(valueSend, 2);
        send(msg);
      }    
   }
   
  }

  if(initial==false){
    //Serial.print("currentReading: ");
    if(currentReading == (NUMBER_OF_READINGS-1)){   
      Serial.println("reset");   
      currentReading = 0;
    }else{      
      currentReading++;
    }  
  }else{
    lcd.clear();   
    initial = false;
  }

 /*if(currentDisplayTime>(displayOffTime+displayOnTime))
  {    
    fallAsleep();        
  }else{
    currentDisplayTime++;        
  }*/
  if(sleeping == false){    
    if(lastPrintTime >= DISPLAY_CHANGE_TIME){
      if(displayLoop>=displayLoops){
        fallAsleep();  
      }
      lastPrintTime = 0;
      printScreen();
    }
    lastPrintTime++;    
  } 
  wait(500);
}
void fallAsleep()
{ 
  Serial.println("fallAsleep");
  lcd.clear();
  lcd.setBacklight(LOW);
  lcd.noDisplay();
  sleeping = true;
}

int printPage = 0;
void wakeMeUp(){
  Serial.println("wakeMeUp");
  lcd.setBacklight(HIGH);
  lcd.display();
  printPage = 0;
  sleeping = false;
  displayLoop=0;
  lastPrintTime = DISPLAY_CHANGE_TIME;
}

void printScreen()
{ 
    // Reset the display
    lcd.clear();
    char lineOne[16] = "";
    char lineTwo[16] = "";

    if (printPage == 0) {
        lcd.setCursor(0, 0);
        lcd.print("MQ135 z: ");
        lcd.print(rzero135);

        lcd.setCursor(0, 1);
        lcd.print("MQ135:   ");
        lcd.print(mq135_value_now);
        
        printPage++;
    }
    else if (printPage == 1) {
      lcd.setCursor(0, 0);
      lcd.print("MQ3: ");
      lcd.print(mq3_value_now);

      lcd.setCursor(0, 1);
      lcd.print("MQ4: ");
      lcd.print(mq4_value_now);

      printPage++;
    }
    else if (printPage == 2) {
      lcd.setCursor(0, 0);
      lcd.print("MQ9: ");
      lcd.print(mq9_value_now);

      //lcd.setCursor(0, 1);
      //lcd.print("Temp: ");
      //lcd.print(temp_value_now);

      printPage++;
    }
    else if (printPage == 3) {
      lcd.setCursor(0, 0);
      lcd.print("Hum:  ");
      lcd.print(hum_value_now);

      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temp_value_now);
      displayLoop++;
      printPage = 0;
    }
}


void roundValue(float value, int decimals,char *dest){
  //char* buff = malloc(decimals+4);
  dtostrf (value, sizeof(dest), decimals, dest);
  //return  dest;
}

void receive(const MyMessage & message) {

}
