
#include "ISD9xx.h"

#include "libFlash.h"
#include "lib\libSPIFlash.h"
#include "libFFX.h"

#include "ffx_io_flash.h"

#ifndef SPI0_BASE
#define SPI0_BASE (0x40000000 + 0x30000)
#endif // SPI0_BASE

static void spi_setcs(const struct tagSFLASH_CTX *ctx, int csOn)
{
	// use the SPI controller's CS control
}

const SFLASH_CTX g_SPIFLASH =
{
	SPI0_BASE,
	1, // channel 0, auto
	spi_setcs,
};

#define SPIFLASH_CTX(arg) (&g_SPIFLASH)

void ffx_io_flash_init(void)
{
	SYS->GPA_ALT.GPA0              = 1; // MOSI0
	SYS->GPA_ALT.GPA2              = 1; // SSB0
	SYS->GPA_ALT.GPA1              = 1; // SCLK
	SYS->GPA_ALT.GPA3              = 1; // MISO0

	if(!SYSCLK->APBCLK.SPI0_EN)
	{
		SYSCLK->APBCLK.SPI0_EN        = 1;
		SYS->IPRSTC2.SPI0_RST         = 1;
		SYS->IPRSTC2.SPI0_RST         = 0;
	}

	// spi flash
	sflash_set(&g_SPIFLASH);
}

int _ffx_disk_read(FFXCTX *pCtx, unsigned char* p, unsigned long addr, unsigned long bytes)
{
	int i;
	unsigned long toOpt;
	unsigned long cnt = 0;
	for(i=0; i<DISK_MEDIA_BLOCKS; i++)
	{
		if(addr < (pCtx->media[i].globe_offset + pCtx->media[i].size))
		{
			if((addr + bytes) > (pCtx->media[i].globe_offset + pCtx->media[i].size))
			{
				//toOpt = (addr + bytes) - (pCtx->media[i].globe_offset + pCtx->media[i].size);
				toOpt = pCtx->media[i].size + pCtx->media[i].globe_offset - addr;
				bytes -= toOpt;
				// write the toOpt
				// memcpy(p, (void*)(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), toOpt);
				if(pCtx->media[i].typeAndInf)
				{
					// spi flash
					sflash_read(SPIFLASH_CTX(pCtx->media[i].typeAndInf),
						(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, toOpt);
				}
				else
				{
					// internal flash
					flash_read((pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, toOpt);
				}
				p += toOpt;
				cnt += toOpt;
				i ++; // move to next block
				addr = pCtx->media[i].globe_offset;
			}
			if(bytes)
			{
				//memcpy(p, (void*)(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), bytes);
				if(pCtx->media[i].typeAndInf)
				{
					// spi flash
					sflash_read(SPIFLASH_CTX(pCtx->media[i].typeAndInf),
						(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, bytes);
				}
				else
				{
					// internal flash
					flash_read((pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, bytes);
				}
				p += bytes;
				cnt += bytes;
			}
			return cnt;
		}
	}
	return cnt;
}

int _ffx_disk_write(FFXCTX *pCtx, const unsigned char* p, unsigned long addr, unsigned long bytes)
{
	int i;
	unsigned long toOpt;
	unsigned long cnt = 0;
	for(i=0; i<DISK_MEDIA_BLOCKS; i++)
	{
		if(addr < (pCtx->media[i].globe_offset + pCtx->media[i].size))
		{
			if((addr + bytes) > (pCtx->media[i].globe_offset + pCtx->media[i].size))
			{
				//toOpt = (addr + bytes) - (pCtx->media[i].globe_offset + pCtx->media[i].size);
				toOpt = pCtx->media[i].size + pCtx->media[i].globe_offset - addr;
				bytes -= toOpt;
				// write the toOpt
				//memcpy((void*)(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), p, toOpt);
				if(pCtx->media[i].typeAndInf)
				{
					// spi flash
					sflash_write(SPIFLASH_CTX(pCtx->media[i].typeAndInf),
						(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, toOpt);
				}
				else
				{
					// internal flash
					flash_write((pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, toOpt);
				}
				p += toOpt;
				cnt += toOpt;
				i ++; // move to next block
				addr = pCtx->media[i].globe_offset;
			}
			if(bytes)
			{
				//memcpy((void*)(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), p, bytes);
				if(pCtx->media[i].typeAndInf)
				{
					// spi flash
					sflash_write(SPIFLASH_CTX(pCtx->media[i].typeAndInf),
						(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, bytes);
				}
				else
				{
					// internal flash
					flash_write((pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), (unsigned long*)p, bytes);
				}
				p += bytes;
				cnt += bytes;
			}
			return cnt;
		}
	}
	return cnt;
}

int _ffx_disk_erase(FFXCTX *pCtx, unsigned long addr, unsigned long bytes)
{
	int i;
	unsigned long toOpt;
	if(addr == ~0 || bytes == ~0)
	{
		addr = 0;
		bytes = pCtx->disk.disk_size;
	}
	for(i=0; i<DISK_MEDIA_BLOCKS;)
	{
		if(addr < (pCtx->media[i].globe_offset + pCtx->media[i].size))
		{
			if((addr + bytes) > (pCtx->media[i].globe_offset + pCtx->media[i].size))
			{
				//toOpt = (addr + bytes) - (pCtx->media[i].globe_offset + pCtx->media[i].size);
				toOpt = pCtx->media[i].size + pCtx->media[i].globe_offset - addr;
				bytes -= toOpt;
				// erase the toOpt
				//memset((void*)(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), 0xFF, toOpt);
				if(pCtx->media[i].typeAndInf)
				{
					// spi flash
					sflash_erase(SPIFLASH_CTX(pCtx->media[i].typeAndInf),
						(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), toOpt);
				}
				else
				{
					// internal flash
					flash_erase((pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), toOpt);
				}
				//i ++; // move to next block
				//addr += pCtx->media[i].globe_offset;
				addr += toOpt;
			}
			else if(bytes)
			{
				//memset((void*)(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), 0xFF, bytes);
				if(pCtx->media[i].typeAndInf)
				{
					// spi flash
					sflash_erase(SPIFLASH_CTX(pCtx->media[i].typeAndInf),
						(pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), bytes);
				}
				else
				{
					// internal flash
					flash_erase((pCtx->media[i].offset + addr - pCtx->media[i].globe_offset), bytes);
				}
				bytes = 0;
			}
			else
			{
				return 0;
			}
		}
		else if(bytes)
		{
			i++;
		}
		else
		{
			return 0;
		}
	}
	return -1;
}
