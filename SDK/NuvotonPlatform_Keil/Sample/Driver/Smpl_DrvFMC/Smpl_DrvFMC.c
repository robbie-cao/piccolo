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

#define KEY_ADDR    0x20003FFC      /* The location of signature */
#define SIGNATURE   0x21557899  /* The signature word is used by AP code to check if simple LD is finished */


extern uint32_t loaderImageBase;
extern uint32_t loaderImageLimit;

#define CONFIG0_TEST_CODE   0x12fffffe
#define CONFIG1_TEST_CODE   0x12020000

void TestErr (int32_t i32Err){
    if(i32Err)
        printf("[FAIL]\n");
    else
        printf("[OK]\n");
}   

/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function									                                           			   */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
	int32_t  i32Ret, i32Err;
	uint32_t u32Data, i, u32ImageSize, j, *pu32Loader;
#ifndef SEMIHOST
	STR_UART_T sParam;
#endif
    uint8_t ch;
    uint32_t config0Bak, config1Bak;
    uint32_t dfBase, dfLimit;
    
	UNLOCKREG();
	SYSCLK->PWRCON.OSC49M_EN = 1;
	SYSCLK->CLKSEL0.HCLK_S = 0; /* Select HCLK source as 48MHz */ 
	SYSCLK->CLKDIV.HCLK_N  = 0;	/* Select no division          */
	SYSCLK->CLKSEL0.OSCFSel = 0;	/* 1= 32MHz, 0=48MHz */

#ifndef SEMIHOST
    /* Multi-Function Pin: Enable UART0:Tx Rx */
	SYS->GPA_ALT.GPA8 = 1;
	SYS->GPA_ALT.GPA9 = 1;
  
	/* UART Setting */
    sParam.u32BaudRate 		= 115200;
    sParam.u8cDataBits 		= DRVUART_DATABITS_8;
    sParam.u8cStopBits 		= DRVUART_STOPBITS_1;
    sParam.u8cParity 		= DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

	/* Set UART Configuration */
	DrvUART_Open(UART_PORT0,&sParam);
#endif   
    printf("\n");
    
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
            printf("  Signature from LDROM boot code detected and cleared\n");
            goto lexit;
        }
    
    printf("\n\n");
    printf("+-------------------------------------------------------------------------+\n");
    printf("|            ISD91xx Flash Memory Controller Driver Sample Code           |\n");
    printf("+-------------------------------------------------------------------------+\n");

    /* Read BS */
    printf("  Boot Mode .................................. ");
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
	else 
		printf("  ISP Fail ................................... [0x%08x]\n", i32Ret);
	/* Read Device ID */
	i32Ret = DrvFMC_ReadDID(&u32Data);
	if (i32Ret == 0)
		printf("  Device ID .................................. [0x%08x]\n", u32Data);
	else 
		printf("  ISP Fail ................................... [0x%08x]\n", i32Ret);
	/* Read Product ID */
	i32Ret = DrvFMC_ReadPID(&u32Data);
	if (i32Ret == 0)
		printf("  Product ID ................................. [0x%08x]\n", u32Data);
	else 
		printf("  ISP Fail ................................... [0x%08x]\n", i32Ret);

	/* Read Data Flash base address */
	u32Data = DrvFMC_ReadDataFlashBaseAddr();
	printf("  Data Flash Base Address .................... [0x%08x]\n", u32Data);

    /* Read and backup the configuration settings */
    printf("  Read config0 ............................... ");
    i32Ret = DrvFMC_Read(CONFIG_BASE, &config0Bak);
    if (i32Ret == 0)
		printf("[0x%08x]\n", config0Bak);
	else
		printf("[FAIL]\n");

    printf("  Read config1 ............................... ");
    i32Ret = DrvFMC_Read(CONFIG_BASE+4, &config1Bak);
    if (i32Ret == 0)
		printf("[0x%08x]\n", config1Bak);
	else
		printf("[FAIL]\n");


    /* Configuration region write test */
    printf("  Erase config region ........................ ");
    DrvFMC_EnableConfigUpdate(TRUE);
	DrvFMC_Erase(CONFIG_BASE);
    DrvFMC_Read(CONFIG_BASE, &u32Data);
    TestErr (u32Data != 0xFFFFFFFF);

    printf("  Program config region ...................... ");
    i32Err = DrvFMC_Write(CONFIG_BASE, CONFIG0_TEST_CODE);
    i32Err |= DrvFMC_Write(CONFIG_BASE+4, CONFIG1_TEST_CODE);
    
    DrvFMC_Read(CONFIG_BASE, &u32Data);
    if(u32Data != CONFIG0_TEST_CODE)
        i32Err = 1;
    DrvFMC_Read(CONFIG_BASE+4, &u32Data);
    if(u32Data != CONFIG1_TEST_CODE)
        i32Err = 1;
    TestErr(i32Err);

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
    TestErr(i32Err);
   
	/* Check memory function in data flash space */
	u32Data = DrvFMC_ReadDataFlashBaseAddr();
	if((u32Data < 0x22000) && (u32Data > 0x3ff)){
        dfBase = u32Data;
        dfLimit= u32Data + 4096;
        printf("  Erase Data ROM:0x%04x..0x%04x ............ ", dfBase, dfLimit-1);
        /* Page Erase Data Flash  */
        for(i=dfBase;i<dfLimit;i+=PAGE_SIZE)
            DrvFMC_Erase(i);
        /* Erase Verify */
        i32Err = 0;
        for(i = dfBase; i < dfLimit; i += 4){     
            DrvFMC_Read(i, &u32Data);
            if(u32Data != 0xFFFFFFFF)
                i32Err = 1;
        }
        TestErr(i32Err);

        printf("  Program Data Flash:0x%04x..0x%04x ........ ", dfBase, dfLimit-1);
        for(i = dfBase; i < dfLimit; i += 4) 
            DrvFMC_Write(i, i);
        i32Err = 0;
        for(i = dfBase; i < dfLimit; i += 4){
            DrvFMC_Read(i, &u32Data);
            if(u32Data != i)
                i32Err = 1;
        }
        TestErr(i32Err);
    }else{
        printf("  Data Flash Base Address not valid .......... [0x%08x]\n", u32Data);
    }
        
    /* Check the data in LD ROM to avoid overwrite them */
    DrvFMC_Read(LDROM_BASE, &u32Data);
    if(u32Data != 0xFFFFFFFF)
        {
        check:            
            printf("\n  WARNING: There is code in LDROM.\n  If you proceed, the code in LDROM will be corrupted.\n");
            printf("  Continue? [y/n]:");
            ch = getchar();
            putchar(ch);
            if(ch == 'n')
                goto lexit;
            if(ch != 'y')
                goto check;
            printf("\n\n");
        }
    DrvFMC_Read(LDROM_BASE, &u32Data);
    /* Enable LDROM update */
    DrvFMC_EnableLDUpdate(1);
    printf("  Erase LD ROM ............................... ");
        DrvFMC_Read(LDROM_BASE, &u32Data);
	/* Page Erase LDROM */
    for(i=0;i<4096;i+=PAGE_SIZE)
        DrvFMC_Erase(LDROM_BASE + i);
	DrvFMC_Read(LDROM_BASE, &u32Data);
	/* Erase Verify */
    i32Err = 0;
	for(i = LDROM_BASE; i < (LDROM_BASE+4096); i += 4){     
            DrvFMC_Read(i, &u32Data);      
            if(u32Data != 0xFFFFFFFF){ 
                    i32Err = 1;
            }         
    }
    TestErr(i32Err);

    printf("  Program LDROM test ......................... ");
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
    TestErr(i32Err);

    /* Check LD image size */
	u32ImageSize = (uint32_t)&loaderImageLimit - (uint32_t)&loaderImageBase;
	
    if(u32ImageSize == 0){
            printf("  ERROR: Loader Image is 0 bytes!\n");
            goto lexit;
        }

    if(u32ImageSize > 4096){
            printf("  ERROR: Loader Image is larger than 4KBytes!\n");
            goto lexit;    
        }


    printf("  Program Simple LD Code ..................... ");
    pu32Loader = (uint32_t *)&loaderImageBase;

    for(i=0;i<u32ImageSize;i+=PAGE_SIZE){
            DrvFMC_Erase(LDROM_BASE + i);    
            for(j=0;j<PAGE_SIZE;j+=4){
                    DrvFMC_Write(LDROM_BASE + i + j, pu32Loader[(i + j) / 4]);
                }
        }

    /* Verify loader */
    i32Err = 0;
    for(i=0;i<u32ImageSize;i+=PAGE_SIZE) {
        for(j=0;j<PAGE_SIZE;j+=4){
            DrvFMC_Read(LDROM_BASE + i + j, &u32Data);
            if(u32Data != pu32Loader[(i+j)/4])
                i32Err = 1;
            if(i + j >= u32ImageSize)
                break;
        }
   }
    TestErr(i32Err);
    /* Reset CPU to boot to LD mode */
    printf("\n  >>> Reset to LD mode <<<\n");
    /* Finish Printing */
    while(UART0->FSR.TE != 1);
    DrvFMC_BootSelect(LDROM);
    DrvSYS_ResetCPU();
 
 lexit:
    
    /* D    isable ISP function */
    DrvFMC_EnableISP(0);
    
    /* Lock protected registers */
	LOCKREG();
    
    printf("\nFMC Sample Code Completed.\n");
    
}




