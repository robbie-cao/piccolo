#include "hw.h"

#include "ISD9xx.h"
#include "Driver\DrvADC.h"
#include "Driver\DrvDPWM.h"
#include "Driver\DrvPDMA.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvSPI.h"
#include "libSPIFlash.h"

static uint32_t ui32TempSPIdivider;
extern const SFLASH_CTX g_SPIFLASH;

unsigned long SysTickCount;

#define SystemCoreClock 49152000
  
//This function do SPI bus control when Flash & LCD share the same SPI bus
void SpiBusCtrl(int iCtrlMode)
{

	if(iCtrlMode)
	{ //Enter LCD mode
		ui32TempSPIdivider = ((volatile SPI_T *)SPI0_BASE)->DIVIDER.DIVIDER;
	    ((volatile SPI_T *)SPI0_BASE)->DIVIDER.DIVIDER = SPIclkDivider_LCD;
        ((volatile SPI_T *)SPI0_BASE)->SSR.SSR = 1; // channel
	    ((volatile SPI_T *)SPI0_BASE)->CNTRL.CLKP = 1;
    }
    else
    { //Leave LCD mode
	    ((volatile SPI_T *)SPI0_BASE)->DIVIDER.DIVIDER = ui32TempSPIdivider;
        ((volatile SPI_T *)SPI0_BASE)->SSR.SSR = 0; // channel
	    ((volatile SPI_T *)SPI0_BASE)->CNTRL.CLKP = 0;
	    sflash_set(&g_SPIFLASH);
    } 
	 
}

void hwInit(void)
{
	DrvSYS_UnlockKeyAddr();

	DrvSYS_SetOscCtrl(E_SYS_OSC49M, 1);

	/* HCLK clock source. */
	DrvSYS_SetHCLKSource(0);

	/* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); 

	DrvSYS_LockKeyAddr();

	SysTick_Config(SystemCoreClock/SYSTICKINTRATE);

	NVIC_EnableIRQ(SysTick_IRQn);
}

__weak void onSysTick(void)
{
}

void SysTick_Handler(void)
{
	SysTickCount ++;
	onSysTick();
}

