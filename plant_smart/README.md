# Smart Plant
This is a sample sketch for a arduino Uno with a LCD screen. It measures the soil moisture of a plant and waters it if it's too dry.
Moreover it has a light sensor.
Each Plant gets its own water pump, because this is much cheaper than a multi valve with only one pump.

# Dependencies
+ [MySensors](https://github.com/mysensors/MySensors)
+ API.h
+ Sensor.h
+ LCD.h
+ LiquidCrystal_I2C F Malpartida's NewLiquidCrystal library
+ Wire.h
+ DallasTemperature
+ TimerObject
+ [Analog MuxDemux library](https://github.com/ajfisher/arduino-analog-multiplexer)

# Parts
+ Arduino Uno [Aliexpress](https://de.aliexpress.com/wholesale?SearchText=arduino+uno)
+ Water level sensor [Aliexpress](https://de.aliexpress.com/item/Free-shipping-Water-Sensor-for-Arduino-water-droplet-detection-depth-with-demo-code/32280545086.html?spm=2114.010208.3.111.FJ0NCX&ws_ab_test=searchweb0_0,searchweb201602_2_10065_10068_433_434_10136_10137_10138_10060_10062_10141_10056_10055_10054_128_301_10059_10531_10099_10530_10103_10102_10101_10096_10052_10144_10053_10050_10107_10142_10051_10106_10143_10526_10529_10084_10083_10080_10082_10081_10110_10111_10112_10113_10114_10033_10078_10079_10073_10070_10122_10123_10124-10531,searchweb201603_8,afswitch_1,ppcSwitch_7,single_sort_0_default&btsid=5c08027c-050d-4e92-b01a-0bca541a6ab6&algo_expid=43b5250b-0382-4a77-8853-7a243b3f6b38-12&algo_pvid=43b5250b-0382-4a77-8853-7a243b3f6b38)
+ LCD display [Aliexpress](https://de.aliexpress.com/wholesale?SearchText=1602+lcd)
+ Soil moisture Sensor [Aliexpress](https://www.aliexpress.com/item/Hot-Sale-10set-Soil-Moisture-Hygrometer-Detection-Humidity-Sensor-Module-For-arduino-Development-Board-DIY-Robot/32328197208.html?spm=2114.01010208.3.18.s9Bfhz&ws_ab_test=searchweb0_0,searchweb201602_1_10152_10065_10151_10068_5010016_10136_10137_10060_10138_10155_10062_437_10154_10056_10055_10054_10059_303_100031_10099_10103_10102_10101_10096_10052_10053_10107_10050_10142_10051_5030017_10084_10083_10080_10082_10081_10177_10110_519_10111_10112_10113_10114_10180_10182_10184_10078_10079_10073_10186_10123_10189_142-10050,searchweb201603_1,ppcSwitch_2&btsid=284a8601-bfcf-4f8f-9ac0-7b6c1fb0f679&algo_expid=7181809f-3f84-42d7-838f-3be78b672e23-2&algo_pvid=7181809f-3f84-42d7-838f-3be78b672e23) 
+ Water pump [Aliexpress](https://www.aliexpress.com/item/5-x-DC-3v-6v-Mini-Micro-Submersible-Water-Pump-Low-Noise-Motor-pump-120L-H/32689942954.html?spm=2114.01010208.3.2.BjdFdP&ws_ab_test=searchweb0_0,searchweb201602_1_10152_10065_10151_10068_5010015_10136_10137_10060_10138_10155_10062_437_10154_10056_10055_10054_10059_303_100031_10099_10103_10102_10101_10096_10052_10053_10107_10050_10142_10051_5030017_10084_10083_10080_10082_10081_10177_10110_519_10111_10112_10113_10114_10180_10182_10184_10078_10079_10073_10186_10123_10189_142,searchweb201603_1,ppcSwitch_2&btsid=a47a037d-c0b3-4006-9f54-0a25df28eb6b&algo_expid=b6e73170-9d9f-407a-ac4c-c80382830cb2-0&algo_pvid=b6e73170-9d9f-407a-ac4c-c80382830cb2)
+ Light sensor [Aliexpress](https://www.aliexpress.com/item/Optical-Sensitive-Resistance-Sensor-Module-Light-Intensity-Detect-Photosensitive-Sensor-for-Arduino/32677701042.html?spm=2114.01010208.3.73.5hdoD5&ws_ab_test=searchweb0_0,searchweb201602_1_10152_10065_10151_10068_5010016_10136_10137_10060_10138_10155_10062_437_10154_10056_10055_10054_10059_303_100031_10099_10103_10102_10101_10096_10052_10053_10107_10050_10142_10051_5030017_10084_10083_10080_10082_10081_10177_10110_519_10111_10112_10113_10114_10180_10182_10184_10078_10079_10073_10186_10123_10189_142,searchweb201603_1,ppcSwitch_2&btsid=4a6b6fa6-d962-431f-86eb-516733b35532&algo_expid=d455411e-4666-42cc-b572-187cc14803e5-12&algo_pvid=d455411e-4666-42cc-b572-187cc14803e5)
+ Power supply

# Installation
### Software
+ Install the dependent libraries
+ Open this sketch in the Arduino IDE
+ Compile the sktech and load it on your arduino mega

### Wiring
+ TODO

### Images
In Homeassistant: ![homeassistant]()
https://home-assistant.io/components/plant/
