#ifndef LORAAPP_H
#define LORAAPP_H

#include <Arduino.h>
#include <LoRa.h>
#include "LoRaSensor.h"
#include "LoRaresource.h"
#include <SoftwareSerial.h>
typedef LoRaResource resource_t;

class LoRaApp {
	public:
		void init(LoRa * comunication,uint16_t myID);
		void sensor_check(void);
		void sensor_send(void);
		void add_resource(LoRaSensor * sensor);
  	        void handler(void);
		void receiver(byte* payload, uint16_t sender, unsigned int length);
  
	private:
		
       		uint8_t len;
		resource_t resources_[10];
		LoRa * com_;
    		char _name[20];
		char buffer[100];
    		uint16_t myID;
		uint8_t rcount;

};
#endif
