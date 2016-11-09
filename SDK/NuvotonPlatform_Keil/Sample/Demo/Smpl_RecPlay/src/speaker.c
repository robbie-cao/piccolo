
#include "ISD9xx.h"

#include "libSpeaker.h"

void speakerInit(void)
{
	if(!SYSCLK->APBCLK.ANA_EN)
	{
		SYSCLK->APBCLK.ANA_EN = 1;
		SYS->IPRSTC2.ANA_RST = 1;
		SYS->IPRSTC2.ANA_RST = 0;
	}

	/* enable DPWM clock */
	SYSCLK->APBCLK.DPWM_EN        = 1;

	/* reset DPWM */
	SYS->IPRSTC2.DPWM_RST         = 1;
	SYS->IPRSTC2.DPWM_RST         = 0;
	SYSCLK->CLKSEL1.DPWM_S        = 1; // clock 2*hclk
}

void speakerClose(void)
{
	DPWM->CTRL.Enable = 0;
	SYS->IPRSTC2.DPWM_RST         = 1;
	SYS->IPRSTC2.DPWM_RST         = 0;
	SYSCLK->APBCLK.DPWM_EN        = 0;
}

void speakerStart(unsigned long HCLK, int SR, int enPDMA)
{
	DPWM->ZOH_DIV = (uint8_t)((HCLK/64)/SR);
	DPWM->DMA.EnablePDMA = enPDMA;
	DPWM->CTRL.Enable = 1;
}

void speakerMute(int en)
{
	DPWM->CTRL.Enable = !en;
}
