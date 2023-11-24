/***************** SETTINGS *******************/
#define MODBUS_TIMEOUT                   30
#define MODBUS_PORT                      2
#define MODBUS_SLAVE_ID                  1

#define SCRIPT_RESPONSE_SUCCESS          0
#define SCRIPT_RESPONSE_ERROR_MB_MSG     1
#define SCRIPT_RESPONSE_ERROR_MB_ID      2
#define SCRIPT_RESPONSE_ERROR_MB_CMD     3
#define SCRIPT_RESPONSE_ERROR_MB_CRC     4
#define SCRIPT_RESPONSE_ERROR_MB_CNT     5
#define SCRIPT_RESPONSE_ERROR_ARG        6
#define SCRIPT_RESPONSE_ERROR_CNFG       7
/**********************************************/

/****************** COMMANDS ******************/
#define MODBUS_READ_COILS                0x01
#define MODBUS_READ_INPUT_STATUS         0x02
#define MODBUS_READ_HOLDING_REGISTERS    0x03
#define MODBUS_READ_INPUT_REGISTERS      0x04
#define MODBUS_FORCE_SINGLE_COIL         0x05
#define MODBUS_PRESET_SINGLE_REGISTER    0x06
#define MODBUS_FORCE_MULTIPLE_COILS      0x0F
#define MODBUS_PRESET_MULTIPLE_REGISTERS 0x10
/**********************************************/

/*************** MODBUS_SETTINGS **************/
#define MODBUS_REGISTER_CARDS_COUNT_IDX  (0)

#define MODBUS_REGISTER_CF_ID_IDX        (0)
#define MODBUS_REGISTER_DEVICE_ID_IDX    (MODBUS_REGISTER_CF_ID_IDX + 2)
#define MODBUS_REGISTER_CARDS_IDX        (MODBUS_REGISTER_DEVICE_ID_IDX + 2)


#define MODBUS_ERROR_MESSAGE_LEN         5

#define MODBUS_REGISTER_SIZE             100
#define MODBUS_MESSAGE_DATA_SIZE         (2 * (MODBUS_REGISTER_SIZE + 2))
/**********************************************/

new request{MODBUS_MESSAGE_DATA_SIZE}      = { }
new response_data_len                      = 0
new registers_values[MODBUS_REGISTER_SIZE] = { 0 }

#define JANUARY   1
#define FEBRUARY  2
#define MARCH     3
#define APRIL     4
#define MAY       5
#define JUNE      6
#define JULY      7
#define AUGUST    8
#define SEPTEMBER 9
#define OCTOBER   10
#define NOVEMBER  11
#define DECEMBER  12

main()
{
	SetVar(globTime, GetVar(TIME))
    SetVar(globResult, SCRIPT_RESPONSE_SUCCESS)
	SetVar(globTimeDelay, 10000)
    new cards_count = GetVar(globCardsCount)
    if (cards_count <= 0) {
        SetVar(globResult, SCRIPT_RESPONSE_ERROR_CNFG)
        return
    }
    
    new unix_time = GetVar(UNIX_TIME)
    Diagnostics("############## CURRENT TIME: %d ##############", unix_time)
    
    new year = 0
    new month = 0
    new date = 0
    new hour = 0
    new minute = 0
    new second = 0
    
    new tmp_time = unix_time
    
    second = tmp_time % 60
    tmp_time /= 60
    
    minute = tmp_time % 60
    tmp_time /= 60
    
    hour = tmp_time % 24
    tmp_time /= 24
    
    year = 1970
    month = 1
    while (tmp_time > 0)
    {
        new days_in_month = get_days_in_month(year, month)
        if (days_in_month > tmp_time) {
            date = tmp_time + 1
            break;
        }
        if (month == 12) {
            month = 0
            year++
        }
        month++
        tmp_time -= days_in_month
    }
    year %= 100
    
    Diagnostics("############## CURRENT TIME: %d-%d-%dT%d:%d:%d ##############", year, month, date, hour, minute, second)
    
	// Set year
	new val = 0x00000000;
    val += ((year & 0x000000FF) << 8)
    val += ((year & 0x0000FF00) >> 8)
	val = ((val) & 0xFFFF)
    new addr = MODBUS_REGISTER_CARDS_IDX + 5 * cards_count + 2
	Diagnostics("COMMAND: modbus write (%d) - %X", addr, val) 
    modbus_master_preset_single_register(1, addr, val)
    addr++
	
	// Set month
	val = 0x00000000;
    val += ((month & 0x000000FF) << 8)
    val += ((month & 0x0000FF00) >> 8)
	val = ((val) & 0xFFFF)
	Diagnostics("COMMAND: modbus write (%d) - %X", addr, val) 
    modbus_master_preset_single_register(1, addr, val)
    addr++
	
	// Set date
	val = 0x00000000;
    val += ((date & 0x000000FF) << 8)
    val += ((date & 0x0000FF00) >> 8)
	val = ((val) & 0xFFFF)
	Diagnostics("COMMAND: modbus write (%d) - %X", addr, val) 
    modbus_master_preset_single_register(1, addr, val)
    addr++
	
	// Set hour
	val = 0x00000000;
    val += ((hour & 0x000000FF) << 8)
    val += ((hour & 0x0000FF00) >> 8)
	val = ((val) & 0xFFFF)
	Diagnostics("COMMAND: modbus write (%d) - %X", addr, val) 
    modbus_master_preset_single_register(1, addr, val)
    addr++
	
	// Set minute
	val = 0x00000000;
    val += ((minute & 0x000000FF) << 8)
    val += ((minute & 0x0000FF00) >> 8)
	val = ((val) & 0xFFFF)
	Diagnostics("COMMAND: modbus write (%d) - %X", addr, val) 
    modbus_master_preset_single_register(1, addr, val)
    addr++
	
	// Set second
	val = 0x00000000;
    val += ((second & 0x000000FF) << 8)
    val += ((second & 0x0000FF00) >> 8)
	val = ((val) & 0xFFFF)
	Diagnostics("COMMAND: modbus write (%d) - %X", addr, val) 
    modbus_master_preset_single_register(1, addr, val)
	
	SetVar(globTimeDelay, 86400)
}

get_days_in_month(year, month_num)
{
    if (year <= 2) {
        return 0
    }
    if (month_num == JANUARY) {
        return 31
    }
    if (month_num == FEBRUARY) {
        return ((year % 4 == 0) ? 29 : 28)
    }
    if (month_num == MARCH) {
        return 31
    }
    if (month_num == APRIL) {
        return 30
    }
    if (month_num == MAY) {
        return 31
    }
    if (month_num == JUNE) {
        return 30
    }
    if (month_num == JULY) {
        return 31
    }
    if (month_num == AUGUST) {
        return 31
    }
    if (month_num == SEPTEMBER) {
        return 30
    }
    if (month_num == OCTOBER) {
        return 31
    }
    if (month_num == NOVEMBER) {
        return 30
    }
    if (month_num == DECEMBER) {
        return 31
    }
    return 0
}

array8_copy(dest{}, src{}, size)
{
    for (new i = 0; i < size; i++) {
        dest{i} = src{i}
    }
}

array32_copy(dest[], src[], size)
{
    for (new i = 0; i < size; i++) {
        dest[i] = src[i]
    }
}

modbus_recieve_data(start_addr, reg_count)
{
    new data[MODBUS_REGISTER_SIZE]       = { 0 }
    new empty_data[MODBUS_REGISTER_SIZE] = { 0 }
    new rec_counter                      = 0
    
    array32_copy(registers_values, empty_data, MODBUS_REGISTER_SIZE)
    
    new byte = 0
    while (PortRead(MODBUS_PORT, byte, MODBUS_TIMEOUT)) {
        data{rec_counter} = byte
        rec_counter++
    }
    
    if (rec_counter <= MODBUS_ERROR_MESSAGE_LEN) {
        SetVar(globResult, SCRIPT_RESPONSE_ERROR_MB_MSG)
        Diagnostics("COMMAND: moddbus receive data error - unexceptable message")
        array32_copy(registers_values, empty_data, MODBUS_REGISTER_SIZE)
        return
    }
    
    new counter = 0
    if (data{counter} != request{counter}) {
        SetVar(globResult, SCRIPT_RESPONSE_ERROR_MB_ID)
        Diagnostics("COMMAND: moddbus receive data error - unexceptable device ID")
        array32_copy(registers_values, empty_data, MODBUS_REGISTER_SIZE)
        return
    }
    counter++
    
    new command = data{counter}
    if (command != request{counter}) {
        SetVar(globResult, SCRIPT_RESPONSE_ERROR_MB_CMD)
        Diagnostics("COMMAND: moddbus receive data error - wrong modbus command")
        array32_copy(registers_values, empty_data, MODBUS_REGISTER_SIZE)
        return
    }
    counter++
    
    response_data_len = 0
    if (_mb_ms_is_read_command(command)) {
        response_data_len = data{counter}
        counter++
    } else if (_mb_ms_is_write_single_reg_command(command) || _mb_ms_is_write_multiple_reg_command(command)) {
        response_data_len = 0
    } else {
        SetVar(globResult, SCRIPT_RESPONSE_ERROR_MB_CMD)
        Diagnostics("COMMAND: moddbus receive data error - unexceptable command")
        array32_copy(registers_values, empty_data, MODBUS_REGISTER_SIZE)
        return
    }
    
    new reg_addr = 0
    if (response_data_len == 0) {
        reg_addr = (data{counter} << 8) + data{counter + 1}
        counter += 2
    }
    
    
    new data_counter = 0
    new response_data[MODBUS_REGISTER_SIZE] = [ 0 ]
    while (data_counter < _mb_ms_get_response_bytes_count(command)) {
        response_data[data_counter / 2] = (data{counter} << 8) + data{counter + 1}
        counter      += 2
        data_counter += 2
    }
    
    new crc     = (data{counter} << 8) + data{counter + 1}
    new tmp_crc = CRC16(data, counter)
    tmp_crc = ((tmp_crc << 8) + (tmp_crc >> 8)) & 0xFFFF
    if (crc != tmp_crc) {
        SetVar(globResult, SCRIPT_RESPONSE_ERROR_MB_CRC)
        Diagnostics("COMMAND: moddbus receive data error - unexceptable command")
        array32_copy(registers_values, empty_data, MODBUS_REGISTER_SIZE)
        return
    }
    
    array32_copy(registers_values, response_data, MODBUS_REGISTER_SIZE)
    Diagnostics("COMMAND: moddbus receive data success")
    for (new i = 0; i < reg_count; i++) {
        new val = 0x00000000
        val += ((registers_values[i] >> 8)  & 0x000000FF)
        val += ((registers_values[i] << 8)  & 0x0000FF00)
        Diagnostics("COMMAND: register %d - %d (0x%X)", start_addr + i, val, val)
    }
}

modbus_master_read_input_registers(slave_id, reg_addr, reg_count)
{
    _mb_ms_send_simple_message(slave_id, MODBUS_READ_INPUT_REGISTERS, reg_addr, reg_count)
    modbus_recieve_data(reg_addr, reg_count)
}

modbus_master_preset_single_register(slave_id, reg_addr, reg_val)
{
    _mb_ms_send_simple_message(slave_id, MODBUS_PRESET_SINGLE_REGISTER, reg_addr, reg_val)
    modbus_recieve_data(reg_addr, 1)
}

modbus_master_read_holding_registers(slave_id, reg_addr, reg_count)
{
    _mb_ms_send_simple_message(slave_id, MODBUS_READ_HOLDING_REGISTERS, reg_addr, reg_count)
    modbus_recieve_data(reg_addr, reg_count)
}

_mb_ms_send_simple_message(slave_id, command, reg_addr, spec_data)
{
    new counter = 0
    
    request{counter++} = slave_id

    request{counter++} = command

    request{counter++} = (reg_addr >> 8)
    request{counter++} = (reg_addr)

    request{counter++} = (spec_data >> 8)
    request{counter++} = (spec_data)

    new crc = CRC16(request, counter)
    request{counter++} = (crc)
    request{counter++} = (crc >> 8)

    send(request, counter)
}

send(data{}, len)
{
    PortWrite(MODBUS_PORT, data, len)
}

_mb_ms_is_read_command(command)
{
    return command == MODBUS_READ_COILS || command == MODBUS_READ_HOLDING_REGISTERS || command == MODBUS_READ_INPUT_REGISTERS || command == MODBUS_READ_INPUT_STATUS
}

_mb_ms_is_write_single_reg_command(command)
{
    return command == MODBUS_FORCE_SINGLE_COIL || command == MODBUS_PRESET_SINGLE_REGISTER
}

_mb_ms_is_write_multiple_reg_command(command)
{
    return command == MODBUS_FORCE_MULTIPLE_COILS || command == MODBUS_PRESET_MULTIPLE_REGISTERS
}

_mb_ms_get_response_bytes_count(command)
{
    if (!_mb_ms_is_read_command(command)) {
        return 2
    }
    if (_mb_ms_is_read_command(command)) {
        return response_data_len
    }
    SetVar(globResult, SCRIPT_RESPONSE_ERROR_MB_CNT)
    Diagnostics("COMMAND: moddbus receive data error - wrong registers bytes count")
    return 0
}