
// qkdang
// for 7534, 7565, ...

#ifndef LCD_NT7565HDR
#define LCD_NT7565HDR

extern int lcd_init(void);

extern void lcd_locate(int x, int y);
extern void lcd_clr(int x, int y, int cx, int cy);
extern void lcd_puthex(unsigned char val);
extern void lcd_puts(const char *val);
extern void lcd_puts_at(int x, int y, const char *val);

extern void lcd_enter(void); // for lcd_dat
extern void lcd_dat(const unsigned char *dat, int cnt, int step); // you have to use lcd_enter/leave
extern void lcd_leave(void); // for lcd_dat

#endif // LCD_NT7565HDR
