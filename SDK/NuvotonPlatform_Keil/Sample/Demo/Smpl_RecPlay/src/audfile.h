#ifndef AUDFILEHDR
#define AUDFILEHDR

#define SYN_CHANNELS 2

#define G722_PCM_SIZE	640
#define G722_DAT_SIZE	40

typedef struct tagAudStreamInf
{
	unsigned short fid;
	unsigned short ftype; // when ftype > 0xFF, it is not FFX file
	unsigned long dat_length;
	unsigned long dat_pos;
	unsigned char *vpdat; // when for FFX file, this is the disk context
	unsigned short sr; // sample rate of
	unsigned short samples; // samples of one frame
}AudStreamInf;

#define MEMFLIP_BUF_SIZE G722_PCM_SIZE
typedef struct tagMemFlip
{
	unsigned char buf[2][MEMFLIP_BUF_SIZE];
	unsigned long cur_idx;
	unsigned long cur_pos;
	unsigned long prev_idx; // for play
}MemFlip;

extern void memFlipReset(void);

extern volatile unsigned long g_play_rec_status;

extern AudStreamInf gAudStreamInf[SYN_CHANNELS];

extern int CheckVPBlock(unsigned long addr);

extern int SetVPForPlay(unsigned long addr, int idx, int stream);
extern int SetFileForPlay(FFXCTX *disk, unsigned char id, unsigned char type, int stream);
extern int PlayStart(void);
extern int PlayPull(void);
extern int PlayStop(void);

extern int RecFile(FFXCTX *disk, unsigned char id, unsigned char type, unsigned long inf);
extern int RecFile_pull(FFXCTX *disk);
extern void RecFile_stop(FFXCTX *disk, unsigned char id, unsigned char type);

#endif // AUDFILEHDR
