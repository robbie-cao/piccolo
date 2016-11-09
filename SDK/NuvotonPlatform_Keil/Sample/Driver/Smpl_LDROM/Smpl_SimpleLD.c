/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvFMC.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"

#define KEY_ADDR    0x20002FFC  /* The location of signature */
#define SIGNATURE   0x21557899  /* The signature word is used by AP code to check if simple LD is finished */

#define CONFIG0_TEST_CODE   0x0F9000FF
#define CONFIG1_TEST_CODE   0x9abcdef0

#define CONFIG_BASE     0x00300000
#define LDROM_BASE      0x00100000
#define PAGE_SIZE       512




void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 22; /* Assume the internal 22MHz RC used */
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}




/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
	int32_t  i32Ret, i32Err;
	uint32_t u32Data, i;
    uint32_t apBase, apLimit;
    uint32_t config0Bak, config1Bak;
    
	UNLOCKREG();
    SYSCLK->PWRCON.OSC49M_EN = 1;
    /* Waiting for 12M Xtal stalble */
  //  SysTimerDelay(5000);
	
	/* Set UART Configuration */
    DrvUART_Init(115200);

//    printf("+-------------------------------------------------------------------------+\n");
//    printf("|       NUC1xx Flash Memory Controller Driver Sample Code for LDROM       |\n");
//    printf("+-------------------------------------------------------------------------+\n");  


	/* Enable ISP function */
	DrvFMC_EnableISP(1);

    /* Read BS */
    printf("  Boot Mode .................................. ");
    if(DrvFMC_GetBootSelect() == APROM)
    {
        printf("[APROM]\n");
        
        printf("  WARNING: This code should be in LD mode but it is in AP mode now.\n");
        while(1); 
    }
    else
        printf("[LDROM]\n");


	/* Read Company ID */
	i32Ret = DrvFMC_ReadCID(&u32Data);
	if (i32Ret == 0)
		printf("  Company ID ................................. [0x%08x]\n", u32Data);

	/* Read Device ID */
	i32Ret = DrvFMC_ReadDID(&u32Data);
	if (i32Ret == 0)
		printf("  Device ID .................................. [0x%08x]\n", u32Data);

	/* Read Data Flash base address */
	u32Data = DrvFMC_ReadDataFlashBaseAddr();
	printf("  Data Flash Base Address .................... [0x%08x]\n", u32Data);

    
    apBase = 0x4000;
    apLimit= 0x8000;

    printf("  Erase AP ROM:0x%04x..0x%04x ................ ", apBase, apLimit-1);
	/* Page Erase LDROM */
    for(i=apBase;i<apLimit;i+=PAGE_SIZE)
        DrvFMC_Erase(i);
	/* Erase Verify */
    i32Err = 0;
	for(i = apBase; i < apLimit; i += 4) 
    {     
		DrvFMC_Read(i, &u32Data);
        
        if(u32Data != 0xFFFFFFFF)
        { 
            i32Err = 1;
        }         
    }
    if(i32Err)
        printf("[FAIL]\n");
    else
        printf("[OK]\n");
	

    printf("  Program AP ROM:0x%04x..0x%04x .............. ", apBase, apLimit-1);
	/* Program AP ROM and read out data to compare it */
    for(i = apBase; i < apLimit; i += 4) 
    {
        DrvFMC_Write(i, i);
    }

    i32Err = 0;
    for(i = apBase; i < apLimit; i += 4) 
    {
		DrvFMC_Read(i, &u32Data);
        if(u32Data != i)
        { 
           i32Err = 1;
        }        
    }
    if(i32Err)
        printf("[FAIL]\n");
    else
        printf("[OK]\n");

    /* Read and backup the configuration settings */
    printf("  Read config0 ............................... ");
    DrvFMC_Read(CONFIG_BASE, &config0Bak);
    printf("[0x%08x]\n", config0Bak);

    printf("  Read config1 ............................... ");
    DrvFMC_Read(CONFIG_BASE+4, &config1Bak);
    printf("[0x%08x]\n", config1Bak);


    /* Configuration region write test */
    printf("  Erase config region ........................ ");
    DrvFMC_EnableConfigUpdate(TRUE);
	DrvFMC_Erase(CONFIG_BASE);
    DrvFMC_Read(CONFIG_BASE, &u32Data);
    if(u32Data != 0xFFFFFFFF)
        printf("[FAIL]\n");
    else
        printf("[OK]\n");

    printf("  Program config region ...................... ");
    DrvFMC_Write(CONFIG_BASE, CONFIG0_TEST_CODE);
    DrvFMC_Write(CONFIG_BASE+4, CONFIG1_TEST_CODE);
    i32Err = 0;    
    DrvFMC_Read(CONFIG_BASE, &u32Data);
    if(u32Data != CONFIG0_TEST_CODE)
        i32Err = 1;
    DrvFMC_Read(CONFIG_BASE+4, &u32Data);
    if(u32Data != CONFIG1_TEST_CODE)
        i32Err = 1;

    if(i32Err)
        printf("[FAIL]\n");
    else
        printf("[OK]\n");

    printf("  Restore config settings .................... ");
    DrvFMC_Erase(CONFIG_BASE);
    DrvFMC_Write(CONFIG_BASE, config0Bak);
    DrvFMC_Write(CONFIG_BASE+4, config1Bak);
	DrvFMC_EnableConfigUpdate(FALSE);

    i32Err = 0;
    DrvFMC_Read(CONFIG_BASE, &u32Data);
    if(u32Data != config0Bak)
        i32Err = 1;
    
    DrvFMC_Read(CONFIG_BASE+4, &u32Data);
    if(u32Data != config1Bak)
        i32Err = 1;

     if(i32Err)
        printf("[FAIL]\n");
    else
        printf("[OK]\n");
   
    /* Write the signature to tell AP code that LD code has finished */
    outpw(KEY_ADDR, SIGNATURE);

	printf("\nFMC Simple LD Code Completed.\n");

    printf("\n  >>> Reset to back to AP mode <<< \n");

    /* Make sure message has printed out */
    while(UART0->FSR.TE == 0);
    
    DrvFMC_BootSelect(APROM);
    DrvSYS_ResetCPU();

 	/* Disable ISP function */
	DrvFMC_EnableISP(0);

	/* Lock protected registers */
	LOCKREG();
}




