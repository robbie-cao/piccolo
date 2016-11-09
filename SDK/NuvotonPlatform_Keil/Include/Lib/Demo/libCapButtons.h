/**
	@file
	@brief capacitor sense button library header

	This library helps user to utilse the ISD91xx cap-sense feature as buttons.
	This library uses cap value change to detect a button press/release. If the value changes +12%/-6%, it will be considered as a press/release.

	<a href="http://www.nuvoton.com">www.nuvoton.com</a>

	@author qkdang@nuvoton.com
*/

#ifndef LIBCAPBUTTONSHDR
#define LIBCAPBUTTONSHDR

/**
	@addtogroup G02_002 libCapButtons
	@{
*/

#define MAX_CAPSENSE_CHNLS 8 ///< max channels. always 8 for ISD9160

/**
	@brief This is the configuration parameter you have to define in your application.

	The libCapBtnParam consists two parts:

	- 1. whether detect a long time press. If (libCapBtnParam & 0x10000) then no long time press will be detected. Use this can let the cap-sense buttons keep working well even there are dirty or water on the surface of the buttons.
	- 2. The debounce time. The debounce time depends on your ticket value. generally a 20ms debounce time used. for a 100Hz (10ms) ticket value, the value is 2.
*/
extern const unsigned long libCapBtnParam;

#ifndef CAPBTNCTX_DEF
#define CAPBTNCTX_DEF
/**
	@brief Internal used structure.
*/
typedef struct tagCapBtnCtx
{
	unsigned long btn_lastScan;	// last scan time
	unsigned long btn_lastVal;	// last value
	unsigned long btn_lastDown;
	unsigned long btn_transVal;
	unsigned long io_mask;
	signed long   capFilLong[MAX_CAPSENSE_CHNLS];
	unsigned long cur_chnl;
}CAPBTNCTX;
#endif // CAPBTNCTX_DEF

/**
	@brief Initialize the module.
	
	This will initialize the context ctx and setup the GPIO/IP.
	@param ctx context of the module.
	@param mask bit mask indicates channels you want to use. for example if you want to use channel 0 and 2: mask = 0x04|0x01;
	@param ISRC internal current source value. Generally for cap-sense buttons, the recommended value is 3.
		@arg @c 0 no internal current source.
		@arg @c 1 0.5uA internal current source.
		@arg @c 2 1uA internal current source.
		@arg @c 3 2.5uA internal current source.
		@arg @c 4 5uA internal current source.
	@param dischargeCycle once the comparetor output high, the pin will pulled low to discharge the electric. This parameter is how long to pull low to discharge.
		@arg @c 0 1 cycle.
		@arg @c 1 2 cycles.
		@arg @c 2 8 cycles.
		@arg @c 3 16 cycles.
	@param countTimes Relaxation cycles to perform. The interrupt will generate after 2^countTimes relaxation cycles. value range 0~7.
*/
extern void cbtnInit(CAPBTNCTX *ctx, unsigned long mask, int ISRC, int dischargeCycle, int countTimes);

/**
	@brief Do a scan. Pull (call) this method periodiclly to do the scan job.
	@param ctx context of the module.
	@param tick current ticket value.
	@retval none-zero change detected
	@retval zero no change detected
*/
extern int cbtnScan(CAPBTNCTX *ctx, unsigned long tick);

/**
	@brief Get current button status.
	@param ctx context of the module.
	@return current button status.
*/
extern unsigned long cbtnGet(CAPBTNCTX *ctx);

/**
	@brief Get last button/buttons pressed
	@param ctx context of the module.
	@return last button/buttons pressed
*/
extern unsigned long cbtnGetLastDown(CAPBTNCTX *ctx);

/**
	@}
*/

#endif
