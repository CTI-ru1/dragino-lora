/*
  LoRa Simple Arduino Server :
  Support Devices: 
  * LoRa Mini
  * LoRa Shield + Arduino;
  * LoRa GPS Shield + Arduino. 
  
  Example sketch showing how to create a simple messageing server, 
  with the RH_RF95 class. RH_RF95 class does not provide for addressing or
  reliability, so you should only use RH_RF95 if you do not need the higher
  level messaging abilities.

  It is designed to work with the other example LoRa Simple Client

  modified 16 11 2016
  by Edwin Chen <support@dragino.com>
  Dragino Technology Co., Limited
*/

#include <Process.h>
#include <SPI.h>
#include <RH_RF95.h>

// Singleton instance of the radio driver
RH_RF95 rf95;

int HEART_LED=A2;
float frequency = 868.0;

void setup() 
{

    pinMode(HEART_LED, OUTPUT);     
    digitalWrite(HEART_LED, HIGH);

// Initialize Bridge
  Bridge.begin(115200);

  // Initialize Serial
  Console.begin(); 
  // Wait until a Serial Monitor is connected.
  while (!Console);
  

  Console.println("Start Sketch");
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
  
  // Defaults BW Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  Console.print("Listening on frequency: ");
  Console.println(frequency);
    digitalWrite(HEART_LED, LOW);
}

void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len))
    {
      digitalWrite(HEART_LED, HIGH);
      RH_RF95::printBuffer("request: ", buf, len);
      Console.print("got request: ");
      Console.println((char*)buf);
      Console.print("RSSI: ");
      Console.println(rf95.lastRssi(), DEC);
      
      sendMQTT(buf);

      digitalWrite(HEART_LED, LOW);

    }
    else
    {
      Serial.println("recv failed");
    }
  }
}




void sendMQTT(char * buf) {
  // Launch "curl" command and get Arduino ascii art logo from the network
  // curl is command line program for transferring data using different internet protocols
  Process p;    // Create a process and call it "p"
  p.begin("mosquitto_pub"); // Process that launch the "cat" command
  p.addParameter("-h");  // Process that launch the "cat" command
  p.addParameter("150.140.5.11");  // Process that launch the "cat" command
  p.addParameter("-t");  // Process that launch the "cat" command
  p.addParameter("test");  // Process that launch the "cat" command
  p.addParameter("-m");  // Process that launch the "cat" command
  p.addParameter(buf);  // Process that launch the "cat" command
  p.run();    // Run the process and wait for its termination

  // Print arduino logo over the Serial
  // A process output can be read with the stream methods
  while (p.available() > 0) {
    char c = p.read();
    Console.print(c);
  }
  // Ensure the last bit of data is sent.
  Console.flush();
}


