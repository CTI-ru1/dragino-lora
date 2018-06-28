#include <LoRaSensor.h>
#include "Arduino.h"

class CG306SoundSensor: 
public LoRaSensor 
{
public:
int pin;
int status;
int numberOfSample;
int value;
int maximum;
unsigned long timestamp;
CG306SoundSensor(char * name, int pin): 
  LoRaSensor(name)
  {
    this->pin = pin;
    this->status = 0;

    this->numberOfSample=1024;
    this->set_name(name);
    this->maximum=0; 
  
    this->value=0;
    this->send=0;
    this->timestamp=millis();		
  
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
	if((millis() -this->timestamp) > 50)
	{
		
		for(int i = 0; i < numberOfSample ; i++){				

			value=analogRead(this->pin);
			if (value>this->maximum){this->maximum=value;}
           
  		}
		this->status=this->maximum;
		
		int newStatus = (int)(20*log10((this->maximum/1023.0*5.0))+85.0);
		if (newStatus==0)
		{
			this->status=32;
		}
		else if (newStatus<120)
		{
			this->status=newStatus;
		}
		else
		{		
			this->status=32;///if doesn not work put 32 db
		}		
		this->timestamp = millis();
	} 
	this->maximum = 0;
  }

};

