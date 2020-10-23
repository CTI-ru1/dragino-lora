
#include "LoRaresource.h"

LoRaResource::LoRaResource() {
}

LoRaResource::LoRaResource(LoRaSensor * sensor) {
    _sensor = sensor;
  
}

bool LoRaResource::periodic_check(){
	_sensor->periodic_check();
}
void LoRaResource::check_and_send(){
	_sensor->check_and_send();
}
void LoRaResource::get_value( char* output_data, uint8_t& output_data_len) {
    _sensor->get_value( output_data, output_data_len);
}

char * LoRaResource::name() {
    return _sensor->get_name();
}

uint8_t LoRaResource::name_length() {
    return strlen(_sensor->get_name());
}


