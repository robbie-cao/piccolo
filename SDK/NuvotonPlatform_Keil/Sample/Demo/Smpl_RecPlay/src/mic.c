
#include "ISD9xx.h"

#include "libMIC.h"

void micInit(int enMicBias, int enZCD)
{
	/* Open Analog block */
	if(!SYSCLK->APBCLK.ANA_EN)
	{
		SYSCLK->APBCLK.ANA_EN = 1;
		SYS->IPRSTC2.ANA_RST = 1;
		SYS->IPRSTC2.ANA_RST = 0;
	}

	/* Power control */
	ANA->SIGCTRL.PU_MOD    = 1;
	ANA->SIGCTRL.PU_IBGEN  = 1;
	ANA->SIGCTRL.PU_BUFADC = 1;
	ANA->SIGCTRL.PU_BUFPGA = 1;
	ANA->SIGCTRL.PU_ZCD    = enZCD;

	/* PGA Setting */
	ANA->SIGCTRL.MUTE_PGA = 0x1;
	ANA->SIGCTRL.MUTE_IPBOOST = 0x0;

    ANA->PGAEN.REF_SEL    = 0; // vmid
    ANA->PGAEN.PU_PGA     = 1;
    ANA->PGAEN.PU_IPBOOST = 1;
    ANA->PGAEN.BOOSTGAIN  = 1; // 26dB

	ANA->PGA_GAIN.GAIN  = ((3200 + 1200) / 75);	  // 32 dB

	/* MIC circuit configuration */
    ANA->VMID.PULLDOWN = 0;
    ANA->VMID.PDLORES  = 0;
    ANA->VMID.PDHIRES  = 1;

	ANA->MICBEN = enMicBias;
    ANA->MICBSEL = 2; // 0.75

	/* ALC Setting */
	(*((volatile unsigned long*)0x400B0048)) = 0xFF01A360; // -9
	//(*((volatile unsigned long*)0x400B0048)) = 0xFF01D360; // -7.5
	//(*((volatile unsigned long*)0x400B0048)) = 0xFF01E360; // target level -6
}


void micClose(void)
{
    /* disable ADC interrupt */
    NVIC_DisableIRQ(ADC_IRQn);

    /* reset ADC */
    SYS->IPRSTC2.ADC_RST = 1;
    SYS->IPRSTC2.ADC_RST = 0;

    /* ADC engine clock disable */
    SYSCLK->APBCLK.ADC_EN = 0;
}

int micStart(unsigned long HCLK, int SR, int iFifoIntLevel, int iFifoIntEna, int enPDMA)
{
	micClose();

    /* Set ADC divisor */
    SYSCLK->CLKDIV.ADC_N = 0;

    /* ADC engine clock enable */
    SYSCLK->APBCLK.ADC_EN = 1;

    /* ADC enable */
    SDADC->EN = 1;

	if(HCLK/(SR<<7))
	{
		SDADC->CLK_DIV = HCLK/(SR<<7) - 1;
	}
	else
	{
		return -1;
	}
    SDADC->DEC.OSR = 1; // 128x

    /* Set FIFO interrupt level */
	if(iFifoIntLevel > 7) return -2;
    SDADC->INT.FIFO_IE_LEV = iFifoIntLevel;
	SDADC->INT.IE = iFifoIntEna;

    /* Set adc input channel */
	ANA->AMUX.EN       = 1;
	ANA->AMUX.MIC_SEL  = 1;
	ANA->AMUX.TEMP_SEL = 0;
	ANA->AMUX.MUXP_SEL = 0;
	ANA->AMUX.MUXN_SEL = 0;

	// Enable ADC PDMA and Trigger PDMA specified Channel 
	if(enPDMA)
	{
		SDADC->INT.IE = 0;
		SDADC->ADCPDMA.RxDmaEn = 1;
	}
	else
	{
		SDADC->ADCPDMA.RxDmaEn = 0;
	}

	ANA->SIGCTRL.MUTE_PGA = 0x0;

	if(SDADC->INT.IE)
	{
		NVIC_EnableIRQ(ADC_IRQn);
	}
	return 0;
}

void micMute(int en)
{
	ANA->SIGCTRL.MUTE_PGA = en;
}
