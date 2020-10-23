
#include "LoRa.h"
#include <SoftwareSerial.h>
#include <RH_RF95.h>
#include <Wire.h>
SoftwareSerial ss(8, 9);
//Arduino Mini
//SoftwareSerial ss(2, 3); 
RH_RF95 rf95(ss);
LoRa::LoRa()
{
	
}
void LoRa::LoRaInit()
{	
	Serial.begin(115200);
    	//Serial.println("RF95 client test.");

   	while (!rf95.init())
    	{
      	 	//Serial.println("init failed");
		delay(1000);
   	}
   	rf95.setFrequency(868.0);
	//rf95.setTxPower(23, false);
}

void LoRa::LoRaSend(String buff)
{	
	uint8_t data[100];
	bool restart=false;
	buff.getBytes(data,sizeof(data));
   	if(rf95.send(data, sizeof(data)))
	{   
		//Serial.println("RF95 send data correctly");
    		if (rf95.waitPacketSent(1000))
		{
		//	Serial.println("Packet send it");
			restart=false;
		}
		else 
		{
		//	Serial.println("Packet not send it\n");
			restart=true;
		}

	}
	if(restart)
	{
		while (!rf95.init())
    		{
      	 		//Serial.println("init fail");
			delay(1000);
   		}
   		rf95.setFrequency(868.0);
		//rf95.setTxPower(23, false);
	}
	
}
