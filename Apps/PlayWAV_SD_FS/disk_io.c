/*-----------------------------------------------------------------------*/
/* Low level disk control module for Win32              (C)ChaN, 2007    */
/*-----------------------------------------------------------------------*/


#include <stdio.h>

#include "DrvSDCARD.h"
#include "DrvGPIO.h"

#include "disk_io.h"

extern void DrvSDCARD_SpiRead(uint32_t addr, uint32_t size, uint8_t *buffer);
extern void DrvSDCARD_SpiWrite(uint32_t addr, uint32_t size,  uint8_t *buffer);

void RoughDelay(uint32_t t)
{
    volatile int32_t delay;

    delay = t;

    while(delay-- >= 0);
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

/* ucDrv : Physical drive nmuber */
DSTATUS disk_initialize (unsigned char ucDrv)
{
	DSTATUS sta;

#if 0
//////SD_PWR
	//DrvGPIO_InitFunction(FUNC_GPIO);
	DrvGPIO_Open(GPB,1,IO_OUTPUT);
	DrvGPIO_ClrBit(GPB,1);

////Detect Card connection
    DrvGPIO_Open(GPB,0,IO_INPUT);
	while((GPIOB->PIN&0x0001)!=0);
	RoughDelay(100000);
#endif

	if(DrvSDCARD_Open() == E_SUCCESS)
	{
		sta = 	RES_OK;
		printf("SDCard Open success\n");
	}
	else
	{
		sta = STA_NOINIT;
		printf("SDCard Open failed\n");
	}
	return sta;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
/* ucDrv : Physical drive nmuber */
DSTATUS disk_status (unsigned char ucDrv)
{
	DSTATUS sta1=STA_OK;
	if (ucDrv)
		sta1 =   STA_NOINIT;
	return sta1;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
/* ucDrv :   Physical drive nmuber (0) */
/* ucBuff:   Pointer to the data buffer to store read data */
/* ulSector: Start sector number (LBA) */
/* ucCount : Sector count (1..255) */
DRESULT disk_read (unsigned char ucDrv,	unsigned char *ucBuff,
                   unsigned long ulSector, unsigned char ucCount)
{
	DRESULT res;
	uint32_t size;

  	if (ucDrv)
  	{
		res = (DRESULT)STA_NOINIT;
	    return res;
	}

/*
    if(ucCount==0||ucCount>=2)
   	{
		res =   (DRESULT)STA_NOINIT;
		return res;
	}
*/

    size = ucCount*512;
	DrvSDCARD_SpiRead(ulSector, size, ucBuff);
	res =RES_OK;	/* Clear STA_NOINIT */;

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
/* Physical drive nmuber (0) */
/* Pointer to the data to be written */
/* Start sector number (LBA) */
/* Sector count (1..255) */
DRESULT disk_write (unsigned char ucDrv, const unsigned char *ucBuff,
					unsigned long ulSector,	unsigned char ucCount)
{
	DRESULT  res;
    uint32_t size;

 	if (ucDrv) {
		res = (DRESULT)STA_NOINIT;
		return res;
	}

/*
    if(ucCount==0||ucCount>=2)
	{
		res = (DRESULT)  STA_NOINIT;
		return res;
	}
*/

    size = ucCount*512;
	DrvSDCARD_SpiWrite(ulSector, size,(uint8_t *)ucBuff);
	res = RES_OK;

	return res;
}


DRESULT disk_ioctl (
        unsigned char pdrv,      /* Physical drive number (0..) */
        unsigned char cmd,       /* Control code */
        void *buff               /* Buffer to send/receive control data */
        )
{
    DRESULT res = RES_OK;

#if 0
    switch (pdrv) {

#ifdef SUPPORT_USBH
        case DRV_USBH :
            return usbh_umas_ioctl(cmd, buff);
#endif

#ifdef SUPPORT_SDIO
        case DRV_SD :
            switch(cmd) {
                case CTRL_SYNC:
                    break;
                case GET_SECTOR_COUNT:
                    *(DWORD*)buff = SD_DiskInfo0.totalSectorN;
                    break;
                case GET_SECTOR_SIZE:
                    *(WORD*)buff = SD_DiskInfo0.sectorSize;
                    break;

                default:
                    res = RES_PARERR;
                    break;
            }
            break;
#endif
        default:
            res = RES_PARERR;
            break;

    }
#endif
    return res;
}
