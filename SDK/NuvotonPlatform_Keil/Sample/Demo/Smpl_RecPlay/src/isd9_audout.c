
#include "hw.h"

#include "ISD9xx.h"

#include "Driver/DrvSYS.h"
#include "Driver/DrvPDMA.h"

#include "libFFX.h"
#include "libSpeaker.h"

#include "audfile.h"

static unsigned long uval_zero = 0;

extern MemFlip codecFlipBuf;

//AudFileInf gAudFileInf;

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA Callback function                                                                           	   */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA3_TA_Callback(uint32_t status)
{    
	DrvPDMA_CHSoftwareReset(eDRVPDMA_CHANNEL_3);
	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
}

/*--------------------------------------------------------------------------------------------------------*/
/* DPWM TX Callback                                                                                       */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA3_Callback(uint32_t status)
{
	codecFlipBuf.cur_idx ^= 0x01;
	if(g_play_rec_status & 0x01)
	{
		PDMA->channel[eDRVPDMA_CHANNEL_3].CSR.SAD_SEL = eDRVPDMA_DIRECTION_INCREMENTED; 
	    PDMA->channel[eDRVPDMA_CHANNEL_3].SAR = (uint32_t)(codecFlipBuf.buf[codecFlipBuf.cur_idx]);
	    DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
	}
	else
	{
		g_play_rec_status |= 0x02;
	    PDMA->channel[eDRVPDMA_CHANNEL_3].CSR.TRIG_EN = 0;
	    PDMA->channel[eDRVPDMA_CHANNEL_3].CSR.PDMACEN = 0;
	}
}

void audOutInit(void)
{
	speakerInit();
}

void audOutClose(void)
{
	speakerClose();
}

void audOutStart(int SR)
{
	STR_PDMA_T sPDMA;

    DrvPDMA_Init();

	PDMA_GCR->PDSSR.DPWM_TXSEL = eDRVPDMA_CHANNEL_3;

	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&uval_zero;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)&DPWM->FIFO;  
    sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_MEM2APB;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED;   
	sPDMA.i32ByteCnt                = MEMFLIP_BUF_SIZE;
	DrvPDMA_Open(eDRVPDMA_CHANNEL_3, &sPDMA);

	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_3, eDRVPDMA_BLKD);

	NVIC_EnableIRQ(PDMA_IRQn);     

	// Install Callback function    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA3_Callback ); 	 
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_3, eDRVPDMA_TABORT, (PFN_DRVPDMA_CALLBACK) PDMA3_TA_Callback ); 	

	g_play_rec_status = 1;

	speakerStart(49152000, SR, 1);

	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_3);
}

void audOutStop(int wait)
{
	// speakerMute(1);

	if(g_play_rec_status & 0x01ul)
	{
		g_play_rec_status &= ~0x01ul;
		if(wait)
		{
			while(!g_play_rec_status)
			{
				__wfi();
			}
		}
	}
}
