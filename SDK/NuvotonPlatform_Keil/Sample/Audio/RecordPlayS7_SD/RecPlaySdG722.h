

void PlaySdG722 (uint16_t AudIndex);

#define	SD_RECORD_DATA_BASE				400			//unit: sector, reserve for multi message recording
#define	SD_RECORD_START_SEC				401			//unit: sector
#define RECORD_SIZE					40*512			//unit: byte

#define S7DATA_BASE_ADDR_ON_SD	0	  	//Should be multiple of 8, it's better on the start of a sector (multiple of 512)
#define SD_SECTOR_SIZE	512	//unit: byte

#define AUDIOBUFFERSIZE 320
#define DPWMSAMPLERATE  16000
#define S7BITRATE       16000
#define S7BANDWIDTH     7000
#define COMPBUFSIZE     40   //According to S7BITRATE, unit: byte


//=========================================================
// The below is based on AudioData.s



//----------------------------------------------------
// Use VPE to generate the *.vp file for each wav file
// VP file has first 4 bytes for header, program needs to skip them
// Use AudioData.s to put each sound stream into project

void PlayClose(void);
void PlayLoop(void);
void PlaySdRecordG722(uint32_t u32SectorOffset);
void RecordClose(void);
void RecordLoop(void);
void Record2SD(uint32_t u32SectorOffset);
void RAM2RAM(uint32_t *Addr1,uint32_t *Addr2,uint32_t WordCount);

