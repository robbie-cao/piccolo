#include "ISD9xx.h"

//==============================================
//BIQ parameters b0,b1,b2,a1,a2,b0,b1,b2,a1,a2,b0,b1,b2,a1,a2
//---------------------
//BIQ 20~400 Hz
/*
	uint32_t u32BiqCoeff[15]={0x07084, 0x71ef8, 0x07084, 0x600ec, 0x0ff18,
								0x0033d, 0x7ff82, 0x0033d, 0x6173b, 0x0eec5, 
								0x01302, 0x00000, 0x7ecfe, 0x61298, 0x0edb6};
//*/
//---------------------
//BIQ 100~1200 Hz
/*
	uint32_t u32BiqCoeff[15]={0x06940, 0x72d7f, 0x06940, 0x604a3, 0x0fbc3,
								0x00e17, 0x01585, 0x00e17, 0x65e64, 0x0d3a9,
								0x0342f, 0x00000, 0x7cbd1, 0x6368a, 0x0cdcb};
//*/
//---------------------
//BIQ 100~2500 Hz
/*
	uint32_t u32BiqCoeff[15]={0x07243, 0x71b7a, 0x07243, 0x60502, 0x0fb63,
								0x03067, 0x05c2a, 0x03067, 0x70f33, 0x0ae08,
								0x06b46, 0x00000, 0x794ba, 0x66fb4, 0x098ca};
//*/
//---------------------
//BIQ 100~4000 Hz
/*
	uint32_t u32BiqCoeff[15]={0x075ce, 0x71464, 0x075ce, 0x60527, 0x0fb3e,
								0x06792, 0x0cc5a, 0x06792, 0x7fe08, 0x09da7,
								0x0abb8, 0x00000, 0x75448, 0x6b293, 0x05ac9};
//*/
//---------------------
//BIQ 20~8000 Hz	 Sr32k
//*
	uint32_t u32BiqCoeff[15]={0x07a9f, 0x70ac3, 0x07a9f, 0x60080, 0x0ff81,
								0x0671b, 0x0cb88, 0x0671b, 0x7fed3, 0x09af4,
								0x0afdc, 0x00000, 0x75024, 0x6aa8a, 0x056ce};
//*/

void Tdelay(void)
{
uint32_t	u32temp;

	for(u32temp=0; u32temp<32; u32temp++);
}


//Default sampling rate is 16K (need matched with ADC sampling rate) 
void BiqSetting(uint8_t u8Path)			//u8Path=0 for ADC, u8Path=1 for DPWM
{
uint32_t u32BiqCount, *pu32Biq;

   	SYSCLK->APBCLK.BIQALC_EN = 1;
	Tdelay();
	BIQ->BIQ_CTRL.RSTn = 1;
	Tdelay();
	//outpw(0x400B0040,inpw(0x400B0040)|BIT31);
	Tdelay();
	BIQ->BIQ_CTRL.PRGCOEF=1;
	Tdelay();
	pu32Biq=(uint32_t *)BIQ_BA;
	for(u32BiqCount=0; u32BiqCount<15; u32BiqCount++)
		*pu32Biq++ = u32BiqCoeff[u32BiqCount];
	Tdelay();
	if(u8Path==1)
	{
		//BIQ->BIQ_CTRL.SR_DIV=511;	 	//for SR 32K
		BIQ->BIQ_CTRL.SR_DIV=1023;	 	//for SR 16K
		BIQ->BIQ_CTRL.UPSR=2;			//for SR 16K
		BIQ->BIQ_CTRL.SELPWM=1;
	}
	else
		BIQ->BIQ_CTRL.SR_DIV=1535;	 	//for SR 32K

	BIQ->BIQ_CTRL.PRGCOEF=0;
	Tdelay();
	BIQ->BIQ_CTRL.EN=1;
}
