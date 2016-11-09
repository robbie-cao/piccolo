/*
kevin.dang
converted from lcd4linux
*/
#define ________ 0x00
#define _______O 0x01
#define ______O_ 0x02
#define ______OO 0x03
#define _____O__ 0x04
#define _____O_O 0x05
#define _____OO_ 0x06
#define _____OOO 0x07
#define ____O___ 0x08
#define ____O__O 0x09
#define ____O_O_ 0x0a
#define ____O_OO 0x0b
#define ____OO__ 0x0c
#define ____OO_O 0x0d
#define ____OOO_ 0x0e
#define ____OOOO 0x0f
#define ___O____ 0x10
#define ___O___O 0x11
#define ___O__O_ 0x12
#define ___O__OO 0x13
#define ___O_O__ 0x14
#define ___O_O_O 0x15
#define ___O_OO_ 0x16
#define ___O_OOO 0x17
#define ___OO___ 0x18
#define ___OO__O 0x19
#define ___OO_O_ 0x1a
#define ___OO_OO 0x1b
#define ___OOO__ 0x1c
#define ___OOO_O 0x1d
#define ___OOOO_ 0x1e
#define ___OOOOO 0x1f
#define __O_____ 0x20
#define __O____O 0x21
#define __O___O_ 0x22
#define __O___OO 0x23
#define __O__O__ 0x24
#define __O__O_O 0x25
#define __O__OO_ 0x26
#define __O__OOO 0x27
#define __O_O___ 0x28
#define __O_O__O 0x29
#define __O_O_O_ 0x2a
#define __O_O_OO 0x2b
#define __O_OO__ 0x2c
#define __O_OO_O 0x2d
#define __O_OOO_ 0x2e
#define __O_OOOO 0x2f
#define __OO____ 0x30
#define __OO___O 0x31
#define __OO__O_ 0x32
#define __OO__OO 0x33
#define __OO_O__ 0x34
#define __OO_O_O 0x35
#define __OO_OO_ 0x36
#define __OO_OOO 0x37
#define __OOO___ 0x38
#define __OOO__O 0x39
#define __OOO_O_ 0x3a
#define __OOO_OO 0x3b
#define __OOOO__ 0x3c
#define __OOOO_O 0x3d
#define __OOOOO_ 0x3e
#define __OOOOOO 0x3f
#define _O______ 0x40
#define _O_____O 0x41
#define _O____O_ 0x42
#define _O____OO 0x43
#define _O___O__ 0x44
#define _O___O_O 0x45
#define _O___OO_ 0x46
#define _O___OOO 0x47
#define _O__O___ 0x48
#define _O__O__O 0x49
#define _O__O_O_ 0x4a
#define _O__O_OO 0x4b
#define _O__OO__ 0x4c
#define _O__OO_O 0x4d
#define _O__OOO_ 0x4e
#define _O__OOOO 0x4f
#define _O_O____ 0x50
#define _O_O___O 0x51
#define _O_O__O_ 0x52
#define _O_O__OO 0x53
#define _O_O_O__ 0x54
#define _O_O_O_O 0x55
#define _O_O_OO_ 0x56
#define _O_O_OOO 0x57
#define _O_OO___ 0x58
#define _O_OO__O 0x59
#define _O_OO_O_ 0x5a
#define _O_OO_OO 0x5b
#define _O_OOO__ 0x5c
#define _O_OOO_O 0x5d
#define _O_OOOO_ 0x5e
#define _O_OOOOO 0x5f
#define _OO_____ 0x60
#define _OO____O 0x61
#define _OO___O_ 0x62
#define _OO___OO 0x63
#define _OO__O__ 0x64
#define _OO__O_O 0x65
#define _OO__OO_ 0x66
#define _OO__OOO 0x67
#define _OO_O___ 0x68
#define _OO_O__O 0x69
#define _OO_O_O_ 0x6a
#define _OO_O_OO 0x6b
#define _OO_OO__ 0x6c
#define _OO_OO_O 0x6d
#define _OO_OOO_ 0x6e
#define _OO_OOOO 0x6f
#define _OOO____ 0x70
#define _OOO___O 0x71
#define _OOO__O_ 0x72
#define _OOO__OO 0x73
#define _OOO_O__ 0x74
#define _OOO_O_O 0x75
#define _OOO_OO_ 0x76
#define _OOO_OOO 0x77
#define _OOOO___ 0x78
#define _OOOO__O 0x79
#define _OOOO_O_ 0x7a
#define _OOOO_OO 0x7b
#define _OOOOO__ 0x7c
#define _OOOOO_O 0x7d
#define _OOOOOO_ 0x7e
#define _OOOOOOO 0x7f

#define O_O__O__ 0xa4
#define OOOOOO__ 0xfc

#define LOWER_CASE

#ifdef LOWER_CASE
const unsigned char Font_6x8[96][5] = {
#else
const unsigned char Font_6x8[70][5] = {
#endif
	{________,
	 ________,
	 ________,
	 ________,
	 ________,
	 }, {
	 ________,
	 ________,
	 _O__OOOO,
	 ________,
	 ________,
	 },	{
	 ________,
	 _____OOO,
	 ________,
	 _____OOO,
	 ________,
	 }, {
	 ___O_O__,
	 _OOOOOOO,
	 ___O_O__,
	 _OOOOOOO,
	 ___O_O__,
	 }, {
	 __O__O__,
	 __O_O_O_,
	 _OOOOOOO,
	 __O_O_O_,
	 ___O__O_,
	 }, {
	 __O___OO,
	 ___O__OO,
	 ____O___,
	 _OO__O__,
	 _OO___O_,
	 }, {
	 __OO_OO_,
	 _O__O__O,
	 _O_O_O_O,
	 __O___O_,
	 _O_O____,
	 }, {
	 ________,
	 _____O_O,
	 ______OO,
	 ________,
	 ________,
	 }, {
	 ________,
	 ___OOO__,
	 __O___O_,
	 _O_____O,
	 ________,
	 }, {
	 ________,
	 _O_____O,
	 __O___O_,
	 ___OOO__,
	 ________,
	 }, {
	 ___O_O__,
	 ____O___,
	 __OOOOO_,
	 ____O___,
	 ___O_O__,
	 }, {
	 ____O___,
	 ____O___,
	 __OOOOO_,
	 ____O___,
	 ____O___,
	 }, {
	 ________,
	 _O_O____,
	 __OO____,
	 ________,
	 ________,
	 }, {
	 ____O___,
	 ____O___,
	 ____O___,
	 ____O___,
	 ____O___,
	 }, {
	 ________,
	 _OO_____,
	 _OO_____,
	 ________,
	 ________,
	 }, {
	 __O_____,
	 ___O____,
	 ____O___,
	 _____O__,
	 ______O_,
	 }, {
	 __OOOOO_,
	 _O_O___O,
	 _O__O__O,
	 _O___O_O,
	 __OOOOO_,
	 }, {
	 ________,
	 _O____O_,
	 _OOOOOOO,
	 _O______,
	 ________,
	 }, {
	 _O____O_,
	 _OO____O,
	 _O_O___O,
	 _O__O__O,
	 _O___OO_,
	 }, {
	 __O____O,
	 _O_____O,
	 _O___O_O,
	 _O__O_OO,
	 __OO___O,
	 }, {
	 ___OO___,
	 ___O_O__,
	 ___O__O_,
	 _OOOOOOO,
	 ___O____,
	 }, {
	 __O_OOOO,
	 _O__O__O,
	 _O__O__O,
	 _O__O__O,
	 __OO___O,
	 }, {
	 __OOOO__,
	 _O__O_O_,
	 _O__O__O,
	 _O__O__O,
	 __OO____,
	 }, {
	 _______O,
	 _OOO___O,
	 ____O__O,
	 _____O_O,
	 ______OO,
	 }, {
	 __OO_OO_,
	 _O__O__O,
	 _O__O__O,
	 _O__O__O,
	 __OO_OO_,
	 }, {
	 _____OO_,
	 _O__O__O,
	 _O__O__O,
	 __O_O__O,
	 ___OOOO_,
	 }, {
	 ________,
	 __OO_OO_,
	 __OO_OO_,
	 ________,
	 ________,
	 }, {
	 ________,
	 _O_O_OO_,
	 __OO_OO_,
	 ________,
	 ________,
	 }, {
	 ____O___,
	 ___O_O__,
	 __O___O_,
	 _O_____O,
	 ________,
	 }, {
	 ___O_O__,
	 ___O_O__,
	 ___O_O__,
	 ___O_O__,
	 ___O_O__,
	 }, {
	 _O_____O,
	 __O___O_,
	 ___O_O__,
	 ____O___,
	 ________,
	 }, {
	 ______O_,
	 _______O,
	 _O_O___O,
	 ____O__O,
	 _____OO_,
	 }, {
	 __OO__O_,
	 _O__O__O,
	 _OOOO__O,
	 _O_____O,
	 __OOOOO_,
	 }, {
	 _OOOOOO_,
	 ___O___O,
	 ___O___O,
	 ___O___O,
	 _OOOOOO_,
	 }, {
	 _OOOOOOO,
	 _O__O__O,
	 _O__O__O,
	 _O__O__O,
	 __OO_OO_,
	 }, {
	 __OOOOO_,
	 _O_____O,
	 _O_____O,
	 _O_____O,
	 __O___O_,
	 }, {
	 _OOOOOOO,
	 _O_____O,
	 _O_____O,
	 __O___O_,
	 ___OOO__,
	 }, {
	 _OOOOOOO,
	 _O__O__O,
	 _O__O__O,
	 _O__O__O,
	 _O_____O,
	 }, {
	 _OOOOOOO,
	 ____O__O,
	 ____O__O,
	 ____O__O,
	 _______O,
	 }, {
	 __OOOOO_,
	 _O_____O,
	 _O__O__O,
	 _O__O__O,
	 _OOOO_O_,
	 }, {
	 _OOOOOOO,
	 ____O___,
	 ____O___,
	 ____O___,
	 _OOOOOOO,
	 }, {
	 ________,
	 _O_____O,
	 _OOOOOOO,
	 _O_____O,
	 ________,
	 }, {
	 __O_____,
	 _O______,
	 _O_____O,
	 __OOOOOO,
	 _______O,
	 }, {
	 _OOOOOOO,
	 ____O___,
	 ___O_O__,
	 __O___O_,
	 _O_____O,
	 }, {
	 _OOOOOOO,
	 _O______,
	 _O______,
	 _O______,
	 _O______,
	 }, {
	 _OOOOOOO,
	 ______O_,
	 ____OO__,
	 ______O_,
	 _OOOOOOO,
	 }, {
	 _OOOOOOO,
	 _____O__,
	 ____O___,
	 ___O____,
	 _OOOOOOO,
	 }, {
	 __OOOOO_,
	 _O_____O,
	 _O_____O,
	 _O_____O,
	 __OOOOO_,
	 }, {
	 _OOOOOOO,
	 ____O__O,
	 ____O__O,
	 ____O__O,
	 _____OO_,
	 }, {
	 __OOOOO_,
	 _O_____O,
	 _O_O___O,
	 __O____O,
	 _O_OOOO_,
	 }, {
	 _OOOOOOO,
	 ____O__O,
	 ___OO__O,
	 __O_O__O,
	 _O___OO_,
	 }, {
	 _O___OO_,
	 _O__O__O,
	 _O__O__O,
	 _O__O__O,
	 __OO___O,
	 }, {
	 _______O,
	 _______O,
	 _OOOOOOO,
	 _______O,
	 _______O,
	 }, {
	 __OOOOOO,
	 _O______,
	 _O______,
	 _O______,
	 __OOOOOO,
	 }, {
	 ___OOOOO,
	 __O_____,
	 _O______,
	 __O_____,
	 ___OOOOO,
	 }, {
	 __OOOOOO,
	 _O______,
	 __OOO___,
	 _O______,
	 __OOOOOO,
	 }, {
	 _OO___OO,
	 ___O_O__,
	 ____O___,
	 ___O_O__,
	 _OO___OO,
	 }, {
	 _____OOO,
	 ____O___,
	 _OOO____,
	 ____O___,
	 _____OOO,
	 }, {
	 _OO____O,
	 _O_O___O,
	 _O__O__O,
	 _O___O_O,
	 _O____OO,
	 }, {
	 ________,
	 _OOOOOOO,
	 _O_____O,
	 _O_____O,
	 ________,
	 }, {
	 ___O_O_O,
	 ___O_OO_,
	 _OOOOO__,
	 ___O_OO_,
	 ___O_O_O,
	 }, {
	 ________,
	 _O_____O,
	 _O_____O,
	 _OOOOOOO,
	 ________,
	 }, {
	 _____O__,
	 ______O_,
	 _______O,
	 ______O_,
	 _____O__,
	 }, {
	 _O______,
	 _O______,
	 _O______,
	 _O______,
	 _O______,
	 }, {
	 ________,
	 _______O,
	 ______O_,
	 _____O__,
	 ________,
	 },
#ifdef LOWER_CASE
	{__O_____,
	 _O_O_O__,
	 _O_O_O__,
	 _O_O_O__,
	 _OOOO___,
	 }, {
	 _OOOOOOO,
	 _O__O___,
	 _O___O__,
	 _O___O__,
	 __OOO___,
	 }, {
	 __OOO___,
	 _O___O__,
	 _O___O__,
	 _O___O__,
	 __O_____,
	 }, {
	 __OOO___,
	 _O___O__,
	 _O___O__,
	 _O__O___,
	 _OOOOOOO,
	 }, {
	 __OOO___,
	 _O_O_O__,
	 _O_O_O__,
	 _O_O_O__,
	 ___OO___,
	 }, {
	 ____O___,
	 _OOOOOO_,
	 ____O__O,
	 _______O,
	 ______O_,
	 }, {
	 ___OO___,
	 O_O__O__,
	 O_O__O__,
	 O_O__O__,
	 _OOOOO__,
	 }, {
	 _OOOOOOO,
	 ____O___,
	 _____O__,
	 _____O__,
	 _OOOO___,
	 }, {
	 ________,
	 _O___O__,
	 _OOOOO_O,
	 _O______,
	 ________,
	 }, {
	 __O_____,
	 _O______,
	 _O___O__,
	 __OOOO_O,
	 ________,
	 }, {
	 ________,
	 _OOOOOOO,
	 ___O____,
	 __O_O___,
	 _O___O__,
	 }, {
	 ________,
	 _O_____O,
	 _OOOOOOO,
	 _O______,
	 ________,
	 }, {
	 _OOOOO__,
	 _____O__,
	 ___OO___,
	 _____O__,
	 _OOOO___,
	 }, {
	 _OOOOO__,
	 _____O__,
	 _____O__,
	 _____O__,
	 _OOOO___,
	 }, {
	 __OOO___,
	 _O___O__,
	 _O___O__,
	 _O___O__,
	 __OOO___,
	 }, {
	 OOOOOO__,
	 __O__O__,
	 __O__O__,
	 __O__O__,
	 ___OO___,
	 }, {
	 ___OO___,
	 __O__O__,
	 __O__O__,
	 __O_O___,
	 OOOOOO__,
	 }, {
	 _OOOOO__,
	 ____O___,
	 _____O__,
	 _____O__,
	 ____O___,
	 }, {
	 _O__O___,
	 _O_O_O__,
	 _O_O_O__,
	 _O_O_O__,
	 __O_____,
	 }, {
	 _____O__,
	 __OOOOOO,
	 _O___O__,
	 _O______,
	 __O_____,
	 }, {
	 __OOOO__,
	 _O______,
	 _O______,
	 __O_____,
	 _OOOOO__,
	 }, {
	 ___OOO__,
	 __O_____,
	 _O______,
	 __O_____,
	 ___OOO__,
	 }, {
	 __OOOO__,
	 _O______,
	 __O_____,
	 _O______,
	 __OOOO__,
	 }, {
	 _O___O__,
	 __O_O___,
	 ___O____,
	 __O_O___,
	 _O___O__,
	 }, {
	 ____OO__,
	 _O_O____,
	 _O_O____,
	 _O_O____,
	 __OOOO__,
	 }, {
	 _O___O__,
	 _OO__O__,
	 _O_O_O__,
	 _O__OO__,
	 _O___O__,
	 },
#endif
	{________,
	 ____O___,
	 __OO_OO_,
	 _O_____O,
	 ________,
	 }, {
	 ________,
	 ________,
	 _OOOOOOO,
	 ________,
	 ________,
	 }, {
	 ________,
	 _O_____O,
	 __OO_OO_,
	 ____O___,
	 ________,
	 }, {
	 ____O___,
	 ____O___,
	 __O_O_O_,
	 ___OOO__,
	 ____O___,
	 }, {
	 ____O___,
	 ___OOO__,
	 __O_O_O_,
	 ____O___,
	 ____O___,
	 },
};
