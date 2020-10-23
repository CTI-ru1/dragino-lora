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
#include <SoftwareSerial.h>
#include <SPI.h>
#include <RH_RF95_grove.h>
#include <stdio.h>
#include <Wire.h>
#include <avr/wdt.h>


SoftwareSerial ss(9, 8);
RH_RF95 rf95(ss);

//Define the timeout to re-start to listen the broadcast info from server to establish network.
//Default: 10 minutes
#define TIMEOUT 600000

//Define the LoRa frequency use for this client
float frequency = 869.5;

// Client ID address in EEPROM.
#define BAUDRATE 115200

int sent_count = 0;//Client send count, increase after sent data.
int client_id = 0xc;

// To resetart the network connection if does not receive data from the gw
int rec_data = 0;


uint16_t crcdata = 0; // crc data
uint16_t recCRCData = 0; //
int detected = 0; // check if detected server's broadcast message
int flag = 0; //
long start = 0;
long total_time = 0;//check how long doesn't receive a server message
unsigned long Soundtimestamp = millis();
int maximum = 0;




//Select gw
uint8_t gw = 1;
void setup()
{
  Serial1.begin(9600);
  //Enable the watchdog
  wdt_enable(WDTO_8S);
  Wire.begin();
  while (!rf95.init())
  {
    Serial.println("init failed");
    delay(1000);
  }
  wdt_reset();
  //Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  // rf95.setSpreadingFactor(8);
  wdt_reset();
  Serial.println("Ready setup");


  wdt_reset();
  delay(1000);
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
        {
          Serial.println("Flag 0");
          flag = 0;
        }

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
        sent_count++;
        char  data[80] = {0};//data to be sent
        char  values[80] = {0};//data to be sent
        data[0] =  'D';
        data[1] =  'S';
        data[2] = gw;
        data[3] = client_id;//put client ID
        data[4] = ':';

        // Send my data
        String bufst = " ";
        bufst = (String) client_id;

        read_sensors(& bufst);

        bufst.getBytes(values, bufst.length() + 1);
        for (int i = 0; i < strlen(values); i++)
        {
          data[i + 5] = values[i];
        }

        int dataLength = strlen(data);//get data length

        uint16_t crcData = CRC16((unsigned char*)data, dataLength); //calculate CRC
        wdt_reset();
        unsigned char sendBuf[60] = {0};
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
        memset(data, 0, sizeof(bufst));
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
        delay(1000);  char inByte = (char)Serial1.read();
      }
      wdt_reset();
    }
    Serial.println("Listen the broadcast");
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
      total_time += millis( ) - start; //get total time out

      if (total_time > TIMEOUT)
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
  }

  delay(100);
}
void read_sensors(String * message) {

  wdt_reset();
  //char sensorvalue[10]={0};
  //Serial.println("Before Send Letters");
  // Serial.println("Send A");
  // delay(10);
  String content = "";
  Serial1.write('A');
  delay(100);
  while (Serial1.available()) {
    int inByte = Serial1.read();
    // if (inByte!='\0')
    //{
    content.concat((char)inByte);
    //}
    //  Serial.println(inByte);
  }
  int index = content.indexOf(',');
  String var = content.substring(0, index);
  Serial.println(var);
  *message += "/pm1," + var;
  int index2 = content.indexOf(',', index + 1);
  var = content.substring(index + 1, index2);
  *message += "+pm25," + var;
  Serial.println(var);
  int index3 = content.indexOf(',', index2 + 1);
  var = content.substring(index2 + 1, index3);
  Serial.println(var);
  *message += "+pm10," + var +"+";

 /* int index4 = content.indexOf(',', index3 + 1);
  var = content.substring(index3 + 1, index4);
  Serial.println(var);
  *message += "+0," + var;

  int index5 = content.indexOf(',', index4 + 1);
  var = content.substring(index4 + 1, index5);
  Serial.println(var);
  *message += "+1," + var;

  int index6 = content.indexOf(',', index5 + 1);
  var = content.substring(index5 + 1, index6);
  Serial.println(var);
  *message += "+2," + var;

  int index7 = content.indexOf(',', index6 + 1);
  var = content.substring(index6 + 1, index7);
  Serial.println(var);
  *message += "+3," + var;

  int index8 = content.indexOf(',', index7 + 1);
  var = content.substring(index7 + 1, index8);
  Serial.println(var);
  *message += "+4," + var+"+";

  /*int index9 = content.indexOf(',', index8 + 1);
  var = content.substring(index8 + 1, index9);
  Serial.println(var);
  *message += "+5," + var + "+";*/
  //*message += "/PM1.0,"+var1+"/PM2.5,"+var2+"/PM10,"+var3+"/03um,"+var4+"/05um,"+var5+"/10um,"+var6+"/25um,"+var7+"/50um,"+var8+"/100um,"+var9;

  /*Serial.println(content);
    int index=content.indexOf(',');
    String var1=content.substring(0,index);
    Serial.println(var1);
    int index2=content.indexOf(',',index+1);
    String var2=content.substring(index+1,index2);
    Serial.println(var2);
    int index3=content.indexOf(',',index2+1);
    String var3=content.substring(index2+1,index3);
    Serial.println(var3);

    int index4=content.indexOf(',',index3+1);
    String var4=content.substring(index3+1,index4);
    Serial.println(var4);
    int index5=content.indexOf(',',index4+1);
    String var5=content.substring(index4+1,index5);
    Serial.println(var5);
    int index6=content.indexOf(',',index5+1);
    String var6=content.substring(index5+1,index6);
    Serial.println(var6);

    int index7=content.indexOf(',',index6+1);
    String var7=content.substring(index6+1,index7);
    Serial.println(var7);
    int index8=content.indexOf(',',index7+1);
    String var8=content.substring(index7+1,index8);
    Serial.println(var8);
    int index9=content.indexOf(',',index8+1);
    String var9=content.substring(index8+1,index9);
    Serial.println(var9);
    //*message += "/PM1.0,"+var1+"/PM2.5,"+var2+"/PM10,"+var3+"/03um,"+var4+"/05um,"+var5+"/10um,"+var6+"/25um,"+var7+"/50um,"+var8+"/100um,"+var9;
    message += "/d1,"+var1+"+d2,"+var2+"+d3,"+var3+"+0,"+var4+"+1,"+var5+"+2,"+var6+"+3,"+var7+"+4,"+var8;*/



  //delay(10);
  /*Serial.println("Send B");
    delay(10);
    Serial1.write('B');
    while (Serial1.available()) {
         int inByte = Serial1.read();
         //if (inByte!='\0')
       //  {
            content.concat((char)inByte);
       //  }
         Serial.println(inByte);
    }
    Serial.println(content);
    message += "/PM2.5," + (String)content;

    content = "";
    delay(10);
    Serial.println("Send C");
    delay(10);
    Serial1.write('C');
    while (Serial1.available()) {
         int inByte = Serial1.read();
       //  if (inByte!='\0')
     //    {
            content.concat((char)inByte);
      //   }
         Serial.println(inByte);
    }
    Serial.println(content);
    message += "/PM10," + (String)content;

    /*content = "";
    Serial1.write('D');
    while (Serial1.available()) {
         int inByte = Serial1.read();
          if (inByte!='\0')
         {
            content.concat((char)inByte);
         }
    }
    message += "/03um," + (String)content;
    content = "";
    Serial1.write('E');
    while (Serial1.available()) {
         int inByte = Serial1.read();
         if (inByte!='\0')
         {
            content.concat((char)inByte);
         }
    }
    message += "/05um," + (String)content;

    content = "";
    Serial1.write('F');
    while (Serial1.available()) {
         int inByte = Serial1.read();
         if (inByte!='\0')
         {
            content.concat((char)inByte);
         }
    }
    message += "/10um," + (String)content;

    /* content = "";
    Serial1.write('G');
    while (Serial1.available()) {
         int inByte = Serial1.read();
         content.concat((char)inByte);
    }
    message += "/25um," + (String)content;

    content = "";
    Serial1.write('H');
    while (Serial1.available()) {
         int inByte = Serial1.read();
         content.concat((char)inByte);
    }
    message += "/50um," + (String)content;

    content = "";
    Serial1.write('B');
    while (Serial1.available()) {
         int inByte = Serial1.read();
         content.concat((char)inByte);
    }
    message += "/100um," + (String)content;*/
  /*dtostrf(value_PM[6],3,2,sensorvalue);
    message += "PM2.5," + (String)sensorvalue;
    dtostrf(value_PM[7],3,2,sensorvalue);
    message += "PM10," + (String)sensorvalue;*/
  wdt_reset();

}
