
#ifndef LIBMICHDR
#define LIBMICHDR

extern void micInit(int enMicBias, int enZCD);
extern int micStart(unsigned long HCLK, int SR, int iFifoIntLevel, int iFifoIntEna, int enPDMA);
extern void micMute(int en);
extern void micClose(void);

#endif // LIBMICHDR
