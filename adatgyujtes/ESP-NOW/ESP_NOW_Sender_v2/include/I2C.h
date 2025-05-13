#ifndef INC_I2C_H_
#define INC_I2C_H_

#include <Arduino.h>
#include <Wire.h>

/**
 * \brief Initialize the I2C Bus
 * 
 * @param The SDA pin number
 * @param The SCL pin number
 * @param I2C frequency
*/
void InitI2C(uint8_t sdaPin=4u, uint8_t sclPin=5u, uint32_t freq=100000);

/**
 * \brief Write one command to the slave address
 * 
 * @param deviceAddress 8bit slave Address
 * @param command 8bit Command
 * 
 * @return true if the send was successfull
 * @return false if somthing went wrong
*/
bool writeCommand(uint8_t deviceAddress, uint8_t command);

/**
 * \brief Write the Selceted Register with the new register value
 * 
 * @param deviceAddress 8bit Slave Device Address
 * @param registerAddres 8bit Register Addres
 * @param registerValue 8bit Register Value
 * 
 * @return true if the send was successfull
 * @return false if somthing went wrong
*/
bool writeRegister(uint8_t deviceAddress, uint8_t registerAddres, uint8_t registerValue);

/**
 * \brief Write the Selected Register with the buffer values
 * 
 * @param deviceAddress 8bit Slave Device Address
 * @param registerAddress 8bit Register Addres
 * @param buffer 8bit data buffer
 * @param numberOfByte The buffer length
 * 
 * @return true if the send was successfull
 * @return false if somthing went wrong
*/
bool writeRegisters(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *buffer, uint16_t numberOfByte);

/**
 * \brief Read number of bytes from the Slave Device
 * 
 * @param deviceAddress 8bit Slave Address
 * @param buffer Output buffer
 * @param numberOfByte The buffer length
*/
void readDevice(uint8_t deviceAddress, uint8_t *buffer, uint8_t numberOfByte);

/**
 * \brief Read the Selceted Register value
 * 
 * @param deviceAddress 8bit Slave Device Address
 * @param registerAddress 8bit Register Address
 * @param buffer 8bit Output buffer
*/
void readRegister(uint8_t deviceAddress, uint8_t registerAddress, uint8_t &buffer);

/**
 * \brief Read the Selected Register values
 * 
 * @param deviceAddress 8bit Slave Device Address
 * @param registerAddress 8bit Register Addres
 * @param buffer Output buffer
 * @param numberOfByte The buffer length
 * 
*/
void readRegisters(uint8_t deviceAddress, uint8_t registerAddress, uint8_t *buffer, uint8_t numberOfByte);


#endif