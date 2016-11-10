/*----------------------------------------------------------------------------------------*/
/*                                                                                        */
/* Copyright(c) 2011 Nuvoton Technology Corp. All rights reserved.                        */
/*                                                                                        */
/*----------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/* Include related headers                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "ISD9xx.h"
#include "NVTTypes.h"

#include "Lib\LibSiren7.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Macro, type and constant definitions                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define AUDIOBUFFERSIZE 320
#define DPWMSAMPLERATE  16000		 		// both 16Kbps & 32Kbps take around 13ms for encode then decode
#define S7BITRATE       32000
#define S7BANDWIDTH     7000
#define COMPBUFSIZE     40   //According to S7BITRATE

volatile __align(4) signed short s16Out_words[COMPBUFSIZE];

/*----------------------------------------------------------------------------------------*/
/* Global variables                                                                       */
/*----------------------------------------------------------------------------------------*/
static sSiren7_CODEC_CTL sEnDeCtl;
static sSiren7_ENC_CTX sS7Enc_Ctx;
static sSiren7_DEC_CTX sS7Dec_Ctx;

extern volatile uint32_t BufferReadyAddr;		//in RecordPCM.c
extern BOOL	bMicBufferReady;					//in RecordPCM.c
extern volatile uint32_t AudBufEmptyAddr;		//in PlayPCM.c

/*----------------------------------------------------------------------------------------*/
/* Define functions prototype                                                             */
/*----------------------------------------------------------------------------------------*/

void S7EncDec(void)
{
	


	LibS7Encode(&sEnDeCtl, &sS7Enc_Ctx, (signed short *)BufferReadyAddr, s16Out_words);  
	LibS7Decode(&sEnDeCtl, &sS7Dec_Ctx, s16Out_words, (signed short *)AudBufEmptyAddr);  
	bMicBufferReady=FALSE;

}



void S7Init(void)
{
    LibS7Init(&sEnDeCtl,S7BITRATE,S7BANDWIDTH);
   
    LibS7EnBufReset(sEnDeCtl.frame_size,&sS7Enc_Ctx);
    LibS7DeBufReset(sEnDeCtl.frame_size,&sS7Dec_Ctx);
}


