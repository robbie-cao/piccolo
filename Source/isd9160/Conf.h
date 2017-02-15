#ifndef __CONF_H__
#define __CONF_H__

#include "Siren.h"

#define VOLUME_MAX              16
#define VOLUME_MIN              2
#define VOLUME_DEFAULT          8
#define VOLUME_STEP             3

#define CHANNEL_COUNT           3

#define SAMPLE_RATE             SAMPLE_RATE_SIREN7
#define BIT_RATE                BIT_RATE_32K
#define BANDWIDTH               7000    // G722 speech bandwidth: 50 ~ 7000 Hz


#define I2C_ADDRESS_0           0x50
#define I2C_ADDRESS_1           0x51
#define I2C_ADDRESS_2           0x52
#define I2C_ADDRESS_3           0x53

#endif

/* vim: set ts=4 sw=4 tw=0 list : */
