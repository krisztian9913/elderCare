#ifndef   BMI270_H
#define   BMI270_H

#include "Arduino.h"
#include "I2C.h"

#define BMI270_ADDRESS  0x68u

typedef enum
{
    CMD = 0x7E,
    PWR_CTRL = 0x7D,
    PWR_CONF = 0x7C,

    OFFSET6 = 0x77,
    OFFSET5 = 0x76,
    OFFSET4 = 0x75,
    OFFSET3 = 0x74,
    OFFSET2 = 0x73,
    OFFSET1 = 0x72,
    OFFSET0 = 0x71,

    NV_CONF = 0x70,

    GYR_SELF_TEST_AXES = 0x6E,
    ACC_SELF_TEST = 0x6D,
    DRV = 0x6C,
    IF_CONF = 0x6B,
    NVM_CONF = 0x6A,
    GYR_CRT_CONF = 0x69,
    AUX_IF_TRIM = 0x68,

    INTERNAL_ERROR = 0x5F,
    INIT_DATA = 0x5E,

    INIT_ADDR_1 = 0x5C,
    INIT_ADDR_0 = 0x5B,

    INIT_CTRL = 0x59,
    INT_MAP_DATA = 0x58,
    INT2_MAP_FEAT = 0x57,
    INT1_MAP_FEAT = 0x56,
    INT_LATCH = 0x55,
    INT2_IO_CTRL = 0x54,
    INT1_IO_CTRL = 0x53,
    ERR_REG_MSK = 0x52,

    AUX_WR_DATA = 0x4F,
    AUX_WR_ADDR = 0x4E,
    AUX_RD_ADDR = 0x4D,
    AUX_IF_CONF = 0x4C,
    AUX_DEV_ID = 0x4B,
    SATURATION = 0x4A,
    FIFO_CONFIG_1 = 0x49,
    FIFO_CONFIG_0 = 0x48,
    FIFO_WTM_1 = 0x47,
    FIFO_WTM_0 = 0x46,
    FIFO_DOWNS = 0x45,
    AUX_CONF = 0x44,
    GYR_RANGE = 0x43,
    GYR_CONF = 0x42,
    ACC_RANGE = 0x41,
    ACC_CONF = 0x40,

    FEATURES_15 = 0x3F,
    FEATURES_14 = 0x3E,
    FEATURES_13 = 0x3D,
    FEATURES_12 = 0x3C,
    FEATURES_11 = 0x3B,
    FEATURES_10 = 0x3A,
    FEATURES_9 = 0x39,
    FEATURES_8 = 0x38,
    FEATURES_7 = 0x37,
    FEATURES_6 = 0x36,
    FEATURES_5 = 0x35,
    FEATURES_4 = 0x34,
    FEATURES_3 = 0x33,
    FEATURES_2 = 0x32,
    FEATURES_1 = 0x31,
    FEATURES_0 = 0x30,

    FEAT_PAGE = 0x2F,

    FIFO_DATA = 0x26,
    FIFO_LENGTH_1 = 0x25,
    FIFO_LENGTH_0 = 0x24,
    TEMPERATURE_1 = 0x23,
    TEMPERATURE_0 = 0x22,
    INTERNAL_STATUS = 0x21,
    WR_GEST_ACT = 0x20,
    SC_OUT_1 = 0x1F,
    SC_OUT_0 = 0x1E,
    INT_STATUS_1 = 0x1D,
    INT_STATUS_0 = 0x1C,
    EVENT = 0x1B,
    SENSORTIME_2 = 0x1A,
    SENSORTIME_1 = 0x19,
    SENSORTime_0 = 0x18,

    DATA_19 = 0x17,
    DATA_18 = 0x16,
    DATA_17 = 0x15,
    DATA_16 = 0x14,
    DATA_15 = 0x13,
    DATA_14 = 0x12,
    DATA_13 = 0x11,
    DATA_12 = 0x10,
    DATA_11 = 0x0F,
    DATA_10 = 0x0E,
    DATA_9 = 0x0D,
    DATA_8 = 0x0C,
    DATA_7 = 0x0B,
    DATA_6 = 0x0A,
    DATA_5 = 0x09,
    DATA_4 = 0x08,
    DATA_3 = 0x07,
    DATA_2 = 0x06,
    DATA_1 = 0x05,
    DATA_0 = 0x04,

    BMI270_STATUS = 0x03,
    ERR_REG = 0x02,
    BMI270_CHIP_ID = 0x00

}bmi270_register_t;

bool bmi270Begin(void);

bool checkInitialization(void);

void setLowPowerMode(uint8_t *buffer);

void uploadFile(uint8_t *configData, uint16_t index, uint16_t writeLen);

void setLowPowerACCMeasure(void);

uint8_t readInterrupt(void);

void readAccelerometerData(uint8_t *buffer);

#endif  /* BMI270_H */