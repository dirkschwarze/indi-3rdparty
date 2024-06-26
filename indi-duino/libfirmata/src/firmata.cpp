/*
 * Copyright 2012 (c) Nacho Mas (mas.ignacio at gmail.com)

   Base on the following works:
	* Firmata GUI example. http://www.pjrc.com/teensy/firmata_test/
	  Copyright 2010, Paul Stoffregen (paul at pjrc.com)

	* firmataplus: http://sourceforge.net/projects/firmataplus/
	  Copyright (c) 2008 - Scott Reid, dataczar.com

   Firmata C++ library. 
*/

#include <firmata.h>
#include <string.h>
#include <stdlib.h>
#include <ctime>

void (*firmata_debug_cb)(const char *file, int line, const char *msg, ...) = NULL;

#define LOG_DEBUG(msg) {if (firmata_debug_cb) firmata_debug_cb(__FILE__, __LINE__, msg);}
#define LOGF_DEBUG(msg, ...) {if (firmata_debug_cb) firmata_debug_cb(__FILE__, __LINE__, msg, __VA_ARGS__);}

Firmata::Firmata()
{
    init("/dev/ttyACM0", FIRMATA_DEFAULT_BAUD);
}

Firmata::Firmata(const char *_serialPort)
{
    init(_serialPort, FIRMATA_DEFAULT_BAUD);
}


Firmata::Firmata(const char *_serialPort, uint32_t baud)
{
    init(_serialPort, baud);
}

Firmata::Firmata(int fd)
{
    init(fd);
}

Firmata::~Firmata()
{
    delete arduino;
}

int Firmata::updateDigitalPort(unsigned char pin, unsigned char mode)
{
    int bit = pin % 8;
    int port = pin / 8;

    if (port >= ARDUINO_DIG_PORTS)
    {
        return (-2);
    }
    // set the bit
    if (mode == ARDUINO_HIGH)
        digitalPortValue[port] |= (1 << bit);

    // clear the bit
    else if (mode == ARDUINO_LOW)
        digitalPortValue[port] &= ~(1 << bit);

    else
    {
        LOGF_DEBUG("Firmata::writeDigitalPin():invalid mode: %d", mode);
        return (-1);
    }
    return port;
}

int Firmata::writeDigitalPin(unsigned char pin, unsigned char mode)
{
    int rv = 0;
    int port;

    port = updateDigitalPort(pin, mode);

    if (port < 0) return port;

    rv |= arduino->sendUchar(FIRMATA_DIGITAL_MESSAGE + port);
    rv |= sendValueAsTwo7bitBytes(digitalPortValue[port]); //ARDUINO_HIGH OR ARDUINO_LOW
    LOGF_DEBUG("Sending DIGITAL_MESSAGE pin:%d, mode:%d, port:%d, port_val:%02X", pin, mode, port, digitalPortValue[port]);
    return (rv);
}

// in Firmata (and MIDI) data bytes are 7-bits. The 8th bit serves as a flag to mark a byte as either command or data.
// therefore you need two data bytes to send 8-bits (a char).
int Firmata::sendValueAsTwo7bitBytes(int value)
{
    int rv = 0;
    rv |= arduino->sendUchar(value & 127);      // LSB
    rv |= arduino->sendUchar(value >> 7 & 127); // MSB
    return rv;
}

int Firmata::setSamplingInterval(int16_t value)
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_SAMPLING_INTERVAL);
    rv |= arduino->sendUchar((unsigned char)(value % 128));
    rv |= arduino->sendUchar((unsigned char)(value >> 7));
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    LOGF_DEBUG("Sending SAMPLING_INTERVAL value:%d", value);
    return (rv);
}

int Firmata::setPinMode(unsigned char pin, unsigned char mode)
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_SET_PIN_MODE);
    rv |= arduino->sendUchar(pin);
    rv |= arduino->sendUchar(mode);
    LOGF_DEBUG("Sending SET_PIN_MODE pin:%d, mode:%d", pin, mode);
    usleep(1000);
    rv |= askPinStateWaitForReply(pin);
    return (rv);
}

int Firmata::setPwmPin(unsigned char pin, uint16_t value)
{
    int rv = 0;
    if ((pin <= 0xf) && (value <= 0x3fff))
    {
        LOGF_DEBUG("Sending ANALOG_MESSAGE pin:%d, value:%d", pin, value);
        rv |= arduino->sendUchar(FIRMATA_ANALOG_MESSAGE + pin);
        rv |= arduino->sendUchar((unsigned char)(value % 128));
        rv |= arduino->sendUchar((unsigned char)(value >> 7));
    }
    else
    {
        LOGF_DEBUG("Sending EXTENDED_ANALOG pin:%d, value:%lu", pin, value);
        rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
        rv |= arduino->sendUchar(FIRMATA_EXTENDED_ANALOG);
        rv |= arduino->sendUchar(pin & 0x7f);
         rv |= arduino->sendUchar(value & 0x7f);
         value >>= 7;
         while (value)
         {
             rv |= arduino->sendUchar(value & 0x7f);
             value >>= 7;
         }
         rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    }
    return (rv);
}
int Firmata::mapAnalogChannels()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_ANALOG_MAPPING_QUERY); // read firmata name & version
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    LOG_DEBUG("Sending ANALOG_MAPPING_QUERY");
    return (rv);
}

int Firmata::askFirmwareVersion()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_REPORT_FIRMWARE); // read firmata name & version
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    LOG_DEBUG("Sending REPORT_FIRMWARE");
    return (rv);
}

int Firmata::askCapabilities()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_CAPABILITY_QUERY);
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    LOG_DEBUG("Sending CAPABILITY_QUERY");
    return (rv);
}

int Firmata::askPinState(int pin)
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_PIN_STATE_QUERY);
    rv |= arduino->sendUchar(pin);
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    LOGF_DEBUG("Sending PIN_STATE_QUERY pin:%d", pin);
    rv |= usleep(1000);
    rv |= OnIdle();
    return (rv);
}

int Firmata::reportDigitalPorts(int enable)
{
    int rv = 0;
    for (int i = 0; i < 16; i++)
    {
        rv |= arduino->sendUchar(FIRMATA_REPORT_DIGITAL | i); // report analog
        rv |= arduino->sendUchar(enable);
        LOGF_DEBUG("Sending REPORT_DIGITAL port:%d, enable:%d", i, enable);
    }
    return (rv);
}

int Firmata::reportAnalogPorts(int enable)
{
    int rv = 0;
    for (int i = 0; i < 16; i++)
    {
        rv |= arduino->sendUchar(FIRMATA_REPORT_ANALOG | i); // report analog
        rv |= arduino->sendUchar(enable);
        LOGF_DEBUG("Sending REPORT_ANALOG pin: A%d, enable:%d", i, enable);
    }
    return (rv);
}

int Firmata::systemReset()
{
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_SYSTEM_RESET);
    LOG_DEBUG("Sending SYSTEM_RESET");
    return (rv);
}

int Firmata::closePort()
{
    if (arduino->closePort() < 0)
    {
        LOGF_DEBUG("Firmata::closePort():arduino->closePort():%s", strerror(errno));
        portOpen = 0;
        return (-1);
    }
    return (0);
}

int Firmata::flushPort()
{
    if (arduino->flushPort() < 0)
    {
        LOGF_DEBUG("Firmata::flushPort():arduino->flushPort():%s", strerror(errno));
        return (-1);
    }
    return (0);
}

int Firmata::sendStringData(char *data)
{
    //TODO Testting
    int rv = 0;
    rv |= arduino->sendUchar(FIRMATA_START_SYSEX);
    rv |= arduino->sendUchar(FIRMATA_STRING_DATA);
    for (unsigned int i = 0; i < strlen(data); i++)
    {
        rv |= sendValueAsTwo7bitBytes(data[i]);
    }
    rv |= arduino->sendUchar(FIRMATA_END_SYSEX);
    LOGF_DEBUG("Sending STRING_DATA: %s", data);
    return (rv);
}

int Firmata::askPinStateWaitForReply(int pin)
{
    OnIdle();
    pin_info[pin].mode = 0xff;
    for (int i = 0; i < 100; i++) { // 1s
        if (i % 10 == 0) askPinState(pin); // try again every 0.1 second
        OnIdle(); // 10ms
        if (pin_info[pin].mode != 0xff) break;
    }
    if (pin_info[pin].mode == 0xff) {
        pin_info[pin].mode = FIRMATA_MODE_INPUT;
        return -1;
    }
    return 0;
}

int Firmata::init(const char *_serialPort, uint32_t baud)
{
    arduino  = new Arduino();
    portOpen = 0;
    if (arduino->openPort(_serialPort, baud) != 0)
    {
        LOGF_DEBUG("sf->openPort(%s) failed: exiting", _serialPort);
        return 1;
    }
    return handshake();
}

int Firmata::init(int fd)
{
    arduino  = new Arduino();
    portOpen = 0;
    if (arduino->openPort(fd) != 0)
    {
        LOGF_DEBUG("sf->openPort(%d) failed: exiting", fd);
        return 1;
    }
    return handshake();
}

int Firmata::handshake()
{
    firmata_name[0] = 0;

    for (int i = 0; i < 3000; i++) // 30s
    {
        if (i % 50 == 0) askFirmwareVersion(); // try again every 0.5s
        OnIdle(); // wait 10ms
        if (strlen(firmata_name) > 0) break;
    }

    if (strlen(firmata_name) == 0) return 1;

    char *requested_name = getenv("INDIDUINO_CHECK_FIRMWARE");
    if (requested_name && strcmp(firmata_name, requested_name) != 0) {
        LOGF_DEBUG("reported firmware '%s' does not match requested '%s'", firmata_name, requested_name);
        return 1;
    }
    portOpen = 1;
    return 0;
}

int Firmata::initState()
{
    string_buffer[0] = 0;

    for (int i = 0; i < ARDUINO_DIG_PORTS; i++)
        digitalPortValue[i] = 0;

    for (int i = 0; i < 100; i++) {
        if (i % 20 == 0) askCapabilities(); // try again every 0.2s
        OnIdle(); // 10ms
        if (have_capabilities) break;
    }

    for (int i = 0; i < 100; i++) {
        if (i % 20 == 0) mapAnalogChannels(); // try again every 0.2s
        OnIdle(); // 10ms
        if (have_analog_mapping) break;
    }

    for (int pin = 0; pin < 128; pin++)
    {
        if (pin_info[pin].supported_modes == 0) continue;
        askPinStateWaitForReply(pin);
    }

    return 0;
}

void Firmata::Parse(const uint8_t *buf, int len)
{
    const uint8_t *p, *end;

    p   = buf;
    end = p + len;
    for (p = buf; p < end; p++)
    {
        uint8_t msn = *p & 0xF0;
        if (msn == 0xE0 || msn == 0x90 || *p == 0xF9)
        {
            parse_command_len = 3;
            parse_count       = 0;
        }
        else if (msn == 0xC0 || msn == 0xD0)
        {
            parse_command_len = 2;
            parse_count       = 0;
        }
        else if (*p == FIRMATA_START_SYSEX)
        {
            parse_count       = 0;
            parse_command_len = sizeof(parse_buf);
        }
        else if (*p == FIRMATA_END_SYSEX)
        {
            parse_command_len = parse_count + 1;
        }
        else if (*p & 0x80)
        {
            parse_command_len = 1;
            parse_count       = 0;
        }
        if (parse_count < (int)sizeof(parse_buf))
        {
            parse_buf[parse_count++] = *p;
        }
        if (parse_count == parse_command_len)
        {
            DoMessage();
            parse_count = parse_command_len = 0;
        }
    }
}

void Firmata::DoMessage(void)
{
    uint8_t cmd = (parse_buf[0] & 0xF0);

    LOGF_DEBUG("Firmata message, %d bytes, %02X", parse_count, parse_buf[0]);

    if (cmd == FIRMATA_ANALOG_MESSAGE && parse_count == 3)
    {
        int analog_ch  = (parse_buf[0] & 0x0F);
        int analog_val = parse_buf[1] | (parse_buf[2] << 7);
        for (int pin = 0; pin < 128; pin++)
        {
            if (pin_info[pin].analog_channel == analog_ch)
            {
                pin_info[pin].value = analog_val;
                LOGF_DEBUG("ANALOG_MESSAGE: pin %d is A%d = %d", pin, analog_ch, analog_val);
                return;
            }
        }
        return;
    }
    if (cmd == FIRMATA_DIGITAL_MESSAGE && parse_count == 3)
    {
        int port_num = (parse_buf[0] & 0x0F);
        int port_val = parse_buf[1] | (parse_buf[2] << 7);
        int pin      = port_num * 8;
        LOGF_DEBUG("DIGITAL_MESSAGE: port_num = %d, port_val = %d", port_num, port_val);
        for (int mask = 1; mask & 0xFF; mask <<= 1, pin++)
        {
            if (pin_info[pin].mode == FIRMATA_MODE_INPUT)
            {
                uint32_t val = (port_val & mask) ? 1 : 0;
                if (pin_info[pin].value != val)
                {
                    LOGF_DEBUG("pin %d is %d", pin, val);
                    pin_info[pin].value = val;
                }
            }
        }
        return;
    }

    if (parse_buf[0] == FIRMATA_START_SYSEX && parse_buf[parse_count - 1] == FIRMATA_END_SYSEX)
    {
        // Sysex message
        LOGF_DEBUG("Firmata Sysex message, %02X", parse_buf[1]);
        if (parse_buf[1] == FIRMATA_REPORT_FIRMWARE)
        {
            char name[140];
            int len = 0;
            for (int i = 4; i < parse_count - 2; i += 2)
            {
                name[len++] = (parse_buf[i] & 0x7F) | ((parse_buf[i + 1] & 0x7F) << 7);
            }
            name[len++] = '-';
            name[len++] = parse_buf[2] + '0';
            name[len++] = '.';
            name[len++] = parse_buf[3] + '0';
            name[len++] = 0;
            LOGF_DEBUG("FIRMWARE:%s", name);
            if (strlen(firmata_name) == 0) {
                strcpy(firmata_name, name);
                time(&version_reply_time);
            }
            else
                if (strcmp(firmata_name, name) == 0) // use repeated firmware reports to check connection, the string is expected to stay unchanged
                    time(&version_reply_time);       // record the time of last correct reply

        }
        else if (parse_buf[1] == FIRMATA_CAPABILITY_RESPONSE)
        {
            int pin, i, n;
            for (pin = 0; pin < 128; pin++)
            {
                pin_info[pin].supported_modes = 0;
            }
            for (i = 2, n = 0, pin = 0; i < parse_count - 1; i++)
            {
                if (parse_buf[i] == 127)
                {
                    pin++;
                    n = 0;
                    continue;
                }
                if (n == 0)
                {
                    // first byte is supported mode
                    pin_info[pin].supported_modes |= (1 << parse_buf[i]);
                    LOGF_DEBUG("CAPABILITY_RESPONSE: pin:%u modes:%04x", pin, (short)pin_info[pin].supported_modes);
                }
                n = n ^ 1;
            }
            have_capabilities = 1;
        }
        else if (parse_buf[1] == FIRMATA_ANALOG_MAPPING_RESPONSE)
        {
            int pin = 0;
            for (int i = 2; i < parse_count - 1; i++)
            {
                pin_info[pin].analog_channel = parse_buf[i];
                LOGF_DEBUG("ANALOG_MAPPING: pin %d is A%d", pin, pin_info[pin].analog_channel);
                pin++;
            }
            for (; pin < 128; pin++)
            {
                pin_info[pin].analog_channel = 127;
            }
            have_analog_mapping = 1;
            return;
        }
        else if (parse_buf[1] == FIRMATA_PIN_STATE_RESPONSE && parse_count >= 6)
        {
            int pin             = parse_buf[2];
            pin_info[pin].mode  = parse_buf[3];
            pin_info[pin].value = parse_buf[4];
            if (parse_count > 6)
                pin_info[pin].value |= (parse_buf[5] << 7);
            if (parse_count > 7)
                pin_info[pin].value |= (parse_buf[6] << 14);
            LOGF_DEBUG("PIN_STATE_RESPONSE: pin:%u. Mode:%u. Value:%llu", pin, pin_info[pin].mode, static_cast<unsigned long long>(pin_info[pin].value));
            if (pin_info[pin].mode == FIRMATA_MODE_OUTPUT)
                updateDigitalPort(pin, pin_info[pin].value ? ARDUINO_HIGH : ARDUINO_LOW);
        }
        else if (parse_buf[1] == FIRMATA_STRING_DATA)
        {
            if ((parse_count - 3) >= MAX_STRING_DATA_LEN)
            {
                LOGF_DEBUG("FIRMATA_STRING_DATA TOO LARGE.%u Parsing up to max %u", (parse_count - 3),
                           MAX_STRING_DATA_LEN);
                parse_count = FIRMATA_STRING_DATA + 3;
            }
            char name[MAX_STRING_DATA_LEN];
            int len = 0;
            for (int i = 2; i < parse_count - 2; i += 2)
            {
                name[len++] = (parse_buf[i] & 0x7F) | ((parse_buf[i + 1] & 0x7F) << 7);
            }
            name[len++] = 0;
            strcpy(string_buffer, name);
            LOGF_DEBUG("STRING_DATA: %s", name);
        }
        else if (parse_buf[1] == FIRMATA_EXTENDED_ANALOG)
        {
            if ((parse_count - 3) > 8)
                LOG_DEBUG("Extended analog max precision uint64_bit");
            int analog_ch = (parse_buf[2] & 0x7F); //UP to 128 analogs
            uint64_t analog_val = 0;
            for (int i = parse_count - 2; i >= 3; i--)
            {
                analog_val = (analog_val << 7) | (parse_buf[i] & 0x7F);
            }
            for (int pin = 0; pin < 128; pin++)
            {
                if (pin_info[pin].analog_channel == analog_ch)
                {
                    pin_info[pin].value = analog_val;
                    LOGF_DEBUG("EXTENDED_ANALOG: pin %d is A%d = %lu", pin, analog_ch, analog_val);
                    break;
                }
            }
        }
        else if (parse_buf[1] == FIRMATA_I2C_REPLY)
        {
            //TODO Testting
            if ((parse_count - 3) > 8)
                LOG_DEBUG("I2C_REPLY max precision uint64_bit (8 bytes)");
            int slaveAddress = (parse_buf[2] & 0x7F);
            slaveAddress     = (slaveAddress << 7) | (parse_buf[3] & 0x7F);
            long i2c_val     = (parse_buf[4] & 0x7F);
            for (int i = 4; i < parse_count - 1; i++)
            {
                i2c_val = (i2c_val << 7) | (parse_buf[i] & 0x7F);
            }
            LOGF_DEBUG("I2C_REPLY value: SlaveAddres %u = %ld", slaveAddress, i2c_val);
        }
        return;
    }
}

int Firmata::OnIdle()
{
    uint8_t buf[1024];
    int r = 1;

    //if (debug) LOGF_DEBUG("Idle event");
    if (r > 0)
    {
        r = arduino->readPort(buf, sizeof(buf));
        if (r < 0)
        {
            // error
            return r;
        }
        if (r > 0)
        {
/*
            for (int i = 0; i < r; i++)
            {
                if (debug)
                    printf("%02X ", buf[i]);
            }
            if (debug)
                printf("\n");
*/
            Parse(buf, r);
            return 0;
        }
    }
    else if (r < 0)
    {
        return r;
    }
    return 0;
}

time_t Firmata::secondsSinceVersionReply()
{
    time_t now;
    time(&now);
    return now - version_reply_time;
}
