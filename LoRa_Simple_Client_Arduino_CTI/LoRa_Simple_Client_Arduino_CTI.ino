/*
  LoRa Simple Client for Arduino :
  Support Devices: LoRa Shield + Arduino 
  
  Example sketch showing how to create a simple messageing client, 
  with the RH_RF95 class. RH_RF95 class does not provide for addressing or
  reliability, so you should only use RH_RF95 if you do not need the higher
  level messaging abilities.

  It is designed to work with the other example LoRa Simple Server
  User need to use the modified RadioHead library from:
  https://github.com/dragino/RadioHead

  modified 16 11 2016
  by Edwin Chen <support@dragino.com>
  Dragino Technology Co., Limited
*/
#include <Process.h>

#include <SPI.h>
#include <RH_RF95.h>

// Singleton instance of the radio driver
RH_RF95 rf95;
float frequency = 868.0;

void setup() 
{
    Bridge.begin(115200);
  // Initialize Serial
  Console.begin(); 
  // Wait until a Serial Monitor is connected.
  while (!Console);
  
  //while (!Serial) ; // Wait for serial port to be available
  Console.println("Start LoRa Client");
  if (!rf95.init())
    Console.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);

  // Setup Spreading Factor (6 ~ 12)
  rf95.setSpreadingFactor(7);
  
  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  rf95.setSignalBandwidth(125000);
  
  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
  rf95.setCodingRate4(5);
}

void loop()
{
  Console.println("Sending to LoRa Server");
  // Send a message to LoRa Server
  uint8_t data[] = "Hello, this is device 1";
  rf95.send(data, sizeof(data));
  
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(3000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Console.print("got reply: ");
      Console.println((char*)buf);
      Console.print("RSSI: ");
      Console.println(rf95.lastRssi(), DEC);    
    }
    else
    {
      Console.println("recv failed");
    }
  }
  else
  {
    Console.println("No reply, is LoRa server running?");
  }
  delay(1000);
}


