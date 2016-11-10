

void PlaySdG722 (uint16_t AudIndex);

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
// Define Audio sound macro to the index number of VP files

#define BRIGHTER			0		   //Item0: first sound
#define	DIMMER				1		   //Item1: second sound
#define	LIGHTS_ARE_OFF		2		   //Item2: third sound
#define	TRY_AGAIN			3		   //Item3: fourth sound
#define LIGHTS_ARE_ON		4		   //Item4: fifth sound


//----------------------------------------------------
// Use VPE to generate the *.vp file for each wav file
// VP file has first 4 bytes for header, program needs to skip them
// Use AudioData.s to put each sound stream into project

void PlayClose(void);
void PlayLoop(void);

