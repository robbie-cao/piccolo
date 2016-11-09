
#include "hw.h"

#include "ISD9xx.h"

#include "Driver/DrvSYS.h"
#include "Driver/DrvPDMA.h"

#include "libFFX.h"
#include "libMIC.h"

#include "audfile.h"

extern MemFlip codecFlipBuf;

/*--------------------------------------------------------------------------------------------------------*/
/* ADC/I2S RX Callback                                                                                    */
/*--------------------------------------------------------------------------------------------------------*/
void PDMA2_Callback(uint32_t status)
{
	codecFlipBuf.cur_idx ^= 0x01;
	if(g_play_rec_status & 0x01)
	{
	    PDMA->channel[eDRVPDMA_CHANNEL_2].DAR = (uint32_t)(codecFlipBuf.buf[codecFlipBuf.cur_idx]);
		DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);
	}
	else
	{
		g_play_rec_status |= 0x02;
	    PDMA->channel[eDRVPDMA_CHANNEL_2].CSR.TRIG_EN = 0;
	    PDMA->channel[eDRVPDMA_CHANNEL_2].CSR.PDMACEN = 0;
	}
}

void audInInit(void)
{
	micInit(1, 0); // don't use ZCD now
}

void audInClose(void)
{
	//micClose();
}

void audInStart(int SR)
{
	STR_PDMA_T sPDMA;  

    DrvPDMA_Init();

	PDMA_GCR->PDSSR.ADC_RXSEL = eDRVPDMA_CHANNEL_2;

	sPDMA.sSrcAddr.u32Addr 			= (uint32_t)&SDADC->ADCOUT;
    sPDMA.sDestAddr.u32Addr 		= (uint32_t)(codecFlipBuf.buf[codecFlipBuf.cur_idx]);
	sPDMA.u8TransWidth 				= eDRVPDMA_WIDTH_16BITS;
	sPDMA.u8Mode 					= eDRVPDMA_MODE_APB2MEM;
	sPDMA.sSrcAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_FIXED; 
	sPDMA.sDestAddr.eAddrDirection 	= eDRVPDMA_DIRECTION_INCREMENTED;   
    sPDMA.i32ByteCnt                = MEMFLIP_BUF_SIZE;
    DrvPDMA_Open(eDRVPDMA_CHANNEL_2, &sPDMA);

	// Enable INT 
	DrvPDMA_EnableInt(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD);

	NVIC_EnableIRQ(PDMA_IRQn);     

	// Install Callback function    
	DrvPDMA_InstallCallBack(eDRVPDMA_CHANNEL_2, eDRVPDMA_BLKD, (PFN_DRVPDMA_CALLBACK) PDMA2_Callback );	

	g_play_rec_status = 1;

	micStart(49152000, SR, 7, 0, 1);

	DrvPDMA_CHEnablelTransfer(eDRVPDMA_CHANNEL_2);
}

void audInStop(int wait)
{
	micMute(1);

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
