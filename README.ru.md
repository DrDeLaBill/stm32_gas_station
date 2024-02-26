<p align="center">
  <h1 align="center">Gas station</h1>
</p>

## Language
- [English](README.md)
- [Русский](README.ru.md)

## Описание

Gas station - это модуль управления заправочной станцией (или ТРК - топливно-раздаточная колонка), предназначенный  для использования внутри организации, позволяющий сотрудникам иметь доступ к получению топлива по личному ключу доступа и ведущий учёт выдачи топлива для каждого ключа.

Порядок работы:
1. После подачи питания, устройство переходит в режим ожидания ключа доступа
2. После поднесения ключа доступа к считывателю, если уникальный код ключа доступа зарегистрирован в памяти устройства, на дисплее появится надпись ACCESS, а затем на дисплее будет отображён текущий доступный лимит для пользователя, если же код ключа доступа не зарегистрирован в памяти устройства, то на дисплее появится надпись DENIED, и устройство перейдёт в режим ожидания.
3. Если предыдущее действие успешно, то на дисплее будет отображён текущий лимит для карты, а именно раз в секунду будет появляться надпись LIMIT, а её будет сменять остаток для данной карты доступа на текущий период в литрах
4. Во время отображения лимита пользователь может ввести количество топлива необходимое ему для заправки с помощью клавиатуры (но не больше значения лимита)
5. После успешного ввода необходимого количества топлива, необходимо нажать кнопку старт или # на клавиатуре
6. Затем пистолет ТРК снимается с корпуса устройства, после чего запускается насос и открываются два клапана, а также начинается подсчёт выданного количества топлива
7. Чтобы началась подача топлива, необходимо нажать рычаг пистолета (если потребление топлива не начнётся в течение двух минут после старта насоса, устройство выключит насос и закроет клапаны, после чего заблокирует доступ для карты до следующего сеанса)
8. В любой момент работы сеанс может быть прерван нажатием кнопки отмена или * на клавиатуре
9. Если устройству остаётся выдать меньше 1 литра топлива, оно закрывает один из клапанов для более медленной подачи
10. При нажатии отмены или после выдачи необходимого количества топлива сеанс работы завершается: выключается насос, закрывается второй клапан, а на экране отображается результат сеанса - количество выданного топлива
11. Чтобы начать новый сеанс работы необходимо заново поднести ключ доступа

Взаимодействие с модулем происходит посредством использования терминала GALILEOSKY. Каждую запись журнала устройства терминал сохраняет как новую точку с пользовательскими тегами:
* пользовательский тег 1 - id записи
* пользовательский тег 2 - время создания записи журнала в секундах (отсчитывается с начала века)
* пользовательский тег 3 - количество выданного топлива в миллилитрах
* пользовательский тег 4 - номер использованной карты доступа

Чтобы изменять настройки ТРК, используются следующие команды, отправляемые на терминал GALILEOSKY:
* ```GETCARD <idx>``` - Получить код карты по индексу
* ```SETCARD <idx>,<value>``` - Записать новый код карты по индексу 
* ```GETLIMIT <idx>``` - Получить лимит для карты по индексу
* ```SETLIMIT <idx>,<value>``` - Записать новый лимит для карты по индексу 
* ```GETLIMITTYPE <idx>``` - Получить тип лимита для карты по индексу 
* ```SETLIMITTYPEDAY <idx>``` - Записать тип лимита - день, для карты по индексу 
* ```SETLIMITTYPEMONTH <idx>``` - Записать тип лимита - месяц, для карты по индексу 
* ```CLEARLIMIT <idx>``` - Обнулить затраченное количество топлива для текущего периода по индексу карты 
* ```GETID``` - Получить уникальный id устройства 
* ```SETID <value>``` - Записать новый id устройства 
* ```GETLOGID``` - Получить id последней отправленной на сервер записи журнала 
* ```SETLOGID <log_id>``` - Сбросить id последней отправленной на сервер записи журнала до log_id 
* ```GETNEXTLOG``` - Получить текущую запись журнала, записанную в modbus регистрах 
* ```GETCONFVER``` - Получить текущую версию конфигурации устройства 
* ```SETCONFVER <value>``` - записать новую версию конфигурации устройства 

## Modbus регистры

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