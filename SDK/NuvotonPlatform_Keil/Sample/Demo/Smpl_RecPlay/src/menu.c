
//#include "lcd_nt7565.h"
#include "lcd_uc1601.h"

typedef struct tagMenuItem
{
	unsigned char x;
	unsigned char y;
	const char * str;
}MENUITEM;

typedef struct tagMenu
{
	unsigned char x;
	unsigned char y;
	unsigned char cx;
	unsigned char cy;
	MENUITEM items[8];
}MENU;

#define CHAR_WIDTH 6
#define LCD_WIDTH 128
MENU mu_0 = 
{
	0, 2, LCD_WIDTH, 7,
	{
		{4, 2, "Alt"},
		{LCD_WIDTH - CHAR_WIDTH*10, 2, "Rec To Cur"},
		{4, 4, "Stop"},
		{LCD_WIDTH - CHAR_WIDTH*8, 4, "Play Rec"},
		{26, 6, "Prev"},
		{LCD_WIDTH - 26 - CHAR_WIDTH*4, 6, "Next"},
		{0, 0, 0},
	}
};

MENU mu_1 = 
{
	0, 2, LCD_WIDTH, 7,
	{
		{4, 2, "Alt"},
		{LCD_WIDTH - CHAR_WIDTH*10, 2, "Rec To New"},
		{4, 4, "Stop"},
		{LCD_WIDTH - CHAR_WIDTH*7, 4, "Play VP"},
		{26,6, "Prev"},
		{LCD_WIDTH - 26 - CHAR_WIDTH*4, 6, "Next"},
		{0, 0, 0},
	}
};

void mu_draw(const MENU* mu)
{
	int i = 0;
	lcd_clr(mu->x, mu->y, mu->cx, mu->cy);
	while(mu->items[i].str)
	{
		lcd_puts_at(mu->items[i].x, mu->items[i].y, mu->items[i].str);
		i ++;
	}
}

void mu_update(int idx)
{
	if(idx)
	{
		mu_draw(&mu_1);
	}
	else
	{
		mu_draw(&mu_0);
	}
}
