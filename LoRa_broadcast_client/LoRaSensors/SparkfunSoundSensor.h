#include <LoRaSensor.h>
#include "Arduino.h"
#ifndef SPARKFUNSOUNDSENSOR
#define SPARKFUNSOUNDSENSOR

class SparkfunSoundSensor: 
public LoRaSensor 
{
public:
int pin;
float status;
int value;
float decibelsValueQuiet;
float decibelsValueMedium;
float decibelsValueLoud;
float dBAnalogQuiet; // calibrated value from analog input (48 dB equivalent)
float dBAnalogMedium;
float dBAnalogLoud;
SparkfunSoundSensor(char * name, int pin): 
  LoRaSensor(name)
  {
    this->pin = pin;
    this->status = 0;
    this->set_name(name);
  
    this->dBAnalogQuiet = 10; // calibrated value from analog input (48 dB equivalent)
    this->dBAnalogMedium = 12;
    this->dBAnalogLoud = 17;
    this->value=0;
    this->send=0;
 	
  
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
	this->decibelsValueQuiet = 49;
   	this->decibelsValueMedium = 65;
    	this->decibelsValueLoud = 70;  
	this->value=analogRead(this->pin);
	
  	if (this->value < 13)
  	{
    		this->decibelsValueQuiet += 20 * log10(this->value/this->dBAnalogQuiet);
    		Serial.print("Quiet :)"); 
    		Serial.println(this->decibelsValueQuiet);
		this->status=this->decibelsValueQuiet;
    	}
  	else if ((this->value > 13) && ( this->value <= 23) )
  	{
    		this->decibelsValueMedium += log10(this->value/this->dBAnalogMedium);
    		Serial.print("Medium");
    		Serial.println(this->decibelsValueMedium);
		this->status=this->decibelsValueMedium;
  	}
  	else if(this->value > 22)
  	{
    		this->decibelsValueLoud += log10(this->value/this->dBAnalogLoud);
    		Serial.print("Loud :(");
    		Serial.println(this->decibelsValueLoud);
		this->status=this->decibelsValueLoud;
  	}
	//delay(500);	
   }		

};
#endif
