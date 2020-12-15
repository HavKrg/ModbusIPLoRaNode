# ModbusIPLoRaNode

A project that is intented to receive a "request message" over LoRa to either read or write data from/to a PLC using modbusIP.



The "request message" needs to follow this format:

#{functionNumber}#{IPAddress}#{startAddress}#{numberOfAddresses},{value1},{value2}...{valueN}\n

Where:

functionNumber = modbus function 1, 3, 4, 5, 6, 15 or 16
IPAddress = IP address of the modbus server
startAddress = modbus address of the first value you want to read/write
numberOfAddresses = the number of values you want to read/write
value1 = the value you want to write to the startAddress
value2 = the value you want to write to startAddress+1
...
valueN = the value you want to write to startAddress+N


## Example 1 read 4 coils starting at address 0: 

"#1#10,0,0,180#0#4\n"

## Example 2 read 5 holding registers starting at address 0:

"#3#10,0,0,180#0#5\n"

## Example 3 write 4 coils starting at address 100:

"#15#10,0,0,180#100#4,0,0,0,0\n"

## Example 4 write 5 holding registers starting att address 100:

"#16#10,0,0,180#100#5,1000,1001,1002,1003,1004\n"


Note that it can only read/write consecutive addresses (100, 101, 102, 103 etc.). If you want to read something from none-consecutive addresses like  100, 105, 110, you need to modify the code or send multiple request.

You should also be carefull not to read/write too many values with each request so that you can continue to be a "good neighbour" on the LoRa spectrum. 

Thanks to 
https://github.com/emelianov/modbus-esp8266/
http://www.airspayce.com/mikem/arduino/RadioHead/

