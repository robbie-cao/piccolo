#ifndef __REG_H__
#define __REG_H__

#include "OP.h"


enum {
    REG_CTL  = 0,    // RW, Ctrl
    REG_IFG  = 1,    // RO, Int Status Flag
    REG_VOL  = 2,    // RO, Volume
    REG_CH2  = 3,    // RO, Ch2 Status
    REG_CH1  = 6,    // RO, Ch1 Status
    REG_CH0  = 9,    // RO, Ch0 Status

    REG_TOTAL  = 12
};

typedef struct {
    uint8_t  reset      : 1;    // CPU Reset
    uint8_t  softRst    : 1;    // Soft Reset
    uint8_t  powerMode  : 3;    // Power Mode: 000 - DPD, 001 - SPD, 010 - Deep Sleep, 011 - Sleep, 1xx - Normal
    uint8_t  rfu        : 2;
    uint8_t  intEn      : 1;    // 0 - Disable, 1 - Enable
} RegCtrl_t;

typedef struct {
    uint8_t  err        : 1;
    uint8_t  pm         : 1;
    uint8_t  vol        : 1;
    uint8_t  rfu        : 2;
    uint8_t  ch2        : 1;
    uint8_t  ch1        : 1;
    uint8_t  ch0        : 1;
} RegIntStatusFlag_t;

typedef struct {
    uint8_t  state      : 2;     // 00 - IDLE, 01 - PLAY, 10 - PAUSE, 11 - COMPLETED
    uint8_t  vol        : 4;     // 0~15
    uint8_t  repeat     : 1;     // Loop play
    uint8_t  dominative : 1;     // 0 - Can be interrupt and reused, 1 - Not interruptable
} RegChStatus_t;

#define AUDIO_CHANNEL_MASK_STATE    (0x03 << 0)
#define AUDIO_CHANNEL_MASK_VOLUME   (0x0F << 2)

typedef struct {
    union {
        RegChStatus_t   bits;
        uint8_t         value;
    } status;
    uint16_t    vpIdx;
} RegChInfo_t;

typedef struct {
    RegCtrl_t           ctl;
    RegIntStatusFlag_t  ifg;
    uint8_t             vol;
    RegChInfo_t         ch[3];
} RegMap_t;

int8_t Reg_Read(uint8_t addr, uint8_t *pBuf, uint8_t size);
int8_t Reg_Write(uint8_t addr, uint8_t *pBuf, uint8_t size);

void Reg_StatusChangeAlert(uint8_t alertBits);
void Reg_StatusClear(void);

void Reg_Init(void);

extern RegMap_t sRegisterMap;


#endif /* __REG_H__ */

/* vim: set ts=4 sw=4 tw=0 list : */
