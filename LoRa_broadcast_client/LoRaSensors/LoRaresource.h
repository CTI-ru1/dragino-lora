

#ifndef LORARESOURCE_H
#define LORARESOURCE_H

#include <Arduino.h>
#include "LoRaSensor.h"

class LoRaResource {
public:

    LoRaResource();
    /**
     * Resource Constructor. User to pass as parameters all required fields: 
     *
     */
    LoRaResource(LoRaSensor * sensor);
    /**
     * Called when a resouce is to be evaluated again.
     */
   
    char * name();
    /**
     * Length of the resource's name.
     */
    uint8_t name_length();
    /**
     * Check if the resource allows the method.
     */
    void get_value( char* output_data, uint8_t& output_data_len); 	
    bool periodic_check();
    void check_and_send();	

private:
 
    LoRaSensor *_sensor;
    
};
#endif

