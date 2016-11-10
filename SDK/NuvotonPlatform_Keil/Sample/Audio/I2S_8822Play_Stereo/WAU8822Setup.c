#include <stdio.h>
#include "Driver/DrvI2C.h"


/*======================================================================*/
/* This sample use ISD9160 to send stereo PCM to WAU8822 with I2S		*/
/* I2C transaction to set up WAU8822 as slave							*/
/*======================================================================*/


uint8_t Device_Addr0 = 0x1A;	 			/* WAU8822 Device ID */
uint8_t Tx_Data0[2];
uint8_t DataCnt0;

//uint8_t Device_Addr0;
//uint8_t Tx_Data0[3];
uint8_t Rx_Data_High;
uint8_t Rx_Data_Low;
uint8_t DataLen0;
volatile uint8_t EndFlag0 = 0;
typedef enum { kI2CWritingWAU88XX_A, kI2CReadingWAU88XX_A, kI2CIdle} I2CRWMode_t;
I2CRWMode_t I2CRWMode = kI2CIdle;
#define GAIN_UPDATE 0x100

void I2C0_Callback_Tx(uint32_t status)
{
	if (status == 0x08)						/* START has been transmitted */
	{
		I2C0->DATA = 0;
		I2C0->DATA = (Device_Addr0<<1);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}
	else if (status == 0x18)				/* SLA+W has been transmitted and ACK has been received */
	{
		I2C0->DATA = Tx_Data0[DataLen0++];
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
			DrvI2C_Ctrl(I2C_PORT0, 0, 1, 1, 0);
			EndFlag0 = 1;
		}
	}
	else
	{
		printf("Status 0x%x is NOT processed\n", status);
	}
}
void I2C0_Callback_Rx(uint32_t status)
{
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
		if (DataLen0 != 1)
		{
			DrvI2C_WriteData(I2C_PORT0, Tx_Data0[DataLen0++]);
			DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
		}
		else
		{
			DrvI2C_Ctrl(I2C_PORT0, 1, 0, 1, 0);			//repeat start
		}
	}
	else if (status == 0x10)				/* Repeat START has been transmitted and prepare SLA+R */
	{
		DrvI2C_WriteData(I2C_PORT0, Device_Addr0<<1 | 0x01);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}
	else if (status == 0x40)				/* SLA+R has been transmitted and ACK has been received */
	{

		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 1);
	}
	else if (status == 0x50)				/* DATA has been received and ACK has been returned */
	{
		Rx_Data_High = DrvI2C_ReadData(I2C_PORT0);
		DrvI2C_Ctrl(I2C_PORT0, 0, 0, 1, 0);
	}
	else if (status == 0x58)				/* DATA has been received and NACK has been returned */
	{
		Rx_Data_Low = DrvI2C_ReadData(I2C_PORT0);
		DrvI2C_Ctrl(I2C_PORT0, 0, 1, 1, 0);
		EndFlag0 = 1;
	}
	else
	{
		printf("Status 0x%x is NOT processed\n", status);
	}
}

void Write_WAU88XX_A (uint8_t addr, uint32_t data)
{
	if (I2CRWMode != kI2CWritingWAU88XX_A)
	{
		I2CRWMode = kI2CWritingWAU88XX_A;
		DrvI2C_UninstallCallBack(I2C_PORT0, I2CFUNC);
		DrvI2C_InstallCallback(I2C_PORT0, I2CFUNC, I2C0_Callback_Tx);
	}
	Device_Addr0 = 0x1a;
	Tx_Data0[0] = (addr<<1) | ((data>>8) & 0x1);
	Tx_Data0[1] = data & 0xff;
	DataLen0 = 0;
	EndFlag0 = 0;
	DrvI2C_Ctrl(I2C_PORT0, 1, 0, 0, 0);
	while (EndFlag0 == 0);
	EndFlag0 = 0;
}
uint32_t Read_WAU88XX_A (uint8_t addr)
{
	if (I2CRWMode != kI2CReadingWAU88XX_A)
	{
		I2CRWMode = kI2CReadingWAU88XX_A;
		DrvI2C_UninstallCallBack(I2C_PORT0, I2CFUNC);
		DrvI2C_InstallCallback(I2C_PORT0, I2CFUNC, I2C0_Callback_Rx);
	}
	Device_Addr0 = 0x1a;
	Tx_Data0[0] = (addr<<1);
	DataLen0 = 0;
	EndFlag0 = 0;
	DrvI2C_Ctrl(I2C_PORT0, 1, 0, 0, 0);
	while (EndFlag0 == 0);
	EndFlag0 = 0;
	return (Rx_Data_High <<8 | Rx_Data_Low);
}

void Delay(int count)
{
	volatile uint32_t i;
	for (i = 0; i < count ; i++);
}

void WriteVerify_WAU88XX_A (uint8_t addr, uint32_t data)
{
	uint32_t retVal;

	Write_WAU88XX_A(addr, data);
	retVal = Read_WAU88XX_A ( addr );
	if(retVal != data)
		printf("I2C - Addr %x Expect %x got %x\n",addr,data,retVal);
}

void  WAU8822_Setup()
{
	//NUC140  setting
	WriteVerify_WAU88XX_A(0x00,  0x000);   /* Reset all registers */
	Delay(0x600);
	WriteVerify_WAU88XX_A(0x01,  0x02F);
	WriteVerify_WAU88XX_A(0x02,  0x1B3);   /* Enable L/R Headphone, ADC Mix/Boost, ADC */
	WriteVerify_WAU88XX_A(0x03,  0x07F);   /* Enable L/R main mixer, DAC */
	WriteVerify_WAU88XX_A(0x04,  0x011);   /* 16-bit word length, I2S format, Stereo */
	WriteVerify_WAU88XX_A(0x05,  0x000);   /* Companding control and loop back mode (all disable) */
//	WriteVerify_WAU88XX_A(0x06,  0x1AD);   /* Divide by 6, 16K */
	WriteVerify_WAU88XX_A(0x06,   0x060);   //Slave, MCLK(=12M)/3 for 256fs, MCLK from ISD9160
//	WriteVerify_WAU88XX_A(0x06,   0x040);   //Slave, MCLK(=8M)/2 for 256fs, MCLK from ISD9160
	WriteVerify_WAU88XX_A(0x07,  0x006);   /* 16K for internal filter cofficients */
	WriteVerify_WAU88XX_A(0x0a, 0x008);   /* DAC softmute is disabled, DAC oversampling rate is 128x */
//	WriteVerify_WAU88XX_A(0x0e, 0x108);   /* ADC HP filter is disabled, ADC oversampling rate is 128x */
//	WriteVerify_WAU88XX_A(0x0f, 0x1EF);   /* ADC left digital volume control */
//	WriteVerify_WAU88XX_A(0x10, 0x1EF);   /* ADC right digital volume control */
//	WriteVerify_WAU88XX_A(0x2b, 0x010);
//	WriteVerify_WAU88XX_A(0x2c, 0x000);   /* LLIN/RLIN is not connected to PGA */
//	WriteVerify_WAU88XX_A(0x2f, 0x050);   /* LLIN connected, and its Gain value */
//	WriteVerify_WAU88XX_A(0x30, 0x050);   /* RLIN connected, and its Gain value */
	WriteVerify_WAU88XX_A(0x31, 0x047);
	WriteVerify_WAU88XX_A(0x32, 0x001);   /* Left DAC connected to LMIX */
	WriteVerify_WAU88XX_A(0x33, 0x001);   /* Right DAC connected to RMIX */
	WriteVerify_WAU88XX_A(0x34, 0x119);   /* Left Headphone volume */
	WriteVerify_WAU88XX_A(0x35, 0x119);   /* Right Headphone volume */
	WriteVerify_WAU88XX_A(0x36, 0x139);   /* LSPKOUT Volume */
	WriteVerify_WAU88XX_A(0x37, 0x139);   /* RSPKOUT Volume */
}
void ADCGainIs(uint8_t gain)
{
	Write_WAU88XX_A(0x2d, GAIN_UPDATE|gain);
	printf("ADC gain =0x%x\n", Read_WAU88XX_A(0x2d));
}
uint32_t ADCGain()
{
 	return Read_WAU88XX_A(0x2d);
}





