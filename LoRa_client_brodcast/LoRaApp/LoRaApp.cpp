#include <LoRaApp.h>
#include <LoRaSensor.h>
#include <inttypes.h>
#include <SoftwareSerial.h>
static unsigned long timestamp = millis();
void LoRaApp::init(LoRa * comunication,uint16_t myID ) {
    this->myID=myID;
    this->rcount = 0;
    com_=comunication;	
}

void LoRaApp::handler() {
	
	//sensor_check();
	sensor_send();
}

void LoRaApp::receiver(byte* payload, uint16_t sender, unsigned int length){

}
void LoRaApp::sensor_check(void) {
	        
 	uint8_t i;
        for (i = 0; i < rcount; i++) {
        	if(resources_[i].periodic_check())
		{
			char buffer2[50]="1";
			resources_[i].get_value(buffer,len);
			String buf=String(buffer2)+String(buffer);
			com_->LoRaSend(buf);
		    	memset(buffer, 0, sizeof(buffer));
		}
			
    	}		
}
void LoRaApp::sensor_send(void) {
	       
	uint8_t i; 
	String buf;
	buf=String(myID)+"/";
	
	if(millis() - timestamp > 30000)
	{			
		for (i = 0; i < this->rcount; i++) {
 
			resources_[i].check_and_send();
			resources_[i].get_value(buffer,len);	
			buf=buf+String(buffer)+"+";	
		
   		 }
		
		com_->LoRaSend(buf);
		timestamp = millis();
		memset(buffer, 0, sizeof(buffer));
	 }		
}

void LoRaApp::add_resource(LoRaSensor * sensor) {
    resources_[this->rcount++] = resource_t(sensor);
}

