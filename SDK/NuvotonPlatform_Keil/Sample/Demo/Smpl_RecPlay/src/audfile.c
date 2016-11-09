

#include "hw.h"

#include "ISD9xx.h"

#include "lib\libSPIFlash.h"
#include "libVPB.h"

#include "libFFX.h"

#include "audfile.h"
#include "Lib\LibSiren7.h"

extern void audOutStart(int SR);
extern void audOutStop(int wait);

extern void audInStart(int SR);
extern void audInStop(int wait);


volatile unsigned long g_play_rec_status;
MemFlip codecFlipBuf;

void memFlipReset(void)
{
	codecFlipBuf.cur_idx = 0;
	codecFlipBuf.cur_pos = 0;
}

//#ifndef G722_STDALONE
//#define G722_STDALONE 1
//#endif // G722_STDALONE
//
//#ifdef G722_STDALONE
////#include "../../Fixed-200505-Rel.2.1/g722/g722.h"
////#include "../../Fixed-200505-Rel.2.1/g722/g722codec.h"
//#include "../../g722romlib/src/g722imp.h"
//
//void g722Init(G722_CODEC_CTL *cctl, signed long bit_rate, signed short bandwidth)
//{
//	// for library global
//	//(*((volatile unsigned long*)(0x20000000 + 0x2000 - 8))) = 0;
//	//(*((volatile unsigned long*)(0x20000000 + 0x2000 - 4))) = 0;
//	cctl->bit_rate = bit_rate;
//	cctl->bandwidth = bandwidth;
//	cctl->number_of_bits_per_frame = cctl->bit_rate/50;
//	if(cctl->bandwidth == 7000)
//	{
//		cctl->number_of_regions = NUMBER_OF_REGIONS;
//		cctl->frame_size = MAX_FRAMESIZE >> 1;
//	}
//	else if(cctl->bandwidth == 14000)
//	{
//		cctl->number_of_regions = MAX_NUMBER_OF_REGIONS;
//		cctl->frame_size = MAX_FRAMESIZE;
//	}
//}

//#define G722FUNC(arg) (((const G722CODEC*)(0x6000))->arg)
#define G722FUNC(arg) (arg)

//#else
//
//#include "..\..\StoreBook\src\Lib\G722.1-Fixed-200505-Rel.2.1\g722.h"
//#include "..\..\StoreBook\src\Lib\G722.1-Fixed-200505-Rel.2.1\g722codec.h"
////#include "../../Fixed-200505-Rel.2.1/g722/g722.h"
////#include "../../Fixed-200505-Rel.2.1/g722/g722codec.h"
//#define G722FUNC(arg) arg
////BASEOPFLAGS aBaseOpFlag;
////BASEOPFLAGS *const gpBaseOpFlags = &aBaseOpFlag;
//#endif // G722_STDALONE

/*
union
{
	G722_ENC_MEM g722enc_mem;
	G722_DEC_MEM g722dec_mem;
}g722_mem;
*/


static unsigned long fileBuf[G722_DAT_SIZE/4];


static sSiren7_CODEC_CTL g722ctl[SYN_CHANNELS];

static union
{
	sSiren7_ENC_CTX g722enc_ctx;
	sSiren7_DEC_CTX g722dec_ctx;
}g722_ctx[SYN_CHANNELS];

#if SYN_CHANNELS > 1
static signed long aud_dec_buf[SYN_CHANNELS][MEMFLIP_BUF_SIZE/4];
#endif

FFXFILE gFFXFiles[SYN_CHANNELS];
AudStreamInf gAudStreams[SYN_CHANNELS];

int SetVPForPlay(unsigned long addr, int idx, int streamid)
{
	int SR;
	unsigned long uval;
	
	uval = (*((unsigned long *)(addr + 4)));
	if(idx >= uval)
	{
		return -1;
	}

	gAudStreams[streamid].fid = 0;
	gAudStreams[streamid].ftype = 0x1FF;
	gAudStreams[streamid].dat_pos = 0;
	gAudStreams[streamid].dat_length = VPBGetItemSize(addr, idx);
	gAudStreams[streamid].vpdat = (unsigned char*)VPBGetItemOffset(addr, idx);
	gAudStreams[streamid].vpdat += addr;

	uval = VPBGetItemInfo(addr, idx);
	switch(VP_GET_CMP(uval))
	{
	case COMP_G722_7KHZ:
		LibS7Init(&g722ctl[streamid], VP_GET_SR(uval), 7000);
		SR = 16000;
		break;
	case COMP_G722_14KHZ:
		LibS7Init(&g722ctl[streamid], VP_GET_SR(uval), 14000);
		SR = 32000;
		break;
	default:
		return -2;
	}

	G722FUNC(LibS7DeBufReset)(g722ctl[streamid].frame_size, &g722_ctx[streamid].g722dec_ctx);

	gAudStreams[streamid].fid = idx + 1;
	gAudStreams[streamid].sr = SR;
	gAudStreams[streamid].samples = (g722ctl[streamid].number_of_bits_per_frame >> 3);

	return 0;
}

int SetFileForPlay(FFXCTX *disk, unsigned char id, unsigned char type, int streamid)
{
	unsigned long uval;
	int SR;

	if(ffxFileOpen(disk, id, type, &gFFXFiles[streamid]) < 0) return -1;

	if(gFFXFiles[streamid].fatInf.extBytes > 4096)
	{
		return -2;
	}

	gAudStreams[streamid].ftype = type;
	gAudStreams[streamid].vpdat = (unsigned char*)disk;

	gAudStreams[streamid].dat_length = (gFFXFiles[streamid].fatInf.blocks << 12) + gFFXFiles[streamid].fatInf.extBytes;

	if((gAudStreams[streamid].dat_length) < (4 + (G722_DAT_SIZE)))
	{
		return -3;
	}

	uval = 0;
	ffxFileRead(disk, &gFFXFiles[streamid], (unsigned char*)(&uval), sizeof(uval));

	gAudStreams[streamid].dat_pos = sizeof(uval);

	switch(VP_GET_CMP(uval))
	{
	case COMP_G722_7KHZ:
		LibS7Init(&g722ctl[streamid], VP_GET_SR(uval), 7000);
		SR = 16000;
		break;
	case COMP_G722_14KHZ:
		LibS7Init(&g722ctl[streamid], VP_GET_SR(uval), 14000);
		SR = 32000;
		break;
	default:
		return -4;
	}

	G722FUNC(LibS7DeBufReset)(g722ctl[streamid].frame_size, &g722_ctx[streamid].g722dec_ctx);

	gAudStreams[streamid].fid = id;
	gAudStreams[streamid].sr = SR;
	gAudStreams[streamid].samples = (g722ctl[streamid].number_of_bits_per_frame >> 3);

	return 0;
}

static int decOneStreamFrame(int streamid, signed short *obuf)
{
	if(gAudStreams[streamid].dat_pos < gAudStreams[streamid].dat_length)
	{
		if(gAudStreams[streamid].ftype > 0xFF)
		{
			G722FUNC(LibS7Decode)(&g722ctl[streamid], &g722_ctx[streamid].g722dec_ctx,
				(signed short*)(&gAudStreams[streamid].vpdat[gAudStreams[streamid].dat_pos]),
				obuf);
			gAudStreams[streamid].dat_pos += (g722ctl[streamid].number_of_bits_per_frame >> 3);
		}
		else
		{
			if(G722_DAT_SIZE == ffxFileRead((FFXCTX *)gAudStreams[streamid].vpdat, &gFFXFiles[streamid], (unsigned char*)(fileBuf), (g722ctl[streamid].number_of_bits_per_frame >> 3)))
			{
				G722FUNC(LibS7Decode)(&g722ctl[streamid], &g722_ctx[streamid].g722dec_ctx, (signed short*)(fileBuf),
						obuf);
				gAudStreams[streamid].dat_pos += (g722ctl[streamid].number_of_bits_per_frame >> 3);
			}
			else
			{
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

static int decOneFrame(void)
{
	int idx;
	int iRet = 0;
	for(idx=0; idx<SYN_CHANNELS; idx++)
	{
		if(gAudStreams[idx].fid)
		{
			if(decOneStreamFrame(idx, (signed short*)aud_dec_buf[idx]))
			{
				iRet ++;
			}
			else
			{
				if(gAudStreams[idx].ftype <= 0xFF)
				{
					ffxFileClose((FFXCTX *)gAudStreams[idx].vpdat, gAudStreams[idx].fid, gAudStreams[idx].ftype, &gFFXFiles[idx]);
				}
				gAudStreams[idx].fid = 0;
			}
		}
	}
	return iRet;
}

static int transOneFrame(signed short *obuf)
{
	// resample
	// mixure
	//int idx;
	int i;
	//int chnl = 0;
	signed long v;
	const int samples = MEMFLIP_BUF_SIZE/2;
	if(gAudStreams[0].fid && gAudStreams[1].fid)
	{
		for(i=0; i<samples; i++)
		{
			v = ((signed short*)aud_dec_buf[0])[i];
			v += ((signed short*)aud_dec_buf[1])[i];
			obuf[i] = v >> 1;
		}
		return 2;
	}
	else if(gAudStreams[0].fid)
	{
		for(i=0; i<samples; i++)
		{
			obuf[i] = ((signed short*)aud_dec_buf[0])[i];
		}
		return 1;
	}
	else if(gAudStreams[1].fid)
	{
		for(i=0; i<samples; i++)
		{
			obuf[i] = ((signed short*)aud_dec_buf[1])[i];
		}
		return 1;
	}
	return -1;
#if 0
	signed long v[MEMFLIP_BUF_SIZE/2];
	for(i=0; i<samples; i++)
	{
		v[i] = 0;
	}
	for(idx=0; idx<SYN_CHANNELS; idx++)
	{
		if(gAudStreams[idx].fid)
		{
			chnl ++;
			for(i=0; i<samples; i++)
			{
				v[i] = ((signed short*)aud_dec_buf[idx])[i];
			}
		}
	}
	switch(chnl)
	{
	case 2:
		for(i=0; i<samples; i++)
		{
			obuf[i] = v[i]>>1;
		}
		break;
	case 3:
		for(i=0; i<samples; i++)
		{
			obuf[i] = v[i]/3;
		}
		break;
	case 4:
		for(i=0; i<samples; i++)
		{
			obuf[i] = v[i]>>2;
		}
		break;
	default:
		for(i=0; i<samples; i++)
		{
			obuf[i] = v[i];
		}
	}
	return 1;
#endif
}

int PlayStart(void)
{
	memFlipReset();

	codecFlipBuf.prev_idx = 0;
	
	if(decOneFrame())
	{
		transOneFrame((signed short*)(codecFlipBuf.buf[codecFlipBuf.prev_idx]));
	}
	else
	{
		return -1;
	}
	codecFlipBuf.prev_idx ^= 1;

	if(decOneFrame())
	{
		transOneFrame((signed short*)(codecFlipBuf.buf[codecFlipBuf.prev_idx]));
	}
	else
	{
		int i;
		unsigned long *p = (unsigned long*)(codecFlipBuf.buf[codecFlipBuf.prev_idx]);
		for(i=0; i<MEMFLIP_BUF_SIZE/4; i++)
		{
			(*p++) = 0;
		}
	}
	codecFlipBuf.prev_idx ^= 1;

	audOutStart(16000); // todo

	return 1;			
}

int PlayPull(void)
{
	if(codecFlipBuf.prev_idx != codecFlipBuf.cur_idx)
	{
		if(decOneFrame())
		{
			transOneFrame((signed short*)(codecFlipBuf.buf[codecFlipBuf.prev_idx]));
			codecFlipBuf.prev_idx = codecFlipBuf.cur_idx;
			return 2;
		}
		audOutStop(1);
		return 0;
	}
	return 1;
}

int PlayStop(void)
{
	int idx;
	audOutStop(1);
	for(idx=0; idx<SYN_CHANNELS; idx++)
	{
		if(gAudStreams[idx].fid && (gAudStreams[idx].ftype <= 0xFF))
		{
			ffxFileClose((FFXCTX *)gAudStreams[idx].vpdat, gAudStreams[idx].fid, gAudStreams[idx].ftype, &gFFXFiles[idx]);
		}
		gAudStreams[idx].fid = 0;
	}
	return 0;
}

int RecFile(FFXCTX *disk, unsigned char id, unsigned char type, unsigned long inf)
{
	int SR;
	switch(VP_GET_CMP(inf))
	{
	case COMP_G722_7KHZ:
		LibS7Init(&g722ctl[0], VP_GET_SR(inf), 7000);
		SR = 16000;
		break;
	case COMP_G722_14KHZ:
		LibS7Init(&g722ctl[0], VP_GET_SR(inf), 14000);
		SR = 32000;
		break;
	default:
		return -4;
	}

	if(ffxFileOpen(disk, id, type, &gFFXFiles[0]) < 0) return -1;

	if(sizeof(inf) != ffxFileWrite(disk, &gFFXFiles[0], (const unsigned char*)(&inf), sizeof(inf)))
	{
		return -5;
	}

	G722FUNC(LibS7EnBufReset)(g722ctl[0].frame_size, &g722_ctx[0].g722enc_ctx);

	memFlipReset();
	codecFlipBuf.prev_idx = 0;

	audInStart(SR);

	return 1;
}

int RecFile_pull(FFXCTX *disk)
{
	if(codecFlipBuf.prev_idx != codecFlipBuf.cur_idx)
	{
		G722FUNC(LibS7Encode)(&g722ctl[0], &g722_ctx[0].g722enc_ctx,
			(signed short*)(codecFlipBuf.buf[codecFlipBuf.prev_idx]), (signed short*)(fileBuf));
		codecFlipBuf.prev_idx = codecFlipBuf.cur_idx;
		if(G722_DAT_SIZE != ffxFileWrite(disk, &gFFXFiles[0], (const unsigned char*)fileBuf, G722_DAT_SIZE))
		{
			audInStop(1);
			return 0;
		}
	}
	return 1;
}

void RecFile_stop(FFXCTX *disk, unsigned char id, unsigned char type)
{
	audInStop(1);
	ffxFileClose(disk, id, type, &gFFXFiles[0]);
}
