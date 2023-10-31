<p align="center">
  <h1 align="center">Gas station</h1>
</p>

Gas station

# Modbus registers

Field        | Table                           | Start reg | Reg count | Unit size | Units count | R/W | Description
 ---         |  ---                            |    ---    |    ---    |    ---    | ---         | --- | ---
cf_id        | Analog Output Holding Registers | 0         | 2         | 4 bytes   | 1           | R/W | Current config version
device_id    | Analog Output Holding Registers | 2         | 2         | 4 bytes   | 1           | R/W | Uniq device ID
cards        | Analog Output Holding Registers | 4         | 40        | 4 bytes   | 20          | R/W | Users cards which are uses on this device
cards_values | Analog Output Holding Registers | 44        | 40        | 4 bytes   | 20          | R/W | Users cards gas value llimit
log_id       | Analog Output Holding Registers | 84        | 2         | 4 bytes   | 1           | R/W | Max log ID number found on server
year         | Analog Output Holding Registers | 86        | 1         | 2 bytes   | 1           | R/W | Current year
month        | Analog Output Holding Registers | 87        | 1         | 2 bytes   | 1           | R/W | Current month
day          | Analog Output Holding Registers | 88        | 1         | 2 bytes   | 1           | R/W | Current day
hour         | Analog Output Holding Registers | 89        | 1         | 2 bytes   | 1           | R/W | Current hour
minute       | Analog Output Holding Registers | 90        | 1         | 2 bytes   | 1           | R/W | Current minute
second       | Analog Output Holding Registers | 91        | 1         | 2 bytes   | 1           | R/W | Current second
cards_cont   | Analog Input Registers          | 0         | 1         | 2 bytes   | 1           |  R  | Max user cards count
log_id       | Analog Input Registers          | 1         | 2         | 4 bytes   | 1           |  R  | Current log ID loaded to the registers
log_year     | Analog Input Registers          | 3         | 1         | 2 bytes   | 1           | R/W | Current year
log_month    | Analog Input Registers          | 4         | 1         | 2 bytes   | 1           | R/W | Current month
log_day      | Analog Input Registers          | 5         | 1         | 2 bytes   | 1           | R/W | Current day
log_hour     | Analog Input Registers          | 6         | 1         | 2 bytes   | 1           | R/W | Current hour
log_minute   | Analog Input Registers          | 7         | 1         | 2 bytes   | 1           | R/W | Current minute
log_second   | Analog Input Registers          | 8         | 1         | 2 bytes   | 1           | R/W | Current second
cf_id        | Analog Input Registers          | 9         | 2         | 4 bytes   | 1           |  R  | Config version that was used when the record was made
card         | Analog Input Registers          | 11        | 2         | 4 bytes   | 1           |  R  | Card that was used for access
used_liters  | Analog Input Registers          | 13        | 2         | 4 bytes   | 1           |  R  | Gas value that was used when the record was made