#include <stdio.h>
//#include "power.h"
#include "Driver/DrvRTC.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvCapSense.h"
#include "chirp.h"
char Cflag=0;
#define FIL_SHIFT 7

/*---------------------------------------------------------------------------------------------------------*/
/* interrupt routines                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/  
void CAPS_IRQHandler (void)
{
    Cflag=1;
	DrvCAPSENSE_INT_Disable();
}

//void TrimOsc(void)
//{
//	int32_t F0, F1, F2, F3, Freq;
//	int32_t i;
//
//
//	F1 = TrimOscillator(32768000, 1);
//	F0 = TrimOscillator(49152000, 0);
//	printf("\nOsc 0 %d Hz Trim %x\nOsc 1 %d Hz Trim %x\n", F0,SYS->OSCTRIM[0].TRIM, F1,SYS->OSCTRIM[1].TRIM);
//	
//	
//}

void LoopDelay(uint32_t delayCnt)
{
    while(delayCnt--)
        {
            __NOP();
            __NOP();
        }
}

void PWMTest() {
	int i,j;
	uint32_t isEnable = DPWM->CTRL.Enable;
	DPWM->CTRL.Enable = 1;
	for (i=0; i <5; i++) {
		for (j=0; j <4800; j++) {
		while (DPWM->STAT.Full == 1);
		DPWM->FIFO = chirp[j];
		}
	}
	DPWM->CTRL.Enable = isEnable;
}
/*---------------------------------------------------------------------------------------------------------*/
/* Set up GPIO interrupt                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/    
void CapSense(void)
{
    int32_t CapFil[1];
	int32_t CapFilLong[1];
	int32_t i=0;
	int32_t Touch=0;
	
	/* Configure GPB4 for CapSense */
	DrvGPIO_Open(GPB, 4, IO_OUTPUT);
    NVIC_EnableIRQ(CAPS_IRQn);
    //SYS->GPB_ALT.GPB0 = 2;
    SYS->GPB_ALT.GPB4 = 2;
    // Set up analog comapartor.
    //ACMP->CMPSEL = 0;
    ACMP->CMPSEL = 4;
	ACMP->CMPCR[0].CMPEN = 1;
    // Set up Isrc
    DrvCAPSENSE_ISRC_En(BIT4);
	DrvCAPSENSE_ISRC_Val(3);
	
	DrvCAPSENSE_Ctrl(LOW_TIME_3,CYCLE_CNT_2);
	DrvCAPSENSE_INT_Enable();
	DrvCAPSENSE_ResetCnt();
	DrvCAPSENSE_CntEnable();

	CapFil[0]=0;
    CapFilLong[0]=0;

	i=0;
	while(1){
        while(Cflag==0)
            LoopDelay(10);
		if(i<2){
		CapFil[0] = ((int32_t)ANA->CAPS_CNT<<FIL_SHIFT)  ;
		CapFilLong[0] = ((int32_t)ANA->CAPS_CNT<<FIL_SHIFT) ;
        }else{
		CapFil[0] += (((int32_t)ANA->CAPS_CNT<<FIL_SHIFT) - CapFil[0])>>4 ;
		CapFilLong[0] += (((int32_t)ANA->CAPS_CNT<<FIL_SHIFT) - CapFilLong[0])>>8 ;
        }
		//if(i%32==0)
		//	printf ("Cval %d %d %d\n",ANA->CAPS_CNT>>8,CapFil[0]>>(FIL_SHIFT+8),CapFilLong[0]>>(FIL_SHIFT+8));
        i++;
		if(	((int32_t)ANA->CAPS_CNT)  > ((CapFilLong[0] + (CapFilLong[0]>>3))>>FIL_SHIFT)) {
			if(Touch == 0){ 
			printf ("Touch %d %d %d\n",	((int32_t)ANA->CAPS_CNT) ,CapFil[0]>>(FIL_SHIFT),((CapFilLong[0] + (CapFilLong[0]>>4))>>FIL_SHIFT));
			Touch =1;
			PWMTest();
			}
		}else
			Touch =0;	
		Cflag=0;

		DrvCAPSENSE_ResetCnt();
		DrvCAPSENSE_INT_Enable();
		DrvCAPSENSE_CntEnable();

    }
    
}

