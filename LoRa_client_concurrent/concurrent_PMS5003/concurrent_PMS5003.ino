/*
  example: concurrent_client
  Support Devices:
    1)LoRa Shield + Arduino
    2)LoRa GPS Shield + Arduino

  Please use write_client_id to set different ID to clients before using this sketch. otherwise, there will
  be conflict in the server side.

  How it works:
  This concurrent client sketch is working together with the concurrent gateway sketch. Before using this sketch, please use the write_client_id sketch to write a client ID in the EEPROM.
  Client ID is the unique id for each client in the LoRa network. write_gateway_id to gateway is not necessary, if not write, gateway id will be 0XFF.
    When the client boot, it will keep listening for the broadcast message from LoRa Gateway.
   When the gateway sketch boot, it will broadcast a message to set up a LoRa Network. If it gets broadcast message, client will send a join request message to gateway,
    when the join request message arrive to gateway, the gateway will send back a join-ack message with client id and add this client to the LoRa Network.
   If the client gets its join-ack message for its join request, it will enter the mode to listen the data-request message from gateway. In this mode,
    if client get a data-request message for this client it will send back a data message to the gateway.
   After client in data_request listening mode, if it has not receive any message from gateway in a timeout, it will go back to the network set up mode to listen the broadcast message.
   Gateway will refresh the LoRa network periodically for adding new client or remove unreachable client.
  This example using the polling method between LoRa node and Gateway, it will minimize the LoRa packets transfer on the air and avoid congestion. It is suitable for a not real time LoRa work.

  Performance test in a room with 100 nodes and 1 gateway shows:
  a) Gateway require about 1.5 minutes to set up this 100 nodes Network
  b) Gateway takes about 2 minutes to do polling for these 100 nodes.

  User can adjust the timing in the sketch from case by case.

  modified 24 Dec 2016
  by Dragino Tech <support@dragino.com>
  Dragino Technology Co., Limited
*/
#ifndef READVCC_CALIBRATION_CONST
#define READVCC_CALIBRATION_CONST 1126400L
#endif

#include <SoftwareSerial.h>
#include <SPI.h>
#include <RH_RF95_grove.h>
//#include <stdio.h>
//#include <Wire.h>
#include <avr/wdt.h>

#include "Seeed_HM330X.h"

SoftwareSerial ss(9, 8);
RH_RF95 rf95(ss);

//Define the timeout to re-start to listen the broadcast info from server to establish network.
//Default: 10 minutes
#define TIMEOUT 300000

//Define the LoRa frequency use for this client
float frequency = 868.3;

// Client ID address in EEPROM.
#define BAUDRATE 115200

int sent_count = 0;//Client send count, increase after sent data.
int client_id = 0x08;

// To resetart the network connection if does not receive data from the gw
int rec_data = 0;


uint16_t crcdata = 0; // crc data
uint16_t recCRCData = 0; //
int detected = 0; // check if detected server's broadcast message
int flag = 0; //
long start = 0;
long total_time = 0;//check how long doesn't receive a server message
//unsigned long Soundtimestamp = millis();
//int maximum = 0;


//PM Sensor


//Select gw
uint8_t gw = 5;


char bufst[60] = {0};

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
struct pms5003data datapm;

long readVcc() {
    long result;
  
    //not used on emonTx V3 - as Vcc is always 3.3V - eliminates bandgap error and need for calibration http://harizanov.com/2013/09/thoughts-on-avr-adc-accuracy/
  
    #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    #elif defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    #elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_AT90USB1286__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    ADCSRB &= ~_BV(MUX5);   // Without this the function always returns -1 on the ATmega2560 http://openenergymonitor.org/emon/node/2253#comment-11432
    #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
    #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  
    #endif
    #if defined(__AVR__)
    delay(2);                                        // Wait for Vref to settle
    ADCSRA |= _BV(ADSC);                             // Convert
    while (bit_is_set(ADCSRA,ADSC));
    result = ADCL;
    result |= ADCH<<8;
    result = READVCC_CALIBRATION_CONST / result;  //1100mV*1024 ADC steps http://openenergymonitor.org/emon/node/1186
    return result;
    #elif defined(__arm__)
    return (3300);                                  //Arduino Due
    #else
    return (3300);                                  //Guess that other un-supported architectures will be running a 3.3V!
    #endif
}


void setup()
{
  delay(10000);
  //Enable the watchdog
  wdt_enable(WDTO_8S);
  Wire.begin();
  while (!rf95.init())
  {
    Serial.println("init failed");
    delay(1000);
  }

  //Serial1.begin(9600);
  wdt_reset();
  //Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  // rf95.setSpreadingFactor(8);
  wdt_reset();
  Serial.println("Ready setup");

  //wdt_reset(); 
  //delay(1000);
}

uint16_t calcByte(uint16_t crc, uint8_t b)
{
  uint32_t i;
  crc = crc ^ (uint32_t)b << 8;
  for ( i = 0; i < 8; i++)
  {
    if ((crc & 0x8000) == 0x8000)
      crc = crc << 1 ^ 0x1021;
    else
      crc = crc << 1;
  }
  return crc & 0xffff;
}

uint16_t CRC16(uint8_t *pBuffer, uint32_t length)
{
  uint16_t wCRC16 = 0;
  uint32_t i;
  if (( pBuffer == 0 ) || ( length == 0 ))
  {
    return 0;
  }
  for ( i = 0; i < length; i++)
  {
    wCRC16 = calcByte(wCRC16, pBuffer[i]);
  }
  return wCRC16;
}

uint16_t recdata( unsigned char* recbuf, int Length)
{
  crcdata = CRC16(recbuf, Length - 2); //Calculate the CRC for the received message
  recCRCData = recbuf[Length - 1]; //get the CRC high byte
  recCRCData = recCRCData << 8; // get the CRC low byte
  recCRCData |= recbuf[Length - 2]; //get the receive CRC
}

void listen_server(void)
{
  if (rf95.waitAvailableTimeout(500))
  {
    wdt_reset();
    Serial.println("Get Message at listen server ");
    rec_data = 1;
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];//buffer to store the server response message
    uint8_t len = sizeof(buf);// data length
    if (rf95.recv(buf, &len))//check if receive data is correct
    {
      wdt_reset();
      Serial.println("waiting for broadcast message");
      Serial.println(buf[0]);
      Serial.println(buf[1]);
      Serial.println(buf[2]);
      Serial.println(buf[3]);
      if (buf[0] == 'B' && buf[1] == 'C' && buf[2] == gw && buf[3] == ':'  ) //Get Broadcast message from Server, send a join request
      {
        wdt_reset();
        int delay_ms = 0;
        delay_ms = random(100, 500); //generate a random delay. this is to avoid the channel congestion.This may happen when all clients start
        //to send the join message to the server at the same time after get the broadcast message
        delay(delay_ms);

        uint8_t join[4] = {0}; // Construct a join message
        join[0] = 'J';
        join[1] = 'R';
        join[2] = gw;
        join[3] = client_id;// Put Client ID
        Serial.print("Send a Join Request Message, Client ID is:");
        Serial.println(join[3]);
        int length = sizeof(join);//get data length
        rf95.send(join, sizeof(join));// Send a Join Message
        wdt_reset();
        if (rf95.waitPacketSent())
        {
          Serial.println("Flag 1");
          flag = 1;
        }
        else
        {//If do not send the the JR correct we reboot
          Serial.println("Flag 0");
          while(1)
          {
            
          }
          flag = 0;
        }
        wdt_reset();
      }
      
      if (flag == 1)
      {
        Serial.println("send Join request,waiting for Join ACK");
        Serial.println(buf[0]);
        Serial.println(buf[1]);
        Serial.println(buf[2]);
        Serial.println(buf[3]);
        if (buf[0] == 'J' && buf[1] == 'A'  && buf[2] == gw && buf[3] == client_id) //successful if get join ACK, otherwise, listen for broadcast again.
        {
          wdt_reset();
          Serial.println("Get Join ACK, Join successfulm entering polling mode");
          detected = 1;
          flag = 0;
        }
      }
    }
  }
  delay(100);
}

void polling_detect(void)
{
  // detect if there is timeout to get response from server.
  if (rf95.waitAvailableTimeout(3000))//check if there is polling request
  {
    wdt_reset();
    rec_data = 1;
    int rssi=rf95.lastRssi();
    Serial.println("Get Message at poling detect ");
    start = millis( );
    Serial.print("lenght message:");
    Serial.println(RH_RF95_MAX_MESSAGE_LEN);
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];//get message
    uint8_t len = sizeof(buf);//get message length
    if (rf95.recv(buf, &len))//check if receive message is correct
    {
      wdt_reset();
      Serial.println(buf[0]);
      Serial.println(buf[1]);
      Serial.println(buf[2]);
      Serial.println(buf[3]);

      if (buf[0] == 'D' && buf[1] == 'R' && buf[2] == gw && buf[3] == client_id ) //check if we receive a data request message
      {
        start = millis( );
        rec_data = 1;
   
        int rssi=rf95.lastRssi();
        
       // sent_count++;
        char  data[70] = {0};//data to be sent
        char  values[75] = {0};//data to be sent
        data[0] =  'D';
        data[1] =  'S';
        data[2] = gw;
        data[3] = client_id;//put client ID
        data[4] = ':';

        // Send my data
        read_sensors( client_id, rssi);

        
        for (int i = 0; i < strlen(bufst); i++)
        {
          data[i + 5] = bufst[i];
        }

        int dataLength = strlen(data);//get data length

        uint16_t crcData = CRC16((unsigned char*)data, dataLength); //calculate CRC
        wdt_reset();
        unsigned char sendBuf[70] = {0};
        strcpy((char*)sendBuf, data); //copy data to sendbuf

        sendBuf[dataLength] = (unsigned char)crcData;

        sendBuf[dataLength + 1] = (unsigned char)(crcData >> 8);

        int length = strlen((char*)sendBuf);//get data length
        Serial.print("Sending data:");
        for (int i = 0; i < 50; i++)
        {
          Serial.print(sendBuf[i]);
        }
        Serial.println("");
        rf95.send(sendBuf, strlen((char*)sendBuf));//send message
        wdt_reset();
        Serial.print("Data sendingg:");
        Serial.println((char *)sendBuf);
        if (rf95.waitPacketSent())
        {
          detected = 1;
          Serial.println("Detected 1");
        }
        wdt_reset();
        total_time = 0;
        //clean memory
        memset(data, 0, sizeof(data));
        memset(bufst, 0, sizeof(bufst));
        memset(sendBuf, 0, sizeof(sendBuf));

      }
      else
      {
        //Check how long we have not received a data request.Client will enter into listening mode if timeout
        Serial.println("Get message, but not data request message");
      }
    }
    else
    {
      Serial.println("No reply, is rf95_server running?");//didn't get a LoRa message.
    }
  }
}

void loop()
{

  //Enable the watchdog
  wdt_enable(WDTO_8S);
  if (detected == 0) //has not joined to the LoRa Network, listen the broadcast.
  {
    if (rec_data == 0)
    {
      wdt_reset();
      while (!rf95.init())
      {
        Serial.println("init failed");
        delay(1000);
      }
      wdt_reset();
    }
    Serial.println("Listen the broadcast");
    start=millis();
    //detect if there is server broadcast package and join the LoRa Network
    listen_server();
    wdt_reset();
  }
  else
  {
    rec_data = 0;

    polling_detect();
    wdt_reset();
    if (rec_data == 0)
    {
      //total_time += millis( ) - start; //get total time out
     //  total_time += millis( );
      //Serial.print("Total time:");
      //Serial.println(total_time);
      Serial.print("Time:");
      Serial.println(millis()-start);
      if (millis()-start > TIMEOUT)
      //if (total_time-start > TIMEOUT)
      {
        detected = 0;
        total_time = 0;
        wdt_reset();
        Serial.println("polling listening time out, listening network set up again");
        while (!rf95.init())
        {
          Serial.println("init failed");
          delay(1000);
        }
        wdt_reset();
      }

    }
    delay(100);
  }

  delay(100);
}

void read_sensors(int id,int rssi) {

  wdt_reset();
  Serial1.begin(9600);
  delay(1000);
  if( readPMSdata(&Serial1))
  {
    delay(100);
    //long vcc=readVcc();
   // int snr = rf95.lastSNR();   

  }
  else
  {
     Serial.println("wrong read delay before new read ..");
    delay(1000);
  }
      sprintf(bufst,"%d/pm1,%u+pm25,%u+pm10,%u+",id,datapm.pm10_standard,datapm.pm25_standard,datapm.pm100_standard);
    Serial1.end();
  delay(1000);
  wdt_reset();

}

boolean readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }
  
  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }
    
  uint8_t buffer[32];    
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i=0; i<30; i++) {
    sum += buffer[i];
  }

  /* debugging
  for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
  }
  Serial.println();
  */
  
  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i=0; i<15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }

  // put it into a nice struct :)
  memcpy((void *)&datapm, (void *)buffer_u16, 30);

 if (sum != datapm.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}
