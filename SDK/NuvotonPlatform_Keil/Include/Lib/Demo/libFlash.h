
// qkdang

#ifndef FLASHHDR
#define FLASHHDR

extern int flash_erase(unsigned long addr, unsigned long len);
extern int flash_write(unsigned long addr, const unsigned long* p, unsigned long bytes);
extern int flash_read(unsigned long addr, unsigned long* p, unsigned long bytes);

#endif // FLASHHDR
