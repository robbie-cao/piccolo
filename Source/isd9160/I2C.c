#include "ISD9160.h"
#include "I2C.h"
#include "Log.h"

#define I2C_DEBUG               0

#if I2C_DEBUG
#define I2C_LOG                 LOG
#else
#define I2C_LOG                 __LOG
#endif

#define LOG_TAG                 "I2C"


#define I2CDATABUFFERSIZE       0x32


// Below variables should be local variable
#if 0
static uint8_t sChStatus[CHANNEL_COUNT];
#endif
static uint8_t sTxDataCnt = 0;
static uint8_t sRxDataCnt = 0;
static uint8_t sRxDataBuf[I2CDATABUFFERSIZE];
static uint8_t sTxDataBuf[I2CDATABUFFERSIZE];
static uint8_t sDataReceived = FALSE;


/*---------------------------------------------------------------------------------------------------------*/
/* Function Prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
// I2CFUNC   = 0
// ARBITLOSS = 1
// BUSERROR  = 2
// TIMEOUT	 = 3
static void I2C0SlaveCallback_I2cFunc(uint32_t status);
static void I2C0SlaveCallback_ArbitLoss(uint32_t status);
static void I2C0SlaveCallback_BusError(uint32_t status);
static void I2C0SlaveCallback_Timeout(uint32_t status);


static void I2C0SlaveCallback_I2cFunc(uint32_t status)
{
    switch (status) {
        case slvAddrAckR  /* 0x60 */:
        case mstLostArbR  /* 0x68 */:
            // SLA+W has been received and ACK has been returned
            I2C_LOG("S+W\r\n");
            // Use trace log API
            // Log message as short as possible as it may impact timing
            // TODO
            sRxDataCnt = 0;
            DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);
            break;
        case slvDataAckR  /* 0x80 */:
            // DATA has been received and ACK has been returned
            I2C_LOG("DAR\r\n");
            sRxDataBuf[sRxDataCnt++] = DrvI2C_ReadData(I2C_PORT0);
            DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);

            if (sRxDataCnt >= I2CDATABUFFERSIZE) {
                sRxDataCnt = 0;
            }

            break;
        case slvStopped   /* 0xA0 */:
            // STOP or Repeat START has been received
            I2C_LOG("SP\r\n");
            DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);

            sDataReceived = TRUE;                 // FIXME - Good enough?
            break;
        case slvAddrAckW  /* 0xA8 */:
        case mstLostArbW  /* 0xB0 */:
            // SLA+R has been received and ACK has been returned
            I2C_LOG("S+R\r\n");
            sTxDataCnt = 0;
            // First data replied to master
            DrvI2C_WriteData(I2C_PORT0, sTxDataBuf[sTxDataCnt++]);
            DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);
            break;
        case slvDataAckW  /* 0xB8 */:
            // DATA has been transmitted and ACK has been received
            I2C_LOG("DAW\r\n");
            // The following data replied to master
            DrvI2C_WriteData(I2C_PORT0, sTxDataBuf[sTxDataCnt++]);
            if (sTxDataCnt > I2CDATABUFFERSIZE - 1) {
                sTxDataCnt = 0;
            }
            DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);
            break;
        case slvDataNackW /* 0xC0 */:
            // DATA has been transmitted and NACK has been received
            I2C_LOG("DNW\r\n");
            DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);
            break;
        case slvLastAckW  /* 0xC8 */:
            // Last DATA has been transmitted and ACK has been received
            I2C_LOG("LAW\r\n");
            DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);
            break;
        case slvAddrAckG  /* 0x70 */:
        case mstLostArbG  /* 0x78 */:
        case slvDataNackR /* 0x88 */:
        case genDataAckR  /* 0x90 */:
        case genDataNackR /* 0x98 */:
        case i2cIdle      /* 0xF8 */:
        default:
            I2C_LOG("NAN - 0x%x\r\n", status);
            break;
    }
}

static void I2C0SlaveCallback_ArbitLoss(uint32_t status)
{
    I2C_LOG("I2C Arbit Loss\r\n");
    // TODO
}

static void I2C0SlaveCallback_BusError(uint32_t status)
{
    I2C_LOG("I2C Bus Error\r\n");
    // TODO
}

static void I2C0SlaveCallback_Timeout(uint32_t status)
{
    I2C_LOG("I2C Timeout\r\n");
    // TODO
}


void I2C_DataReceiveidHandler(void)
{
#if UNIFIED_OP_CMD
    uint8_t res = SYS_GOOD;
    OpCmd_t *pCmd = (OpCmd_t *)sRxDataBuf;

    // Big Endian(CC2541 - 8051) -> Little Endian(ISD9160 - ARM M0)
    pCmd->vpIdx = ENDIAN_CONVERT_16(pCmd->vpIdx);

    res = OP_Handler(pCmd);

    if (res) {
        LOGE(LOG_TAG, "Op Handler: %d\r\n", res);
    } else {
        LOGD(LOG_TAG, "Op Handler: %d\r\n", res);
    }

    sRxDataCnt = 0;
#else
    OpCmd_t *pCmd = (OpCmd_t *)sRxDataBuf;
    uint8_t u8NewVol = VOLUME_MAX;
    uint8_t sReadCmdTime = 0;
    // Check pCmd->len and cmd data validility
    // TODO

    switch (pCmd->cmd)
    {
        case OP_SYS_RESET:
            Sys_Reset();
            break;
        case OP_SYS_CONFIG:
            break;
        case OP_SYS_OPEN:
            Sys_Open();
            break;
        case OP_SYS_CLOSE:
            Sys_Close();
            break;
        case OP_SYS_SELFTEST:
            Sys_SelfTest();
            break;
        case OP_SYS_DUMP:
            // TODO
            break;
        case OP_DATA_READ:
        case OP_DATA_WRITE:
        case OP_DATA_ERASE:
        case OP_FLASH_READ:
        case OP_FLASH_WRITE:
        case OP_FLASH_ERASE:
            // TODO
            break;
        case OP_PLAY_VP:
            // Use pCmd->vpIdx
            // TODO
            PlaySpi_PlayVpByChannel(sRxDataBuf[3] << 8 | sRxDataBuf[4], sRxDataBuf[2]);
            break;
        case OP_PLAY_VP_LOOP:
            // Use pCmd->vpIdx
            // TODO
            PlaySpi_PlayVpLoopByChannel(sRxDataBuf[3] << 8 | sRxDataBuf[4], sRxDataBuf[2], sRxDataBuf[5]);
            break;
        case OP_PLAY_VM:
        case OP_PLAY_VM_LOOP:
        case OP_PLAY_VM_INDIRECT:
        case OP_PLAY_SILENCE:
            PlaySpi_PlaySilenceByChannel(pCmd->chIdx);
            break;
        case OP_PLAY_SILENCE_LOOP:
            PlaySpi_PlaySilenceLoopByChannel(pCmd->chIdx);
            break;
        case OP_PLAY_INSERT_SILENCE:
            PlaySpi_PlayInsertSilenceByChannel(pCmd->chIdx);
            break;
        case OP_PB_STOP:
            PlaySpi_StopChannel(pCmd->chIdx);
            // Use new API
            // TODO
            break;
        case OP_PB_REPLAY:
            PlaySpi_PlayBackReplayByChannel(pCmd->chIdx);
            break;
        case OP_PB_PAUSE:
            PlaySpi_PlayBackPauseByChannel(pCmd->chIdx);
            break;
        case OP_PB_RESUME:
            PlaySpi_PlayBackResumeByChannel(pCmd->chIdx);
            break;
        case OP_PB_REPEAT:
            PlaySpi_PlayBackRepeatByChannel(pCmd->chIdx);
            break;
        case OP_PB_PLAY_PAUSE_RESUME:
            PlaySpi_PauseResume(pCmd->chIdx);
            // Use new API
            // TODO
            break;
        case OP_PB_STOP_ALL:
            PlaySpi_PlayBackStopAll();
            break;
        case OP_PB_PAUSE_ALL:
            PlaySpi_PlayBackPauseAll();
            break;
        case OP_PB_CANCEL_LAST_ONE:
            PlaySpi_PlayBackCancelLastOne();
            break;
        case OP_PB_CANCEL_ALL:
            PlaySpi_PlayBackCancelAll();
            break;
        case OP_PB_CLEAR_ALL:
            PlaySpi_PlayBackClearAll();
            break;
        case OP_VOLUME_SET:
            PlaySpi_SetVolume(pCmd->chIdx);     // Byte 3 (chIdx is volume)
            break;
        case OP_VOLUME_UP:
            u8NewVol = PlaySpi_GetVolume();
            if (u8NewVol >= VOLUME_MAX) {
                u8NewVol = VOLUME_MAX;
            } else {
                u8NewVol += VOLUME_STEP;
            }
            PlaySpi_SetVolume(u8NewVol);
            break;
        case OP_VOLUME_DOWN:
            u8NewVol = PlaySpi_GetVolume();
            if (u8NewVol <= VOLUME_MIN) {
                u8NewVol = VOLUME_MIN;
            } else {
                u8NewVol -= VOLUME_STEP;
            }
            PlaySpi_SetVolume(u8NewVol);
            break;
        case OP_VOLUME_MUTE:
        case OP_VOLUME_UNMUTE:
        case OP_VOLUME_SET_ALL:
        case OP_VOLUME_UP_ALL:
        case OP_VOLUME_DOWN_ALL:
            break;
        case OP_PM_POWER_UP:
        case OP_PM_POWER_DOWN:
            // TODO
            break;
        case OP_PM_SPD:
            PM_StandbyPowerDown();
            break;
        case OP_PM_DPD:
            PM_DeepPowerDown();
            break;
        case OP_PM_DS:
            PM_DeepSleep();
            break;
        case OP_PM_STOP:
            PM_Stop();
            break;
        case OP_PM_WAKEUP:
            PM_Wakeup();
            break;
        case OP_READ_CHIPID:
            break;
        case OP_READ_INT:
            break;
        case OP_READ_STATUS:
            sTxDataBuf[0] = PlaySpi_GetPlayingStatus();
            sTxDataBuf[1] = PlaySpi_GetVolume();
            sReadCmdTime += 1;
            break;
        case OP_READ_CHANNEL_STATUS:
        case OP_READ_VOL:
        case OP_READ_CONFIG:
        case OP_CHECK_DEVICE_STATUS:
        case OP_CHECK_JOB_QUEUE:
        case OP_CHECK_FLASH_TYPE:
        case OP_CHECK_FLASH_STATUS:
            // TODO
            break;
        default:
            break;
    }

    sRxDataCnt = 0;
#endif
}

uint8_t I2C_DataReceived(void)
{
    return sDataReceived;
}

void I2C_DataClear(void)
{
    sDataReceived = FALSE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* InitialI2C                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void InitialI2C(void)
{
    /* GPIO initial and select operation mode for I2C */
    DrvGPIO_InitFunction(FUNC_I2C0);                // Set PA.10 = I2C0 SDA; Set PA.11 = I2C0 SCL

    DrvI2C_Open(I2C_PORT0, (DrvSYS_GetHCLK() * 1000), 48000);   // Clock = 48Kbps; as slave this does not matter

    DrvI2C_SetAddress(I2C_PORT0, 0, I2C_ADDRESS_0, 0x00);    // Address is 0b1010000 = 0x50, addr0[0] = 1 --> slave mode,
    DrvI2C_SetAddress(I2C_PORT0, 1, I2C_ADDRESS_1, 0x00);    // Address is 0b1010001 = 0x51, addr0[0] = 1 --> slave mode,
    DrvI2C_SetAddress(I2C_PORT0, 2, I2C_ADDRESS_2, 0x00);    // Address is 0b1010010 = 0x52, addr0[0] = 1 --> slave mode,
    DrvI2C_SetAddress(I2C_PORT0, 3, I2C_ADDRESS_3, 0x00);    // Address is 0b1010011 = 0x53, addr0[0] = 1 --> slave mode,

    /* Set AA bit, I2C0 as slave */
    DrvI2C_Ctrl(I2C_PORT0, 0, 0, 0, 1);

    sRxDataCnt = 0;
    memset(sRxDataBuf, 0, sizeof(sRxDataBuf));

    DrvI2C_EnableInt(I2C_PORT0);                    // Enable I2C0 interrupt and set corresponding NVIC bit

    /*
     * I2CFUNC   = 0,
     * ARBITLOSS = 1,
     * BUSERROR  = 2,
     * TIMEOUT	 = 3
     */
    DrvI2C_InstallCallback(I2C_PORT0, I2CFUNC, I2C0SlaveCallback_I2cFunc);
    DrvI2C_InstallCallback(I2C_PORT0, ARBITLOSS, I2C0SlaveCallback_ArbitLoss);
    DrvI2C_InstallCallback(I2C_PORT0, BUSERROR, I2C0SlaveCallback_BusError);
    DrvI2C_InstallCallback(I2C_PORT0, TIMEOUT, I2C0SlaveCallback_Timeout);
}

/* vim: set ts=4 sw=4 tw=0 list : */
