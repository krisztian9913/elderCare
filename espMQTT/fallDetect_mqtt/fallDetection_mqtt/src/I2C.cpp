#include "I2C.h"

TwoWire I2C = TwoWire(1);

void InitI2C(uint8_t sdaPin, uint8_t sclPin, uint32_t freq)
{
    Wire.begin(sdaPin, sclPin, (uint32_t)freq);
}

bool writeCommand(uint8_t deviceAddress, uint8_t command)
{
    uint8_t status;

    Wire.beginTransmission(deviceAddress);
    Wire.write(command);
    status = Wire.endTransmission(true);

    if(status == 0)
    {
        return true;
    }

    return false;
}

bool writeRegister(uint8_t deviceAddress, uint8_t registerAddres, uint8_t registerValue)
{
    uint8_t status;

    Wire.beginTransmission(deviceAddress);
    Wire.write(registerAddres);
    Wire.write(registerValue);

    status = Wire.endTransmission(true);

    if(status == 0)
    {
        return true;
    }

    return false;
}

bool writeRegisters(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *buffer, uint16_t numberOfByte)
{
    uint16_t status, i;

    Wire.beginTransmission(deviceAddress);

    Wire.write(registerAddress);

    for(i = 0; i < numberOfByte; i++)
    {
        Wire.write(buffer[i]);
    }

    status = Wire.endTransmission(true);

    if(status == 0)
    {
        return true;
    }

    return false;
}

void readDevice(uint8_t deviceAddress, uint8_t *buffer, uint8_t numberOfByte)
{
    uint8_t i;

    Wire.requestFrom(deviceAddress, numberOfByte);

    for(i = 0; i < numberOfByte; i++)
    {
        buffer[i] = Wire.read();
    }
}

void readRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t &buffer)
{
    Wire.beginTransmission(deviceAddress);
    Wire.write(registerAddress);
    Wire.endTransmission(true);

    Wire.requestFrom(deviceAddress, 1u);

    buffer = Wire.read();
}

void readRegisters(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *buffer, uint8_t numberOfByte)
{

    uint8_t i;

    Wire.beginTransmission(deviceAddress);
    Wire.write(registerAddress);
    Wire.endTransmission(true);

    Wire.requestFrom(deviceAddress, numberOfByte);

    for(i = 0; i < numberOfByte; i++)
    {
        buffer[i] = Wire.read();
    }
}