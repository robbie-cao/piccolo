/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/* Siren7 (G.722) licensed from Polycom Technology                                                         */
/*---------------------------------------------------------------------------------------------------------*/

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

#include "ISD9160.h"

#include "version.h"


// Below variables should be local variable
#if 0
static uint8_t sChStatus[CHANNEL_COUNT];
#endif


/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
static void InitialSystemClock(void);


/*---------------------------------------------------------------------------------------------------------*/
/* InitialSystemClock                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
static void InitialSystemClock(void)
{
    /* Unlock the protected registers */
    UNLOCKREG();

    /* HCLK clock source. */
    DrvSYS_SetHCLKSource(0);

    LOCKREG();

    /* HCLK clock frequency = HCLK clock source / (HCLK_N + 1) */
    DrvSYS_SetClockDivider(E_SYS_HCLK_DIV, 0);
}


/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void)
{
    InitialSystemClock();

    /* Initialize the operating system */
    osal_init_system();

    InitialUART();
    InitialI2C();
#if ENABLE_GPIO
    InitialGPIO();
#endif

    LOG("\r\n");
    LOG("%s\r\n", PRODUCT_NAME);
    LOG("%s\r\n", VERSION_HW);
    LOG("%s\r\n", VERSION_FW);
    LOG("%s\r\n", BUILD_DATE);

    LOG("Aud\r\n");

    SpiFlash_Open();

    while (1) {
        if (I2C_DataReceived()) {
            I2C_DataReceiveidHandler();
            I2C_DataClear();
        }

        if (PlaySpi_GetPlayingStatus()) {
            PlaySpi_PlayLoop();
        } else {
            PlaySpi_Close();
        }
    }

#if defined ( POWER_SAVING )
    osal_pwrmgr_device( PWRMGR_BATTERY );
#endif

    /* Start OSAL */
    osal_start_system(); // No Return from here

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 list : */
