/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2011 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvFMC.h"
#include "Driver\DrvBODTALARM.h"
#include "ISD9xx.h"
#define SYSTICK_ENABLE              0                                          /* Config-Bit to start or stop the SysTick Timer                         */
#define SYSTICK_TICKINT             1                                          /* Config-Bit to enable or disable the SysTick interrupt                 */
#define SYSTICK_CLKSOURCE           2                                          /* Clocksource has the offset 2 in SysTick Control and Status Register   */
#define SYSTICK_MAXCOUNT       ((1<<24) -1)                                    /* SysTick MaxCount    



/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
char *cBODvol={"2.12.22.42.52.72.83.04.5"};

/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/


void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 22; /* Assume the internal 22MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}


void DrvBODTALARM_BODISR(void)
{
	char cBuffer[3];
	uint8_t u8addr;
	
	u8addr = 3*(BOD->BOD_SEL.BOD_LVL);
	cBuffer[0] = *(cBODvol+u8addr);
	cBuffer[1] = *(cBODvol+u8addr+1);
	cBuffer[2] = *(cBODvol+u8addr+2);
	printf("Brown Out Detect voltage level below %s\n",cBuffer);

    if(BOD->BOD_SEL.BOD_LVL > 0)
        BOD->BOD_SEL.BOD_LVL--;
}        


void DrvBODTALARM_TALARMISR(void)
{
	printf("Temperature Sense event occurs\n");
}
/*---------------------------------------------------------------------------------------------*/
/* BODTALARM Test Sample 				                                                           */
/* Test Item					                                                               */
/* It sends the messages to HyperTerminal.											           */
/*---------------------------------------------------------------------------------------------*/

int32_t main()
{
	STR_UART_T sParam;
    uint32_t u32config0;
    
	UNLOCKREG();
    SYSCLK->PWRCON.XTL32K_EN = 1;
    LOCKREG();
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

    DrvUART_Open(UART_PORT0,&sParam);
  
    printf("+----------------------------------------------------------------+\n");
    printf("|                     BOD&TALAM Sample Code                      |\n");           
    printf("+----------------------------------------------------------------+\n");
    printf("| Brown out detection from 4.5V to 2.1V                          |\n");
    printf("| Temperature Alarm Sense Level = 105C                           |\n");
    printf("+----------------------------------------------------------------+\n");
    printf("\n");

    /* Configure Temperature Alarm */
    DrvBOD_SetTALARMselect(TALARM_SEL_105C);
    DrvBOD_SetTALARMIE(TALARM_EN_IEENABLE);
    DrvBOD_InstallISR(DrvBODTALARM_TALARMISR,1); 
    DrvBOD_EnableTALARM(TALARM_EN_ENENABLE);
    
    /* Configure BOD */
    DrvBOD_SelectBODVolt(BODLVL_30V);
    DrvFMC_Read(CONFIG_BASE, &u32config0);
    u32config0 |= 0x00800000; //BOD enable
    DrvFMC_Write(CONFIG_BASE, u32config0);    
    DrvBOD_InstallISR(DrvBODTALARM_BODISR,0); 
    printf("Set Config0[23] = 1, Please reset again....\n"); 
    
    while(1);
}	

