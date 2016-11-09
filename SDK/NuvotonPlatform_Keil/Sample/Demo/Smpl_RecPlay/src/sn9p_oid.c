
#include "sn9p_oid.h"

typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef int BOOL;
typedef unsigned long UINT32;
typedef long INT32;

#include "ISD9xx.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSYS.h"

//============================

#define SDA_PIN				15	
#define SCK_PIN				14
#define UserCmd_PowerOnOID                         0x53
#define UserCmd_PowerDownOID                       0x56
#define UserCmd_AutoSleepFunEnable                 0xA0
#define UserCmd_AutoSleepFunDisable                0xA3
#define UserCmd_TriggerToClearAutoSleepTimmer      0xA6
#define UserCmd_ClearAutoSleepTimmerIfOIDDetect    0xAC
#define UserCmd_NonClearAutoSleepTimmerIfOIDDetect 0x50
#define UserCmd_CheckOIDStatus                     0x30

UINT16	u16OID_H_Word;
UINT16	u16OID_L_Word;

//-----------------------
// System timer
void SysTimerDelay(uint32_t us)
{
#if 0
    SysTick->LOAD = us * 48; 
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
#else
	volatile unsigned long x = us<<2 - us;
	while(x)x--;
#endif
}



//------------------
//OID related API

#define SET_SDATA_as_Input()	DrvGPIO_Open(GPA,SDA_PIN,IO_INPUT)
#define SET_SDATA_as_Output()	DrvGPIO_Open(GPA,SDA_PIN,IO_OUTPUT)
#define OID_delay()		 		SysTimerDelay(1)

/*
void OID_delay(void)
{
	UINT32 i;
	i = 25;
	while(i--);	
}
*/
 
void Set_OID_SDATA(UINT8 H_or_L)
{
 	if (H_or_L)
		DrvGPIO_SetBit(GPA,SDA_PIN);
	else
		DrvGPIO_ClrBit(GPA,SDA_PIN);
}

 void Set_OID_SCLK(UINT8 H_or_L)
{
	if (H_or_L)
		DrvGPIO_SetBit(GPA,SCK_PIN);	
	else
		DrvGPIO_ClrBit(GPA,SCK_PIN);
}


BOOL Get_OID_SDATA_Status(void)
{
	if (DrvGPIO_GetBit(GPA,SDA_PIN)== 1)
		return TRUE;
	else
		return FALSE;
}


void OID_1_Clock(void)
{
	Set_OID_SCLK(1);
	OID_delay();
	Set_OID_SCLK(0);
	OID_delay();
}



void OID_Read_23_bit(void)
{
	UINT8 i;
	UINT32 	u32temp_OID = 0;
	
	//---<< 1 >>-------------------
	Set_OID_SCLK(1);
	OID_delay();
	SET_SDATA_as_Output();
	Set_OID_SDATA(0);	
	OID_delay();
	Set_OID_SCLK(0);
	OID_delay();
	Set_OID_SCLK(1);
	
	//-----------------------------
	SET_SDATA_as_Input();
	OID_delay();
	
	//---<< Read bit 22 >>---------
	Set_OID_SCLK(0);
	if(Get_OID_SDATA_Status())	
		(u32temp_OID |= 0x01);
	OID_delay();	//delay
	
	//---<< Read bit 21 to 0 >>----
	for (i=0;i<22;i++)
	{		
		u32temp_OID = (u32temp_OID << 1);	
		OID_1_Clock();
		if(Get_OID_SDATA_Status())	
			(u32temp_OID |= 0x01);	
	}
	//-----------------------------
 	u16OID_L_Word = (UINT16)(u32temp_OID & 0x0000FFFF);
	u16OID_H_Word = (UINT16)((u32temp_OID >> 16) & 0x0000FFFF);
}


void Write_UserCmd(UINT16 UserCmd)
{
	UINT8 i;
	
	SET_SDATA_as_Output();
	UserCmd |= 0x0100;
	for (i=0;i<9;i++)
	{
		Set_OID_SCLK(1);
		if (UserCmd & 0x0100)		// bit 8
		{
			Set_OID_SDATA(1);
		}
		else
		{
			Set_OID_SDATA(0);
		}
		UserCmd = UserCmd << 1;
		OID_delay();	//delay
		Set_OID_SCLK(0);
		OID_delay();	//delay
	}		
	SET_SDATA_as_Input();
}


BOOL Check_OID_Data(void)
{
	if ((u16OID_H_Word == 0x0000) && (u16OID_L_Word == 0x0000))
		return FALSE;
	if ((u16OID_H_Word & 0x0003) != 0x0000)
	{	
		if ((u16OID_L_Word == 0xFFFB) 
		 || (u16OID_L_Word == 0xFFFC) 
		 || (u16OID_L_Word == 0xFFFD) 
		 || (u16OID_L_Word == 0xFFFE) 
		 || (u16OID_L_Word == 0xFFFF))
			return FALSE;		
	}
	else if  (((u16OID_H_Word & 0x0040) != 0x0040) || ((u16OID_H_Word & 0x0020) == 0x0020))
	{
		return FALSE;
	}
	else if (u16OID_L_Word == 0xFFFF) 
	{
		return FALSE;
	}
	return TRUE;
}

 

BOOL Read_OID_data(void)
{
	if (Get_OID_SDATA_Status())
	{
		return FALSE;
	}
	else
	{		
		OID_Read_23_bit();
		SysTimerDelay(100000);			//delay 100ms
		return (Check_OID_Data());
	}
}



void oid_init(void)
{
	int timeo = 10;

	DrvGPIO_Open(GPA,SDA_PIN,IO_INPUT);		//GPA0 OID Data (input)											
	DrvGPIO_Open(GPA,SCK_PIN,IO_OUTPUT);	//GPA1 OID clock (output)
	DrvGPIO_ClrBit(GPA,SCK_PIN);	 		//Set clock low
	
	SET_SDATA_as_Input();
	Set_OID_SCLK(1);
	SysTimerDelay(80000); 		//delay 80ms
	Set_OID_SCLK(0);
	SysTimerDelay(200000);	  	//delay 200ms
	
	while (Get_OID_SDATA_Status() && timeo--)
	{
		SysTimerDelay(200000);	 	//delay 200ms
	}
    	
	
	OID_Read_23_bit();	
	SysTimerDelay(20000);		//delay 20ms	
	Write_UserCmd(UserCmd_AutoSleepFunDisable);	
  	SysTimerDelay(20000);	 	//delay 20ms
}

int oid_pull(void)
{
	/*
	while(1)
	{
		OID_Read_23_bit();
	}
	*/
	if(Read_OID_data())
	{
		return (u16OID_L_Word + 1);
	}
	return 0;
}
