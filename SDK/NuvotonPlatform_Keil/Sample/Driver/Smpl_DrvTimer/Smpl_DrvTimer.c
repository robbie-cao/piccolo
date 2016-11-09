/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2009 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>

/*---------------------------------------------------------------------------------------------------------*/
/*  Include hear file   	    				                                                           */
/*---------------------------------------------------------------------------------------------------------*/

#include "ISD9xx.h"
#include "Driver/DrvTimer.h"
#include "Driver/DrvUART.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSYS.h"

volatile uint8_t b8WDTINT = FALSE;

/*---------------------------------------------------------------------------------------------------------*/
/*  Sample Code Menu		    				                                                           */
/*---------------------------------------------------------------------------------------------------------*/
static void TestItem (void)
{
    printf("\n\n");
    printf("+-----------------------------------------------------------+\n");
    printf("|               Timer Sample Program                        |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("| Timer Callback                                      - [1] |\n");
    printf("| Timer GetTick                                       - [2] |\n");
    printf("| Timer Delay                                         - [3] |\n");
    printf("| WDT Callback                                        - [4] |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("| Quit                                        	    - [ESC] |\n");
    printf("+-----------------------------------------------------------+\n");
    printf("Select key : ");

}

/*---------------------------------------------------------------------------------------------------------*/
/*  Callback funtion    	    				                                                           */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint32_t u32Timer0Cnt = 0;

void TMR_Callback()
{
	 printf("Timer callback #%d, ticks %d\n", u32Timer0Cnt++, DrvTIMER_GetTicks(TMR0));
}

void WDT_Callback()
{
    b8WDTINT = TRUE;

    DrvWDT_Ioctl(E_WDT_IOC_RESET_TIMER, 0);
	printf("  \n");
    printf("  WDT interrupt !!!\n");
	printf("  \n");
	
		
    DrvWDT_Close();
}


void SysTimerDelay(uint32_t us)
{
    SysTick->LOAD = us * 48; 
    SysTick->VAL   =  (0x00);
    SysTick->CTRL = (1 << SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE);

    /* Waiting for down-count to zero */
    while((SysTick->CTRL & (1 << 16)) == 0);
}


/*---------------------------------------------------------------------------------------------------------*/
/*  MAIN function			    				                                                           */
/*---------------------------------------------------------------------------------------------------------*/

int main (void)
{
    int32_t tick;
    int8_t item;
	STR_UART_T param;

	UNLOCKREG();
	
	SYSCLK->PWRCON.XTL32K_EN = 1;	     
    SYSCLK->PWRCON.OSC49M_EN = 1;	     
    SYSCLK->PWRCON.OSC10K_EN = 1;
	SYSCLK->CLKSEL1.WDG_S = 2;

    SysTimerDelay(5000);

	DrvGPIO_InitFunction(FUNC_UART0);
	
    param.u32BaudRate        = 115200;
    param.u8cDataBits        = DRVUART_DATABITS_8;
    param.u8cStopBits        = DRVUART_STOPBITS_1;
    param.u8cParity          = DRVUART_PARITY_NONE;
    param.u8cRxTriggerLevel  = DRVUART_FIFO_62BYTES;
    param.u8TimeOut        	 = 0;
    DrvUART_Open(UART_PORT0, &param);
	
	DrvTIMER_Init();
   	printf(" TIMER Sample Code \n");
    
	DrvSYS_SetIPClockSource(E_SYS_TMR0_CLKSRC,2);
	DrvSYS_SetIPClockSource(E_SYS_TMR1_CLKSRC,2);

    do
    {
        TestItem();
        item = getchar();
        printf("%c\n",item);
        switch(item)
        {
            case '1':    
            {   
                u32Timer0Cnt = 0;
                
                /* Using TIMER0 PERIODIC_MODE , 2 tick /sec */
                DrvTIMER_Open(TMR0,2,PERIODIC_MODE);  
                
				/* Install Callback function "call_back"  when Interrupt happen twice time */
                DrvTIMER_SetTimerEvent(TMR0,2, (TIMER_CALLBACK)TMR_Callback,0);
			    
				/* Enable TIMER0 Intettupt */
                DrvTIMER_EnableInt(TMR0);	
				
                /* Waiting for first timer callback */
                while(u32Timer0Cnt == 0);
                
                printf("  --------------------- [Delay for 4 ticks ] ---------------------\n");
				/* Delay 4 ticks*/
                DrvTIMER_Delay(4);
                printf("  --------------------- [  Delay times up  ] ---------------------\n");			    


                printf("  Waiting for 10 times timer callbacks ...\n");
                while(u32Timer0Cnt < 10);
				
			
				
				/* Close TIMER0 */
                DrvTIMER_Close(TMR0);
 				
				printf("Case 1 Finished \n");
                
                break;
            }
            case '2':    
            {   
                DrvTIMER_Open(TMR1,1,PERIODIC_MODE);
                tick = 0;
                do
                {
                    
                    if(tick != DrvTIMER_GetTicks(TMR1))                  /* print when 10 multiple  */
                    {
                        tick = DrvTIMER_GetTicks(TMR1);
                        printf ("tick = %d \n", tick);
                        if(tick > 10) break;
                    }               

                }while(1);
                tick = 0;
                DrvTIMER_ResetTicks(TMR1);
                DrvTIMER_Close(TMR1);
                printf("Case 2 Finished \n");
                break;
            }
            case '3':    
            {   
                printf(" Delay for 3 seconds ...\n");
                DrvTIMER_Open(TMR0,1000,PERIODIC_MODE);    				/* Timer Basic Operation */
                DrvTIMER_Delay(3000);
                DrvTIMER_Close(TMR0);
			    
			    printf("Case 3 Finished \n");
                break;
            }
            
            case '4':    
            { 
                
				printf("WDT Callback Function Test\n");
				printf("WDT interval: LEVEL6\n");

				b8WDTINT = FALSE;
                DrvWDT_Open(E_WDT_LEVEL6);
				DrvWDT_InstallISR((WDT_CALLBACK)WDT_Callback);
                DrvWDT_Ioctl(E_WDT_IOC_ENABLE_INT, 0);                                			                  
				DrvWDT_Ioctl(E_WDT_IOC_START_TIMER, 0);                                			                
				while (1)
                {
                    if ( b8WDTINT )
                        break;
                }

				printf("Case 4 Finished.\n");                
                
				break;
            }
           
            default:
            {
                break;
            }    
        }
    }while(item != 27);

  	printf("\nExit TIMER Sample Code ...\n");
    
	return 0;
}




