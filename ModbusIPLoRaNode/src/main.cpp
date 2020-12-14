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
#define sizeOfDataAddressBuffer 5
#define sizeOfNumberOfAddressBuffer 5
#define sizeOfValuesBuffer 100

/*************************************
        FUNCTION DECLARATIONS
*************************************/
int resetBuffers();
int processRequest(int functionType, IPAddress modbusServer, char *dataBuffer);
int parseFunctionType();
int parseIPAddress();
int parseIncomingData();

int readCoils(IPAddress modbusServer, char *dataBuffer);
int readHoldingRegisters(IPAddress modbusServer, char *dataBuffer);
int readInternalRegisters(IPAddress modbusServer, char *dataBuffer);
int writeCoils(IPAddress modbusServer, char *dataBuffer);
int writeHoldingRegisters(IPAddress modbusServer, char *dataBuffer);

int returnBadRequest();

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
RH_RF95 rf95(SS, DI0);   // LORA RADIO
RHMesh manager(rf95, 1); // MESH MANAGER

/*************************************
          WIFI SETTINGS
*************************************/
byte myMAC[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress myIP(10, 0, 0, 199);
IPAddress myDNS(10, 0, 0, 1);
IPAddress myGateway(10, 0, 0, 1);
char SSID[] = "ASUS";
char PASSWORD[] = "All3Fugl3r";

/*************************************
          MODBUS SETTINGS
*************************************/
ModbusIP mb;

void setup()
{
  // Initialize Serial connection
  Serial.begin(BAUD);
  if (!Serial)
  {
    Serial.println("Serial init failed");
  }
  else
  {
    Serial.println("Serial init complete");
  }

  // Initialize RF95 radio
  if (!rf95.init())
  {
    Serial.println("RF95 init failed");
  }
  else
  {
    Serial.println("RF95 init complete");
  }
  // Initialize LoRa Mesh manager
  if (!manager.init())
  {
    Serial.println("LoRa Mesh manager init failed");
  }
  else
  {
    Serial.println("LoRa Mesh manager init complete");
  }

  // Initialize WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
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
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN] = {'\0'};

// #{functionNumber}#{IPAddress}#{startAddress}#{numberOfAddresses},{value1},{value2}...{valueN}\n
// uint8_t buf[] = "#1#10,0,0,180#0#1,0\n"; // READ SINGLE COIL TEST
// uint8_t buf[] = "#3#10,0,0,180#0#1,0\n"; // READ SINGLE HOLDING REGISTER TEST
// uint8_t buf[] = "#4#10,0,0,180#0#1,0\n"; // READ SINGLE INTERNAL REGISTER TEST
// uint8_t buf[] = "#5#10,0,0,180#1#1,0\n"; // WRITE SINGLE COIL TEST
// uint8_t buf[] = "#6#10,0,0,180#0#1,9999\n"; // WRITE SINGLE HOLDING REGISTER TEST
// uint8_t buf[] = "#15#10,0,0,180#0#4,0,0,0,0\n"; // WRITE MULTIPLE COILS TEST
// uint8_t buf[] = "#16#10,0,0,180#0#5,100,101,102,103, 104\n"; // WRITE MULTIPLE  HOLDING REGISTERS TEST


char IPBuffer[sizeOfIPBuffer];
char functionTypeBuffer[sizeOfFunctionTypeBuffer];
char dataBuffer[sizeOfDataBuffer];
int parsedIP[sizeOfParsedIP];
char dataAddressBuffer[sizeOfDataAddressBuffer];
char numberOfAddressesBuffer[sizeOfNumberOfAddressBuffer];
char valuesBuffer[sizeOfValuesBuffer];

char *ptr;

void loop()
{
  uint8_t len = sizeof(buf);
  uint8_t from;

  if (manager.recvfromAck(buf, &len, &from))
  {
    Serial.println("RECEIVED REQUEST");
    // Print incomming message when it arrives
    Serial.print("0x");
    Serial.print(from, HEX);
    Serial.print(": ");
    Serial.println((char*)buf);

    if (from == 100 && buf[0] == '#')
    {
      parseIncomingData();
      resetBuffers();


  if (DEBUG)
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

  if (DEBUG)
  {
    Serial.println("************Parsed Data************");
    Serial.printf("Function type : %d\n", functionType);
    Serial.print("IP Address : ");
    Serial.println(modbusServer);
    Serial.printf("dataBuffer : %s\n", dataBuffer);
    Serial.println("************************************");
    Serial.println();
  }

  processRequest(functionType, modbusServer, dataBuffer);
    }
    else
    {
      Serial.printf("Received bad request: %s", buf);
    }
    
    


    // Send a reply back to the originator client
    if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
    Serial.println("sendtoWait failed");
  }
  

  

  
}

int parseIncomingData()
{
  //
  ptr = strtok((char *)buf, "#");
  strcpy(functionTypeBuffer, ptr);
  ptr = strtok(NULL, "#");
  strcpy(IPBuffer, ptr);
  ptr = strtok(NULL, "\n");
  strcpy(dataBuffer, ptr);

  return 0;
}

int parseFunctionType()
{
  int functionType = 0;

  ptr = strtok((char *)functionTypeBuffer, "#");

  functionType = atoi(ptr);

  return functionType;
}

int parseIPAddress()
{
  ptr = strtok((char *)IPBuffer, ",");
  for (size_t i = 0; i < 4; i++)
  {
    parsedIP[i] = atoi(ptr);
    ptr = strtok(NULL, ",");
  }

  return 0;
}

int processRequest(int functionType, IPAddress modbusServer, char *dataBuffer)
{
  switch (functionType)
  {
  case 1:
    return readCoils(modbusServer, dataBuffer);
  case 3:
    return readHoldingRegisters(modbusServer, dataBuffer);
  case 4:
    return readInternalRegisters(modbusServer, dataBuffer);
  case 5:
    return writeCoils(modbusServer, dataBuffer);
  case 6:
    return writeHoldingRegisters(modbusServer, dataBuffer);
  case 15:
    return writeCoils(modbusServer, dataBuffer);
  case 16:
    return writeHoldingRegisters(modbusServer, dataBuffer);
  default:
    return returnBadRequest();
  }
  return 1;
}

int readCoils(IPAddress modbusServer, char *dataBuffer)
{
  // Incoming message should look like this: "{startAddress}#{numberOfCoils}"
  ptr = strtok(dataBuffer, "#");
  int startAddress = atoi(ptr);
  // Serial.println(startAddress);
  ptr = strtok(NULL, "#");
  int numberOfCoils = atoi(ptr);
  // Serial.println(numberOfCoils);

   bool coilData[numberOfCoils];

  if (mb.isConnected(modbusServer))
  {
    Serial.println("Reading data from modbus server");
    for (size_t i = 0; i < numberOfCoils; i++)
    {
      mb.readCoil(modbusServer, startAddress+i, &coilData[i]);
      delay(50);
      mb.task();
    }
    
  }
  else
  {
    Serial.println("Connecting to modbus server");
    mb.connect(modbusServer);
    for (size_t i = 0; i < numberOfCoils; i++)
    {
      mb.readCoil(modbusServer, startAddress+i, &coilData[i]);
      delay(50);
      mb.task();
    }
  }
  delay(50);
  mb.task();
  Serial.printf("Got request to read %d coils starting at address %d \nI found these values : ", numberOfCoils, startAddress);
  for (size_t i = 0; i < numberOfCoils; i++)
  {
    Serial.printf("%d ", coilData[i]);
  }
  

  return 0;
}

int readHoldingRegisters(IPAddress modbusServer, char *dataBuffer)
{
  // Incoming message should look like this: {startAddress}#{numberOfAddresses}
  ptr = strtok(dataBuffer, "#");
  int startAddress = atoi(ptr);
  ptr = strtok(NULL, "#");
  int numberOfAddresses = atoi(ptr);
  uint16_t registerData[numberOfAddresses];


  if (mb.isConnected(modbusServer))
  {
    Serial.println("Reading data from modbus server");
    for (size_t i = 0; i < numberOfAddresses; i++)
    {
      mb.readHreg(modbusServer, startAddress+i, &registerData[i]);
      delay(50);
      mb.task();
    }
    
  }
  else
  {
    Serial.println("Connecting to modbus server");
    mb.connect(modbusServer);
    for (size_t i = 0; i < numberOfAddresses; i++)
    {
      mb.readHreg(modbusServer, startAddress+i, &registerData[i]);
      delay(50);
      mb.task();
    }
  }
  delay(50);
  mb.task();
  Serial.printf("Got request to read %d holding registers starting at address %d \nI found these values : ", numberOfAddresses, startAddress);
  for (size_t i = 0; i < numberOfAddresses; i++)
  {
    Serial.printf("%d ", registerData[i]);
  }
  

  return 0;
}

int readInternalRegisters(IPAddress modbusServer, char *dataBuffer)
{
  // Incoming message should look like this: "{startAddress}#{numberOfAddresses}"
  ptr = strtok(dataBuffer, "#");
  int startAddress = atoi(ptr);
  ptr = strtok(NULL, "#");
  int numberOfAddresses = atoi(ptr);
  uint16_t registerData[numberOfAddresses];

if (mb.isConnected(modbusServer))
  {
    Serial.println("Reading data from modbus server");
    for (size_t i = 0; i < numberOfAddresses; i++)
    {
      mb.readIreg(modbusServer, startAddress+i, &registerData[i]);
      delay(50);
      mb.task();
    }
    
  }
  else
  {
    Serial.println("Connecting to modbus server");
    mb.connect(modbusServer);
    for (size_t i = 0; i < numberOfAddresses; i++)
    {
      mb.readIreg(modbusServer, startAddress+i, &registerData[i]);
      delay(50);
      mb.task();
    }
  }
  delay(50);
  mb.task();
  Serial.printf("Got request to read %d input registers starting at address %d \nI found these values : ", numberOfAddresses, startAddress);
  for (size_t i = 0; i < numberOfAddresses; i++)
  {
    Serial.printf("%d ", registerData[i]);
  }
  


  return 0;
}

int writeCoils(IPAddress modbusServer, char *dataBuffer)
{
  ptr = strtok(dataBuffer, "#");
  int startAddress = atoi(ptr);
  ptr = strtok(NULL, ",");
  int numberOfCoils = atoi(ptr);
  bool coilData[numberOfCoils];

  for (size_t i = 0; i < numberOfCoils; i++)
  {
    ptr = strtok(NULL, ",");
    coilData[i] = atoi(ptr)!=0;
  }
  

  if (mb.isConnected(modbusServer))
  {
    Serial.println("Reading data from modbus server");
    for (size_t i = 0; i < numberOfCoils; i++)
    {
      mb.writeCoil(modbusServer, startAddress+i, coilData[i]);
      delay(50);
      mb.task();
    }
    
  }
  else
  {
    Serial.println("Connecting to modbus server");
    mb.connect(modbusServer);
    for (size_t i = 0; i < numberOfCoils; i++)
    {
      mb.writeCoil(modbusServer, startAddress+i, coilData[i]);
      delay(50);
      mb.task();
    }
  }
  delay(50);
  mb.task();
  
}

int writeHoldingRegisters(IPAddress modbusServer, char *dataBuffer)
{
  ptr = strtok(dataBuffer, "#");
  int startAddress = atoi(ptr);
  ptr = strtok(NULL, ",");
  int numberOfRegisters = atoi(ptr);
  uint16_t registerData[numberOfRegisters];

  for (size_t i = 0; i < numberOfRegisters; i++)
  {
    ptr = strtok(NULL, ",");
    registerData[i] = atoi(ptr);
  }
  
  if (mb.isConnected(modbusServer))
  {
    Serial.println("Reading data from modbus server");
    for (size_t i = 0; i < numberOfRegisters; i++)
    {
      mb.writeHreg(modbusServer, startAddress+i, registerData[i]);
      delay(50);
      mb.task();
    }
    
  }
  else
  {
    Serial.println("Connecting to modbus server");
    mb.connect(modbusServer);
    for (size_t i = 0; i < numberOfRegisters; i++)
    {
      mb.writeHreg(modbusServer, startAddress+i, registerData[i]);
      delay(50);
      mb.task();
    }
  }
  delay(50);
  mb.task();

}

int returnBadRequest()
{

  Serial.println("Invalid function code");
  return 1;
}

int resetBuffers()
{
  memset(parsedIP, '\0', sizeOfParsedIP);
  memset(dataBuffer, '\0', sizeOfDataBuffer);
  memset(IPBuffer, '\0', sizeOfIPBuffer);
  memset(functionTypeBuffer, '\0', sizeOfFunctionTypeBuffer);
}