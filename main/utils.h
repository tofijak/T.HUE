#ifndef __utils_h
#define __utils_h

#include <Arduino.h>
#include <EEPROM.h>



void connectToWiFi();
float ReadBatteryVoltage();
void storeStruct(void *data_source, size_t size, char startPosition);
void loadStruct(void *data_dest, size_t size, char loadPosition);

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}


#endif