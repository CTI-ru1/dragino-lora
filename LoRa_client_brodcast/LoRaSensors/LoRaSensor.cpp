
#include "LoRaSensor.h"

char * LoRaSensor::get_name() {
    return name;
}
void LoRaSensor::set_name(char * name) {
    strcpy(this->name, name);
}
bool LoRaSensor::periodic_check(void){
}
void LoRaSensor::check_and_send(void){
}

int LoRaSensor::get_status() {
    return status;
}

void LoRaSensor::get_value( char* output_data, uint8_t& output_data_len)
{
}
