# Plant sensor
This is a sample sketch for a arduino Nano with a LCD screen. It measures the soil moisture of multiple plants.
Moreover it has a light, humidity and temperature sensor.

# Dependencies
+ [MySensors](https://github.com/mysensors/MySensors)
+ SPI.h
+ Sensor.h
+ LCD.h
+ LiquidCrystal_I2C F Malpartida's NewLiquidCrystal library
+ DHT.h
+ TimerObject

# Parts
+ Arduino Uno [Aliexpress](https://de.aliexpress.com/wholesale?SearchText=arduino+uno)
+ LCD display [Aliexpress](https://de.aliexpress.com/wholesale?SearchText=1602+lcd)
+ Soil moisture Sensor [Aliexpress](https://www.aliexpress.com/item/Hot-Sale-10set-Soil-Moisture-Hygrometer-Detection-Humidity-Sensor-Module-For-arduino-Development-Board-DIY-Robot/32328197208.html?spm=2114.01010208.3.18.s9Bfhz&ws_ab_test=searchweb0_0,searchweb201602_1_10152_10065_10151_10068_5010016_10136_10137_10060_10138_10155_10062_437_10154_10056_10055_10054_10059_303_100031_10099_10103_10102_10101_10096_10052_10053_10107_10050_10142_10051_5030017_10084_10083_10080_10082_10081_10177_10110_519_10111_10112_10113_10114_10180_10182_10184_10078_10079_10073_10186_10123_10189_142-10050,searchweb201603_1,ppcSwitch_2&btsid=284a8601-bfcf-4f8f-9ac0-7b6c1fb0f679&algo_expid=7181809f-3f84-42d7-838f-3be78b672e23-2&algo_pvid=7181809f-3f84-42d7-838f-3be78b672e23) 
+ Light sensor [Aliexpress](https://www.aliexpress.com/item/Optical-Sensitive-Resistance-Sensor-Module-Light-Intensity-Detect-Photosensitive-Sensor-for-Arduino/32677701042.html?spm=2114.01010208.3.73.5hdoD5&ws_ab_test=searchweb0_0,searchweb201602_1_10152_10065_10151_10068_5010016_10136_10137_10060_10138_10155_10062_437_10154_10056_10055_10054_10059_303_100031_10099_10103_10102_10101_10096_10052_10053_10107_10050_10142_10051_5030017_10084_10083_10080_10082_10081_10177_10110_519_10111_10112_10113_10114_10180_10182_10184_10078_10079_10073_10186_10123_10189_142,searchweb201603_1,ppcSwitch_2&btsid=4a6b6fa6-d962-431f-86eb-516733b35532&algo_expid=d455411e-4666-42cc-b572-187cc14803e5-12&algo_pvid=d455411e-4666-42cc-b572-187cc14803e5)
+ Temperature/Humidity Sesor (DHT22) [Aliexpress](https://de.aliexpress.com/wholesale?SearchText=DHT22)
+ Power supply

# Installation
### Software
+ Install the dependent libraries
+ Open this sketch in the Arduino IDE
+ Compile the sktech and load it on your arduino mega
### Wiring
+ TODO
