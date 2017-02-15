/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvDPWM.h"

#include "SDaccess.h"

//--------------------
// Buffer and global variables for playing
#define AUDIOBUFFERSIZE     0x100       // 256 samples
uint32_t    u32BufferAddr0, u32BufferAddr1;
volatile uint32_t AudioSampleCount, PDMA1CallBackCount, AudioDataAddr, BufferEmptyAddr, BufferReadyAddr;
BOOL        bPCMPlaying, bBufferEmpty, PDMA1Done;
uint8_t     u8LastTwoBufferCount;
uint32_t	AudioDataCount,u32DataSector = 0;


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback(void);
//void PDMA1forDPWM(void);




/*---------------------------------------------------------------------------------------------------------*/
/* InitialDPWM                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
void InitialDPWM(uint32_t u32SampleRate)
{
    DrvDPWM_Open();
    DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLKX2);
    //DrvDPWM_SetDPWMClk(E_DRVDPWM_DPWMCLK_HCLK);
    DrvDPWM_SetSampleRate(u32SampleRate);
    DrvDPWM_Enable();
}


/*---------------------------------------------------------------------------------------------------------*/
/* Set PDMA0 to move ADC FIFO to MIC buffer with wrapped-around mode                                       */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1forDPWM(void)
{
    STR_PDMA_T sPDMA;

    sPDMA.sSrcAddr.u32Addr         = BufferReadyAddr;
    sPDMA.sDestAddr.u32Addr        = (uint32_t)&DPWM->FIFO;
    sPDMA.u8Mode                   = eDRVPDMA_MODE_MEM2APB;;
    sPDMA.u8TransWidth             = eDRVPDMA_WIDTH_16BITS;
    sPDMA.sSrcAddr.eAddrDirection  = eDRVPDMA_DIRECTION_WRAPAROUND;
    sPDMA.sSrcAddr.eAddrDirection  = eDRVPDMA_DIRECTION_INCREMENTED;;
    sPDMA.sDestAddr.eAddrDirection = eDRVPDMA_DIRECTION_FIXED;
    sPDMA.u8WrapBcr                = eDRVPDMA_WRA_WRAP_HALF_INT;        // Interrupt condition set fro Half buffer & buffer end
    sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 4;         // Full MIC buffer length (byte), wrap around
    //sPDMA.i32ByteCnt = AUDIOBUFFERSIZE * 2;       // Full MIC buffer length (byte)
    DrvPDMA_Open(eDRVPDMA_CHANNEL_1, &sPDMA);

    // PDMA Setting
    //PDMA_GCR->PDSSR.ADC_RXSEL = eDRVPDMA_CHANNEL_2;
    DrvPDMA_SetCHForAPBDevice(
            eDRVPDMA_CHANNEL_1,
            eDRVPDMA_DPWM,
            eDRVPDMA_WRITE_APB
            );

    // Enable DPWM DMA
    DrvDPWM_EnablePDMA();
    // Enable INT
    DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR);            // For WARPROUND
    //DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD );        // For INCREMENTED
    // Install Callback function
    DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_WAR, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback );
    //DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_1, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA1_Callback );
    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_1);

    PDMA1Done = FALSE;
}


/*---------------------------------------------------------------------------------------------------------*/
/* DPWM Callback                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA1_Callback()
{
    if ((PDMA1CallBackCount & 0x1) == 0)
    {
        BufferReadyAddr = u32BufferAddr1;
        BufferEmptyAddr = u32BufferAddr0;
    }
    else
    {
        BufferReadyAddr = u32BufferAddr0;
        BufferEmptyAddr = u32BufferAddr1;
    }


    PDMA1CallBackCount++;
    if (u8LastTwoBufferCount <= 2)
    {
        PDMA1forDPWM();                 // for Incremental
        u8LastTwoBufferCount++;
        if (u8LastTwoBufferCount >= 2) {
            bPCMPlaying = FALSE;
        }
    }
    else
    {
        bBufferEmpty = TRUE;
        PDMA1forDPWM();                 // for Incremental
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* Copy Data from SD Card to SRAM                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void CopySoundData(uint32_t TotalPCMCount)
{
    // Read data to empty buffer from SD card
    //u32DataSector = AudioDataAddr / SD_SECTOR_SIZE;
    disk_read (0, (unsigned char *)BufferEmptyAddr, u32DataSector, 1);
    u32DataSector += 1;

    AudioSampleCount = AudioSampleCount + AUDIOBUFFERSIZE;
    AudioDataAddr = AudioDataAddr + (AUDIOBUFFERSIZE * 2);
    if (AudioSampleCount >= TotalPCMCount) {
        u8LastTwoBufferCount = 0;
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* Play sound initialization                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySound(uint32_t DataAddr, uint32_t TotalPCMCount)
{
    InitialDPWM(16000);
    AudioDataAddr        = DataAddr;
    bPCMPlaying          = TRUE;
    u8LastTwoBufferCount = 0xFF;
    AudioSampleCount     = 0;
    PDMA1CallBackCount   = 0;

    BufferEmptyAddr = u32BufferAddr0;
    CopySoundData(TotalPCMCount);
    BufferReadyAddr = u32BufferAddr0;
    PDMA1forDPWM();

    BufferEmptyAddr = u32BufferAddr1;
    bBufferEmpty = TRUE;
}



/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
void PlaySPIFlash(uint32_t PlayStartAddr, uint32_t TotalPCMCount)
{
    // uint8_t  u8Option;
    // int32_t  i32Err;
    // uint32_t u32temp;
    __align(4) int16_t AudioBuffer[2][AUDIOBUFFERSIZE];

    u32BufferAddr0 = (uint32_t) &AudioBuffer[0][0];
    u32BufferAddr1 = (uint32_t) &AudioBuffer[1][0];

    printf("+-------------------------------------------------------------------------+\n");
    printf("|       Playing Sampling PCM Start                                        |\n");
    printf("+-------------------------------------------------------------------------+\n");


    DrvPDMA_Init();         // PDMA initialization
    UNLOCKREG();

    PlaySound(PlayStartAddr, TotalPCMCount);


    while (bPCMPlaying == TRUE)
    {
        if (bBufferEmpty == TRUE)
        {
            CopySoundData(TotalPCMCount);
            bBufferEmpty = FALSE;
        }


        if ((PDMA1Done == TRUE) && (bBufferEmpty == FALSE)) {
            PDMA1forDPWM();
        }

    }

    //while (bBufferEmpty == FALSE);           // Waiting last audio buffer played

    DrvPDMA_Close();
    DrvDPWM_Close();
    UNLOCKREG();

    printf("Play Done\n");

    /* Lock protected registers */
}



