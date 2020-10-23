#ifndef CURRENT_SENSOR
#define CURRENT_SENSOR

#include <EmonLib.h>                   // Include EmonLib Library
#include <LoRaSensor.h>
class CurrentSensor:
public LoRaSensor
{
public:

 // unsigned long timestamp;
  EnergyMonitor em; 
 // float status;	
  CurrentSensor (char * name,  int pin, EnergyMonitor emon)
  {    
    this->pin=pin;	
    this->statusf=0.00;
    this->set_name(name);
    this->send=0;	
    pinMode(this->pin,INPUT);	
    //timestamp = 0;
    this->em=emon;
    this->em.current(this->pin, 30);      // Current: input pin, calibration.	
  }

  void get_value(char* output_data, uint8_t& output_data_len)
  {

	char* sensorname=get_name();
	char sensorvalue[6];
	dtostrf(this->statusf,4,2,sensorvalue);
    	output_data_len = sprintf( (char*)output_data, "%s,%s",sensorname, sensorvalue); 

  }
  bool periodic_check(){
	send=0;
	return send;
  }
  void check_and_send(void)
  {
		//timestamp = millis();
		double val=0;
		this->statusf = (float)this->em.calcIrms(1480);  // Calculate Irms only
		
		
		float value=(float)(val*1000);
		//this->status=val;
		//Serial.println(value);
		/*if (value<100)
		{
			this->status=0;
		}
		else
		{
			this->status=value;		
		}*/
		Serial.print(this->get_name());
		Serial.print(":");
		Serial.println(this->statusf);
		delay(1000);
  }
};
#endif
