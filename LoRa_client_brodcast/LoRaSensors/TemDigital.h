#include <LoRaSensor.h>
#include <math.h>
#include <Wire.h>
#include <TH02_dev.h>
#ifndef TEMDIGITAL
#define TEMDIGITAL 

class TemDigital: public LoRaSensor 
{
 public:
  int pin;
  float status;	
  TemDigital(char * name, int pin):
  LoRaSensor(name)
  {
    	
    this->pin = pin;
    this->status = 0.00; 
    this->set_name(name);
    this->send=0;
    TH02.begin(this->pin);
    delay(15);		 
  }
   void get_value(char* output_data, uint8_t& output_data_len)
  {
	char* sensorname=get_name();
	char sensorvalue[6];
	dtostrf(this->status,4,2,sensorvalue);
    	output_data_len = sprintf( (char*)output_data, "%s,%s",sensorname, sensorvalue); 

  }

  bool periodic_check(){
	send=0;
	return send;
  }
  void check_and_send(void)
  {
		//Serial.print("Checking temperature:");
		TH02.PowerOn();
		delay(150); 
		float tem=TH02.ReadTemperature();
		TH02.PowerOff();
		delay(150);
		//Serial.println(this->status);
		this->status=tem;
	    	
  }	
};
#endif
