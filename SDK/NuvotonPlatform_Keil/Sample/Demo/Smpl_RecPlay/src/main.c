
#include "ISD9xx.h"

#include "Driver\DrvSYS.h"

#include "hw.h"

#include "libSPIFlash.h"
#include "libVPB.h"
#include "libFFX.h"

#include "libButtons.h"
#include "libCapButtons.h"
//#include "libLightCDS.h"

#include "ffx_io_flash.h"

#include "audfile.h"
#include "sn9p_oid.h"

//#include "lcd_nt7565.h"
#include "lcd_uc1601.h"

#include "stdio.h"


FFXCTX g_disks[1] =
{
 {
	{
		0, // disk_fat_base
		0, // disk_fat_f0_idx
		((1024 + 512)*1024), // disk_size
	}, // disk: FFXDISKCTX
	{
		{
			0, // offset
			0, // globe_offset
			(1024 + 512)*1024, // size
			1, // typeAndInf, 1 for spi flash
		}, // FFXMEDIACTX
	}, // media[DISK_MEDIA_BLOCKS]: FFXMEDIACTX
 },
};

#define DISK_CTX_0 g_disks

BTNCTX gBtnCtx;
#define BTN_CTX() (&gBtnCtx)
const unsigned long libButtonsDebounceTicks = 2;

CAPBTNCTX gCapBtnCtx;
#define CAP_BTN_CTX() (&gCapBtnCtx)
const unsigned long libCapBtnParam = 2 | (0 & 0x010000);

const unsigned long vp_base_addr = 48*1024;
const unsigned long svp_base_addr = (1024+512)*1024;

extern void audOutInit(void);
extern void audInInit(void);

#define BTN_ALT   (1ul << 7)
#define BTN_REC   (1ul << 2)
#define BTN_PLAY  (1ul << 3)
#define BTN_STOP  (1ul << 6)
#define BTN_PREV  (1ul << 5)
#define BTN_NEXT  (1ul << 4)

#define IS_VP_TASK(arg) (arg & 0x80)

#define TASK_NONE_VP   0x80
#define TASK_NONE_FILE 0x00
#define TASK_PLAY_VP   0x81
#define TASK_PLAY_FILE 0x01
#define TASK_REC_FILE  0x02

#define TASK_NONE      0x00
#define TASK_PLAY      0x01
#define TASK_REC       0x02

//#define LED_FOR_BUTTON

#ifdef LED_FOR_BUTTON
#define LED_PIN_BTN_INIT() SYS->GPA_ALT.GPA4 = 0; DrvGPIO_Open(GPA, 4, IO_OUTPUT)
#define LED_FOR_BTN_ON() DrvGPIO_ClrBit(GPA, 4)
#define LED_FOR_BTN_OFF() DrvGPIO_SetBit(GPA, 4)
#else
#define LED_PIN_BTN_INIT()
#define LED_FOR_BTN_ON()
#define LED_FOR_BTN_OFF()
#endif

int vp_items = 0;
int cur_vp_idx = 0;
int cur_task = 0;
int cur_file_id = -1;
int cur_file_type = 0;

int BKG_VP_IDX = 0;
#define VP_VM_ITEMS 0
int vp_vm_items = VP_VM_ITEMS;
const unsigned long *cur_vm_pos = 0;
#if VP_VM_ITEMS
int vp_hide_items = 5;
const unsigned long vm1[] = {0x80000000|0, 0x80000000|3, 0x80000000|1, 0};
const unsigned long vm2[] = {0x80000000|0, 0x80000000|4, 0x80000000|2, 0};
const unsigned long *vms[] = {vm1, vm2};
#else
int vp_hide_items = 0;
const unsigned long *vms[1];
#endif

#define VALID_VP_ITEMS() (vp_items - vp_hide_items + vp_vm_items)

FFXFILE aFile;

void onSysTick(void)
{
}

void playNextFile(int offset)
{
	int iRet;
	if(cur_file_id < 0)
	{
		iRet = ffxFileOpenEx(DISK_CTX_0, 0, 0, &aFile, -2);
		if(iRet >= 0)
		{
			cur_file_id = aFile.fatInf.id;
			cur_file_type = aFile.fatInf.type;
		}
	}
	else
	{
		iRet = ffxFileOpenEx(DISK_CTX_0, cur_file_id, cur_file_type, &aFile, offset);
		if(iRet >= 0)
		{
			cur_file_id = aFile.fatInf.id;
			cur_file_type = aFile.fatInf.type;
		}
	}
	if(cur_file_id >= 0)
	{
		cur_task = TASK_PLAY_FILE;
		if(SetFileForPlay(DISK_CTX_0, cur_file_id, cur_file_type, 0) < 0)
		{
			ffxFileDelete(DISK_CTX_0, cur_file_id, cur_file_type);
			cur_file_id = -1;
		}
		PlayStart();
	}
}

void stop(void)
{
	switch(cur_task)
	{
	case TASK_PLAY_VP:
		cur_vm_pos = 0;
		PlayStop();
		cur_task = TASK_NONE_VP;
		break;
	case TASK_PLAY_FILE:
		PlayStop();
		cur_task = TASK_NONE_FILE;
		break;
	case TASK_REC_FILE:
		RecFile_stop(DISK_CTX_0, cur_file_id, 0x7F);
		cur_task = TASK_NONE_FILE;
		break;
	}
	lcd_puts_at(0, 0, "     Ready            ");
}

static int prev_vpvm_idx = 0;

void playVPVM(unsigned long addr, int idx)
{
	BKG_VP_IDX = prev_vpvm_idx;
	if(BKG_VP_IDX < vp_vm_items)
	{
		BKG_VP_IDX = vp_vm_items;
	}
	prev_vpvm_idx = idx;

	if(idx < vp_vm_items)
	{
		// vm
		cur_vm_pos = (const unsigned long *)vms[idx];
		if(cur_vm_pos)
		{
			if(*cur_vm_pos)
			{
				if((*cur_vm_pos) & 0x80000000)
				{
					if(((*cur_vm_pos) & 0x7FFFFFFF) < vp_items)
					{
						cur_task = TASK_PLAY_VP;
						SetVPForPlay(addr, ((*cur_vm_pos) & 0x3FFFFFFF), 0);
						PlayStart();
					}
				}
			}
			else
			{
				cur_vm_pos = 0; // end of vm
			}
		}
	}
	else
	{
		// vp
		cur_vm_pos = 0;
		cur_task = TASK_PLAY_VP;
		SetVPForPlay(addr, idx - vp_vm_items + vp_hide_items, 0);
		PlayStart();
	}
}

int diskInit(void)
{
	int iRet = 0;

	ffx_io_flash_init();

	if(ffxOpenDisk(DISK_CTX_0) < 0)
	{
		iRet = ffxFormat(DISK_CTX_0);
		iRet = ffxOpenDisk(DISK_CTX_0);
	}

	return iRet;
}

void pull(void)
{
	switch(cur_task)
	{
	case TASK_PLAY_VP:
		if(!PlayPull())
		{
			if(cur_vm_pos)
			{
				cur_vm_pos ++;
				if(*cur_vm_pos)
				{
					if((*cur_vm_pos) & 0x80000000)
					{
						if(((*cur_vm_pos) & 0x7FFFFFFF) < vp_items)
						{
							cur_task = TASK_PLAY_VP;
							SetVPForPlay(vp_base_addr, ((*cur_vm_pos) & 0x3FFFFFFF), 0);
							PlayStart();
							break;
						}
					}
				}
				else
				{
					cur_vm_pos = 0; // end of vm
				}
			}
			stop();
		}
		break;
	case TASK_PLAY_FILE:
		if(!PlayPull())
		{
			stop();
		}
		break;
	case TASK_REC_FILE:
		if(!RecFile_pull(DISK_CTX_0))
		{
			stop();
		}
		break;
	}
}

// rename

extern void mu_update(int idx);

int main(void)
{
	unsigned char cVal;
	int iRet;
	unsigned long uBtn;

	// HW init
	hwInit();

	LED_PIN_BTN_INIT();

	//btnInit(BTN_CTX(), 0x50004000, GPIOA_BUTTONS_MASK);

	lcd_init();
	lcd_clr(0, 0, 128, 64/4);

	__enable_irq();

#ifdef OID_MODULE
	oid_init();
#endif

	audOutInit();
	audInInit();

#ifdef OID_MODULE
	uOidTmo = SysTickCount;
#endif

	// enable LDO
	if(!SYSCLK->APBCLK.ANA_EN)
	{
		SYSCLK->APBCLK.ANA_EN = 1;
		SYS->IPRSTC2.ANA_RST = 1;
		SYS->IPRSTC2.ANA_RST = 0;
	}

	DrvSYS_UnlockKeyAddr();

	// Open LDO
	ANA->LDOSET = 0;
	ANA->LDOPD.PD = 0;

	// biq disabled, a known bug
	SYSCLK->APBCLK.BIQALC_EN = 1;
	BIQ->BIQ_CTRL.RSTn = 1;
	//BIQ->BIQ_CTRL.RSTn = 0; // not using biq

	DrvSYS_LockKeyAddr();

	diskInit();

	vp_items = VPBGetItemCount(vp_base_addr);
	if(vp_hide_items > vp_items)
	{
		vp_hide_items = vp_items;
	}

#ifdef GPIOBTN //using GPIO button
	btnInit(BTN_CTX(), 0x50004040, 0x0FC);
#else //using capsense button 
	// audio init will destory the settings for capsense, need to check why
	cbtnInit(CAP_BTN_CTX(), 0x0FC, 4, 3, 7); // need irq
#endif //GPIOBTN
	lcd_locate(20, 0);
	lcd_puts("Ready");
	lcd_locate(0, 1);
	cVal = 0x02;
//	lcd_enter();
    SpiBusCtrl(ENTERLCDMODE);
	lcd_dat(&cVal, 128, 0);
//	lcd_leave();
    SpiBusCtrl(LEAVELCDMODE);


	mu_update(0);

	while(1)
	{
#ifdef OID_MODULE
		if(uOidTmo - SysTickCount > 5)
		{
			uOidTmo = SysTickCount;
			iOidPageNew = oid_pull();
		}
#endif
#ifdef GPIOBTN //using GPIO button
	    if(btnScan(BTN_CTX(), SysTickCount))
#else //using capsense button 
		if(cbtnScan(CAP_BTN_CTX(), SysTickCount))
#endif //GPIOBTN
		{
#ifdef LED_FOR_BUTTON
#ifdef GPIOBTN //using GPIO button
	        if(btnGetLastDown(BTN_CTX()))
#else //using capsense button 
			if(cbtnGetLastDown(CAP_BTN_CTX()))
#endif //GPIOBTN
			{
				LED_FOR_BTN_ON();
			}
			else
			{
				LED_FOR_BTN_OFF();
			}
#endif
			/*
			if(cbtnGetLastDown(CAP_BTN_CTX()))
			{
				lcd_locate(64, 0);
				lcd_puts("BTN DOWN");
			}
			else
			{
				lcd_locate(64, 0);
				lcd_puts("BTN UP  ");
			}
			*/
#ifdef GPIOBTN //using GPIO button
			uBtn = btnGetLastDown(BTN_CTX());
#else //using capsense button 
			uBtn = cbtnGetLastDown(CAP_BTN_CTX());
#endif //GPIOBTN
			if(uBtn & BTN_ALT)
			{
				if((cur_task & 0x7F) == 0x01)
				{
					SetVPForPlay(vp_base_addr, BKG_VP_IDX - vp_vm_items + vp_hide_items, 1);
					lcd_puts_at(0, 0, "   Play 2 channels    ");
				}
				else
				{
					mu_update(1);
				}
			}
			else
			{
				mu_update(0);
			}

#ifdef GPIOBTN //using GPIO button
			uBtn = btnGet(BTN_CTX());
#else //using capsense button 
			uBtn = cbtnGet(CAP_BTN_CTX());
#endif //GPIOBTN

//Display key pressing underline
			cVal = (uBtn & 0x04)?0x02:0;
			lcd_locate(60 + 10, 3);
//			lcd_enter();
		    SpiBusCtrl(ENTERLCDMODE);
			lcd_dat(&cVal, 60, 0);
//			lcd_leave();
		    SpiBusCtrl(LEAVELCDMODE);
			cVal = (uBtn & 0x08)?0x02:0;
			lcd_locate(60 + 10, 5);
//			lcd_enter();
		    SpiBusCtrl(ENTERLCDMODE);
			lcd_dat(&cVal, 60, 0);
//			lcd_leave();
		    SpiBusCtrl(LEAVELCDMODE);
			cVal = (uBtn & 0x10)?0x02:0;
			lcd_locate(60 + 10, 7);
//			lcd_enter();
		    SpiBusCtrl(ENTERLCDMODE);
			lcd_dat(&cVal, 60, 0);
//			lcd_leave();
		    SpiBusCtrl(LEAVELCDMODE);

			cVal = (uBtn & 0x20)?0x02:0;
			lcd_locate(0, 7);
//			lcd_enter();
		    SpiBusCtrl(ENTERLCDMODE);
			lcd_dat(&cVal, 60, 0);
//			lcd_leave();
		    SpiBusCtrl(LEAVELCDMODE);
			cVal = (uBtn & 0x40)?0x02:0;
			lcd_locate(0, 5);
//			lcd_enter();
		    SpiBusCtrl(ENTERLCDMODE);
			lcd_dat(&cVal, 30, 0);
//			lcd_leave();
		    SpiBusCtrl(LEAVELCDMODE);
			cVal = (uBtn & 0x80)?0x02:0;
			lcd_locate(0, 3);
//			lcd_enter();
    		SpiBusCtrl(ENTERLCDMODE);
			lcd_dat(&cVal, 30, 0);
//			lcd_leave();
		    SpiBusCtrl(LEAVELCDMODE);
/////////////////////////////////////////////
			if(uBtn & BTN_STOP)
			{
				stop();
			}
			if(uBtn & BTN_PLAY)
			{
				stop();

#ifdef GPIOBTN //using GPIO button
				if(BTN_ALT & btnGet(BTN_CTX()))
#else //using capsense button 
				if(BTN_ALT & cbtnGet(CAP_BTN_CTX()))
#endif //GPIOBTN
				{
					if(VALID_VP_ITEMS())
					{
						playVPVM(vp_base_addr, cur_vp_idx);
						lcd_puts_at(0, 0, "  Playing voice prompt");
					}
				}
				else
				{
					if(cur_file_id >= 0)
					{
						cur_task = TASK_PLAY_FILE;
						if(SetFileForPlay(DISK_CTX_0, cur_file_id, cur_file_type, 0) < 0)
						{
							ffxFileDelete(DISK_CTX_0, cur_file_id, cur_file_type);
							cur_file_id = -1;
						}
						PlayStart();
					}
					else
					{
						playNextFile(1);
					}
					lcd_puts_at(0, 0, "   Playing record     ");
				}
			}
			if(uBtn & BTN_REC)
			{
				stop();

#ifdef GPIOBTN //using GPIO button
				if(BTN_ALT & btnGet(BTN_CTX()))
#else //using capsense button 
				if(BTN_ALT & cbtnGet(CAP_BTN_CTX()))
#endif //GPIOBTN
				{
					// new place
					iRet = ffxGetFreeEntryId(DISK_CTX_0);
					if(iRet > 0)
					{
						cur_file_id = iRet;
						cur_file_type = 0x7F;
					}
				}
				else
				{
					// current place
					if(cur_file_id < 0)
					{
						if(ffxFileOpenEx(DISK_CTX_0, 0, 0, &aFile, -2) >= 0)
						{
							cur_file_id = aFile.fatInf.id;
							cur_file_type = aFile.fatInf.type;
						}
						else
						{
							cur_file_id = 1;
							cur_file_type = 0x7F;
						}
					}
				}
				if(cur_file_id > 0)
				{
					cur_task = TASK_REC_FILE;
					ffxFileDelete(DISK_CTX_0, cur_file_id, cur_file_type);
					ffxFatAddEntry(DISK_CTX_0, cur_file_id, cur_file_type);
					if(RecFile(DISK_CTX_0, cur_file_id, cur_file_type, 0x7C000000 + 16000) < 0)
					{
						ffxFileDelete(DISK_CTX_0, cur_file_id, cur_file_type);
						cur_file_id = -1;
						lcd_puts_at(0, 0, "   Error              ");
					}
					else
					{
						lcd_puts_at(0, 0, "   Recording          ");
					}
				}
			}
			if(uBtn & BTN_PREV)
			{
				stop();

#ifdef GPIOBTN //using GPIO button
				if((IS_VP_TASK(cur_task)) || (BTN_ALT & btnGet(BTN_CTX())))
#else //using capsense button 
				if((IS_VP_TASK(cur_task)) || (BTN_ALT & cbtnGet(CAP_BTN_CTX())))
#endif //GPIOBTN
				{
					if(cur_vp_idx > 0)cur_vp_idx --;
					if(VALID_VP_ITEMS())
					{
						playVPVM(vp_base_addr, cur_vp_idx);
						lcd_puts_at(0, 0, "  Playing voice prompt");
					}
				}
				else
				{
					playNextFile(-1);
					lcd_puts_at(0, 0, "   Playing record     ");
				}
			}
			if(uBtn & BTN_NEXT)
			{
				stop();

#ifdef GPIOBTN //using GPIO button
				if((IS_VP_TASK(cur_task)) || (BTN_ALT & btnGet(BTN_CTX())))
#else //using capsense button 
				if((IS_VP_TASK(cur_task)) || (BTN_ALT & cbtnGet(CAP_BTN_CTX())))
#endif //GPIOBTN
				{
					if(VALID_VP_ITEMS())
					{
						if(cur_vp_idx < (VALID_VP_ITEMS() - 1))cur_vp_idx ++;
						playVPVM(vp_base_addr, cur_vp_idx);
						lcd_puts_at(0, 0, "  Playing voice prompt");
					}
				}
				else
				{
					playNextFile(1);
					lcd_puts_at(0, 0, "   Playing record     ");
				}
			}
		}

		pull();

	}
}
