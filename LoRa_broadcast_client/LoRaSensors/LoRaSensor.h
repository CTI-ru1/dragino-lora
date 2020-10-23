
#define CONF_RESOURCE_NAME_SIZE 8
#ifndef LORASENSOR_H
#define LORASENSOR_H
#include <Arduino.h>
class LoRaSensor {
public:

    LoRaSensor() {
        this->changed = false;
    }

    LoRaSensor(char * name) {
       this->changed = false;
    }
    
    
    char* get_name();
    void set_name(char * name);
    virtual void get_value( char* output_data, uint8_t& output_data_len);	
    virtual bool periodic_check(void);
    virtual void check_and_send(void);		
    int get_status();
    

private:
    char name[CONF_RESOURCE_NAME_SIZE];

protected:
    int status;
    float statusf;	
    int pin;
    bool changed;
    bool send;
			
};

#endif
