/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvGPIO.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"

#define SIGNATURE       0x125ab234
#define FLAG_ADDR       0x20003FFC



/*---------------------------------------------------------------------------------------------------------*/
/*  Simple calculation test function						                                               */
/*---------------------------------------------------------------------------------------------------------*/
#define PI_NUM  256
int32_t f[PI_NUM+1];
uint32_t piTbl[19] = {
3141,
5926,
5358,
9793,
2384,
6264,
3383,
2795,
 288,
4197,
1693,
9937,
5105,
8209,
7494,
4592,
3078,
1640,
6284

};

int32_t piResult[19];

int32_t pi(void) 
{
    int32_t i,i32Err;
    int32_t a=10000,b=0,c=PI_NUM,d=0,e=0,g=0;     


    for(;b-c;)
        f[b++]=a/5;
    
    i = 0;
    for(;d=0,g=c*2;c -=14,/*printf("%.4d\n",e+d/a),*/ piResult[i++] = e+d/a,e=d%a)
    {
        if(i==19)
            break;

        for(b=c; d+=f[b]*a,f[b]=d%--g,d/=g--,--b; d*=b);
    }
    i32Err = 0;
    for(i=0;i<19;i++)
    {
        if(piTbl[i] != piResult[i])
            i32Err = -1;    
    }

    return i32Err;
}

void Delay(uint32_t x)
{
    int32_t i;
    
    for(i=0;i<x;i++)
    {
        __NOP();
        __NOP();
    }

}

void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 22; /* Assume the internal 22MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}

void clkTest(void)
{
// WW: DrvSYS_GetVersion(void)
	printf("DrvSYS_GetVersion: %d\n", DrvSYS_GetVersion());

// WW: DrvSYS_GetEXTClock(void)
	printf("DrvSYS_GetEXTClock: %d KHz\n", DrvSYS_GetEXTClock());

// WW: DrvSYS_GetHCLK(void)
	printf("DrvSYS_GetHCLK: %d KHz\n", DrvSYS_GetHCLK());

// WW: DrvSYS_SetClockDivider
	DrvSYS_SetClockDivider(E_SYS_ADC_DIV, 0x55);//0x5000_0218[23:16]
	DrvSYS_SetClockDivider(E_SYS_ADC_DIV, 0x5A12);//0x5000_0218[23:16]
	DrvSYS_SetClockDivider(E_SYS_UART_DIV, 0x3);//0x5000_0218[11:8]
	DrvSYS_SetClockDivider(E_SYS_UART_DIV, 0xB);//0x5000_0218[11:8]
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0x8);//0x5000_0218[3:0]
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0xE);////0x5000_0218[3:0]

// WW: DrvSYS_SetHCLKSource
	DrvSYS_SetHCLKSource(0);//0x5000_0210[2:0]
	DrvSYS_SetHCLKSource(1);//0x5000_0210[2:0]
	DrvSYS_SetHCLKSource(2);//0x5000_0210[2:0]

// WW: DrvSYS_SetIPClock
	DrvSYS_SetIPClock(E_SYS_PDMA_CLK, 0);//0x5000_0204[1]
	DrvSYS_SetIPClock(E_SYS_PDMA_CLK, 1);//0x5000_0204[1]
	DrvSYS_SetIPClock(E_SYS_ISP_CLK, 1);//0x5000_0204[2]
	DrvSYS_SetIPClock(E_SYS_ISP_CLK, 0);//0x5000_0204[2]
	DrvSYS_SetIPClock(E_SYS_WDG_CLK, 1);//0x5000_0208[4]
	DrvSYS_SetIPClock(E_SYS_WDG_CLK, 0);//0x5000_0208[4]
	DrvSYS_SetIPClock(E_SYS_RTC_CLK, 1);//0x5000_0208[5]
	DrvSYS_SetIPClock(E_SYS_RTC_CLK, 0);//0x5000_0208[5]
	DrvSYS_SetIPClock(E_SYS_TMR0_CLK, 1);//0x5000_0208[6]
	DrvSYS_SetIPClock(E_SYS_TMR0_CLK, 0);//0x5000_0208[6]
	DrvSYS_SetIPClock(E_SYS_TMR1_CLK, 1);//0x5000_0208[7]
	DrvSYS_SetIPClock(E_SYS_TMR1_CLK, 0);//0x5000_0208[7]
	DrvSYS_SetIPClock(E_SYS_I2C0_CLK, 1);//0x5000_0208[8]
	DrvSYS_SetIPClock(E_SYS_I2C0_CLK, 0);//0x5000_0208[8]
	DrvSYS_SetIPClock(E_SYS_SPI0_CLK, 1);//0x5000_0208[12]
	DrvSYS_SetIPClock(E_SYS_SPI0_CLK, 0);//0x5000_0208[12]
	DrvSYS_SetIPClock(E_SYS_DPWM_CLK, 1);//0x5000_0208[13]
	DrvSYS_SetIPClock(E_SYS_DPWM_CLK, 0);//0x5000_0208[13]
	DrvSYS_SetIPClock(E_SYS_UART0_CLK, 1);//0x5000_0208[16]
	DrvSYS_SetIPClock(E_SYS_UART0_CLK, 0);//0x5000_0208[16]
	DrvSYS_SetIPClock(E_SYS_BIQ_CLK, 1);//0x5000_0208[18]
	DrvSYS_SetIPClock(E_SYS_BIQ_CLK, 0);//0x5000_0208[18]
	DrvSYS_SetIPClock(E_SYS_CRC_CLK, 1);//0x5000_0208[19]
	DrvSYS_SetIPClock(E_SYS_CRC_CLK, 0);//0x5000_0208[19]
	DrvSYS_SetIPClock(E_SYS_PWM01_CLK, 1);//0x5000_0208[20]
	DrvSYS_SetIPClock(E_SYS_PWM01_CLK, 0);//0x5000_0208[20]
	DrvSYS_SetIPClock(E_SYS_ACMP_CLK, 1);//0x5000_0208[22]
	DrvSYS_SetIPClock(E_SYS_ACMP_CLK, 0);//0x5000_0208[22]
	DrvSYS_SetIPClock(E_SYS_SBRAM_CLK, 1);//0x5000_0208[26]
	DrvSYS_SetIPClock(E_SYS_SBRAM_CLK, 0);//0x5000_0208[26]
	DrvSYS_SetIPClock(E_SYS_ADC_CLK, 1);//0x5000_0208[28]
	DrvSYS_SetIPClock(E_SYS_ADC_CLK, 0);//0x5000_0208[28]
	DrvSYS_SetIPClock(E_SYS_I2S_CLK, 1);//0x5000_0208[29]
	DrvSYS_SetIPClock(E_SYS_I2S_CLK, 0);//0x5000_0208[29]
	DrvSYS_SetIPClock(E_SYS_ANA_CLK, 1);//0x5000_0208[30]
	DrvSYS_SetIPClock(E_SYS_ANA_CLK, 0);//0x5000_0208[30]

// WW: DrvSYS_SetIPClockSource
	DrvSYS_SetIPClockSource(E_SYS_I2S_CLKSRC, 1); //0x5000_021C[1:0]
	DrvSYS_SetIPClockSource(E_SYS_I2S_CLKSRC, 3); //0x5000_021C[1:0]
	DrvSYS_SetIPClockSource(E_SYS_PWM01_CLKSRC, 2);//0x5000_0214[29:28]
	DrvSYS_SetIPClockSource(E_SYS_PWM01_CLKSRC, 1);//0x5000_0214[29:28]
	DrvSYS_SetIPClockSource(E_SYS_TMR1_CLKSRC, 2);//0x5000_0214[14:12]
	DrvSYS_SetIPClockSource(E_SYS_TMR1_CLKSRC, 3);//0x5000_0214[14:12]
	DrvSYS_SetIPClockSource(E_SYS_TMR0_CLKSRC, 2);//0x5000_0214[10:8]
	DrvSYS_SetIPClockSource(E_SYS_TMR0_CLKSRC, 1);//0x5000_0214[10:8]
	DrvSYS_SetIPClockSource(E_SYS_DPWM_CLKSRC, 1);//0x5000_0214[4]
	DrvSYS_SetIPClockSource(E_SYS_DPWM_CLKSRC, 0);//0x5000_0214[4]
	DrvSYS_SetIPClockSource(E_SYS_WDG_CLKSRC, 2);//0x5000_0214[1:0]
	DrvSYS_SetIPClockSource(E_SYS_WDG_CLKSRC, 1);//0x5000_0214[1:0]

// WW: DrvSYS_SetOscCtrl
	DrvSYS_SetOscCtrl(E_SYS_XTL32K, 1); //0x5000_0200[1]
	DrvSYS_SetOscCtrl(E_SYS_XTL32K, 0); //0x5000_0200[1]
	DrvSYS_SetOscCtrl(E_SYS_OSC49M, 1); //0x5000_0200[2]
	DrvSYS_SetOscCtrl(E_SYS_OSC49M, 0); //0x5000_0200[2]
	DrvSYS_SetOscCtrl(E_SYS_OSC10K, 1); //0x5000_0200[3]
	DrvSYS_SetOscCtrl(E_SYS_OSC10K, 0); //0x5000_0200[3]

// WW: DrvSYS_SetSysTickSource
	DrvSYS_SetSysTickSource(1);//0x5000_0210[5:3]
	DrvSYS_SetSysTickSource(3);//0x5000_0210[5:3]
	DrvSYS_SetSysTickSource(2);//0x5000_0210[5:3]
	DrvSYS_SetSysTickSource(7);//0x5000_0210[5:3]
	DrvSYS_SetSysTickSource(6);//0x5000_0210[5:3]
}

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
    int32_t  i32ret;
    uint32_t u32data;
	STR_UART_T sParam;
    int32_t i;
//    uint32_t u32Freq;
    
	UNLOCKREG();
//    SYSCLK->PWRCON.XTL12M_EN = 1;
    /* Waiting for 12M Xtal stalble */
    SysTimerDelay(5000);

	/* Set UART Pin */
	DrvGPIO_InitFunction(FUNC_UART0);
	
	/* UART Setting */
    sParam.u32BaudRate 		= 115200;
    sParam.u8cDataBits 		= DRVUART_DATABITS_8;
    sParam.u8cStopBits 		= DRVUART_STOPBITS_1;
    sParam.u8cParity 		= DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

	/* Set UART Configuration */
	DrvUART_Open(UART_PORT0,&sParam);



	printf("+------------------------------------------------------------------------+\n");
    printf("|                       System Driver Sample Code                        |\n");
    printf("|                                                                        |\n");
    printf("+------------------------------------------------------------------------+\n");                    
///////////////////////////////////////////////////////////////////////////////////////////
	clkTest();
///////////////////////////////////////////////////////////////////////////////////////////

    if(M32(FLAG_ADDR) == SIGNATURE)
    {
        printf("  CPU Reset success!\n");
        M32(FLAG_ADDR) = 0;
        printf("  Press any key to continue ...\n");
        getchar();
    }

/*---------------------------------------------------------------------------------------------------------*/
/* Misc system function test                                                                               */
/*---------------------------------------------------------------------------------------------------------*/

	/* Read Product ID */
	//u32data = DrvSYS_ReadProductID();
	//printf("Product ID 0x%x\n", u32data);
    	
	/* Get reset source from last operation */
	u32data = DrvSYS_GetRstSrc();
	printf("Reset Source 0x%x\n", u32data);
	
	/* Clear reset source */
	//DrvSYS_ClearRstSrc(0x00);
	DrvSYS_ClearRstSrc(u32data);

	/* Reset IP (I2C0) */
	DrvSYS_ResetIP(E_SYS_I2C0_RST);
	
	/* Unlock key address register before reset CPU */
	i32ret = DrvSYS_UnlockKeyAddr();
	if (i32ret == 0)
	{
		printf("Key Address is Unlocked\n");	
	}

	/* Enable Brown Our Detector */
//	DrvSYS_EnableBOD(1);
	
	/* Set Brown Our Detector voltage 2.2V */
//	DrvSYS_SelectBODVolt(0);
	
	/* Enable Brown Out Interrupt function and install its callback function */
//	DrvSYS_EnableBODRst(0, BOD_CallbackFn);

	/* Configure PLL clock */
//	DrvSYS_Open(12000, 50000);
	
	/* Get PLL clock frequency */
//	u32data = DrvSYS_GetPLLClock();
//	printf("PLL clock %d KHz\n", u32data);
	
	/* Set IP clock source(ADC clock source from 12M) */
	DrvSYS_SetIPClockSource(E_SYS_ADC_CLKSRC, 0x00);
	
	/* Enable IP clock(I2C0) */
	DrvSYS_SetIPClock(E_SYS_I2C0_CLK, 1);

/*---------------------------------------------------------------------------------------------------------*/
/* PLL clock confiruation test                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
    
    printf("-------------------------[ Test PLL ]-----------------------------\n");


//    u32Freq = 14000;
    for(i=0;i<10;i++)
    {
//        DrvSYS_Open(12000, u32Freq + 4000 * i);
        printf("  Change system clock to %d kHz ......................... ", DrvSYS_GetHCLK());
        
        /* The delay loop is used to check if the CPU speed is incresing */
        Delay(0x400000);
        
        if(pi())
        {
            printf("[FAIL]\n");
        }
        else
        {
            printf("[OK]\n");
        }
    }

	
    
    /* Write a signature work to SRAM to check if it is reset by software */   
    M32(FLAG_ADDR) = SIGNATURE;	
    printf("\n\n  >>> Reset CPU <<<\n");

    /* Waiting for message send out */
    while(UART0->FSR.TE == 0);


	/* Reset CPU */
	DrvSYS_ResetCPU();



}




