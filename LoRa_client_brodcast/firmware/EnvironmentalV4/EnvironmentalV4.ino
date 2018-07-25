
//#include "EEPROM.h"
#include <Arduino.h>
//Include Aplication Libraries
#include <LoRaApp.h>
#include <TheLoRaSensors.h>
#include <LoRa.h>
#include <Wire.h>

//Aplication object
//Chose one id for each device
uint16_t myID = 0x07;
LoRaApp ap;
LoRa * comunication = new LoRa();

//Runs only once
void setup() {
  Wire.begin();
  // init aplication service

  ap.init(comunication,myID);
  comunication->LoRaInit();
  add_sensors();
  //Necesary delay for power meter
  //delay(10000);
  delay(1000);
}

void loop() {
  ap.handler();
  // comunication->LoRaSend();
 // delay(1000);
}

void add_sensors() {

  //The temperature object of TH02 must be the first on be added it:
  //Because the sensor must be power off befor the other digital sensors can be adde it and by defeault is on.
  //On the first parameter name the sensor value as you will receive and on the second introduce the digital pin for
  //Power On/Off the THI02 sensor
  //Temperature object
  TemDigital* tem = new TemDigital("temp", 5);
  ap.add_resource(tem);
  //Humidity object
  HumDigital* hum = new HumDigital("humid");
  ap.add_resource(hum);
  //  Light object
  LightDigital* light = new LightDigital("light");
  ap.add_resource(light);
  //Sound object
  CG306SoundSensor* sound = new CG306SoundSensor("sound", A2);
  ap.add_resource(sound);
  //SparkfunSoundSensor* sound2 = new SparkfunSoundSensor("light", A0);
  //ap.add_resource(sound2);
  pirSensor* pir = new pirSensor("pir", 4);
  ap.add_resource(pir);
    

}

