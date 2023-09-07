<p align="center">
  <h1 align="center">Gas station</h1>
</p>

Gas station

# Modbus registers

Field        | Table                           | Start reg | Reg count | Unit size | Units count | R/W | Description
 ---         |  ---                            |    ---    |    ---    |    ---    | ---         | --- | ---
cf_id        | Analog Output Holding Registers | 0         | 2         | 4 bytes   | 1           | R/W | Current config version
device_id    | Analog Output Holding Registers | 2         | 8         | 1 byte    | 16          | R/W | Uniq device ID
cards        | Analog Output Holding Registers | 10        | 40        | 4 bytes   | 20          | R/W | Users cards which are uses on this device
cards_values | Analog Output Holding Registers | 50        | 40        | 4 bytes   | 20          | R/W | Users cards gas value llimit
log_id       | Analog Output Holding Registers | 90        | 2         | 4 bytes   | 1           | R/W | Max log ID number found on server
year         | Analog Output Holding Registers | 92        | 1         | 2 bytes   | 1           | R/W | Current year
month        | Analog Output Holding Registers | 93        | 1         | 2 bytes   | 1           | R/W | Current month
day          | Analog Output Holding Registers | 94        | 1         | 2 bytes   | 1           | R/W | Current day
hour         | Analog Output Holding Registers | 95        | 1         | 2 bytes   | 1           | R/W | Current hour
minute       | Analog Output Holding Registers | 96        | 1         | 2 bytes   | 1           | R/W | Current minute
second       | Analog Output Holding Registers | 97        | 1         | 2 bytes   | 1           | R/W | Current second
log_id       | Analog Input Registers          | 0         | 2         | 4 bytes   | 1           |  R  | Current log ID loaded to the registers
time         | Analog Input Registers          | 2         | 3         | 1 byte    | 6           |  R  | Log record time
cf_id        | Analog Input Registers          | 5         | 2         | 4 bytes   | 1           |  R  | Config version that was used when the record was made
card         | Analog Input Registers          | 7         | 2         | 4 bytes   | 1           |  R  | Card that was used for access
used_liters  | Analog Input Registers          | 9         | 2         | 4 bytes   | 1           |  R  | Gas value that was used when the record was made