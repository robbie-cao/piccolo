#include <stdio.h>
#include <string.h>

#include "ISD9xx.h"

#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvSYS.h"

#include "Lib\libSPIFlash.h"
#include "Lib\LibSiren7.h"

#include "NVTTypes.h"
#include "PlaySpiG722.h"
#include "SpiFlash.h"
#include "Conf.h"
#include "Log.h"

#include "OP.h"
#include "PM.h"
#include "SYS.h"
#include "Reg.h"

#define OP_DATA_BUFFER_SIZE     32

// Swap buffer for cmds which read something from 9160
uint8_t sOpDataBuffer[OP_DATA_BUFFER_SIZE];


/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
static int8_t OP_Reset(OpCmd_t *pCmd);
static int8_t OP_SoftReset(OpCmd_t *pCmd);
static int8_t OP_SetPowerMode(OpCmd_t *pCmd);
static int8_t OP_SetIntEn(OpCmd_t *pCmd);

static int8_t OP_Play_VP(OpCmd_t *pCmd);
static int8_t OP_Play_VP_Loop(OpCmd_t *pCmd);

static int8_t OP_Stop(OpCmd_t *pCmd);
static int8_t OP_Replay(OpCmd_t *pCmd);
static int8_t OP_Pause(OpCmd_t *pCmd);
static int8_t OP_Resume(OpCmd_t *pCmd);
static int8_t OP_PlayPauseResume(OpCmd_t *pCmd);
static int8_t OP_StopAll(OpCmd_t *pCmd);
static int8_t OP_PauseAll(OpCmd_t *pCmd);

static int8_t OP_VolumeSet(OpCmd_t *pCmd);
static int8_t OP_VolumeUp(OpCmd_t *pCmd);
static int8_t OP_VolumeDown(OpCmd_t *pCmd);
static int8_t OP_VolumeMute(OpCmd_t *pCmd);
static int8_t OP_VolumeUnmute(OpCmd_t *pCmd);
static int8_t OP_VolumeUpAll(OpCmd_t *pCmd);
static int8_t OP_VolumeDownAll(OpCmd_t *pCmd);

static int8_t OP_ReadChipId(OpCmd_t *pCmd);
static int8_t OP_ReadStatus(OpCmd_t *pCmd);
static int8_t OP_ReadChannelStatus(OpCmd_t *pCmd);
static int8_t OP_ReadVolume(OpCmd_t *pCmd);
static int8_t OP_ReadConfig(OpCmd_t *pCmd);

// Global
//
OpCmdHandlerItem_t opCmdHandler[] =
{
    { OP_SYS_RESET            , OP_Reset             } ,
    { OP_SYS_SOFTRESET        , OP_SoftReset         } ,
    { OP_SYS_SETPOWERMODE     , OP_SetPowerMode      } ,
    { OP_SYS_SETINTEN             , OP_SetIntEn      } ,

    { OP_PLAY_VP              , OP_Play_VP           } ,
    { OP_PLAY_VP_LOOP         , OP_Play_VP_Loop      } ,

    { OP_PB_STOP              , OP_Stop              } ,
    { OP_PB_REPLAY            , OP_Replay            } ,
    { OP_PB_PAUSE             , OP_Pause             } ,
    { OP_PB_RESUME            , OP_Resume            } ,
    { OP_PB_PLAY_PAUSE_RESUME , OP_PlayPauseResume   } ,
    { OP_PB_STOP_ALL          , OP_StopAll           } ,
    { OP_PB_PAUSE_ALL         , OP_PauseAll          } ,

    { OP_VOLUME_SET           , OP_VolumeSet         } ,
    { OP_VOLUME_UP            , OP_VolumeUp          } ,
    { OP_VOLUME_DOWN          , OP_VolumeDown        } ,
    { OP_VOLUME_MUTE          , OP_VolumeMute        } ,
    { OP_VOLUME_UNMUTE        , OP_VolumeUnmute      } ,
    { OP_VOLUME_UP_ALL        , OP_VolumeUpAll       } ,
    { OP_VOLUME_DOWN_ALL      , OP_VolumeDownAll     } ,

    { OP_READ_CHIPID          , OP_ReadChipId        } ,
    { OP_READ_STATUS          , OP_ReadStatus        } ,
    { OP_READ_CHANNEL_STATUS  , OP_ReadChannelStatus } ,
    { OP_READ_VOL             , OP_ReadVolume        } ,
    { OP_READ_CONFIG          , OP_ReadConfig        } ,
};

#define OP_HANDLER_CNT()    (sizeof(opCmdHandler) / sizeof(opCmdHandler[0]))

int8_t OP_Handler(OpCmd_t *pCmd)
{
    uint8_t i;

    if (!pCmd) {
        return SYS_INVALID_PARAM;
    }

    for (i = 0; i < OP_HANDLER_CNT(); i++) {
        if (pCmd->cmd == opCmdHandler[i].cmd) {
            return opCmdHandler[i].handler(pCmd);
        }
    }

    return SYS_NOT_SUPPORT;
}

static int8_t OP_Reset(OpCmd_t *pCmd)
{
    Sys_Reset();

    return SYS_GOOD;
}

static int8_t OP_SoftReset(OpCmd_t *pCmd)
{
    Sys_SoftReset();

    return SYS_GOOD;
}

static int8_t OP_SetPowerMode(OpCmd_t *pCmd)
{
    switch (pCmd->data) {
        case ISD9160_POWER_STATE_DPD:
            PM_DeepPowerDown();
            break;
        case ISD9160_POWER_STATE_SPD:
            PM_StandbyPowerDown();
            break;
        case ISD9160_POWER_STATE_DEEPSLEEP:
            PM_DeepSleep();
            break;
        case ISD9160_POWER_STATE_SLEEP:
            // TODO
        case ISD9160_POWER_STATE_NORMAL:
        default:
            PM_Wakeup();
            break;
    }

    return SYS_GOOD;
}

static int8_t OP_SetIntEn(OpCmd_t *pCmd)
{
    sRegisterMap.ctl.intEn = !!pCmd->data;

    return SYS_GOOD;
}


static int8_t OP_Play_VP(OpCmd_t *pCmd)
{
    PlaySpi_PlayVpByChannel(pCmd->chIdx, pCmd->vpIdx);

    return SYS_GOOD;
}

static int8_t OP_Play_VP_Loop(OpCmd_t *pCmd)
{
    PlaySpi_PlayVpLoopByChannel(pCmd->chIdx, pCmd->vpIdx, 1);

    return SYS_GOOD;
}


static int8_t OP_Stop(OpCmd_t *pCmd)
{
    PlaySpi_PlayBackStopByChannel(pCmd->chIdx);

    return SYS_GOOD;
}

static int8_t OP_Replay(OpCmd_t *pCmd)
{
    PlaySpi_PlayBackReplayByChannel(pCmd->chIdx);

    return SYS_GOOD;
}

static int8_t OP_Pause(OpCmd_t *pCmd)
{
    PlaySpi_PlayBackPauseByChannel(pCmd->chIdx);

    return SYS_GOOD;
}

static int8_t OP_Resume(OpCmd_t *pCmd)
{
    PlaySpi_PlayBackResumeByChannel(pCmd->chIdx);

    return SYS_GOOD;
}

static int8_t OP_PlayPauseResume(OpCmd_t *pCmd)
{
    PlaySpi_PlayBackPlayPauseResumeByChannel(pCmd->chIdx);

    return SYS_GOOD;
}

static int8_t OP_StopAll(OpCmd_t *pCmd)
{
    PlaySpi_PlayBackStopAll();

    return SYS_GOOD;
}

static int8_t OP_PauseAll(OpCmd_t *pCmd)
{
    PlaySpi_PlayBackPauseAll();

    return SYS_GOOD;
}


static int8_t OP_VolumeSet(OpCmd_t *pCmd)
{
    sRegisterMap.vol = (pCmd->data <= VOLUME_MAX) ? pCmd->extra[0] : VOLUME_MAX;
    return SYS_GOOD;
}

static int8_t OP_VolumeUp(OpCmd_t *pCmd)
{
    // TODO
    return SYS_GOOD;
}

static int8_t OP_VolumeDown(OpCmd_t *pCmd)
{
    // TODO
    return SYS_GOOD;
}

static int8_t OP_VolumeMute(OpCmd_t *pCmd)
{
    // TODO
    return SYS_GOOD;
}

static int8_t OP_VolumeUnmute(OpCmd_t *pCmd)
{
    // TODO
    return SYS_GOOD;
}

static int8_t OP_VolumeUpAll(OpCmd_t *pCmd)
{
    // TODO
    return SYS_GOOD;
}

static int8_t OP_VolumeDownAll(OpCmd_t *pCmd)
{
    // TODO
    return SYS_GOOD;
}


static int8_t OP_ReadChipId(OpCmd_t *pCmd)
{
    // Read Chip ID from SOC and copy it to sOpDataBuffer
    // TODO
    return SYS_GOOD;
}

static int8_t OP_ReadStatus(OpCmd_t *pCmd)
{
    RegMap_t *pRegMap = (RegMap_t *)&sOpDataBuffer;

    memcpy(sOpDataBuffer, &sRegisterMap, sizeof(RegMap_t));
    // Little Endian(ISD9160 - ARM M0) -> Big Endian(CC2541 - 8051)
    pRegMap->ch[0].vpIdx = ENDIAN_CONVERT_16(pRegMap->ch[0].vpIdx);
    pRegMap->ch[1].vpIdx = ENDIAN_CONVERT_16(pRegMap->ch[1].vpIdx);
    pRegMap->ch[2].vpIdx = ENDIAN_CONVERT_16(pRegMap->ch[2].vpIdx);

    return SYS_GOOD;
}

static int8_t OP_ReadChannelStatus(OpCmd_t *pCmd)
{
    if (pCmd->data >= CHANNEL_COUNT) {
        return SYS_INVALID_PARAM;
    }
    memcpy(sOpDataBuffer, sRegisterMap.ch + pCmd->data, sizeof(RegChInfo_t));

    return SYS_GOOD;
}

static int8_t OP_ReadVolume(OpCmd_t *pCmd)
{
    sOpDataBuffer[0] = sRegisterMap.vol;
    // TODO
    return SYS_GOOD;
}

static int8_t OP_ReadConfig(OpCmd_t *pCmd)
{
    // Copy config data to sOpDataBuffer
    // TODO
    return SYS_GOOD;
}


/* vim: set ts=4 sw=4 tw=0 list : */
