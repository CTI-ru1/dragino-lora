
//#include "EEPROM.h"
#include <Arduino.h>
//Include Aplication Libraries
#include <LoRaApp.h>
#include <TheLoRaSensors.h>
#include <LoRa.h>
#include <Wire.h>
#include <EmonLib.h> 
EnergyMonitor emon1;
EnergyMonitor emon2;
EnergyMonitor emon3;
//Aplication object
//Chose one id for each device
uint16_t myID = 0x01;
LoRaApp ap;
LoRa * comunication = new LoRa();

//Runs only once
void setup() {
  Wire.begin();
  // init aplication service

  ap.init(comunication,myID);
  comunication->LoRaInit();
  add_sensors();
  delay(10000);
}

void loop() {
  ap.handler();
}

void add_sensors() {

  //CUrrent cosumption device
  CurrentSensor * current1 = new CurrentSensor("cur/1",1,emon1);
  ap.add_resource(current1);
  CurrentSensor * current2 = new CurrentSensor("cur/2",2,emon2);
  ap.add_resource(current2);
  CurrentSensor * current3 = new CurrentSensor("cur/3",3,emon3);
  ap.add_resource(current3);

    

}

