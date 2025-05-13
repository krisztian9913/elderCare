#include "I2C.h"

TwoWire I2C = TwoWire(0);

void InitI2C(uint8_t sdaPin, uint8_t sclPin, uint32_t freq)
{
    I2C.begin(sdaPin, sclPin, (uint32_t)freq);
}

bool writeCommand(uint8_t deviceAddress, uint8_t command)
{
    uint8_t status;

    I2C.beginTransmission(deviceAddress);
    I2C.write(command);
    status = I2C.endTransmission(true);

    if(status == 0)
    {
        return true;
    }

    return false;
}

bool writeRegister(uint8_t deviceAddress, uint8_t registerAddres, uint8_t registerValue)
{
    uint8_t status;

    I2C.beginTransmission(deviceAddress);
    I2C.write(registerAddres);
    I2C.write(registerValue);

    status = I2C.endTransmission(true);

    if(status == 0)
    {
        return true;
    }

    return false;
}

bool writeRegisters(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *buffer, uint16_t numberOfByte)
{
    uint16_t status, i;

    I2C.beginTransmission(deviceAddress);

    I2C.write(registerAddress);

    for(i = 0; i < numberOfByte; i++)
    {
        I2C.write(buffer[i]);
    }

    status = I2C.endTransmission(true);

    if(status == 0)
    {
        return true;
    }

    return false;
}

void readDevice(uint8_t deviceAddress, uint8_t *buffer, uint8_t numberOfByte)
{
    uint8_t i;

    I2C.requestFrom(deviceAddress, numberOfByte);

    for(i = 0; i < numberOfByte; i++)
    {
        buffer[i] = I2C.read();
    }
}

void readRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t &buffer)
{
    I2C.beginTransmission(deviceAddress);
    I2C.write(registerAddress);
    I2C.endTransmission(true);

    I2C.requestFrom(deviceAddress, 1u);

    buffer = I2C.read();
}

void readRegisters(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *buffer, uint8_t numberOfByte)
{

    uint8_t i;

    I2C.beginTransmission(deviceAddress);
    I2C.write(registerAddress);
    I2C.endTransmission(true);

    I2C.requestFrom(deviceAddress, numberOfByte);

    for(i = 0; i < numberOfByte; i++)
    {
        buffer[i] = I2C.read();
    }
}