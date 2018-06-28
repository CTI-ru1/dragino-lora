#include <LoRaSensor.h>

class pirSensor : 
public LoRaSensor 
{
public:
  int pin, status;
  pirSensor(char * name, int pin): 
  LoRaSensor(name)
  {
    this->pin = pin;
    this->status =0;
    pinMode(pin, INPUT);
    this->set_name(name);
    this->send=0;	
  }
  void get_value(char* output_data, uint8_t& output_data_len)
  {
	char* sensorname=get_name();
    	output_data_len = sprintf( (char*)output_data, "%s,%d",sensorname, this->status); 
  }

  bool periodic_check(void)
  {
	send=0;
	return send;
  } 			
  void check_and_send(void)
  {
       int newStatus = digitalRead(this->pin); // read the value from the sensor
       if(newStatus != this->status)
       {
        	this->status = newStatus;
	
       }
  }	
};
