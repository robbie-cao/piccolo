
#ifndef FFXIOFLASHHDR
#define FFXIOFLASHHDR

#ifndef SPI0_BASE
#define SPI0_BASE (0x40000000 + 0x30000)
#endif // SPI0_BASE

extern const SFLASH_CTX g_SPIFLASH;

#define SPIFLASH_CTX(arg) (&g_SPIFLASH)

extern void ffx_io_flash_init(void);

#endif // FFXIOFLASHHDR
