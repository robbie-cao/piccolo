#ifndef __SIREN_H__
#define __SIREN_H__

/*
 * https://en.wikipedia.org/wiki/Comparison_of_audio_coding_formats
 */

enum {
    AUDIO_FORMAT_AAC = 0,
    AUDIO_FORMAT_AC3,
    AUDIO_FORMAT_AMR,
    AUDIO_FORMAT_AMR_WB,
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_MPEG1,
    AUDIO_FORMAT_MPEG2,
    AUDIO_FORMAT_SIREN7,
    AUDIO_FORMAT_SIREN14,
    AUDIO_FORMAT_SIREN22,

    AUDIO_FORMAT_TOTAL
};

#define SAMPLE_RATE_8K          8000
#define SAMPLE_RATE_12K         12000
#define SAMPLE_RATE_16K         16000
#define SAMPLE_RATE_22_05K      22050
#define SAMPLE_RATE_24K         24000
#define SAMPLE_RATE_32K         32000
#define SAMPLE_RATE_44_1K       44100
#define SAMPLE_RATE_48K         48000

#define SAMPLE_RATE_SIREN7      SAMPLE_RATE_16K
#define SAMPLE_RATE_SIREN14     SAMPLE_RATE_32K
#define SAMPLE_RATE_SIREN22     SAMPLE_RATE_48K


#define BIT_RATE_8K             8000
#define BIT_RATE_16K            16000
#define BIT_RATE_24K            24000
#define BIT_RATE_32K            32000
#define BIT_RATE_40K            40000
#define BIT_RATE_48K            48000
#define BIT_RATE_56K            56000
#define BIT_RATE_64K            64000
#define BIT_RATE_80K            80000
#define BIT_RATE_96K            96000
#define BIT_RATE_112K           112000
#define BIT_RATE_128K           128000
#define BIT_RATE_144K           144000
#define BIT_RATE_160K           160000
#define BIT_RATE_192K           192000
#define BIT_RATE_224K           224000
#define BIT_RATE_256K           256000
#define BIT_RATE_320K           320000
#define BIT_RATE_384K           384000
#define BIT_RATE_640K           640000


#define BITS_PER_SAMPLE_G711    8
#define BITS_PER_SAMPLE_G711_0  8
#define BITS_PER_SAMPLE_G711_1  16
#define BITS_PER_SAMPLE_G718    16
#define BITS_PER_SAMPLE_G719    16
#define BITS_PER_SAMPLE_G721    13
#define BITS_PER_SAMPLE_G722    14
#define BITS_PER_SAMPLE_G722_1  16
#define BITS_PER_SAMPLE_G722_2  14      // AMR-WB
#define BITS_PER_SAMPLE_G723    13
#define BITS_PER_SAMPLE_G726    13
#define BITS_PER_SAMPLE_G727    13
#define BITS_PER_SAMPLE_G728    13
#define BITS_PER_SAMPLE_G729    13
#define BITS_PER_SAMPLE_G729_1  16

#endif /* __SIREN_H__ */

/* vim: set ts=4 sw=4 tw=0 list : */
