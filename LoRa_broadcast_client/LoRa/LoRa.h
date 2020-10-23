
#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <RH_RF95.h>
#include <Wire.h>

class LoRa {

public:
	LoRa();
	void LoRaInit();
	void LoRaSend(String buff);
	
};

#endif
