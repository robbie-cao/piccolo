/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "Driver\DrvFMC.h"
#include "Driver\DrvGPIO.h"
#include "Driver\DrvUART.h"
#include "Driver\DrvSYS.h"

#define KEY_ADDR    0x20002FFC      /* The location of signature */
#define SIGNATURE   0x21557899  /* The signature word is used by AP code to check if simple LD is finished */

#define LDROM_BASE      0x00100000
#define PAGE_SIZE       512

extern uint32_t loaderImageBase;
extern uint32_t loaderImageLimit;


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
	uint32_t u32Data, i, u32ImageSize, j, *pu32Loader;

    uint8_t ch;
  
	UNLOCKREG();
    SYSCLK->PWRCON.OSC49M_EN = 1;
    /* Waiting for 12M Xtal stalble */
    //SysTimerDelay(5000);

	DrvUART_Init(115200);

    
	/* Enable ISP function */
	DrvFMC_EnableISP(1);

    /* Check the signature to check if Simple LD code is finished or not */
    if(inpw(KEY_ADDR) == SIGNATURE)
    {
        /* Just clear SIGNATURE and finish the sample code if Simple LD code has been executed. */
        outpw(KEY_ADDR, 0);
     /* Read BS */
        printf("  Boot Mode .................................. ");
        if(DrvFMC_GetBootSelect() == APROM)
            printf("[APROM]\n");
        else
        {
            printf("[LDROM]\n");
            printf("  WARNING: The driver sample code must execute in AP mode!\n");
        }
        goto lexit;
    }
    
    printf("\n\n");
    printf("+-------------------------------------------------------------------------+\n");
    printf("|            NUC1xx Flash Memory Controller Driver Sample Code            |\n");
    printf("+-------------------------------------------------------------------------+\n");

    /* Read BS */
    printf("  Boot Mode");
    if(DrvFMC_GetBootSelect() == APROM)
        printf("[APROM]\n");
    else
    {
        printf("[LDROM]\n");
        printf("  WARNING: The driver sample code must execute in AP mode!\n");
        goto lexit;
    }

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

    /* Check the data in LD ROM to avoid overwrite them */
    DrvFMC_Read(LDROM_BASE, &u32Data);
    if(u32Data != 0xFFFFFFFF)
    {
        printf("\n  WARNING: There is code in LD ROM.\n  If you proceed, the code in LD ROM will be corrupted.\n");
        printf("  Continue? [y/n]:");
        ch = getchar();
        putchar(ch);
        if(ch != 'y')
            goto lexit;
        printf("\n\n");
    }

	/* Enable LDROM update */
	DrvFMC_EnableLDUpdate(1);

    printf("  Erase LD ROM ............................... ");
	/* Page Erase LDROM */
    for(i=0;i<4096;i+=PAGE_SIZE)
        DrvFMC_Erase(LDROM_BASE + i);
	/* Erase Verify */
    i32Err = 0;
	for(i = LDROM_BASE; i < (LDROM_BASE+4096); i += 4) 
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
	

    printf("  Program LD ROM test ........................ ");
	/* Program LD ROM and read out data to compare it */
    for(i = LDROM_BASE; i < (LDROM_BASE+4096); i += 4) 
    {
        DrvFMC_Write(i, i);
    }

    i32Err = 0;
    for(i = LDROM_BASE; i < (LDROM_BASE+4096); i += 4) 
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


    /* Check LD image size */
    u32ImageSize = (uint32_t)&loaderImageLimit - (uint32_t)&loaderImageBase;
    if(u32ImageSize == 0)
    {
        printf("  ERROR: Loader Image is 0 bytes!\n");
        goto lexit;
    }

    if(u32ImageSize > 4096)
    {
        printf("  ERROR: Loader Image is larger than 4KBytes!\n");
        goto lexit;    
    }


    printf("  Program Simple LD Code ..................... ");
    pu32Loader = (uint32_t *)&loaderImageBase;
    for(i=0;i<u32ImageSize;i+=PAGE_SIZE)
    {
        DrvFMC_Erase(LDROM_BASE + i);    
        for(j=0;j<PAGE_SIZE;j+=4)
        {
            DrvFMC_Write(LDROM_BASE + i + j, pu32Loader[(i + j) / 4]);
        }
    }

    /* Verify loader */
    i32Err = 0;
    for(i=0;i<u32ImageSize;i+=PAGE_SIZE)
    {
        for(j=0;j<PAGE_SIZE;j+=4)
        {
            DrvFMC_Read(LDROM_BASE + i + j, &u32Data);
            if(u32Data != pu32Loader[(i+j)/4])
                i32Err = 1;
            
            if(i + j >= u32ImageSize)
                break;
        }


    }

    if(i32Err)
    {
        printf("[FAIL]\n");
    }
    else
    {
        printf("[OK]\n");
        
        /* Reset CPU to boot to LD mode */
        printf("\n  >>> Reset to LD mode <<<\n");
        DrvFMC_BootSelect(LDROM);
        DrvSYS_ResetCPU();
    }


lexit:

 	/* Disable ISP function */
	DrvFMC_EnableISP(0);

	/* Lock protected registers */
	LOCKREG();
    
	printf("\nFMC Sample Code Completed.\n");

}




