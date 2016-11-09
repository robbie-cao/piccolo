
/**
	@file ffx.h
	@brief ffx file syste header; qkdang, Feb, 2011
	@details
	ffx means: Flash filesystem X. <br>
	disk size: 12K to 256M bytes in 4K blocks <br>
	files: file is indexed by a number from 1 to 254, not a string name. file extension is also a number from 0 to 127.
	but max file is limited to 65535 and also the FAT size.
	directories: not supported. <br>
	operations: format, file create/open/close/read/write/delete/seek. <br>
	open mode (equivelance): read only, write only of empty file, append (seek to end then write). write in a middle of a file is not supported now. <br> 
	seek is not implemented now.
*/

#ifndef FFX_HDR
#define FFX_HDR

#define DISK_FAT_SIZE 4096
#define DISK_BUF_SIZE 128
#define MAX_FAT_READ_OF_BUF (DISK_FAT_SIZE/DISK_BUF_SIZE)

#ifndef _CVI_
#ifndef _MSC_VER
#ifndef _WIN32
#define DWORD_WRITE_ALIGN	1
#endif
#endif
#endif

#define TIDY_AFTER_DELETE

#define DISK_MEDIA_BLOCKS 2

#pragma pack(1) // storage structures pack 1

typedef struct tagFFXMEDIACTX
{
	unsigned long offset;
	unsigned long globe_offset;
	unsigned long size;
	unsigned long typeAndInf;
}FFXMEDIACTX;

typedef struct tagFFXDISKCTX
{
	unsigned short disk_fat_base;
	unsigned short disk_fat_f0_idx;
	unsigned long  disk_size;
	unsigned char  disk_buf[DISK_BUF_SIZE];
}FFXDISKCTX;

typedef struct tagFFXCTX
{
	FFXDISKCTX  disk; // for ffx
	FFXMEDIACTX media[DISK_MEDIA_BLOCKS]; // for disk io
}FFXCTX;

typedef struct tagFFXFAT
{
	unsigned char  id;
	unsigned char  type;
	unsigned short block; // start block
	unsigned short blocks;
	unsigned short extBytes;
}FFXFAT;
#define SIZE_OF_FFXFAT 8

// for 4 bytes align operation
typedef struct tagFFXFAT_A
{
	unsigned char  id;
	unsigned char  type;
	unsigned short block; // start block
}FFXFATA;
typedef struct tagFFXFAT_B
{
	unsigned short blocks;
	unsigned short extBytes;
}FFXFATB;

#define ADDR_TO_FAT_ENTRY(addr) ((addr) >> 3)
#define FAT_ENTRY_TO_ADDR(idx) (pCtx->disk.disk_fat_base + ((idx) << 3))
#define COUNT_OF_FAT_ENTRY(len) ((len) >> 3)

typedef struct tagFFXFILE
{
	FFXFAT fatInf;
	unsigned short cur_block;
	unsigned short cur_pos_in_block;
	unsigned long  cur_pos;
}FFXFILE;

#pragma pack() // restore default pack

#ifdef  __cplusplus
extern "C" {
#endif

/**
	@brief format the disk
*/
int ffxFormat(FFXCTX *pCtx);


/**
	@brief open a formated disk
*/
int ffxOpenDisk(FFXCTX *pCtx);

/**
	@brief create new file entry
*/
int ffxFatAddEntry(FFXCTX *pCtx, unsigned char num, unsigned char ext);

/**
	@brief tidy the fat table
*/
int ffxFatTidy(FFXCTX *pCtx, unsigned short blkLast);

int ffxGetFreeEntryId(FFXCTX *pCtx);

/**
	@brief open a file
	@param offset 0: specified file; 1: next file relative to specified -1: prev file. other: first file
	@return num + ext. -1 means error
*/
int ffxFileOpenEx(FFXCTX *pCtx, unsigned char num, unsigned char ext, FFXFILE *pFile, int offset);

/**
	@brief Open the file by number num.
	@param diskid the disk id
	@param num the file number, from 1 to 254
	@param pFile the FFXFILE object pointer for further operation
	@return The index of the file in FAT. -1 means error
*/
int ffxFileOpen(FFXCTX *pCtx, unsigned char num, unsigned char ext, FFXFILE *pFile);


/**
	@brief overwrite the file id/type
	@param diskid the disk id
*/
int ffxFileNameOverwrite(FFXCTX *pCtx, unsigned char num, unsigned char ext, unsigned char toNum, unsigned char toExt);

/**
	@brief Close the file by number num.
	@param diskid the disk id
	@param num the file number, from 1 to 254
	@param pFile the FFXFILE object pointer for further operation
	@return The index of the file in FAT. -1 means error
*/
int ffxFileClose(FFXCTX *pCtx, unsigned char num, unsigned char ext, FFXFILE *pFile);

/**
	@brief Delete the file by number num.
	@param diskid the disk id
	@param num the file number, from 1 to 254
	@return 0 means OK. others means error
*/
int ffxFileDelete(FFXCTX *pCtx, unsigned char num, unsigned char ext);

/**
	@brief Read file
	@param pFile the FFXFILE object pointer
	@return The bytes read.
*/
unsigned long ffxFileRead(FFXCTX *pCtx, FFXFILE *pFile, unsigned char *buf, unsigned long bytesToRead);

/**
	@brief Write file
	@param pFile the FFXFILE object pointer
	@return The bytes written.
*/
unsigned long ffxFileWrite(FFXCTX *pCtx, FFXFILE *pFile, const unsigned char *buf, unsigned long bytesToWrite);

/**
	@brief Seek position of file
	@param pFile the FFXFILE object pointer
	@return result position (current position).
*/
unsigned long ffxFileSeek(FFXCTX *pCtx, FFXFILE *pFile, unsigned long pos);


int _ffx_disk_read(FFXCTX *pCtx, unsigned char* p, unsigned long addr, unsigned long bytes);
int _ffx_disk_write(FFXCTX *pCtx, const unsigned char* p, unsigned long addr, unsigned long bytes);
int _ffx_disk_erase(FFXCTX *pCtx, unsigned long addr, unsigned long size);

#ifdef  __cplusplus
}
#endif

#endif // FFX_HDR
