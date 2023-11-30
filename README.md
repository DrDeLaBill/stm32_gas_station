<p align="center">
  <h1 align="center">Gas station</h1>
</p>

## Language
- [English](README.md)
- [Русский](README.ru.md)

## Description

Gas station is a fuel transfer column control module. It is used within the organization for control employees access to getting fuel using private access card, and for control each card fuel consumption.

Operating procedure:
1. After enabling power supply the device goes into access card wait mode
2. After the access card is brought to the reader, if the card code is registered in the device memory the ACCESS label appears on the display, and then the fuel limit of the current user. If the card code isn't registered in the device memory the DENIED label appears on the display and and the device goes into access card wait mode.
3. If the previous action is successful, the limit for the current card will be appears on the display (the LIMIT label will appear once a second and it will be replaced be the reminder for the current card for the current period in liters)
4. While the limit is displays the user can enter needed fuel amount using the keyboard (but not more then the reminder value)
5. After successful entering needed fuel amount, it's to necessary press the start button or # on the keyboard
6. Then the gas station gun is got from the device body, the pump starts and two valves open, and the calculation of the issued fuel amount begins.
7. To start the fuel supply, it's needed to pressing the lever of the gas station gun (if the fuel consumption does not start within 2 minutes, the device will disable the pump and close the valves, after that it will block card access for current session)
8. At any time of the session the device can be stopped by pressing stop button or * on the keyboard
9. If the device has to give out less than 1 liter of the fuel it closes one of the valves for the a slower supply
10. With the cancel pressing or after issuing needed fuel amount the current session is ended: the pump disables, the second valve closes and the result fuel amount appears on the display
11. To start a new session, you need to bring the access card again

## Modbus registers

Field        | Table                           | Start reg | Reg count | Unit size | Units count | R/W | Description
 ---         |  ---                            |    ---    |    ---    |    ---    | ---         | --- | ---
cf_id        | Analog Output Holding Registers | 0         | 2         | 4 bytes   | 1           | R/W | Current config version
device_id    | Analog Output Holding Registers | 2         | 2         | 4 bytes   | 1           | R/W | Uniq device ID
cards        | Analog Output Holding Registers | 4         | 40        | 4 bytes   | 20          | R/W | Users cards which are uses on this device
limits       | Analog Output Holding Registers | 44        | 40        | 4 bytes   | 20          | R/W | Card limits
limit_types  | Analog Output Holding Registers | 84        | 20        | 2 bytes   | 20          | R/W | Card limit types
log_id       | Analog Output Holding Registers | 104       | 2         | 4 bytes   | 1           | R/W | Max log ID number found on server
clear_limit  | Analog Output Holding Registers | 106       | 1         | 2 bytes   | 1           | R/W | Flag that allows clearing used liters
clear_id     | Analog Output Holding Registers | 107       | 2         | 4 bytes   | 1           | R/W | Card id for clear used liters
year         | Analog Output Holding Registers | 109       | 1         | 2 bytes   | 1           | R/W | Current year
month        | Analog Output Holding Registers | 110       | 1         | 2 bytes   | 1           | R/W | Current month
day          | Analog Output Holding Registers | 111       | 1         | 2 bytes   | 1           | R/W | Current day
hour         | Analog Output Holding Registers | 112       | 1         | 2 bytes   | 1           | R/W | Current hour
minute       | Analog Output Holding Registers | 113       | 1         | 2 bytes   | 1           | R/W | Current minute
second       | Analog Output Holding Registers | 114       | 1         | 2 bytes   | 1           | R/W | Current second
cards_cont   | Analog Input Registers          | 0         | 1         | 2 bytes   | 1           |  R  | Max user cards count
log_id       | Analog Input Registers          | 1         | 2         | 4 bytes   | 1           |  R  | Log id of the record
log_year     | Analog Input Registers          | 3         | 1         | 2 bytes   | 1           |  R  | Record year
log_month    | Analog Input Registers          | 4         | 1         | 2 bytes   | 1           |  R  | Record month
log_day      | Analog Input Registers          | 5         | 1         | 2 bytes   | 1           |  R  | Record day
log_hour     | Analog Input Registers          | 6         | 1         | 2 bytes   | 1           |  R  | Record hour
log_minute   | Analog Input Registers          | 7         | 1         | 2 bytes   | 1           |  R  | Record minute
log_second   | Analog Input Registers          | 8         | 1         | 2 bytes   | 1           |  R  | Record second
card         | Analog Input Registers          | 9         | 2         | 4 bytes   | 1           |  R  | Card that was used for access
used_liters  | Analog Input Registers          | 11        | 2         | 4 bytes   | 1           |  R  | Gas value that was used when the record was made