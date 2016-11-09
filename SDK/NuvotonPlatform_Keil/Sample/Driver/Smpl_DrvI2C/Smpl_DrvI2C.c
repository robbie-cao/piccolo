/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvGPIO.h"
#include "Driver\DrvI2C.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvUART.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
uint8_t Device_Addr0;
uint8_t Tx_Data0[3];
uint8_t Rx_Data0;
uint8_t DataLen0;
volatile uint8_t EndFlag0 = 0;

uint8_t  Addr1[3] = {0};
uint8_t  DataLen1;
uint8_t  Slave_Buff1[32] = {0};
uint16_t Slave_Buff_Addr1;


/*---------------------------------------------------------------------------------------------------------*/
/*  I2C0 (Master) Rx Callback Function									                                   */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_Callback_Rx(uint32_t status)
{
//	printf("I2C0_Callback_Rx. Status 0x%x\n", status);

	if (status == 0x08)					   	/* START has been transmitted and prepare SLA+W */
	{
		DrvI2C_WriteData(I2C_PORT0, Device_Addr0<<1);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}	
	else if (status == 0x18)				/* SLA+W has been transmitted and ACK has been received */
	{
		DrvI2C_WriteData(I2C_PORT0, Tx_Data0[DataLen0++]);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}
	else if (status == 0x20)				/* SLA+W has been transmitted and NACK has been received */
	{
		DrvI2C_Ctrl(I2C_PORT0, 1, 1, 1, 0);
	}
	else if (status == 0x28)				/* DATA has been transmitted and ACK has been received */
	{
		if (DataLen0 != 2)
		{
			DrvI2C_WriteData(I2C_PORT0, Tx_Data0[DataLen0++]);
			DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
		}
		else
		{
			DrvI2C_Ctrl(I2C_PORT0, 1, 0, 1, 0);
		}		
	}
	else if (status == 0x10)				/* Repeat START has been transmitted and prepare SLA+R */
	{
		DrvI2C_WriteData(I2C_PORT0, Device_Addr0<<1 | 0x01);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}
	else if (status == 0x40)				/* SLA+R has been transmitted and ACK has been received */
	{
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}		
	else if (status == 0x58)				/* DATA has been received and NACK has been returned */
	{
		Rx_Data0 = DrvI2C_ReadData(I2C_PORT0);
		DrvI2C_Ctrl(I2C_PORT0, 0, 1, 1, 0);
		EndFlag0 = 1;
	}
	else
	{
		printf("Status 0x%x is NOT processed\n", status);
	}			
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C0 (Master) Tx Callback Function									                                   */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_Callback_Tx(uint32_t status)
{
//	printf("I2C0_Callback_Tx. Status 0x%x\n", status);
	
	if (status == 0x08)						/* START has been transmitted */
	{
		DrvI2C_WriteData(I2C_PORT0, Device_Addr0<<1);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}	
	else if (status == 0x18)				/* SLA+W has been transmitted and ACK has been received */
	{
		DrvI2C_WriteData(I2C_PORT0, Tx_Data0[DataLen0++]);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}
	else if (status == 0x20)				/* SLA+W has been transmitted and NACK has been received */
	{
		
		DrvI2C_Ctrl(I2C_PORT0, 1, 1, 1, 0);
	}	
	else if (status == 0x28)				/* DATA has been transmitted and ACK has been received */
	{
		if (DataLen0 != 3)
		{
			DrvI2C_WriteData(I2C_PORT0, Tx_Data0[DataLen0++]);
			DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
		}
		else
		{
			DrvI2C_Ctrl(I2C_PORT0, 0, 1, 1, 0);
			EndFlag0 = 1;
		}		
	}
	else
	{
		printf("Status 0x%x is NOT processed\n", status);
	}		
}

/*---------------------------------------------------------------------------------------------------------*/
/*  I2C1 (Slave) Callback Function									                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*
void I2C1_Callback_Slave(uint32_t status)
{
	
	if ((status == 0x60) || (status == 0x68))	  	// SLA+W has been received and ACK has been returned
	{
		DataLen1 = 0; 
		DrvI2C_Ctrl(I2C_PORT1, 0, 0, 1, 1);
	}	
	else if (status == 0x80)					  	// DATA has been received and ACK has been returned 
	{
       	Addr1[DataLen1++] = DrvI2C_ReadData(I2C_PORT1);
  		
  		if (DataLen1 == 2)
  		{
  			Slave_Buff_Addr1 = (Addr1[0] << 8) + Addr1[1];
  		}

  		if ((DataLen1 == 3) && (Slave_Buff_Addr1 < 32))		
  		{
  			Slave_Buff1[Slave_Buff_Addr1] = Addr1[2];
			DataLen1 = 0;  		
  		}

  		DrvI2C_Ctrl(I2C_PORT1, 0, 0, 1, 1);
	}	
	else if ((status == 0xB0) || (status == 0xA8))  // SLA+R has been received and ACK has been returned 
	{
		DrvI2C_WriteData(I2C_PORT1, Slave_Buff1[Slave_Buff_Addr1++]);
		if (Slave_Buff_Addr1 >= 32)
			Slave_Buff_Addr1 = 0;
  		DrvI2C_Ctrl(I2C_PORT1, 0, 0, 1, 1);
	}												
	else if (status == 0xC0)						// DATA has been transmitted and NACK has been received 
	{
  		DrvI2C_Ctrl(I2C_PORT1, 0, 0, 1, 1);
	}
	else if (status == 0xA0)						// STOP or Repeat START has been received 
	{
		DataLen1 = 0;
		DrvI2C_Ctrl(I2C_PORT1, 0, 0, 1, 1);
	}
	else
	{
		printf("Status 0x%x is NOT processed\n", status);
	}			
}
*/

void I2CMasterTxWrite(E_I2C_PORT port, uint8_t u8data[])
{
	uint8_t i;
	uint32_t tmp;

	if (port)
	{						   
		;
	}	
	else
	{
		//////////////////////
		// send a start signal
		//////////////////////
		DrvI2C_Ctrl(port, 1, 0, 0, 0);	
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x08)
		while(DrvI2C_GetStatus(port)!= 0x08)
		{
			__NOP();
        	__NOP();
		}
		
		///////////////////////////
		// send (deviceID[7:1] + W)
		///////////////////////////
		DrvI2C_WriteData(port, u8data[0]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x18)
		while(DrvI2C_GetStatus(port)!= 0x18)
		{
			__NOP();
       		__NOP();
		}

		///////////////////////////
		// send address
		///////////////////////////
		DrvI2C_WriteData(port, u8data[1]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x28)
		while(DrvI2C_GetStatus(port)!= 0x28)
		{
			__NOP();
       		__NOP();
		}

		// Generate a stop signal
		DrvI2C_Ctrl(port, 0, 1, 1, 0);

		// Generate a start signal
		DrvI2C_Ctrl(port, 1, 0, 0, 0);
		while(DrvI2C_GetStatus(port)!= 0x08)
		{
			__NOP();
        	__NOP();
		}

		///////////////////////////
		// send (deviceID[7:1] + W)
		///////////////////////////
		DrvI2C_WriteData(port, u8data[2]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x18)
		while(DrvI2C_GetStatus(port)!= 0x18)
		{
			__NOP();
       		__NOP();
		}

		///////////////////////////
		// send data
		///////////////////////////
		DrvI2C_WriteData(port, u8data[3]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x28)
		while(DrvI2C_GetStatus(port)!= 0x28)
		{
			__NOP();
       		__NOP();
		}

		///////////////////////////
		// send data
		///////////////////////////
		DrvI2C_WriteData(port, u8data[4]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x28)
		while(DrvI2C_GetStatus(port)!= 0x28)
		{
			__NOP();
       		__NOP();
		}

//		tmp = SYSCLK->CLKDIV.HCLK_N;
//		tmp = SYSCLK->CLKSEL0.HCLK_S ;
//		tmp = 	(sizeof u8data/sizeof u8data[0]);
//		tmp = tmp - 1;
//		for(i = 0; i < (sizeof u8data/sizeof u8data[0] - 1); i++)
//		for(i = 0; i < tmp; i++)
//		for(i = 0; i < 2; i++)
//		{
//			DrvI2C_WriteData(port, u8data[i]); // (address + R/W) 
//			DrvI2C_Ctrl(port, 0, 0, 1, 0);
//			while(DrvI2C_GetIntFlag(port)==0)
//			{
//				__NOP();
//        		__NOP();
//			}
//		}
		// Generate a stop signal
		DrvI2C_Ctrl(port, 0, 1, 1, 0);
	}
}

void I2CMasterTxRead(E_I2C_PORT port, uint8_t u8data[])
{
	uint8_t i;
	uint32_t tmp;

	if (port)
	{						   
		;
	}	
	else
	{
		//////////////////////
		// send a start signal
		//////////////////////
		DrvI2C_Ctrl(port, 1, 0, 0, 0);	
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x08)
		while(DrvI2C_GetStatus(port)!= 0x08)
		{
			__NOP();
        	__NOP();
		}
		
		///////////////////////////
		// send (deviceID[7:1] + W)
		///////////////////////////
		DrvI2C_WriteData(port, u8data[0]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x18)
		while(DrvI2C_GetStatus(port)!= 0x18)
		{
			__NOP();
       		__NOP();
		}

		///////////////////////////
		// send address
		///////////////////////////
		DrvI2C_WriteData(port, u8data[1]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x28)
		while(DrvI2C_GetStatus(port)!= 0x28)
		{
			__NOP();
       		__NOP();
		}

		// Generate a stop signal
		DrvI2C_Ctrl(port, 0, 1, 1, 0);

		// Generate a start signal
		DrvI2C_Ctrl(port, 1, 0, 0, 0);
		while(DrvI2C_GetStatus(port)!= 0x08)
		{
			__NOP();
        	__NOP();
		}

		///////////////////////////
		// send (deviceID[7:1] + W)
		///////////////////////////
		DrvI2C_WriteData(port, u8data[2]); 
		DrvI2C_Ctrl(port, 0, 0, 1, 0);
//		while(DrvI2C_GetIntFlag(port)==0 && DrvI2C_GetStatus(port)== 0x18)
		while(DrvI2C_GetStatus(port)!= 0x40)
		{
			__NOP();
       		__NOP();
		}

		DrvI2C_Ctrl(port, 0, 0, 1, 1);
		// wait for data
		while(DrvI2C_GetStatus(port)!= 0x50)
		{
			__NOP();
       		__NOP();
		}

		DrvI2C_Ctrl(port, 0, 0, 1, 0);
		// wait for data
		while(DrvI2C_GetStatus(port)!= 0x58)
		{
			__NOP();
       		__NOP();
		}

//		tmp = SYSCLK->CLKDIV.HCLK_N;
//		tmp = SYSCLK->CLKSEL0.HCLK_S ;
//		tmp = 	(sizeof u8data/sizeof u8data[0]);
//		tmp = tmp - 1;
//		for(i = 0; i < (sizeof u8data/sizeof u8data[0] - 1); i++)
//		for(i = 0; i < tmp; i++)
//		for(i = 0; i < 2; i++)
//		{
//			DrvI2C_WriteData(port, u8data[i]); // (address + R/W) 
//			DrvI2C_Ctrl(port, 0, 0, 1, 0);
//			while(DrvI2C_GetIntFlag(port)==0)
//			{
//				__NOP();
//        		__NOP();
//			}
//		}
		// Generate a stop signal
		DrvI2C_Ctrl(port, 0, 1, 1, 0);
	}
}


void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 49; /* Assume the internal 49MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}

/* only to test driver
void i2Ctest(void)
{
	int i;
	uint32_t u32HCLK, u32data;
	uint8_t  Slave_Buff1[32] = {0};
	uint8_t  TxRead[3]		= {0x34, 0x22, 0x35}; // deviceID address
	uint8_t  TxWrite[5]		= {0x34, 0x22, 0x34, 0x01, 0x22}; // deviceID address
	uint8_t  data1[2]	= {0x35}; // deviceID 

	DrvI2C_Close(I2C_PORT0);


	u32HCLK = DrvSYS_GetHCLK() * 1000;

	//Set I2C I/O 
	DrvGPIO_InitFunction(FUNC_I2C0);

	// enable I2C function, and set clock = 100Kbps 
	DrvI2C_Open(I2C_PORT0, u32HCLK, 30000);

	//DrvI2C_Ctrl(E_I2C_PORT port, STA, STO, SI, AA)
	// I2C address of 8822: 0x1A
//	DrvI2C_SendData(I2C_PORT0, 0x35); // read 8822
//	DrvI2C_SendData(I2C_PORT0, 0x35); // 8822

//	DrvI2C_SendData(I2C_PORT0, dataArry);
	I2CMasterTxWrite(I2C_PORT0, TxWrite);
	I2CMasterTxRead(I2C_PORT0, TxRead);


	// Set AA bit, I2C1 as slave (To simulate a 24LXX EEPROM)
	//DrvI2C_Ctrl(I2C_PORT0, 1, 1, 1, 1);

	DrvI2C_Close(I2C_PORT0);

	// set I2C data
	// enable I2C function
//    DrvI2C_WriteData(I2C_PORT0, 0xAA);
//	I2C0->CON.ENSI = 1;


	while(1)
	{
//		I2C0->CON.STA = 1;
//	DrvI2C_Ctrl(I2C_PORT0, 1, 0, 0, 0);
//	DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);

// 	   DrvI2C_WriteData(I2C_PORT0, 0xAA);
//	DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);

//		I2C0->CON.STO = 1;
	}	
}


void i2Ctest1(void)
{
	uint32_t u32data, u32HCLK, i;
	uint8_t  TxRead[3]		= {0x34, 0x22, 0x35}; // deviceID address
	uint8_t  TxWrite[5]		= {0x34, 0x22, 0x34, 0x01, 0x22}; // deviceID address
    uint8_t  TxReadData;


// WW: DrvI2C_ReadData or in Line: 69
	I2CMasterTxRead(I2C_PORT0, TxRead);
 	TxReadData = DrvI2C_ReadData(I2C_PORT0);  // check if the waveform and the TxReadData are the same.
	printf("TxReadData = %d \n", TxReadData);


//WW: DrvI2C_Close
//	DrvI2C_Close(I2C_PORT0);  // check if 0x4002_0000[6] = 1 --> 0
//	I2CMasterTxRead(I2C_PORT0, TxRead);  // check if there is no waveform

	// Get I2C0 clock 
	u32data = DrvI2C_GetClock(I2C_PORT0, u32HCLK);
	printf("I2C0 clock %d Hz\n", u32data);

	// Set I2C0 slave addresses 
	DrvI2C_SetAddress(I2C_PORT0, 0, 0x15, 0); // 0x4002_0004[7:1] = 0x15; 0x4002_0004[0]=0
	DrvI2C_SetAddress(I2C_PORT0, 1, 0x35, 0); // 0x4002_0018[7:1] = 0x35; 0x4002_0004[0]=0
	DrvI2C_SetAddress(I2C_PORT0, 2, 0x55, 0); // 0x4002_001C[7:1] = 0x55; 0x4002_0004[0]=0
	DrvI2C_SetAddress(I2C_PORT0, 3, 0x75, 0); // 0x4002_0020[7:1] = 0x75; 0x4002_0004[0]=0

	// Set I2C1 slave addresses 
//	DrvI2C_SetAddress(I2C_PORT1, 0, 0x16, 0);
//	DrvI2C_SetAddress(I2C_PORT1, 1, 0x36, 0);
//	DrvI2C_SetAddress(I2C_PORT1, 2, 0x56, 0);
//	DrvI2C_SetAddress(I2C_PORT1, 3, 0x76, 0);	


//WW: DrvI2C_SetAddressMask
	DrvI2C_SetAddressMask(I2C_PORT0, 0, 0x22);	//0x4002_0024[7:1] = 0x22;
	DrvI2C_SetAddressMask(I2C_PORT0, 0, 0x11);	//0x4002_0024[7:1] = 0x11;
	DrvI2C_SetAddressMask(I2C_PORT0, 0, 0x5);	//0x4002_0024[7:1] = 0xA;

 
//WW: DrvI2C_SetClock; DrvI2C_GetClock
	DrvI2C_SetClock(I2C_PORT0, u32HCLK, 300000);  //I2C clock = PCLK / (4x(divider+1))
    SysTimerDelay(10000);
//	I2CMasterTxRead(I2C_PORT0, TxRead);
	I2CMasterTxWrite(I2C_PORT0, TxWrite);
	u32data = DrvI2C_GetClock(I2C_PORT0, u32HCLK);
	printf("300000: I2C0 clock %d Hz\n", u32data);
	DrvI2C_SetClock(I2C_PORT0, u32HCLK, 200000);  //I2C clock = PCLK / (4x(divider+1))
    SysTimerDelay(10000);
//	I2CMasterTxRead(I2C_PORT0, TxRead);
	I2CMasterTxWrite(I2C_PORT0, TxWrite);
	u32data = DrvI2C_GetClock(I2C_PORT0, u32HCLK);
	printf("200000: I2C0 clock %d Hz\n", u32data);
	DrvI2C_SetClock(I2C_PORT0, u32HCLK, 50000);  //I2C clock = PCLK / (4x(divider+1))
    SysTimerDelay(10000);
//	I2CMasterTxRead(I2C_PORT0, TxRead);
	I2CMasterTxWrite(I2C_PORT0, TxWrite);
	u32data = DrvI2C_GetClock(I2C_PORT0, u32HCLK);
	printf("50000: I2C0 clock %d Hz\n", u32data);
	DrvI2C_SetClock(I2C_PORT0, u32HCLK, 10000);  //I2C clock = PCLK / (4x(divider+1))
    SysTimerDelay(10000);
//	I2CMasterTxRead(I2C_PORT0, TxRead);
	I2CMasterTxWrite(I2C_PORT0, TxWrite);
	u32data = DrvI2C_GetClock(I2C_PORT0, u32HCLK);
	printf("10000: I2C0 clock %d Hz\n", u32data);
	DrvI2C_SetClock(I2C_PORT0, u32HCLK, 100000);  //I2C clock = PCLK / (4x(divider+1))
    SysTimerDelay(10000);
//	I2CMasterTxRead(I2C_PORT0, TxRead);
	I2CMasterTxWrite(I2C_PORT0, TxWrite);
	u32data = DrvI2C_GetClock(I2C_PORT0, u32HCLK);
	printf("100000: I2C0 clock %d Hz\n", u32data);

// WW: DrvI2C_EnableTimeoutCount; DrvI2C_ClearTimeoutFlag
	DrvI2C_EnableTimeoutCount(I2C_PORT0, 1, 1);// 0x4002_0014[2] = 1; 0x4002_0014[1] = 1;
	for(i = 0; i < 100000; i++); // delay for flag and then check if 0x4002_0014[0](TIF) = 1
	DrvI2C_EnableTimeoutCount(I2C_PORT0, 0, 0);// 0x4002_0014[2] = 0; 0x4002_0014[1] = 0;
	DrvI2C_ClearTimeoutFlag(I2C_PORT0); // 0x4002_0014[0](TIF) = 1 --> 0

// WW: DrvI2C_EnableInt; DrvI2C_DisableInt
	DrvI2C_EnableInt(I2C_PORT0); // 0x4002_0000[7] = 1;
	DrvI2C_DisableInt(I2C_PORT0);// 0x4002_0000[7] = 0;

// WW: DrvI2C_GetVersion(void)
	printf("DrvI2C_GetVersion: %d\n", DrvI2C_GetVersion());
}
*/

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
	uint32_t u32HCLK, i;
	STR_UART_T sParam;

	UNLOCKREG();
    SYSCLK->PWRCON.XTL32K_EN = 1;

    /* Waiting for 12M Xtal stalble */
    SysTimerDelay(5000);

	/* Set UART I/O */
	DrvGPIO_InitFunction(FUNC_UART0);
	
	/* UART Setting */
    sParam.u32BaudRate 		= 115200;
    sParam.u8cDataBits 		= DRVUART_DATABITS_8;
    sParam.u8cStopBits 		= DRVUART_STOPBITS_1;
    sParam.u8cParity 		= DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

	/* Set UART Configuration */
	DrvUART_Open(UART_PORT0, &sParam);
 
    printf("+---------------------------------------------------------------------+\n");
    printf("|                       I2C Driver Sample Code                        |\n");
    printf("|                                                                     |\n");
    printf("+---------------------------------------------------------------------+\n");
    printf("  I/O Configuration:\n");

    printf("    GPA10 <--> 8822\n");	//SDA
    printf("    GPA11 <--> 8822\n");	//SCL
    printf("\n");

 	
	u32HCLK = DrvSYS_GetHCLK() * 1000;
    printf("u32HCLK = DrvSYS_GetHCLK() * 1000 = %d\n", u32HCLK);	//SDA


    /* Set I2C I/O */
    DrvGPIO_InitFunction(FUNC_I2C0);
//    DrvGPIO_InitFunction(FUNC_I2C1);

	/* Open I2C0 and I2C1, and set clock = 100Kbps */
	DrvI2C_Open(I2C_PORT0, u32HCLK, 100000);
//	DrvI2C_Open(I2C_PORT1, u32HCLK, 100000);

/////////////////////////////////////////////////////////////////
//	i2Ctest1();
/////////////////////////////////////////////////////////////////

    for (i = 0; i < 32; i++)
	{
		Slave_Buff1[i] = 0;
	}	
	
	/* Set AA bit, I2C1 as slave (To simulate a 24LXX EEPROM) */
//	DrvI2C_Ctrl(I2C_PORT1, 0, 0, 0, 1);
		
	/* Enable I2C0 interrupt and set corresponding NVIC bit */
	DrvI2C_EnableInt(I2C_PORT0);
		
	/* Enable I2C1 interrupt and set corresponding NVIC bit */
//	DrvI2C_EnableInt(I2C_PORT1);
		
	/* Install I2C1 call back function for slave */
//	DrvI2C_InstallCallback(I2C_PORT1, I2CFUNC, I2C1_Callback_Slave);

	Device_Addr0 = 0x1A; //0x16; 8822: 0x1A
	Tx_Data0[0] = 0x00;
	Tx_Data0[1] = 0x00;
		
//	for (i = 0; i < 32; i++)
	for (i = 0; i < 1; i++)
	{
		Tx_Data0[2] = 0x10 + i;
		DataLen0 = 0;
		EndFlag0 = 0;
		
		/* Install I2C0 call back function for write data to slave */
		DrvI2C_InstallCallback(I2C_PORT0, I2CFUNC, I2C0_Callback_Tx);
		
		/* I2C0 as master sends START signal */
		DrvI2C_Ctrl(I2C_PORT0, 1, 0, 0, 0);
		
		/* Wait I2C0 Tx Finish */
		while (EndFlag0 == 0);
		EndFlag0 = 0;

		/* Uninstall I2C0 call back function for write data to slave */
		DrvI2C_UninstallCallBack(I2C_PORT0, I2CFUNC);
		
		/* Install I2C0 call back function for read data from slave */
		DrvI2C_InstallCallback(I2C_PORT0, I2CFUNC, I2C0_Callback_Rx);

		DataLen0 = 0;
		Device_Addr0 = 0x1A; //0x16; ; 8822: 0x1A
		DrvI2C_Ctrl(I2C_PORT0, 1, 0, 0, 0);
		
		/* Wait I2C0 Rx Finish */
		while (EndFlag0 == 0);

		/* Compare data */
		if (Rx_Data0 != Tx_Data0[2])
		{
//			printf("I2C0 Byte Write/Read Failed, Data 0x%x\n", Rx_Data0);
			return -1;
		}			
	}
	printf("I2C0(Master) to I2C1(Slave) Test OK\n");

	/* Disable I2C0 interrupt and clear corresponding NVIC bit */
	DrvI2C_DisableInt(I2C_PORT0);
	
	/* Disable I2C1 interrupt and clear corresponding NVIC bit */
//	DrvI2C_DisableInt(I2C_PORT1);

	/* Close I2C0 and I2C1 */
	DrvI2C_Close(I2C_PORT0);
//	DrvI2C_Close(I2C_PORT1);

	return 0;
}




