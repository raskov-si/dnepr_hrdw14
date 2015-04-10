#include "HAL/MCU/inc/mcf5282_i2c_stmch.h"

enum    _STM_I2C_SLAVE_STATES {
    STATE_INIT                  = 0,
    STATE_ACK                   = 1,
    STATE_NOACK                 = 2,
    STATE_START                 = 3,
    STATE_DATA                  = 4,
    STATE_STOP                  = 5,
    STATE_STARTSTOP             = 6
};




