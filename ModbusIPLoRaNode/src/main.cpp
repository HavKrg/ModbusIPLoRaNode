#include <Arduino.h>
#include <RHMesh.h>
#include <RH_RF95.h>

 /*************************************
             DEFINITIONS
 *************************************/

#define BAUD 115200


/*************************************
            LORA PINS
*************************************/
 #define SCK 5      // GPIO5  -- SX1276's SCK
 #define MISO 19    // GPIO19 -- SX1276's MISO
 #define MOSI 27    // GPIO27 -- SX1276's MOSI
 #define SS 18      // GPIO18 -- SX1276's CS
 #define RST 23     // GPIO14 -- SX1276's RESET
 #define DI0 26     // GPIO26 -- SX1276's IRQ(Interrupt Request)
 #define BAND 868E6 // LORA FREQUENCY


/*************************************
            LORA Settings
*************************************/
RH_RF95 rf95(SS, DI0);    // LORA RADIO
RHMesh manager(rf95, 1);  // MESH MANAGER




void setup() {
  // Initialize Serial connection
  Serial.begin(BAUD);
  if(!Serial){
    Serial.println("Serial init failed");
  }
  else{
    Serial.println("Serial init complete");
  }

  // Initialize RF95 radio
  if(!rf95.init()){
    Serial.println("RF95 init failed");
  }
  else{
    Serial.println("RF95 init complete");
  }

  if(!manager.init()){
    Serial.println("LoRa Mesh manager init failed");
  }
  else{
    Serial.println("LoRa Mesh manager init complete");
  }
  
/*************************************
            Radio configuration
*************************************/
rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128); // Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range. 
// rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128); // Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range. 
// rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512); // Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range. 
// rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096); // Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, low data rate, CRC on. Slow+long range. 
// rf95.setModemConfig(RH_RF95::Bw125Cr45Sf2048); // Bw = 125 kHz, Cr = 4/5, Sf = 2048chips/symbol, CRC on. Slow+long range. 

rf95.setFrequency(BAND); // Sets the transmitter and receiver centre frequency. 

rf95.setTxPower(13, false); // Sets the transmitter power output level, and configures the transmitter pin.

manager.setTimeout(3000); // set timeout for manager (needed for low bandwidth modem config)

}

// Reply message replace 'X' with the node number
uint8_t data[] = "2";
// Buffer for incoming message
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];


void loop()
{
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager.recvfromAck(buf, &len, &from))
  {
    // Print incomming message when it arrives
    Serial.print("0x");
    Serial.print(from, HEX);
    Serial.print(": ");
    Serial.println((char*)buf);
 
    // Send a reply back to the originator client
    if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
    Serial.println("sendtoWait failed");
  }
}
