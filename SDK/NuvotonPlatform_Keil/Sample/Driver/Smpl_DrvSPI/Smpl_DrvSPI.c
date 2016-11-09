/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvSPI.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvGPIO.h"

#define WAIT_FOR_NEXT
#define TEST_COUNT 255
#define TEST_CYCLES 255
void SPI0_Callback(uint32_t u32UserData);

uint32_t au32SourceData[TEST_COUNT];
uint32_t au32DestinationData[TEST_COUNT];

volatile uint32_t SPI0_INT_Flag;


void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 22; /* Assume the internal 22MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}

int SpiCheck(int cycles)
{
	int i;
	int i32Err, u32DataCount;

	printf("\nSPI0 Loopback test ");
    i32Err = 0;
    for(i=0;i<cycles;i++)
    {
        u32DataCount=0;
		SPI0_INT_Flag = 0;

		if((i&0x1FF) == 0)
            putchar('.');

    	/* write the first data of source buffer to Tx register of SPI0. And start transmission. */
    	DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
    
    	while(1){
    		if(SPI0_INT_Flag==1){
    			SPI0_INT_Flag = 0;
    			if(u32DataCount<(TEST_COUNT-1))	{
                    /* Read the previous retrived data and trigger next transfer. */
    				DrvSPI_SingleReadWrite(eDRVSPI_PORT0, &au32DestinationData[u32DataCount],
                                           au32SourceData[u32DataCount+1]);
    				u32DataCount++;
    			}else{
                    /* Just read the previous retrived data but trigger next transfer,
                       because this is the last transfer. */
    				DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[u32DataCount], 1);
    				break;
    			}
    		}
    	}
    	
     	for(u32DataCount=0; u32DataCount<TEST_COUNT; u32DataCount++){
    		if(au32DestinationData[u32DataCount]!=au32SourceData[u32DataCount])
    		    i32Err = 1;
    	}
        if(i32Err)
            break;
    }
    
    if(i32Err)
	    printf(" [FAIL]\n");
    else
        printf(" [OK]\n");

 	return(i32Err);

}
int main(void)
{
	uint32_t u32DataCount;
	STR_UART_T param;
    char c;
    int i, tmp;
    
    /* Unlock the protected registers */	
	UNLOCKREG();
 	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as OSC48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */
	LOCKREG();

	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); /* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */

	DrvGPIO_InitFunction(FUNC_UART0);
	
	param.u32BaudRate        = 115200;
	param.u8cDataBits        = DRVUART_DATABITS_8;
	param.u8cStopBits        = DRVUART_STOPBITS_1;
	param.u8cParity          = DRVUART_PARITY_NONE;
	param.u8cRxTriggerLevel  = DRVUART_FIFO_1BYTES;
	param.u8TimeOut        	 = 0;
	DrvUART_Open(UART_PORT0, &param);
	
	/* set the soure data and clear the destination buffer */
	for(u32DataCount=0; u32DataCount<TEST_COUNT; u32DataCount++)
	{
		au32SourceData[u32DataCount] = (u32DataCount & 0xff) + 
									   (((u32DataCount & 0xff)+1) << 8 ) +
									   (((u32DataCount & 0xff)+2) <<16 ) + 
									   (((u32DataCount & 0xff)+3) <<24 ) ;
		au32DestinationData[u32DataCount] = 0;
	}
	
	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE1, 32);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 2500000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    
	printf("\n\n");
	printf("+----------------------------------------------------------------------+\n");		
	printf("|                       SPI Driver Sample Code                         |\n");
	printf("|                                                                      |\n");
    printf("+----------------------------------------------------------------------+\n");
	printf("\n");
	printf("SPI Driver version: %x\n", DrvSPI_GetVersion());
	printf("Configure SPI0 as a master. Loopback MISO to MOSI for test.\n");
	printf("Bit length of a transaction: 32\n");
	printf("SPI clock rate: %d Hz\n", DrvSPI_GetClock1(eDRVSPI_PORT0));
    printf("The I/O connection for SPI0 loopback:\n ");
    printf("    SPI0_MISO0(GPA3) <--> SPI0_MISO0(GPA0)\n\n");
	printf("Please connect loopback, and press any key to start transmission.\n");
	getchar();
    

    
	SpiCheck(TEST_CYCLES);

    printf("\nNow Check some various SPI options.\n");
    
    printf("\nSPI 8 bit transmission.\n");
#ifdef WAIT_FOR_NEXT
	printf("Press any key to start transmission.\n");
	getchar();
#endif
	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 8-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 8);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 2500000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    
	/* set the soure data and clear the destination buffer */
	for(u32DataCount=0; u32DataCount<TEST_COUNT; u32DataCount++)
	{
		au32SourceData[u32DataCount] = (u32DataCount & 0xff) ;
		au32DestinationData[u32DataCount] = 0;
	}
	SpiCheck(TEST_CYCLES);

    printf("\nSPI 16 bit transmission.\n");

	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 8-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE6, 16);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1500000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    
	/* set the soure data and clear the destination buffer */
	for(u32DataCount=0; u32DataCount<TEST_COUNT; u32DataCount++)
	{
		au32SourceData[u32DataCount] = (u32DataCount & 0xff) + 
									   (((u32DataCount & 0xff)+1) << 8 );
		au32DestinationData[u32DataCount] = 0;
	}
#ifdef WAIT_FOR_NEXT
	printf("Press any key to start transmission.\n");
	getchar();
#endif
	SpiCheck(TEST_CYCLES);


    printf("\nCheck Endian transmission.\nMonitor SPI bus with scope to see effect of Byte Endian Order\n");

	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 32);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 32 bit transmission BYTE_ENDIAN=0\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif
	}
	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 32);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_MSB_FIRST);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 32 bit transmission BYTE_ENDIAN=1\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif
	}

	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 24-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 24);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_LSB_FIRST);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 24 bit transmission BYTE_ENDIAN=0\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif
	}

	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 24-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 24);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_MSB_FIRST);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 24 bit transmission BYTE_ENDIAN=1\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif
	}
	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 16-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 16);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_LSB_FIRST);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 16 bit transmission BYTE_ENDIAN=0\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif        
	}

	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 16-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 16);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_MSB_FIRST);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 16 bit transmission BYTE_ENDIAN=1\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif
	}
	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 8-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 8);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_LSB_FIRST);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 8 bit transmission BYTE_ENDIAN=0\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif
	}

	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 8-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 8);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_MSB_FIRST);
	c=1;
	while(c!='n'){
        au32SourceData[0] = 0x12345678;
        DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);
        while(SPI0_INT_Flag==0);
        SPI0_INT_Flag = 0;
        DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &au32DestinationData[0], 1);
        printf("Sent %x, Received %x 8 bit transmission BYTE_ENDIAN=1\n", 	au32SourceData[0],au32DestinationData[0]);
#ifdef WAIT_FOR_NEXT
        printf("Press n for next test any other key to repeat transmission.\n");
        c=getchar();
#else
        c='n';
#endif
	}

    
	DrvSPI_Close(eDRVSPI_PORT0);
	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE5, 32);
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);
	/* Enable the automatic slave select function and set the specified slave select pin. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	/* SPI clock rate 1MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 1000000, 0);
    DrvSPI_SetByteEndian (eDRVSPI_PORT0,eDRVSPI_LSB_FIRST);
	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
	NVIC_SetPriority (SPI0_IRQn, 2);
    for(i=500000; i< (DrvSYS_GetHCLK() * 1000); i+=256000){
        DrvSPI_SetClock(eDRVSPI_PORT0, i, 0);
        printf("Speed test SPI clock rate: %d Hz\n", DrvSPI_GetClock1(eDRVSPI_PORT0));
        tmp = SpiCheck(1000);
        if(tmp)
            break;
    }
    printf("\n\nSPI driver sample code is complete.\n");
	DrvSPI_Close(eDRVSPI_PORT0);

	return 1;
}

void SPI0_Callback(uint32_t u32UserData)
{
	SPI0_INT_Flag = 1;
}

