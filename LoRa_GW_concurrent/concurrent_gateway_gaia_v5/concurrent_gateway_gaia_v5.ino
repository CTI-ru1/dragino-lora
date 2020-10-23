/*
  example: concurrent_gateway
  Support Devices:
    1)LG01

  How it works:
  This concurrent client sketch is working together with the concurrent gateway
  sketch. Before using this sketch, please use the write_client_id sketch to
  write a client ID in the EEPROM.
  Client ID is the unique id for each client in the LoRa network.
  write_gateway_id to gateway is not necessary, if not write, gateway id will be
  0XFF.
  * When the client boot, it will keep listening for the broadcast message
    from LoRa Gateway.
  * When the gateway sketch boot, it will broadcast a message to set up a LoRa
    Network. If it gets broadcast message, client will send a join request message
    to gateway, when the join request message arrive to gateway, the gateway will
    send back a join-ack message with client id and add this client to the LoRa
    Network.
  * If the client gets its join-ack message for its join request, it will enter
    the mode to listen the data-request message from gateway. In this mode,
    if client get a data-request message for this client it will send back a data
    message to the gateway.
  * After client in data_request li.1stening mode, if it has not receive any
    message from gateway in a timeout, it will go back to the network set up mode
    to listen the broadcast message.
  * Gateway will refresh the LoRa network periodically for adding new client or
    remove unreachable client.
  This example using the polling method between LoRa node and Gateway, it will
  minimize the LoRa packets transfer on the air and avoid congestion. It is
  suitable for a not real time LoRa work.

  Performance test in a room with 100 nodes and 1 gateway shows:
  a) Gateway require about 1.5 minutes to set up this 100 nodes Network
  b) Gateway takes about 2 minutes to do polling for these 100 nodes.

  modified 24 Dec 2016
  by Dragino Tech <support@dragino.com>
  Dragino Technology Co., Limited
*/

#include <Console.h>
#include <Process.h>
#include <RH_RF95.h>
#include <SPI.h>
//#include <EEPROM.h>
//#include "encrypt.h"
#include <avr/wdt.h>

//#define USE_PROGMEM
// Disable F macro
#ifndef USE_PROGMEM
#define F(x) x
#endif
// Disable debugging
//#define F(x) ""

RH_RF95 rf95;
// ENCRYPT  encrypt_decrypt;

// Define the LoRa frequency use for this client
float frequency = 868.0;
int gateway_id  = 3;

// MAX Clients support, more clients will increase time to establish network and polling

// Possible issue: As we incress number of nodes we run out of memory
#define MAX_CLIENT 20

// Time to refresh the network. Will add new clients after refresh network
#define REFRESH_TIME 900000  // 15mmin
//#define REFRESH_TIME 30000
// The no response retry count before stop to broadcast message and finish network setup.
#define MAX_BROADCAST_RETRY 15

// How Many Retry to kick a client if no response for a data request command.
#define MAX_REMOVE_RETRY 3

#define BAUDRATE 115200

// Client ID address in EEPROM.
#define ID_ADDRESS 0x00

// Polling periode
#define POLL_PERIODE 0
#define POLL_NODE_PERIODE 0

#define MAX_BUFFER 90

// MQTT string formats
#ifndef USE_PROGMEM
const char msg_rssi_str[]   = {"rssi,%d+snr,%d+pl,%d+\0"};
const char msg_status_str[] = {"gaia %u/msg,%u+ret,%u+pub,%u+inv,%u+crc,%u+cyc,%u+\0"};
#else
const char msg_rssi_str[] PROGMEM   = {"rssi,%d+snr,%d+pl,%d+\0"};
const char msg_status_str[] PROGMEM = {"gaia %u/msg,%u+ret,%u+pub,%u+inv,%u+crc,%u+cyc,%u+\0"};
#endif

// String sensor_data = "";
char clients[MAX_CLIENT] = {0};  // store the clients's ID
int client_numbers       = 0;    // Client Numbers has been stored
int broadcast_retry      = 0;    // counter for broadcast message on no response.
int network_setup_finish = 0;    // flag to indicate network set up is finished.
int b                    = 0;    // network set up count.
long poll_start_time     = 0;    //
long delay_start_time    = 0;    //
long total_poll_time     = 0;    //
int no_response_count    = 0;    // counter for no response from Client.
// Select gw
int CrcFlag = 0;

// Network status variables
unsigned short msm[MAX_CLIENT]  = {0};
unsigned short pub[MAX_CLIENT]  = {0};
unsigned short nret[MAX_CLIENT] = {0};
unsigned short ncrc[MAX_CLIENT] = {0};
unsigned short nmal[MAX_CLIENT] = {0};
unsigned short ncyl             = 0;

// Use dragino hostname
// String hostname;
uint8_t buf[MAX_BUFFER] = {0};

void setup()
{
    // Enable the watchdog
    wdt_enable(WDTO_8S);
    Bridge.begin(BAUDRATE);
    // Console.begin();
    if (!rf95.init()) Console.println(F("Init failed"));
    // Setup ISM frequency
    wdt_reset();
    rf95.setFrequency(frequency);
    // Setup Power,dBm
    rf95.setTxPower(13);
    wdt_reset();
}

uint16_t calcByte(uint16_t crc, uint8_t b)
{
    uint32_t i = 0;
    crc        = crc ^ (uint32_t)b << 8;

    for (i = 0; i < 8; i++) {
        if ((crc & 0x8000) == 0x8000)
            crc = crc << 1 ^ 0x1021;
        else
            crc = crc << 1;
    }
    return crc & 0xffff;
}

uint16_t CRC16(uint8_t* pBuffer, uint32_t length)
{
    uint16_t wCRC16 = 0;
    uint32_t i      = 0;
    if ((pBuffer == 0) || (length == 0)) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        wCRC16 = calcByte(wCRC16, pBuffer[i]);
    }
    return wCRC16;
}
uint16_t recdata(unsigned char* recbuf, int Length)
{
    uint16_t crcdata    = 0;
    uint16_t recCRCData = 0;
    crcdata             = CRC16(recbuf, Length - 2);  // Calculate the CRC for the received message
    recCRCData          = recbuf[Length - 1];         // get the CRC high byte
    recCRCData          = recCRCData << 8;            // get the CRC low byte
    recCRCData |= recbuf[Length - 2];                 // get the receive CRC
    if (crcdata == recCRCData) {
        CrcFlag    = 1;
        crcdata    = 0;
        recCRCData = 0;
        Console.println(F("CRC++++"));
    } else {
        CrcFlag    = 0;
        crcdata    = 0;
        recCRCData = 0;
    }
}

void set_up_network(void)
{
    uint8_t broadcast[4] = {0};  // Broadcast Message
    broadcast[0]         = 'B';
    broadcast[1]         = 'C';
    broadcast[2]         = gateway_id;
    broadcast[3]         = ':';
    int length           = sizeof(broadcast);  // get broadcast message length

    for (b = 0; b < 200; b++)  // loop to set up network
    {
        Console.println(F("Sending broadcast message"));
        wdt_reset();
        rf95.send(broadcast, sizeof(broadcast));
        wdt_reset();
        rf95.waitPacketSent();
        wdt_reset();
        if (rf95.waitAvailableTimeout(500))  // Check the incoming message
        {
            wdt_reset();
            Console.println(F("Getting an incoming message"));
            buf[0]      = '\0';
            uint8_t len = sizeof(buf);  //
            if (rf95.recv(buf, &len))   //
            {
                wdt_reset();
                if (buf[0] == 'J' && buf[1] == 'R' &&
                    buf[2] == gateway_id)  // Get a Join request, Store the Client ID
                {
                    if (buf[3] == 0)  // Error Message
                    {
                        Console.println(F("Error"));
                    } else {
                        for (int b = 0; b < 5; b++)  // Send 5 Join ACK to the Client,make sure it
                                                     // arrive.   MAX_BROADCAST_RETRY)
                        {
                            uint8_t JoinAck[4] = {0};  // Join ACK message
                            JoinAck[0]         = 'J';
                            JoinAck[1]         = 'A';  // client id to be polled
                            JoinAck[2]         = gateway_id;
                            JoinAck[3]         = buf[3];           // Put Client ID
                            int length         = sizeof(JoinAck);  // Get Data Length
                            Console.print(F("Sent Join ACK to client: "));
                            Console.println(buf[3]);
                            wdt_reset();
                            rf95.send(JoinAck, sizeof(JoinAck));
                            wdt_reset();
                            rf95.waitPacketSent();
                            wdt_reset();
                            broadcast_retry = 0;
                            delay(500);
                        }
                        int pos;
                        for (pos = 0; pos < sizeof(clients);
                             pos++)  // check if this client already stored
                        {
                            if (clients[pos] == buf[3]) {
                                Console.println(F("Client already stored"));
                                break;
                            }
                        }
                        if (pos == sizeof(clients)) {
                            clients[client_numbers] = buf[3];  // store the client id.
                            client_numbers++;

                            Console.print(F("New client id: "));
                            Console.println(buf[3]);
                            Console.print(F("Clients number: "));
                            Console.println(client_numbers);
                            broadcast_retry = 0;  // get a new client , reset retry count
                        }
                    }
                }
            }
        } else {
            broadcast_retry++;
            if (broadcast_retry > MAX_BROADCAST_RETRY)  // check if retry count exceed the max
                                                        // retry, if yes, network set up finish.
            {
                network_setup_finish = 1;
                broadcast_retry      = 0;
                break;
            }
        }
    }
    wdt_reset();
    delay(100);
}

void polling_clients(void)
{
    int polling_count = 0;

    for (polling_count = 0; polling_count < client_numbers;
         polling_count++)  // send data requst to every client one by one
    {
        wdt_reset();
        // delay(POLL_NODE_PERIODE);
        uint8_t query[4] = {0};  // Data Request Message
        if (clients[polling_count] == 0) {
            Console.println(F("The client connected here has left"));
            break;
        }
        query[3] = clients[polling_count];
        for (int poll_cnt = 0; poll_cnt < MAX_REMOVE_RETRY;
             poll_cnt++)  // Send a data requst message
        {
            uint8_t query[4] = {0};  // Data Request Message
            query[0]         = 'D';
            query[1]         = 'R';
            query[2]         = gateway_id;
            query[3]         = clients[polling_count];  // client id to be polled
            Console.print(F("Request Data from client: "));
            Console.println(query[3]);

            int length = sizeof(query);  // Get Length
            wdt_reset();
            rf95.send(query, sizeof(query));  // send poll message
            wdt_reset();
            rf95.waitPacketSent();
            wdt_reset();
            if (rf95.waitAvailableTimeout(2000))  //
            {
                wdt_reset();
                Console.println(F("Get incoming message"));
                memset(buf, 0, sizeof(buf));
                // buf[0]='\0';
                uint8_t len = sizeof(buf);
                wdt_reset();
                if (rf95.recv(buf, &len)) {
                    wdt_reset();
                    // Console.println("Correct data received");
                    no_response_count = 0;
                    recdata(buf, len);  // crc calcualte

                    if (CrcFlag == 1)  // Calculate Successful, match
                    {
                        // Console.println("CRC calculated correctly");
                        CrcFlag = 0;
                        if (buf[0] == 'D' && buf[1] == 'S' && buf[2] == gateway_id &&
                            buf[3] == query[3]) {
                            // Console.println("DS data received");
                            char rssi[30];
                            wdt_reset();
#ifndef USE_PROGMEM
                            int n = snprintf(rssi, sizeof(rssi), msg_rssi_str, rf95.lastRssi(),
                                             rf95.lastSNR(), len);
#else
                            int n = snprintf_P(rssi, sizeof(rssi), msg_rssi_str, rf95.lastRssi(),
                                               rf95.lastSNR(), len);
#endif
                            int snr = rf95.lastSNR();
                            Console.print(F("SNR: "));
                            Console.println(snr);
                            wdt_reset();
                            // char * mes=buf+5;
                            buf[0] = 'g';
                            buf[1] = 'a';
                            buf[2] = 'i';
                            buf[3] = 'a';
                            buf[4] = ' ';
                            int i  = 0;
                            while ((i < n) && (len - 2 + i < MAX_BUFFER)) {
                                buf[len - 2 + i] = rssi[i];
                                ;
                                i++;
                            }
                            /*buf[len-2]='r';
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
                        buf[len+4+i]='\0';*/
                            wdt_reset();

                            // Increse the message receive correct for each node
                            msm[polling_count]++;

                            if (sendMQTT(buf) == 250) {
                                // If the publish is corect increass the counter for  the specific
                                // node
                                pub[polling_count]++;
                            }
                            wdt_reset();
                            no_response_count = 0;
                            break;
                        } else {
                            nmal[polling_count]++;
                            no_response_count++;
                        }
                    } else {
                        // As it is if exist CRC error try always
                        // Change if exist CRC error try 3 tiems too
                        ncrc[polling_count]++;
                        no_response_count++;
                        Console.println(F("CRC Error"));
                    }
                }
            } else {
                nret[polling_count]++;
                no_response_count++;
                if (no_response_count ==
                    MAX_REMOVE_RETRY)  // no response from client, remove it after max retry
                {
                    no_response_count = 0;

                    Console.print(F("Client not responding ID: "));
                    Console.println(clients[polling_count]);

                    Console.print(F("Client number: "));
                    Console.println(client_numbers);
                }
            }
        }
    }
    wdt_reset();
}

void loop()
{
    if (network_setup_finish == 0) {
        Console.println(F("Set up the network"));
        set_up_network();  // set up network
        Console.println(F("Set up!!"));
        poll_start_time = millis();
        wdt_reset();

    } else {
        ncyl++;  // Incress number of cycles

        polling_clients();  // request data from clients

        Console.print(F("Clients number: "));
        Console.println(client_numbers);
    }

    // Polling periode
    /* delay_start_time = millis( );
         Console.println(delay_start_time);
        while((delay_start_time+POLL_PERIODE)>millis())
        {
          delay(10);
          Console.print("Value:");
          Console.println(delay_start_time+POLL_PERIODE);
          wdt_reset();

        }*/

    if (millis() - poll_start_time > REFRESH_TIME)  // refresh the network
    {
        Console.println(F("Refresh network after timeout"));
        network_setup_finish = 0;
        total_poll_time      = 0;

        // Create Network status message for each node
        memset(buf, 0, sizeof(buf));
        int i = 0;
        for (i = 0; i < client_numbers; i++) {
            // sprintf(buf,"gaia %u/msm,%u+pub,%u+\0",i+1,msm[i],pub[i]);
#ifndef USE_PROGMEM
            int n = snprintf(buf, sizeof(buf), msg_status_str, clients[i], msm[i], nret[i], pub[i],
                             nmal[i], ncrc[i], ncyl);
#else
            int n = snprintf_P(buf, sizeof(buf), msg_status_str, clients[i], msm[i], nret[i],
                               pub[i], nmal[i], ncrc[i], ncyl);
#endif
            // sprintf(buf,"gaia");
            int i = 0;
            while (i < sizeof(buf)) {
                Console.print((char)buf[i]);
                i++;
            }
            Console.println();

            sendMQTT(buf);
            //  delay(10);
        }
        ncyl = 0;
        memset(nmal, 0, sizeof(nret));
        memset(nret, 0, sizeof(nret));
        memset(msm, 0, sizeof(msm));
        memset(pub, 0, sizeof(pub));
        memset(ncrc, 0, sizeof(ncrc));
    }
}

int sendMQTT(char* MQbuf)
{
    // Launch "curl" command and get Arduino ascii art logo from the network
    // curl is command line program for transferring data using different internet protocols
    Console.println(F("SendMqtt"));

    Process p;  // Create a process and call it "p"
    int re = p.runShellCommand(MQbuf);
    Console.print(F("Return: "));
    Console.println(re);
    // Ensure the last bit of data is sent.*/
    Console.flush();
    wdt_reset();
    return re;
}
