/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright (c) Nuvoton Technology Corp. All rights reserved.                                             */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include "ISD9xx.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvSPI.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvUART.h"

/*---------------------*/
/* Constant Definition */
/*---------------------*/
#define TEST_COUNT 8
#define SPI_MASTER
//#define SPI_TWO_PORT

/*-----------------*/
/* Global Variable */
/*-----------------*/
volatile uint32_t SPI0_INT_Flag;

/*--------------------*/
/* Function Prototype */
/*--------------------*/
void SysTimerDelay(uint32_t us);
void SPI0_Callback(uint32_t u32UserData);
void InitialSystemClock(void);
void InitialSPIPort(void);


/*---------------------------------------------------------------------------------------------------------*/
/* InitialUART                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
void InitialUART(void)
{
	STR_UART_T sParam;

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
}




void TwoPortTxTest(void)
{
	uint32_t au32SourceData[TEST_COUNT];
	uint32_t u32DataCount;
    int32_t  i;
    uint32_t u32Tmp[2];

	// change GPB0 to SSB1
	SYS->GPB_ALT.GPB0 = 1;


    DrvSPI_Set2BitSerialDataIOMode(eDRVSPI_PORT0, TRUE);

	/* set the soure data */
	for(u32DataCount=0; u32DataCount<TEST_COUNT; u32DataCount++)
	{
		au32SourceData[u32DataCount] = u32DataCount;//305419896+u32DataCount; //1431655765; // //271601776;
	}

    /* Clear Tx register of SPI0 to avoid send non-zero data.  Just for safe. */
    u32Tmp[0] = 0;
	u32Tmp[1] = 1;
    DrvSPI_SetTxRegister(eDRVSPI_PORT0, &u32Tmp[0], 2); 

	/* SPI0 Transmit test */
    for(i=0;i<1;i++)
    {
        u32DataCount=2;
		SPI0_INT_Flag = 0;

    	/* write the first data of source buffer to Tx register of SPI0. And start transmission. */
		DrvSPI_BurstWrite(eDRVSPI_PORT0, &au32SourceData[0]);

    	while(1)
    	{
    		if(SPI0_INT_Flag==1)
    		{
    			SPI0_INT_Flag = 0;    			
    			if(u32DataCount<(TEST_COUNT))
    			{
    				/* continue to send next word */					
					DrvSPI_BurstWrite(eDRVSPI_PORT0, &au32SourceData[u32DataCount]);
					u32DataCount=u32DataCount+2;				
    			}
    			else
    			{
					/* The last transfer, break and leave the loop */
    				break;
    			}
    		}
    	}
    }
}

void TwoPortRxTest(void)
{
    uint32_t u32Tmp[2];
	uint32_t u32Buf[2];

	// change GPB0 to SSB1
	SYS->GPB_ALT.GPB0 = 1;

    DrvSPI_Set2BitSerialDataIOMode(eDRVSPI_PORT0, TRUE);

    /* Clear Tx register of SPI0 to avoid send non-zero data.  Just for safe. */
    u32Tmp[0] = 0;
	u32Tmp[1] = 1;
    DrvSPI_SetTxRegister(eDRVSPI_PORT0, &u32Tmp[0], 2); 

	/* SPI0 Receive test */
	SPI0_INT_Flag = 0;

    /* set the GO_BUSY bit of SPI0 */
    DrvSPI_SetGo(eDRVSPI_PORT0);

    while(1)
    {
    	if(SPI0_INT_Flag==1)
    	{
    		SPI0_INT_Flag = 0;

			DrvSPI_BurstRead(eDRVSPI_PORT0, &u32Buf[0]);

			printf("Port0=%x Port1=%x, \n",u32Buf[0], u32Buf[1]);
				
    	}
    }
}

void SPITxSingle(void)
{
	uint32_t au32SourceData[TEST_COUNT];
	uint32_t u32DataCount;
    int32_t  i;
    uint32_t u32Tmp[2];
	//uint32_t u32Buf;


	/* set the soure data */
	for(u32DataCount=0; u32DataCount<TEST_COUNT; u32DataCount++)
	{
		au32SourceData[u32DataCount] = u32DataCount;//305419896+u32DataCount; //1431655765; //u32DataCount; //271601776; 305419896;
	}

    /* Clear Tx register of SPI0 to avoid send non-zero data.  Just for safe. */
    u32Tmp[0] = 0;
	u32Tmp[1] = 1;
    DrvSPI_SetTxRegister(eDRVSPI_PORT0, &u32Tmp[0], 1); 

	/* SPI0 Transmit test */
    for(i=0;i<1;i++)
    {
        u32DataCount=1;
		SPI0_INT_Flag = 0;

        /* set the GO_BUSY bit of SPI0 */
    	//DrvSPI_SetGo(eDRVSPI_PORT0);
    	/* write the first data of source buffer to Tx register of SPI0. And start transmission. */
    	DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[0]);

    	while(1)
    	{
    		if(SPI0_INT_Flag==1)
    		{
    			SPI0_INT_Flag = 0;    			

				// dump out
				//DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &u32Buf, 1);
				//printf("%x",u32Buf);

    			if(u32DataCount<(TEST_COUNT))
    			{
    				/* continue to send next word */
					SysTimerDelay(10000);					
    				DrvSPI_SingleWrite(eDRVSPI_PORT0, &au32SourceData[u32DataCount]);
					u32DataCount++;				
    			}
    			else
    			{
					/* The last transfer, break and leave the loop */
    				break;
    			}
    		}
    	}
    }

}

void SPIRxSingle(void)
{
    uint32_t u32Tmp[2];
	uint32_t u32Buf;

    /* Clear Tx register of SPI0 to avoid send non-zero data.  Just for safe. */
    u32Tmp[0] = 0;
	u32Tmp[1] = 1;
    DrvSPI_SetTxRegister(eDRVSPI_PORT0, &u32Tmp[0], 1); 

	/* SPI0 Receive test */
	SPI0_INT_Flag = 0;

    /* set the GO_BUSY bit of SPI0 */
    DrvSPI_SetGo(eDRVSPI_PORT0);

    while(1)
    {
    	if(SPI0_INT_Flag==1)
    	{
    		SPI0_INT_Flag = 0;    			

			// dump out
			//DrvSPI_DumpRxRegister(eDRVSPI_PORT0, &u32Buf, 1);

			DrvSPI_SingleRead(eDRVSPI_PORT0, &u32Buf);

			printf("Port0=%x \n",u32Buf);
				

    	}
    }

}

void SPITxBurst(void)
{
	uint32_t au32SourceData[TEST_COUNT];
	uint32_t u32DataCount;
    int32_t  i;
    uint32_t u32Tmp[2];

	//burst setting
    //DrvSPI_BurstTransfer(eDRVSPI_PORT0,2,17);

	/* set the soure data */
	for(u32DataCount=0; u32DataCount<TEST_COUNT; u32DataCount++)
	{
		au32SourceData[u32DataCount] = 305419896+u32DataCount; //1431655765; //u32DataCount; //271601776; 305419896;
	}

    /* Clear Tx register of SPI0 to avoid send non-zero data.  Just for safe. */
    u32Tmp[0] = 0;
	u32Tmp[1] = 1;
    DrvSPI_SetTxRegister(eDRVSPI_PORT0, &u32Tmp[0], 1); 

	/* SPI0 Transmit test */
    for(i=0;i<1;i++)
    {
        u32DataCount=2;
		SPI0_INT_Flag = 0;

    	/* write the first data of source buffer to Tx register of SPI0. And start transmission. */
    	DrvSPI_BurstWrite(eDRVSPI_PORT0, &au32SourceData[0]);

    	while(1)
    	{
    		if(SPI0_INT_Flag==1)
    		{
    			SPI0_INT_Flag = 0;    			
    			if(u32DataCount<(TEST_COUNT))
    			{
    				/* continue to send next word */										
					DrvSPI_BurstWrite(eDRVSPI_PORT0, &au32SourceData[u32DataCount]);
					u32DataCount = u32DataCount + 2;					
    			}
    			else
    			{
					/* The last transfer, break and leave the loop */
    				break;
    			}
    		}
    	}
    }

}

void SPI0_Callback(uint32_t u32UserData)
{
	SPI0_INT_Flag = 1;
}


void InitialSystemClock(void)
{
    /* Unlock the protected registers */	
	UNLOCKREG();
 
	/* HCLK clock source. 0:internal 49.152MHz RC oscillator */
	DrvSYS_SetHCLKSource(0);

	LOCKREG();

	/* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
	DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0); 
}


void InitialSPIPortMaster(void)
{
	/* Enable SPI0 function */
	//DrvGPIO_InitFunction(FUNC_SPI0);

	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE1, 32);
	//DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_MASTER, eDRVSPI_TYPE1, 8);
	
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);
	//DrvSPI_DisableAutoCS(eDRVSPI_PORT0);

	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);

	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 3000000, 0);

	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
}

void InitialSPIPortSlave(void)
{
	/* Enable SPI0 function */
	//DrvGPIO_InitFunction(FUNC_SPI0);

	/* Configure SPI0 as a master, 32-bit transaction */
	DrvSPI_Open(eDRVSPI_PORT0, eDRVSPI_SLAVE, eDRVSPI_TYPE1, 32);

	
	/* Enable the automatic slave select function of SS0. */
	DrvSPI_EnableAutoCS(eDRVSPI_PORT0, eDRVSPI_SS0);

	/* Set the active level of slave select. */
	DrvSPI_SetSlaveSelectActiveLevel(eDRVSPI_PORT0, eDRVSPI_ACTIVE_LOW_FALLING);

	/* SPI clock rate 3MHz */
	DrvSPI_SetClock(eDRVSPI_PORT0, 3000000, 0);

	/* Enable the SPI0 interrupt and install the callback function. */
	DrvSPI_EnableInt(eDRVSPI_PORT0, SPI0_Callback, 0);
}

void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 49; /* Assume the internal 49MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}

void ByteEndianTest0(void)
{
	DrvSPI_SetByteEndian(eDRVSPI_PORT0, FALSE);
	SPITxSingle();

	DrvSPI_SetByteEndian(eDRVSPI_PORT0, TRUE);
	SPITxSingle();
}

void ByteEndianTest1(void)
{
	DrvSPI_SetByteEndian(eDRVSPI_PORT0, FALSE);
	TwoPortTxTest();

	DrvSPI_SetByteEndian(eDRVSPI_PORT0, TRUE);
	TwoPortTxTest();
}

void EndianTest0(void)
{
    DrvSPI_SetEndian(eDRVSPI_PORT0, eDRVSPI_MSB_FIRST);
	SPITxSingle();

	DrvSPI_SetEndian(eDRVSPI_PORT0, eDRVSPI_MSB_FIRST);
	SPITxSingle();
}

void EndianTest1(void)
{
	DrvSPI_SetEndian(eDRVSPI_PORT0, eDRVSPI_MSB_FIRST);
	TwoPortTxTest();

	DrvSPI_SetEndian(eDRVSPI_PORT0, eDRVSPI_LSB_FIRST);
	TwoPortTxTest();
}

void TxLengthTest0(void)
{
	DrvSPI_SetBitLength(eDRVSPI_PORT0, 32);
	SPITxSingle();

	DrvSPI_SetBitLength(eDRVSPI_PORT0, 24);
	SPITxSingle();

	DrvSPI_SetBitLength(eDRVSPI_PORT0, 16);
	SPITxSingle();

	DrvSPI_SetBitLength(eDRVSPI_PORT0, 8);
	SPITxSingle();
}

void TxLengthTest1(void)
{
	DrvSPI_SetBitLength(eDRVSPI_PORT0, 32);
	TwoPortTxTest();

	DrvSPI_SetBitLength(eDRVSPI_PORT0, 24);
	TwoPortTxTest();

	DrvSPI_SetBitLength(eDRVSPI_PORT0, 16);
	TwoPortTxTest();

	DrvSPI_SetBitLength(eDRVSPI_PORT0, 8);
	TwoPortTxTest();
}

/*------------------------------------------------------------------------------------------------------*/
/*	main						                                                                        */
/*------------------------------------------------------------------------------------------------------*/
int main(void)
{
	InitialSystemClock();

	InitialUART();

	printf("\n========SPI Test code========\n");
	printf("SPI Driver version:%x\n",DrvSPI_GetVersion());
#ifdef SPI_MASTER
	#ifndef SPI_TWO_PORT
//SPI0 Master------------------------------------------------------------
	printf("SPI Master, Byte Endian Test\n");
	InitialSPIPortMaster();
	ByteEndianTest0();

	printf("SPI Master, Endian Test\n");
	InitialSPIPortMaster();
	EndianTest0();

	printf("SPI Master, Burst Test\n");
	InitialSPIPortMaster();
	SPITxBurst();
	#else
//SPI1 Master------------------------------------------------------------
	printf("SPI Master,Two Port, Tx Test\n");
	InitialSPIPortMaster();
	TwoPortTxTest();

	printf("SPI Master,Two Port, Byte Endian Test\n");
	InitialSPIPortMaster();
	ByteEndianTest1();

	printf("SPI Master,Two Port, Endian Test\n");
	InitialSPIPortMaster();
	EndianTest1();

	printf("SPI Master,Two Port, Tx Length Test\n");
	InitialSPIPortMaster();
	TxLengthTest1();
	#endif
#else
	#ifndef SPI_TWO_PORT
//SPI0 Slave -------------------------------------------------------
	printf("SPI Slave, Rx Test\n");
	InitialSPIPortSlave();
	SPIRxSingle();
	#else
//SPI1 Slave -------------------------------------------------------
	printf("SPI Slave, Two Port, Rx Test\n");
	InitialSPIPortSlave();
	TwoPortRxTest();
	#endif
#endif
	/* When the SPI port is no longer used, close the port.*/	
	DrvSPI_Close(eDRVSPI_PORT0);

	printf("SPI sample code end!\n");
	return 0;
}
