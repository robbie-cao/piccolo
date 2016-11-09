
#ifndef HW_HDR
#define HW_HDR

#define SYSTICKINTRATE 100

#ifndef SPI0_BASE
#define SPI0_BASE (0x40000000 + 0x30000)
#endif // SPI0_BASE

#define SPIclkDivider_LCD  5  //4MHz
#define ENTERLCDMODE 1
#define LEAVELCDMODE 0

extern unsigned long SysTickCount;

extern void hwInit(void);
extern void onSysTick(void);
extern void SpiBusCtrl(int iCtrlMode);

#endif // HW_HDR
