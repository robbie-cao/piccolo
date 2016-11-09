//
// qingkun.dang,
// 

#ifndef LIBBTNHDR
#define LIBBTNHDR

// default value 2 with a 10ms tick value
extern const unsigned long libButtonsDebounceTicks;

typedef struct tagBtnCtx
{
	unsigned long btn_lastScan;	// last scan time
	unsigned long btn_lastVal;	// last value
	unsigned long btn_transVal;	// transition value, will be debounced
	unsigned long btn_lastDown;
	unsigned long io_base;
	unsigned long io_mask;
}BTNCTX;

extern void btnInit(BTNCTX *ctx, unsigned long base, unsigned long mask);
extern int btnScan(BTNCTX *ctx, unsigned long tick);
extern unsigned long btnGet(BTNCTX *ctx);
extern unsigned long btnGetLastDown(BTNCTX *ctx);

#endif
