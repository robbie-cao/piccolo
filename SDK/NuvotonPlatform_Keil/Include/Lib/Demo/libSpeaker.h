
#ifndef LIBSPEAKERHDR
#define LIBSPEAKERHDR

extern void speakerInit(void);
extern void speakerClose(void);
extern void speakerStart(unsigned long HCLK, int SR, int enPDMA);
extern void speakerMute(int en);

#endif
