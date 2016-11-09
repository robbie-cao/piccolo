#include <stdio.h>
#include "Driver/DrvI2C.h"


//=============================================================
// I2C transaction to set up WAU8822
//=============================================================

uint8_t Device_Addr0;
uint8_t Tx_Data0[3];
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

void Write_WAU88XX_A (uint8_t addr, uint32_t data) {
	if (I2CRWMode != kI2CWritingWAU88XX_A)	{		
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
uint32_t Read_WAU88XX_A (uint8_t addr) {
	if (I2CRWMode != kI2CReadingWAU88XX_A)	{
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

void WriteVerify_WAU88XX_A (uint8_t addr, uint32_t data) {
	uint32_t retVal;

	Write_WAU88XX_A(addr, data);
	retVal = Read_WAU88XX_A ( addr );
	if(retVal != data)
		printf("I2C - Addr %x Expect %x got %x\n",addr,data,retVal);
}

void
WAU88XX_EnterADCTestMode() {

//#ifdef DUAL_8822
//   
//    int32_t temp;
//	// 8822_2 is on GPIOB[3:2] as SCL/SDA 
//	System_Manager->GPBALT.GPB2 = 1;
// 	System_Manager->GPBALT.GPB3 = 1;
// 	System_Manager->GPAALT.GPA10 = 0;
// 	System_Manager->GPAALT.GPA11 = 0;
//   	
	// GPIOB[1] is MCLK
	SYS->GPB_ALT.GPB1 = 1;
	I2S->CON.I2SEN = 1;
	// Enable MCLK output
	I2S->CON.MCLKEN = 1;
	I2S->CLKDIV.BCLK_DIV = 1;
   	I2S->CLKDIV.MCLK_DIV = 6 ;
//	System_Manager->IPRSTC2.I2C0_RST = 1;
//	System_Manager->IPRSTC2.I2C0_RST = 0;	
//	I2C0->CON.ENSI = 1;
//	I2C0->CLK = 255;
//   	I2C0->High_Speed = 1;
//
//	Device_Addr0 = 0x1a;
//	I2C0->CON.EI = 1;
//	NVIC_EnableIRQ(I2C0_IRQn);
//
//	WriteVerify_WAU88XX_A(0x01, 0x1FB);		  // turn on all analog things
//	WriteVerify_WAU88XX_A(0x02, 0x1BF);		  // turn on all analog things
//	WriteVerify_WAU88XX_A(0x03, 0x1EF);       // turn on all analog things
//
//
////#endif


	SYS->GPB_ALT.GPB2 = 1;
 	SYS->GPB_ALT.GPB3 = 1;
 	SYS->GPA_ALT.GPA10 = 1;
 	SYS->GPA_ALT.GPA11 = 1;

	
	SYS->IPRSTC2.I2C0_RST = 1;
	SYS->IPRSTC2.I2C0_RST = 0;	
	I2C0->CON.ENSI = 1;
	I2C0->CLK = 255;
   	I2C0->High_Speed = 1;

	Device_Addr0 = 0x1a;
	I2C0->CON.EI = 1;
	NVIC_EnableIRQ(I2C0_IRQn);


	//NUC140  setting
	WriteVerify_WAU88XX_A(0x00,  0x000);   /* Reset all registers */ 
	Delay(0x200);		
	WriteVerify_WAU88XX_A(0x01,  0x02F);        
	WriteVerify_WAU88XX_A(0x02,  0x1B3);   /* Enable L/R Headphone, ADC Mix/Boost, ADC */
	WriteVerify_WAU88XX_A(0x03,  0x00F);   /* Enable L/R main mixer, DAC */			
	WriteVerify_WAU88XX_A(0x04,  0x010);   /* 16-bit word length, I2S format, Stereo */		
	WriteVerify_WAU88XX_A(0x05,  0x000);   /* Companding control and loop back mode (all disable) */
	WriteVerify_WAU88XX_A(0x06,  0x1AD);   /* Divide by 6, 16K */
	WriteVerify_WAU88XX_A(0x07,  0x006);   /* 16K for internal filter cofficients */
	WriteVerify_WAU88XX_A(0x0a, 0x008);   /* DAC softmute is disabled, DAC oversampling rate is 128x */
	WriteVerify_WAU88XX_A(0x0e, 0x108);   /* ADC HP filter is disabled, ADC oversampling rate is 128x */
	WriteVerify_WAU88XX_A(0x0f, 0x1EF);   /* ADC left digital volume control */
	WriteVerify_WAU88XX_A(0x10, 0x1EF);   /* ADC right digital volume control */		
	WriteVerify_WAU88XX_A(0x2c, 0x000);   /* LLIN/RLIN is not connected to PGA */
	WriteVerify_WAU88XX_A(0x2f, 0x050);   /* LLIN connected, and its Gain value */
	WriteVerify_WAU88XX_A(0x30, 0x050);   /* RLIN connected, and its Gain value */
	WriteVerify_WAU88XX_A(0x32, 0x001);   /* Left DAC connected to LMIX */
	WriteVerify_WAU88XX_A(0x33, 0x001);   /* Right DAC connected to RMIX */

	



	
//	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x17, Read_WAU88XX_A(0x17), 0x001);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x01, Read_WAU88XX_A(0x01), 0x1FB);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x02, Read_WAU88XX_A(0x02), 0x1BF);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x03, Read_WAU88XX_A(0x03), 0x1EF);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x05, Read_WAU88XX_A(0x05), 0x001);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x06, Read_WAU88XX_A(0x06), 0x001);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x0e, Read_WAU88XX_A(0x0e), 0x108);
//	printf("Addr@%x=%x; Expected=%x;\n", 0x34, Read_WAU88XX_A(0x34), 0x03F);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x35, Read_WAU88XX_A(0x35), 0x039);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x36, Read_WAU88XX_A(0x36), 0x039);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x37, Read_WAU88XX_A(0x37), 0x039);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x2d, Read_WAU88XX_A(0x2d), 0x011);	
//	printf("Addr@%x=%x; Expected=%x;\n", 0x2f, Read_WAU88XX_A(0x2f), 0x100);		


}
void ADCGainIs(uint8_t gain) {
	Write_WAU88XX_A(0x2d, GAIN_UPDATE|gain);
	printf("ADC gain =0x%x\n", Read_WAU88XX_A(0x2d));
}
uint32_t ADCGain() {
 	return Read_WAU88XX_A(0x2d);
}
