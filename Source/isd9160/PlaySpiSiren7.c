/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/* Siren7 (G.722) licensed from Polycom Technology                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>

#include "ISD9xx.h"

#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvGPIO.h"

#include "Lib\libSPIFlash.h"
#include "Lib\LibSiren7.h"

#include "NVTTypes.h"
#include "PlaySpiG722.h"
#include "SpiFlash.h"
#include "Conf.h"
#include "Log.h"


/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define AUDIOBUFFERSIZE   320
#define COMPBUFSIZE       40      // According to BIT_RATE

#define AUDIO_MIX_OPT         1   // Audio Mixing optimize and clear PUPU
#define DPWM_RESET_CLOSE      0   // Not reset speaker driver when stop playing
#define GPIO_TEST             0   // GPIO test


/*---------------------------------------------------------------------------------------------------------*/
/* Buffer and variables for playing                                                                        */
/*---------------------------------------------------------------------------------------------------------*/

// Global
//

// Local
__align(4) int16_t sAudioBuffer[2][AUDIOBUFFERSIZE];
#if AUDIO_MIX_OPT
static uint8_t     su8Vol[CHANNEL_COUNT] = {VOLUME_MAX, VOLUME_MAX, VOLUME_MAX};
static uint8_t     su8VolPrev[CHANNEL_COUNT] = {VOLUME_MAX, VOLUME_MAX, VOLUME_MAX};
static uint8_t     sCurrVol = VOLUME_MAX;
static uint8_t     su8Mix[CHANNEL_COUNT] = {0};
#else
static uint8_t     su8Vol     = VOLUME_MAX;
static uint8_t     su8VolPrev = VOLUME_MAX;
#endif
static uint8_t     su8Playing = PLAYING_NONE;


// Encoded Siren7 data size
static uint32_t    sTotalG722SizeCh[CHANNEL_COUNT];
static uint32_t    sAudioSampleCountCh[CHANNEL_COUNT];
static uint32_t    sAudioDataAddrCh[CHANNEL_COUNT];
static uint32_t    su32StartAddrCh[CHANNEL_COUNT];
static uint32_t    sPDMA1CallBackCount;
static uint32_t    sBufferEmptyAddr;
static uint32_t    sBufferReadyAddr;
static BOOL        sbBufferEmpty;
static BOOL        sbChLoopPlay[CHANNEL_COUNT];     // For loop play
static BOOL        sbPauseFlag;                     // For pause resume

static uint32_t    su32BufferAddr0 = (uint32_t) &sAudioBuffer[0][0];
static uint32_t    su32BufferAddr1 = (uint32_t) &sAudioBuffer[1][0];

static sSiren7_CODEC_CTL   sEnDeCtlCh[CHANNEL_COUNT];
static sSiren7_DEC_CTX     sS7Dec_CtxCh[CHANNEL_COUNT];

/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
static void InitialDPWM(uint32_t u32SampleRate);
static void PDMA1forDPWM(void);
static void PDMA1Callback(void);
#if AUDIO_MIX_OPT
static void VolumeControl(int16_t *pi16Src, uint16_t u16Count, uint8_t ch);
#else
static void VolumeControl(int16_t *pi16Src, uint16_t u16Count);
#endif
static void CopySpiSoundData(void);
static void PlayChannelSound(uint8_t ch);



/*---------------------------------------------------------------------------------------------------------*/
/* Local Functions                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/* InitialDPWM                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
static void InitialDPWM(uint32_t u32SampleRate)
{
    DrvDPWM_Open();
    DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLKX2);
    DrvDPWM_SetSampleRate(u32SampleRate);
    DrvDPWM_Enable();
}


/*---------------------------------------------------------------------------------------------------------*/
/* Set PDMA0 to move ADC FIFO to MIC buffer with wrapped-around mode                                       */
/*---------------------------------------------------------------------------------------------------------*/
static void PDMA1forDPWM(void)
{
    STR_PDMA_T sPDMA;

    sPDMA.sSrcAddr.u32Addr         = sBufferReadyAddr;
    sPDMA.sDestAddr.u32Addr        = (uint32_t)&DPWM->FIFO;
    sPDMA.u8Mode                   = eDRVPDMA_MODE_MEM2APB;;
    sPDMA.u8TransWidth             = eDRVPDMA_WIDTH_16BITS;
    sPDMA.sSrcAddr.eAddrDirection  = eDRVPDMA_DIRECTION_WRAPAROUND;
    sPDMA.sDestAddr.eAddrDirection = eDRVPDMA_DIRECTION_FIXED;
    sPDMA.u8WrapBcr                = eDRVPDMA_WRA_WRAP_HALF_INT;    // Interrupt condition set fro Half buffer & buffer end
    sPDMA.i32ByteCnt               = AUDIOBUFFERSIZE * 4;           // Full MIC buffer length (byte), Wrap around
                                                                    // FIXME: why 4?

    DrvPDMA_Open(eDRVPDMA_CHANNEL_1, &sPDMA);

    // PDMA Setting
    DrvPDMA_SetCHForAPBDevice(
            eDRVPDMA_CHANNEL_1,
            eDRVPDMA_DPWM,
            eDRVPDMA_WRITE_APB
            );

    // Enable DPWM DMA
    DrvDPWM_EnablePDMA();
    // Enable INT
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR);            // For WARPROUND
    // Install Callback function
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR, (PFN_DRVPDMA_CALLBACK) PDMA1Callback);     // For Wrap
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);
}


/*---------------------------------------------------------------------------------------------------------*/
/* DPWM Callback                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
static void PDMA1Callback(void)
{
    if (sPDMA1CallBackCount & 0x1) {
        sBufferReadyAddr = su32BufferAddr0;
        sBufferEmptyAddr = su32BufferAddr1;
    } else {
        sBufferReadyAddr = su32BufferAddr1;
        sBufferEmptyAddr = su32BufferAddr0;
    }

    sPDMA1CallBackCount++;

    sbBufferEmpty = TRUE;
}


#if AUDIO_MIX_OPT
static void VolumeControl(int16_t *pi16Src, uint16_t u16Count, uint8_t ch)
{
    // 0=< u8Vol <= 16
    uint8_t u8Vol;
    int16_t i16PcmValue = 0;   // Init as 0

    u8Vol = su8VolPrev[ch];
    pi16Src = pi16Src + u16Count;

    // Change volume during zero crossing
    if ((*pi16Src < 50) && (*pi16Src > -50)) {
        // TODO - Hardcode is UGLY!
        // FIXME - Why 50?
        u8Vol = su8Vol[ch];
        su8VolPrev[ch] = su8Vol[ch];
    }

    if (u8Vol & BIT0) {
        i16PcmValue += (*pi16Src >> 4);
    }
    if (u8Vol & BIT1) {
        i16PcmValue += (*pi16Src >> 3);
    }
    if (u8Vol & BIT2) {
        i16PcmValue += (*pi16Src >> 2);
    }
    if (u8Vol & BIT3) {
        i16PcmValue += (*pi16Src >> 1);
    }
    if ((u8Vol & BIT4) > 0) {
        i16PcmValue = *pi16Src;                    // No '+' here, FIXME - Why?
    }

    *pi16Src = i16PcmValue;
}
#else
static void VolumeControl(int16_t *pi16Src, uint16_t u16Count)
{
    // 0=< u8Vol <= 16
    uint8_t u8Vol;
    int16_t i16PcmValue = 0;   // Init as 0

    u8Vol = su8VolPrev;
    pi16Src = pi16Src + u16Count;

    // Change volume during zero crossing
    if ((*pi16Src < 50) && (*pi16Src > -50)) {
        // TODO - Hardcode is UGLY!
        // FIXME - Why 50?
        u8Vol = su8Vol;
        su8VolPrev = su8Vol;
    }

    if (u8Vol & BIT0) {
        i16PcmValue += (*pi16Src >> 4);
    }
    if (u8Vol & BIT1) {
        i16PcmValue += (*pi16Src >> 3);
    }
    if (u8Vol & BIT2) {
        i16PcmValue += (*pi16Src >> 2);
    }
    if (u8Vol & BIT3) {
        i16PcmValue += (*pi16Src >> 1);
    }
    if ((u8Vol & BIT4) > 0) {
        i16PcmValue = *pi16Src;                    // No '+' here, FIXME - Why?
    }

    *pi16Src = i16PcmValue;
    pi16Src = pi16Src + u16Count;
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Copy Data from SPI flash to SRAM                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
static void CopySpiSoundData(void)
{
    __align(4) uint32_t SpiDataBuffer[20];      // buffer size = dctl.number_of_bits_per_frame/8, 20 for 16Kbps, 10 for 8Kbps
    // bit rate?
    // relationship between bit rate and sample rate?
    // FIXME
    // Hardcode is UGLY!
    // 20 -> define
    __align(4) int16_t  BufferTempAddr[CHANNEL_COUNT][AUDIOBUFFERSIZE];
    int16_t *pi16BufferValue = (PINT16)sBufferEmptyAddr;
    uint32_t u32Count;
    uint16_t pcmValue = 0;


    // Optimization
    // TODO
   if (su8Playing & PLAYING_CH001) {
        sflash_read(&g_SPIFLASH, (unsigned long)sAudioDataAddrCh[0], (unsigned long*)SpiDataBuffer, (unsigned long)(COMPBUFSIZE<<1));
        LibS7Decode(&sEnDeCtlCh[0], &sS7Dec_CtxCh[0], (signed short *)SpiDataBuffer, (signed short *)BufferTempAddr[0]);

        sAudioDataAddrCh[0]    = sAudioDataAddrCh[0] + (COMPBUFSIZE * 2);
        sAudioSampleCountCh[0] = sAudioSampleCountCh[0] + AUDIOBUFFERSIZE;
#if AUDIO_MIX_OPT
        if (su8Playing != PLAYING_ALL) {
            for (u32Count = 0; u32Count < AUDIOBUFFERSIZE; u32Count++) {
                VolumeControl((int16_t *)BufferTempAddr[0], u32Count, 0);      // Add to control the volume
            }
        }
#endif
    }

    if (su8Playing & PLAYING_CH010) {
        sflash_read(&g_SPIFLASH, (unsigned long)sAudioDataAddrCh[1], (unsigned long*)SpiDataBuffer, (unsigned long)(COMPBUFSIZE << 1));
        LibS7Decode(&sEnDeCtlCh[1], &sS7Dec_CtxCh[1], (signed short *)SpiDataBuffer, (signed short *)BufferTempAddr[1]);

        sAudioDataAddrCh[1]    = sAudioDataAddrCh[1] + (COMPBUFSIZE * 2);
        sAudioSampleCountCh[1] = sAudioSampleCountCh[1] + AUDIOBUFFERSIZE;
#if AUDIO_MIX_OPT
        if (su8Playing != PLAYING_ALL) {
            for (u32Count = 0; u32Count < AUDIOBUFFERSIZE; u32Count++) {
                VolumeControl((int16_t *)BufferTempAddr[1], u32Count, 1);      // Add to control the volume
            }
        }
#endif
    }

    if (su8Playing & PLAYING_CH100) {
        sflash_read(&g_SPIFLASH, (unsigned long)sAudioDataAddrCh[2], (unsigned long*)SpiDataBuffer, (unsigned long)(COMPBUFSIZE << 1));
        LibS7Decode(&sEnDeCtlCh[2], &sS7Dec_CtxCh[2], (signed short *)SpiDataBuffer, (signed short *)BufferTempAddr[2]);

        sAudioDataAddrCh[2]    = sAudioDataAddrCh[2] + (COMPBUFSIZE * 2);
        sAudioSampleCountCh[2] = sAudioSampleCountCh[2] + AUDIOBUFFERSIZE;
#if AUDIO_MIX_OPT
        if (su8Playing != PLAYING_ALL) {
            for (u32Count = 0; u32Count < AUDIOBUFFERSIZE; u32Count++) {
                VolumeControl((int16_t *)BufferTempAddr[2], u32Count, 2);      // Add to control the volume
            }
        }
#endif
    }

    ///////////////// Mixer linear /////////////////

#if AUDIO_MIX_OPT
    for (u32Count = 0; u32Count < AUDIOBUFFERSIZE; u32Count++)
    {
        switch (su8Playing)
        {
            case PLAYING_CH001:
                *(pi16BufferValue+u32Count) = BufferTempAddr[0][u32Count];  // Prefore
                break;
            case PLAYING_CH010:
                *(pi16BufferValue+u32Count) = BufferTempAddr[1][u32Count];
                break;
            case PLAYING_CH011:
                if ((BufferTempAddr[0][u32Count] < 0) && (BufferTempAddr[1][u32Count] < 0))
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[1][u32Count])
                         + ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]) >> 15)) >> 1;
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[1][u32Count])
                         - ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]) >> 15)) >> 1;
                }
                break;
            case PLAYING_CH100:
                *(pi16BufferValue+u32Count) = BufferTempAddr[2][u32Count];
                break;
            case PLAYING_CH101:
                if ((BufferTempAddr[0][u32Count] < 0) && (BufferTempAddr[2][u32Count] < 0 ))
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[2][u32Count])
                         + ((BufferTempAddr[0][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[2][u32Count])
                         - ((BufferTempAddr[0][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                break;
            case PLAYING_CH110:
                if ((BufferTempAddr[1][u32Count] < 0) && (BufferTempAddr[2][u32Count] < 0 ))
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[1][u32Count] + BufferTempAddr[2][u32Count])
                         + ((BufferTempAddr[1][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[1][u32Count] + BufferTempAddr[2][u32Count])
                         - ((BufferTempAddr[1][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                break;
            case PLAYING_ALL:
                    if ((BufferTempAddr[0][u32Count] < 0) && (BufferTempAddr[1][u32Count] < 0 ))
                    {
                        *(pi16BufferValue+u32Count) =
                            (BufferTempAddr[0][u32Count] * su8Mix[0] + BufferTempAddr[1][u32Count] * su8Mix[1])
                            + ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]*su8Mix[0]*su8Mix[1]) >> 15) >> 1;
                    }
                    else
                    {
                        *(pi16BufferValue+u32Count) =
                            (BufferTempAddr[0][u32Count] * su8Mix[0] + BufferTempAddr[1][u32Count] * su8Mix[1])
                            - ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]*su8Mix[0]*su8Mix[1]) >> 15) >> 1;
                              }

                    if ((*(pi16BufferValue+u32Count) < 0) && (BufferTempAddr[2][u32Count] < 0 ))
                    {
                        *(pi16BufferValue+u32Count) =
                            ((*(pi16BufferValue+u32Count) + BufferTempAddr[2][u32Count] * su8Mix[2])
                             + ((*(pi16BufferValue+u32Count) * BufferTempAddr[2][u32Count] * su8Mix[2]) >> 15));
                    }
                    else
                    {
                        *(pi16BufferValue+u32Count) =
                            ((*(pi16BufferValue+u32Count) + BufferTempAddr[2][u32Count] * su8Mix[2])
                             - ((*(pi16BufferValue+u32Count) * BufferTempAddr[2][u32Count] * su8Mix[2]) >> 15));
                    }
                pcmValue = 0;
                if (su8Vol[0] & BIT0) {
                    pcmValue += (*(pi16BufferValue+u32Count) >> 4);
                }
                if (su8Vol[0] & BIT1) {
                    pcmValue += (*(pi16BufferValue+u32Count) >> 3);
                }
                if (su8Vol[0] & BIT2) {
                    pcmValue += (*(pi16BufferValue+u32Count) >> 2);
                }
                if (su8Vol[0] & BIT3) {
                    pcmValue += (*(pi16BufferValue+u32Count) >> 1);
                }
                if ((su8Vol[0] & BIT4) > 0) {
                    pcmValue = *(pi16BufferValue+u32Count);                    // No '+' here, FIXME - Why?
                }
                *(pi16BufferValue+u32Count) = pcmValue;
                break;
            default:
                *(pi16BufferValue + u32Count) = 0;
                break;

        }
    }



#else
    for (u32Count = 0; u32Count < AUDIOBUFFERSIZE; u32Count++)
    {
        switch (su8Playing)
        {
            case PLAYING_CH001:
                VolumeControl((int16_t *)BufferTempAddr[0], u32Count);      // Add to control the volume
                *(pi16BufferValue+u32Count) = BufferTempAddr[0][u32Count];  // Prefore
                break;
            case PLAYING_CH010:
                VolumeControl((int16_t *)BufferTempAddr[1], u32Count);      // Add to control the volume
                *(pi16BufferValue+u32Count) = BufferTempAddr[1][u32Count];
                break;
            case PLAYING_CH011:
                if ((BufferTempAddr[0][u32Count] < 0) && (BufferTempAddr[1][u32Count] < 0))
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[1][u32Count])
                         + ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]) >> 15)) >> 1;
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[1][u32Count])
                         - ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]) >> 15)) >> 1;
                }
                break;
            case PLAYING_CH100:
                VolumeControl((int16_t *)BufferTempAddr[2], u32Count);      // Add to control the volume
                *(pi16BufferValue+u32Count) = BufferTempAddr[2][u32Count];
                break;
            case PLAYING_CH101:
                if ((BufferTempAddr[0][u32Count] < 0) && (BufferTempAddr[2][u32Count] < 0 ))
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[2][u32Count])
                         + ((BufferTempAddr[0][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[0][u32Count] + BufferTempAddr[2][u32Count])
                         - ((BufferTempAddr[0][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                break;
            case PLAYING_CH110:
                if ((BufferTempAddr[1][u32Count] < 0) && (BufferTempAddr[2][u32Count] < 0 ))
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[1][u32Count] + BufferTempAddr[2][u32Count])
                         + ((BufferTempAddr[1][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        ((BufferTempAddr[1][u32Count] + BufferTempAddr[2][u32Count])
                         - ((BufferTempAddr[1][u32Count] * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                break;
            case PLAYING_ALL:
                if ((BufferTempAddr[0][u32Count] < 0) && (BufferTempAddr[1][u32Count] < 0 ))
                {
                    *(pi16BufferValue+u32Count) =
                        (BufferTempAddr[0][u32Count] + BufferTempAddr[1][u32Count])
                        + ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]) >> 15);
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        (BufferTempAddr[0][u32Count] + BufferTempAddr[1][u32Count])
                        - ((BufferTempAddr[0][u32Count] * BufferTempAddr[1][u32Count]) >> 15);
                }

                if ((*(pi16BufferValue+u32Count) < 0) && (BufferTempAddr[2][u32Count] < 0 ))
                {
                    *(pi16BufferValue+u32Count) =
                        ((*(pi16BufferValue+u32Count) + BufferTempAddr[2][u32Count])
                         + ((*(pi16BufferValue+u32Count) * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                else
                {
                    *(pi16BufferValue+u32Count) =
                        ((*(pi16BufferValue+u32Count) + BufferTempAddr[2][u32Count])
                         - ((*(pi16BufferValue+u32Count) * BufferTempAddr[2][u32Count]) >> 15)) >> 1;
                }
                break;
        }
    }
#endif
    ///////////////// End of Mixer /////////////////

    // FIXME
    // Why 4?
    // 4 - > define

    if (sAudioSampleCountCh[0] >= (sTotalG722SizeCh[0] * 4))
    {
        if (sbChLoopPlay[0])
        {
            sAudioSampleCountCh[0] = 0;
            sAudioDataAddrCh[0] = su32StartAddrCh[0];
        }
        else
        {
            su8Playing &= ~PLAYING_CH001; // Disable Ch0
        }
    }

    if (sAudioSampleCountCh[1] >= (sTotalG722SizeCh[1] * 4))
    {
        if (sbChLoopPlay[1])
        {
            sAudioSampleCountCh[1] = 0;
            sAudioDataAddrCh[1] = su32StartAddrCh[1];
        }
        else
        {
            su8Playing &= ~PLAYING_CH010; // Disable Ch1
        }
    }

    if (sAudioSampleCountCh[2] >= (sTotalG722SizeCh[2] * 4))
    {
        if (sbChLoopPlay[2])
        {
            sAudioSampleCountCh[2] = 0;
            sAudioDataAddrCh[2] = su32StartAddrCh[2];
        }
        else
        {
            su8Playing &= ~PLAYING_CH100; // Disable Ch2
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
static void PlayChannelSound(uint8_t ch)
{
    uint8_t i = 100;        // FIXME: why 100?
    if (ch >= CHANNEL_COUNT) {
        LOG("Ch out of range!\r\n");
        return ;
    }
#if AUDIO_MIX_OPT
    su8Vol[ch] = sCurrVol;
#endif

    sAudioDataAddrCh[ch]= su32StartAddrCh[ch];
    sAudioSampleCountCh[ch] = 0;

    if (!su8Playing) {
        su8Playing = (1 << ch);
#if AUDIO_MIX_OPT
        su8Mix[ch] = 1;
#endif

#if DPWM_RESET_CLOSE
        sPDMA1CallBackCount = 0;

        sBufferEmptyAddr = su32BufferAddr0;
        CopySpiSoundData();
        sBufferReadyAddr = su32BufferAddr0;
        DrvPDMA_Init();
        while (i--);        // TODO -> Sys_TimerDelay
        InitialDPWM(SAMPLE_RATE);
        PDMA1forDPWM();
#else
        InitialDPWM(SAMPLE_RATE);
        sPDMA1CallBackCount = 0;
        sBufferEmptyAddr = su32BufferAddr0;
        CopySpiSoundData();
        sBufferReadyAddr = su32BufferAddr0;
        DrvPDMA_Init();
        while (i--);        // TODO -> Sys_TimerDelay
        PDMA1forDPWM();
#endif


        sBufferEmptyAddr = su32BufferAddr1;
        sbBufferEmpty = TRUE;
    } else {
        su8Playing |= (1 << ch);

    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* Global Functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySpi_Close(void)
{
#if DPWM_RESET_CLOSE
    uint8_t i = 100;    // FIXME: why 100?
    //DrvDPWM_Close();
    while (i--);        // TODO -> Sys_TimerDelay
    DrvPDMA_Close();
#else
    DrvPDMA_Close();
    DrvDPWM_Close();
#endif
}


void PlaySpi_PauseResume(uint8_t ch)
{
    if (ch >= CHANNEL_COUNT) {
        LOG("Ch out of range!\r\n");
        return ;
    }

    // Can pause/resume by channel?
    // TODO

    sbPauseFlag = sbPauseFlag ^ 0x1;            // FIXME - Why?
    if (sbPauseFlag) {
        PDMA->GCR.HCLK_EN &= (~BIT1);           // Pause, PDMA channel 1 (BIT1) clock disable
    } else {
        PDMA->GCR.HCLK_EN |= BIT1;              // Resume, PDMA channel 1 (BIT1) clock enable
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* Play Loop                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySpi_PlayLoop(void)
{
    if (sbBufferEmpty) {
        CopySpiSoundData();
        sbBufferEmpty = FALSE;
    }
}

void PlaySpi_StopChannel(uint8_t ch)
{
#if GPIO_TEST
    DrvGPIO_Open(GPA, 13, IO_QUASI);
    DrvGPIO_SetPortBits(GPA, 0);
#endif

    if (ch >= CHANNEL_COUNT) {
        LOG("Ch out of range!\r\n");
        return ;
    }

#if AUDIO_MIX_OPT
    su8Mix[ch] = 0;
    su8Vol[ch] = 0;
#else
    su8Playing &= ~(1 << ch);
#endif
    sbChLoopPlay[ch] = FALSE;
    if (!su8Playing) {
        PlaySpi_Close();
    }
#if GPIO_TEST
    DrvGPIO_Close(GPA, 13);
#endif
}

void PlaySpi_PlayVpByChannel(uint16_t vpIdx, uint8_t ch)
{
    uint32_t  u32AudPointer;
    uint32_t  Temp0, Temp1;

#if GPIO_TEST
    DrvGPIO_Open(GPA, 13, IO_QUASI);
    DrvGPIO_SetPortBits(GPA, 0);
#endif

    if (ch >= CHANNEL_COUNT) {
        LOG("Ch out of range!\r\n");
        return ;
    }

    LibS7Init(&sEnDeCtlCh[ch], BIT_RATE, BANDWIDTH);
    LibS7DeBufReset(sEnDeCtlCh[ch].frame_size, &sS7Dec_CtxCh[ch]);

    //DrvPDMA_Init();    // PDMA initialization
    u32AudPointer = S7DATA_BASE_ADDR_ON_SPI + 8*(vpIdx+1);

    sflash_read(&g_SPIFLASH, (unsigned long)u32AudPointer, (unsigned long*)&Temp0, (unsigned long)4);
    sflash_read(&g_SPIFLASH, (unsigned long)(u32AudPointer+8), (unsigned long*)&Temp1, (unsigned long)4);
    su32StartAddrCh[ch] = Temp0 + S7DATA_BASE_ADDR_ON_SPI;
    sTotalG722SizeCh[ch] = Temp1 - Temp0;

    PlayChannelSound(ch);
#if GPIO_TEST
    //DrvGPIO_Close(GPA, 13);
#endif
}

void PlaySpi_PlayVpLoopByChannel(uint16_t vpIdx, uint8_t ch, BOOL bLoopPlay)
{
    uint32_t  u32AudPointer;
    uint32_t  Temp0, Temp1;

    if (ch >= CHANNEL_COUNT) {
        LOG("Ch out of range!\r\n");
        return ;
    }



    sbChLoopPlay[ch] = bLoopPlay; // For loop play

    LibS7Init(&sEnDeCtlCh[ch], BIT_RATE, BANDWIDTH);
    LibS7DeBufReset(sEnDeCtlCh[ch].frame_size, &sS7Dec_CtxCh[ch]);

    //DrvPDMA_Init();    // PDMA initialization
    u32AudPointer = S7DATA_BASE_ADDR_ON_SPI + 8 * (vpIdx + 1);

    sflash_read(&g_SPIFLASH, (unsigned long)u32AudPointer, (unsigned long*)&Temp0, (unsigned long)4);
    sflash_read(&g_SPIFLASH, (unsigned long)(u32AudPointer+8), (unsigned long*)&Temp1, (unsigned long)4);
    su32StartAddrCh[ch]= Temp0 + S7DATA_BASE_ADDR_ON_SPI;
    sTotalG722SizeCh[ch] = Temp1 - Temp0;

    PlayChannelSound(ch);
}

void PlaySpi_PlaySilenceByChannel(uint8_t ch)
{
}

void PlaySpi_PlaySilenceLoopByChannel(uint8_t ch)
{
}

void PlaySpi_PlayInsertSilenceByChannel(uint8_t ch)
{
}


void PlaySpi_PlayBackStopByChannel(uint8_t ch)
{
}

void PlaySpi_PlayBackReplayByChannel(uint8_t ch)
{
}

void PlaySpi_PlayBackPauseByChannel(uint8_t ch)
{
}

void PlaySpi_PlayBackResumeByChannel(uint8_t ch)
{
}

void PlaySpi_PlayBackRepeatByChannel(uint8_t ch)
{
}

void PlaySpi_PlayBackPlayPauseResumeByChannel(uint8_t ch)
{
}

void PlaySpi_PlayBackStopAll(void)
{
}

void PlaySpi_PlayBackPauseAll(void)
{
}


void PlaySpi_PlayBackCancelLastOne(void)
{
}

void PlaySpi_PlayBackCancelAll(void)
{
}

void PlaySpi_PlayBackClearAll(void)
{
}


void PlaySpi_VolumeSetByChannel(uint8_t vol, uint8_t ch)
{
}

void PlaySpi_VolumeUpByChannel(uint8_t ch)
{
}

void PlaySpi_VolumeDownByChannel(uint8_t ch)
{
}

void PlaySpi_VolumeMute(void)
{
}

void PlaySpi_VolumeUnmute(void)
{
}

void PlaySpi_VolumeSetAll(void)
{
}

void PlaySpi_VolumeUpAll(void)
{
}

void PlaySpi_VolumeDownAll(void)
{
}


void PlaySpi_SetVolume(uint8_t vol)
{
#if AUDIO_MIX_OPT
    sCurrVol = vol;
#else
    su8Vol = vol;
#endif
}

uint8_t PlaySpi_GetVolume(void)
{
#if AUDIO_MIX_OPT
    return sCurrVol;
#else
    return su8Vol;
#endif
}

uint8_t PlaySpi_GetPlayingStatus(void)
{
    return su8Playing;
}

/* vim: set ts=4 sw=4 tw=0 list : */
