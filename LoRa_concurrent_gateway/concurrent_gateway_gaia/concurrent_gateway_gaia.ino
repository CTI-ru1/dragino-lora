/*
  example: concurrent_gateway
  Support Devices: 
    1)LG01

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
    
  modified 24 Dec 2016
  by Dragino Tech <support@dragino.com>
  Dragino Technology Co., Limited
*/
#include <SPI.h>
#include <RH_RF95.h>
#include <Console.h>
#include <Process.h>
#include <EEPROM.h>
//#include "encrypt.h"

RH_RF95 rf95;
//ENCRYPT  encrypt_decrypt;

//Define the LoRa frequency use for this client
float frequency = 868.0;

//MAX Clients support, more clients will increase time to establish network and polling
#define MAX_CLIENT 100

//Time to refresh the network. Will add new clients after refresh network
#define REFRESH_TIME 500000

//The no response retry count before stop to broadcast message and finish network setup.  
#define MAX_BROADCAST_RETRY 15

//How Many Retry to kick a client if no response for a data request command. 
#define MAX_REMOVE_RETRY 3 // 

#define BAUDRATE 115200 

// Client ID address in EEPROM.
#define ID_ADDRESS 0x00
String sensor_data = "";

char clients[MAX_CLIENT] = {0}; //store the clients's ID 
int client_numbers = 0;// Client Numbers has been stored 
int broadcast_retry = 0 ;//counter for broadcast message on no response. 
int network_setup_finish = 0 ;//flag to indicate network set up is finished. 
int b = 0;// network set up count. 
long poll_start_time =0;//
long total_poll_time = 0;//
int no_response_count = 0;//counter for no response from Client. 
int gateway_id = 0;
int CrcFlag = 0;


String hostname;
uint8_t buf[80] = {0};//
void setup() 
{    
     Bridge.begin(BAUDRATE);
     //Console.begin();
     if (!rf95.init())
      Console.println("init failed");
     //Setup ISM frequency
     rf95.setFrequency(frequency);
     // Setup Power,dBm
     rf95.setTxPower(13);  
     gateway_id = EEPROM.read(ID_ADDRESS);//Get gateway id.  

     
     //Take the hostname from the Gateway
      String context;
      Process p;
      p.begin("cat");
      p.addParameter("/tmp/hosts/dhcp");
      p.run();
      while (p.available() > 0) {
        char c = p.read();
        context.concat(c);  
      } 
      int n=context.lastIndexOf(' ');
      hostname=context.substring(n+1,context.length()-1);
      Console.print("Hostname:");
      Console.println(hostname);
     Console.flush(); 
}

uint16_t calcByte(uint16_t crc, uint8_t b)
{
  uint32_t i = 0;
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

uint16_t CRC16(uint8_t *pBuffer,uint32_t length)
{
    uint16_t wCRC16 = 0;
    uint32_t i = 0;
    if (( pBuffer==0 )||( length==0 ))
    {
      return 0;
    }
    for ( i = 0; i < length; i++)
    { 
      wCRC16 = calcByte(wCRC16, pBuffer[i]);
    }
    return wCRC16;
}
uint16_t recdata( unsigned char* recbuf,int Length)
{
    uint16_t crcdata = 0;
    uint16_t recCRCData = 0;
    crcdata = CRC16(recbuf,Length-2);//Calculate the CRC for the received message
    recCRCData = recbuf[Length-1];//get the CRC high byte
    recCRCData = recCRCData<<8;// get the CRC low byte
    recCRCData |= recbuf[Length-2];//get the receive CRC
    if(crcdata == recCRCData)
    {
      CrcFlag = 1;
      crcdata = 0;
      recCRCData = 0;
      Console.println("CRC++++"); 
    }
    else
    {
      CrcFlag = 0 ;
      crcdata = 0;
      recCRCData = 0;      
    }
}
  
void set_up_network(void)
{    
         uint8_t broadcast[4] = {0}; //Broadcast Message
         broadcast[0] = 'B';
         broadcast[1] = 'C';
         broadcast[2] = ':';  
         broadcast[3] = gateway_id;       
         int length = sizeof(broadcast);//get broadcast message length
     
         for( b = 0;b < 200;b++)//loop to set up network
         {
            Console.println("send broadcast message"); 
            rf95.send(broadcast , sizeof(broadcast));
            rf95.waitPacketSent();

            if (rf95.waitAvailableTimeout(500))//Check the incoming message
            {  
              Console.println("Get an incoming message"); 
              buf[0]='\0';
              uint8_t len = sizeof(buf);//
              if (rf95.recv(buf, &len))//
              {                 
                if(buf[0] == 'J' && buf[1] == 'R' && buf[2] == ':')//Get a Join request, Store the Client ID
                  {                          
                    if(buf[3] == 0)// Error Message
                     {
                      Serial.println("Error");
                     }
                    else
                     {
                       for(int b = 0;b < 5;b++)//Send 5 Join ACK to the Client,make sure it arrive.  
                       {
                         uint8_t JoinAck[4] = {0}; //Join ACK message
                         JoinAck[0] = 'J';
                         JoinAck[1] = 'A';
                         JoinAck[2] = ':';
                         JoinAck[3] = buf[3];//Put Client ID 
                         int length = sizeof(JoinAck);//Get Data Length
                         Console.print("Send Join ACK to client:");
                         Console.println(buf[3]);                          
                         rf95.send(JoinAck, sizeof(JoinAck));
                         rf95.waitPacketSent(); 
                         broadcast_retry = 0;    
                         delay(500);
                       }                                  
                      int pos ;
                      for( pos=0;pos<sizeof(clients);pos++)//check if this client already stored
                       {
 
                         if(clients[pos] == buf[3])
                         {
                            Console.println("client already store");
                            break ;
                         }
                       }
                       if(pos == sizeof(clients))
                       {
                          clients[client_numbers] = buf[3];//store the client id.  
                          client_numbers++;
                          
                          Console.print("New client id:");
                          Console.println(buf[3]);
                          Console.print("Client numbers:");
                          Console.println(client_numbers);
                          broadcast_retry = 0; //get a new client , reset retry count
                        }
                     }    
                 }    
              }
           }
          else
          {
            broadcast_retry++;
            if(broadcast_retry > MAX_BROADCAST_RETRY)//check if retry count exceed the max retry, if yes, network set up finish. 
            {
              network_setup_finish = 1;
              broadcast_retry = 0;
              break;
            }
          } 
       } 
     delay(100);        
}  

void polling_clients(void)
{
    int polling_count = 0 ;

    for(polling_count = 0; polling_count < client_numbers; polling_count++)//send data requst to every client one by one
     {
      delay(100);
      uint8_t query[4] = {0}; //Data Request Message
      if (clients[polling_count]==0)
      {
        Console.println("the client connect here has leatf");
        break;
      }
      query[3] = clients[polling_count] ;
      for(int poll_cnt = 0; poll_cnt < MAX_REMOVE_RETRY; poll_cnt++)//Send a data requst message
       {  
          uint8_t query[4] = {0}; //Data Request Message
          query[0] = 'D';
          query[1] = 'R';
          query[2] = ':';
          query[3] = clients[polling_count] ;// client id to be polled
          Console.print("Request Data from client: ");
          Console.print(query[3]);

         
          int length = sizeof(query);//Get Length
          rf95.send(query, sizeof(query));//send poll message
          rf95.waitPacketSent();                   
         if (rf95.waitAvailableTimeout(3000))//
         {          
           Console.println("Get incoming message ");
          
           buf[0]='\0'; 
           uint8_t len = sizeof(buf);     
           if (rf95.recv(buf, &len))
            {              
               no_response_count = 0;  
               recdata(buf,len);//crc calcualte
            
               if(CrcFlag == 1)//Calculate Successful, match
               {
                CrcFlag = 0;
                if( buf[0] == 'D' && buf[1] == 'S' && buf[3] == query[3])
                 {

                  char rssi[5];
                  int n=sprintf(rssi,"%d",rf95.lastRssi());
                  //char * mes=buf+5;
                  buf[0]='g';
                  buf[1]='a';
                  buf[2]='i';
                  buf[3]='a';
                  buf[4]=' ';
                  buf[len-2]='r';
                  buf[len-1]='s';
                  buf[len]='s';
                  buf[len+1]='i';
                  buf[len+2]=',';
                  int i=0;
                  while(i<n) 
                  {
                    buf[len+3+i]=rssi[i];
                    i++;
                  } 
                  buf[len+3+i]='+';
                  buf[len+4+i]='\0';
                  
                  sendMQTT(buf);
                  no_response_count = 0;
                  break;
                 }
                 else
                 {
                    no_response_count ++;
                 }
               }
              else
               {
                Console.println( "CRC Error");
               } 
            }
         }
        else
        {
         no_response_count ++;
         if(no_response_count == MAX_REMOVE_RETRY )//no response from client, remove it after max retry
         {
           no_response_count = 0;
       
           Console.print( "Client is nor responds ID:");
           Console.println(clients[polling_count]);    

           Console.print("Client number");
           Console.println(client_numbers);
          }
        }      
       }
    } 
}
 
void loop()
{   
      if(network_setup_finish == 0)
      {
        Console.println("set up the network "); 
        set_up_network();//set up network
        Console.println("set up!!"); 
        poll_start_time = millis( );
  

      }
      else
      {
    
        polling_clients();// request data from clients

        Console.print("Client number:");
        Console.println(client_numbers);
      }
      delay(2000);
      total_poll_time += millis( ) -poll_start_time;//get total poll time
    
     if(total_poll_time > REFRESH_TIME)//refresh the network 
     {
       Console.println( "Refresh network after timeout");
       network_setup_finish = 0;
       total_poll_time = 0;
     }
}


void sendMQTT(char * MQbuf) {
  // Launch "curl" command and get Arduino ascii art logo from the network
  // curl is command line program for transferring data using different internet protocols
    
 
  Process p;    // Create a process and call it "p"
  p.runShellCommand(MQbuf);
  // Ensure the last bit of data is sent.*/
  Console.flush();
}


