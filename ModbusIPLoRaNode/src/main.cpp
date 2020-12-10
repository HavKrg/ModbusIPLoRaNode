#include <Arduino.h>
#include <RHMesh.h>
#include <RH_RF95.h>
#include <WiFi.h>
#include <ModbusIP_ESP8266.h>



/*************************************
            DEFINITIONS
*************************************/

#define BAUD 9600
#define DEBUG 1
#define sizeOfIPBuffer 15
#define sizeOfFunctionTypeBuffer 5
#define sizeOfDataBuffer 100
#define sizeOfParsedIP 4

/*************************************
        FUNCTION DECLARATIONS
*************************************/
int checkMessageType();
int readCoilStatus(IPAddress modbusServer);
int readHoldingRegisters(IPAddress modbusServer);
int readInternalRegisters(IPAddress modbusServer);
int writeSingleCoil(IPAddress modbusServer);
int writeSingleRegister(IPAddress modbusServer);
int writeMultipleCoils(IPAddress modbusServer);
int writeMultipleRegisters(IPAddress modbusServer);
int returnBadRequest();
int parseFunctionType();
int parseIPAddress();
int parseIncomingData();
int resetBuffers();


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
            LORA SETTINGS
*************************************/
RH_RF95 rf95(SS, DI0);    // LORA RADIO
RHMesh manager(rf95, 1);  // MESH MANAGER

/*************************************
          WIFI SETTINGS
*************************************/
byte myMAC[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress myIP(10, 0, 0, 199);
IPAddress myDNS(10, 0, 0, 1);
IPAddress myGateway(10, 0, 0, 1);
char SSID[] = "ASUS";
char PASSWORD[] = "All3Fugl3r";

/*************************************
          MODBUS SETTINGS
*************************************/
ModbusIP mb;



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
  // Initialize LoRa Mesh manager
  if(!manager.init()){
    Serial.println("LoRa Mesh manager init failed");
  }
  else{
    Serial.println("LoRa Mesh manager init complete");
  }

  // Initialize WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());


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

// Reply message variable
uint8_t data[RH_MESH_MAX_MESSAGE_LEN] = {'\0'};
// Buffer for incoming message
//uint8_t buf[RH_MESH_MAX_MESSAGE_LEN] = {'\0'};
//uint8_t buf[] = {uint8_t(1), uint8_t(10), uint8_t(0), uint8_t(0), uint8_t(180), uint8_t('#'), uint8_t(3)};
uint8_t buf[] = "#H#10,0,0,180#3";

char IPBuffer[sizeOfIPBuffer];
char functionTypeBuffer[sizeOfFunctionTypeBuffer];
char dataBuffer[sizeOfDataBuffer];
int parsedIP[sizeOfParsedIP];

char *ptr;

void loop()
{
  // uint8_t len = sizeof(buf);
  // uint8_t from;
  // if (manager.recvfromAck(buf, &len, &from))
  // {

  //   // Print incomming message when it arrives
  //   Serial.print("0x");
  //   Serial.print(from, HEX);
  //   Serial.print(": ");
  //   Serial.println((char*)buf);

  //   checkMessageType(buf[0]);

  //   // Send a reply back to the originator client
  //   if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
  //   Serial.println("sendtoWait failed");
  // }
  delay(5000);

  resetBuffers();

  parseIncomingData();
  
  
  if(DEBUG)
  {
    Serial.println("************Buffer data************");
    Serial.printf("functionTypeBuffer : %s\n", functionTypeBuffer);
    Serial.printf("IPBuffer : %s\n", IPBuffer);
    Serial.printf("dataBuffer : %s\n", dataBuffer);
    Serial.println("************************************");
    Serial.println();
  }
  
  int functionType = parseFunctionType();
  parseIPAddress();
  IPAddress modbusServer(parsedIP[0], parsedIP[1], parsedIP[2], parsedIP[3]);
  

  if(DEBUG){
    Serial.println("************Parsed Data************");
    Serial.printf("Function type : %d\n", functionType);
    Serial.print("IP Address : ");
    Serial.println(modbusServer);
    Serial.printf("dataBuffer : %s\n", dataBuffer);
    Serial.println("************************************");
    Serial.println();
  }
  
  
  

  Serial.println("Loop complete");
  delay(500000);
}

int parseIncomingData(){
  //
  ptr = strtok((char *)buf, "#");
  strcpy(functionTypeBuffer, ptr);
  ptr = strtok(NULL, "#");
  strcpy(IPBuffer, ptr);
  ptr = strtok(NULL, "#");
  strcpy(dataBuffer, ptr);
}

int parseFunctionType(){
  int functionType = 0;

  ptr = strtok((char *)functionTypeBuffer, "#");

  functionType = atoi(ptr);

  return functionType;
}

int parseIPAddress(){
  ptr = strtok((char *)IPBuffer, ",");
  for (size_t i = 0; i < 4; i++)
  {
    parsedIP[i] = atoi(ptr);
    ptr = strtok(NULL, ",");
  }
  
  return 0;
}



int readCoilStatus(IPAddress modbusServer)
{
  // Incoming message should look like this: "1{IP1}{IP2}{IP3}{IP4}#{coilAddress}"
  // Serial.println("3");
  // Serial.print("Buffer : ");
  // Serial.println((char *) buf);
  // char *ptr = strtok((char *)buf, "#");

  // Serial.print("pointer is at : ");
  // Serial.println((char *)ptr);

  // int coilAddress = (int)ptr;
  // bool coilData = 0;

  // if(mb.isConnected(modbusServer))
  // {
  //   mb.readCoil(modbusServer, coilAddress, &coilData);
  // }
  // else
  // {
  //   mb.connect(modbusServer);
  // }
  // mb.task();
  // Serial.println(coilData);

  return 0;
}

int readHoldingRegisters(IPAddress modbusServer)
{
  // Incoming message should look like this: "3#{holdingRegisterAddress}"
  int holdingRegisterAddress = 0;
  return 0;
}

int readInternalRegisters(IPAddress modbusServer)
{
  // Incoming message should look like this: "4#{internalRegisterAddress}"
  int internalRegisterAddress = 0;
  return 0;
}

int writeSingleCoil(IPAddress modbusServer)
{
  // Incoming message should look like this: "5#{coilAddress}#{coilData}"
  int coilAddress = 0;
  int coilData = 0;
  return 0;
}

int writeSingleRegister(IPAddress modbusServer)
{
  // Incoming message should look like this: "6#{registerAddress}#{registerData}"
  int registerAddress = 0;
  int registerData = 0;
  return 0;
}

int writeMultipleCoils(IPAddress modbusServer)
{
  // Incoming message should look like this: "15#{startAddress}#{numberOfCoils}#{coilData1},{coilData2}...,{coilDataN}"
  int startAddress = 0;
  int numberOfCoils = 0;
  int coilData[numberOfCoils];
  return 0;
}

int writeMultipleRegisters(IPAddress modbusServer)
{
  // Incoming message should look like this: "16#{startAddress}#{numberOfRegisters}#{registerData1},{registerData2}...,{registerDataN}"
  int startAddress = 0;
  int numberOfRegisters = 0;
  int registerData[numberOfRegisters];
  return 0;
}

int returnBadRequest()
{


  return 0;
}

int resetBuffers(){
  memset(parsedIP, '\0', sizeOfParsedIP);
  memset(dataBuffer, '\0', sizeOfDataBuffer);
  memset(IPBuffer, '\0', sizeOfIPBuffer);
  memset(functionTypeBuffer, '\0', sizeOfFunctionTypeBuffer);
}