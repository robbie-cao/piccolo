

void PlaySpiG722 (uint16_t AudIndex);
#define	DATA_BASE_ADDR_ON_SPI	0x0

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

