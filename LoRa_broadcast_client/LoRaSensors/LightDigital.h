#include <LoRaSensor.h>
#include <math.h>
#include <Wire.h>
#include <Digital_Light_TSL2561.h>
#ifndef LIGHTDIGITAL
#define LIGHTDIGITAL 

class LightDigital : 
public LoRaSensor 
{
 public:
  int pin, status;
  	
  LightDigital(char * name):
  LoRaSensor(name)
  {	
    TSL2561.init();	
    this->status = 0; 
    this->set_name(name);
    this->send=0;
    delay(15);				  
  }
   void get_value(char* output_data, uint8_t& output_data_len)
  {
	char* sensorname=get_name();
	
    	output_data_len = sprintf( (char*)output_data, "%s,%d",sensorname, this->status); 

  }
 
  bool periodic_check(){
	send=0;
	return send;
  }
  void check_and_send(void)
  {
		//Serial.print("Checking light:");
		delay(150);
		signed long light=TSL2561.readVisibleLux();
		delay(150);
		//Serial.println(light);
		this->status=(int)light;
	    	
  }	
};
#endif
