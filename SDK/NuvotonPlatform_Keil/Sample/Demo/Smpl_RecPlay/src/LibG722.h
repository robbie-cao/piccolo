#ifndef G722IMPHDR
#define G722IMPHDR

#define G722_ENCODER 1
#define G722_DECODER 1
#define G722_STDALONE 1

#ifdef MAX_SAMPLE_RATE
#undef MAX_SAMPLE_RATE
#endif
#define MAX_SAMPLE_RATE 32000
#define MAX_FRAMESIZE   (MAX_SAMPLE_RATE/50)

#ifndef MAX_DCT_LENGTH
#ifndef DCT_LENGTH
#define DCT_LENGTH          320
#endif // DCT_LENGTH
#ifdef SIREN14
#define MAX_DCT_LENGTH      640
#else
#define MAX_DCT_LENGTH  DCT_LENGTH
#endif
#endif // MAX_DCT_LENGTH

#ifndef NUMBER_OF_REGIONS
#define NUMBER_OF_REGIONS 14
#endif

#ifndef MAX_NUMBER_OF_REGIONS
#ifdef SIREN14
#define MAX_NUMBER_OF_REGIONS   28
#else
#define MAX_NUMBER_OF_REGIONS NUMBER_OF_REGIONS
#endif
#endif

#pragma pack(4)

typedef struct tagG722_CODEC_CTL
{
	signed  long  bit_rate;
	signed short  bandwidth;
	signed short  number_of_bits_per_frame;
	signed short  number_of_regions;
	signed short  frame_size;
}G722_CODEC_CTL;

typedef struct tagG722_ENC_CTX
{
    signed short  history[MAX_FRAMESIZE];
    signed short  mlt_coefs[MAX_FRAMESIZE]; // this is not history data
}G722_ENC_CTX;

#ifndef Rand_Obj_defined
#define Rand_Obj_defined
typedef struct
{
    signed short seed0;
    signed short seed1;
    signed short seed2;
    signed short seed3;
}Rand_Obj;
#endif // Rand_Obj_defined

typedef struct tagG722_DEC_CTX
{
    signed short  old_mag_shift;
#ifndef NO_FRAME_ERROR_CHECK
    signed short  old_decoder_mlt_coefs[MAX_DCT_LENGTH];
#endif
    signed short  old_samples[MAX_DCT_LENGTH>>1];
    Rand_Obj randobj;
    signed short  decoder_mlt_coefs[MAX_DCT_LENGTH]; // this is not history data
}G722_DEC_CTX;

#pragma pack()

extern void g722encReset(const G722_CODEC_CTL *cctl, G722_ENC_CTX *ctx);
extern void g722enc(const G722_CODEC_CTL *cctl, G722_ENC_CTX *ctx,
			 signed short *in, signed short *out);
extern void g722decReset(const G722_CODEC_CTL *cctl, G722_DEC_CTX *ctx);
extern void g722dec(const G722_CODEC_CTL *cctl, G722_DEC_CTX *ctx,
			 signed short *in, signed short *out);

typedef struct tagG722CODEC
{
	//void (*g722Init)(G722_CODEC_CTL *cctl, signed long bit_rate, signed short bandwidth);
#ifdef G722_ENCODER
	void (*g722encReset)(const G722_CODEC_CTL *cctl, G722_ENC_CTX *ctx);
	void (*g722enc)(const G722_CODEC_CTL *cctl, G722_ENC_CTX *ctx, signed short *in, signed short *out);
#endif // G722_ENCODER
#ifdef G722_DECODER
	void (*g722decReset)(const G722_CODEC_CTL *cctl, G722_DEC_CTX *ctx);
	void (*g722dec)(const G722_CODEC_CTL *cctl, G722_DEC_CTX *ctx, signed short *in, signed short *out);
#endif // G722_DECODER
}G722CODEC;

#endif // G722IMPHDR
