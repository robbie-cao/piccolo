
// qkdang

/**
	@file
	@brief capacitor sense button library header

	This library helps user to utilse the ISD91xx cap-sense feature as buttons.
	This library uses cap value change to detect a button press/release. If the value changes +12%/-6%, it will be considered as a press/release.

	<a href="http://www.nuvoton.com">www.nuvoton.com</a>

	@author qkdang@nuvoton.com
*/

#ifndef __libVPB_H__
#define __libVPB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VP_VERSION 1

/*
	The 32 bits of the VP info:
	26~31: 6 bits formats,
	   25: Is stereo
	 0~15: 16 bits SR; or data rates
	16~24: reserved
	if the 32 bits are all 1, that means the file is a VP file: a 32 bits info header + data
*/

// from old VPE 15100
#define COMP_SIMPLE_ADPCM        0
#define COMP_E2_ADPCM            1
#define COMP_VBR_ADPCM           2
#define COMP_MSVBR_ADPCM         3
#define COMP_BLOCK_COMP          4
#define COMP_UNKNOWN             7
#define COMP_8BIT                20
#define COMP_10BIT               21
#define COMP_12BIT               22
#define COMP_16BIT               23
#define COMP_PARAM_ADPCM         26
#define COMP_G711_ULAW           29
#define COMP_DULAW               30

// new defined
#define COMP_G722_7KHZ           31 ///< G722, 7KHz format
#define COMP_G722_14KHZ          32 ///< G722, 14KHz format
#define COMP_VM                  63 ///< This is voice macro, then the SR is instructions of this macro

#define VP_GET_SR(arg) (arg & 0x0FFFF) ///< Extract the SR from the information word
#define VP_GET_CMP(arg) (((unsigned long)arg)>>26) ///< Get the format from the information word
#define VP_GET_CHNL(arg) ((arg & 0x02000000)?2:1) ///< Get channels (max 2 channels) from the information word
#define VP_SET_SR(arg, sr) do{arg &= 0xFFFF0000; arg |= sr;}while(0) ///< Set the SR of the information word
#define VP_SET_CMP(arg, cmp) do{arg &= 0x03FFFFFF; arg |= (cmp << 26);}while(0) ///< set the format of the information word
#define VP_SET_CHNL(arg, ch) do{arg &= 0xFDFFFFFF; if(ch > 1)arg |= 0x02000000;}while(0) ///< set the channel of the information word

#pragma pack(1)

typedef struct tagVPHdr
{
	unsigned long offset; ///< The offset of this voice prompt data.
	unsigned long info; ///< The info data of this voice prompt.
}VPHdr;

#define VPHDR_MUL_SHIFT_TIMES 3 // 2^3 = sizeof(VPHdr)

typedef struct tagVPBlockHdr
{
	unsigned short flag; ///< The flag of voice prompt block. always 'V''P'.
	unsigned short ver; ///< version of the VP block
	unsigned long items; ///< Items of this VP block: how many voice prompts.
}VPBlockHdr;

typedef struct tagVPInf
{
	unsigned long offset; ///< The offset of this voice prompt data.
	unsigned long info; ///< The info data of this voice prompt.
	unsigned long size; ///< The size of the voice prompt.
}VPInf;

#pragma pack()

/**
	@brief Get items of this VP block. the start address of the block is "addr"
*/
int VPBGetItemCount(unsigned long addr);

/**
	@brief Get version of this VP block. the start address of the block is "addr"
*/
int VPBGetVersion(unsigned long addr);

/**
	@brief Get item (a voice prompt) idx's offset. The real address is the offset plus addr.
*/
unsigned long VPBGetItemOffset(unsigned long addr, int idx);

/**
	@brief Get the voice prompt length
*/
unsigned long VPBGetItemSize(unsigned long addr, int idx);

/**
	@brief Get the voice prompt info data
*/
unsigned long VPBGetItemInfo(unsigned long addr, int idx);

int SVPBGetItemCount(const SFLASH_CTX *ctx, unsigned long addr);
int SVPBGetVersion(const SFLASH_CTX *ctx, unsigned long addr);
unsigned long SVPBGetItemOffset(const SFLASH_CTX *ctx, unsigned long addr, int idx);
unsigned long SVPBGetItemSize(const SFLASH_CTX *ctx, unsigned long addr, int idx);
unsigned long SVPBGetItemInfo(const SFLASH_CTX *ctx, unsigned long addr, int idx);
int SVPBGetVPInf(const SFLASH_CTX *ctx, unsigned long addr, int idx, VPInf *pInf);

#ifdef __cplusplus
}
#endif

#endif  /* ndef __libVPB_H__ */
