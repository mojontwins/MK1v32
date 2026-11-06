// Yun CPC - the fixed sprite cells (gfx/cpc/sprites.png, 22 cells). Hand-written.
// cpc/sprites.bin is mkts_om mode=sprites output: 64 bytes per cell (4 bytes x 16
// lines), sprite pens only. Cells 0-7 Yun, 8-9 bubble, 10-11 platform, 12-13
// fanty, 14-15 trapped fanty, 16-19 monococo (bush, coming out, out, trapped),
// 20 coco, 21 explosion. The names are the ZX ones (sprcnv2 labels), for C;
// assembly tables use _spr_fixed + cell * 64.

extern unsigned char spr_fixed [];
#asm
	._spr_fixed
		BINARY "cpc/sprites.bin"
#endasm

#define sprite_1_a	(spr_fixed + 0)
#define sprite_2_a	(spr_fixed + 64)
#define sprite_3_a	(spr_fixed + 128)
#define sprite_4_a	(spr_fixed + 192)
#define sprite_5_a	(spr_fixed + 256)
#define sprite_6_a	(spr_fixed + 320)
#define sprite_7_a	(spr_fixed + 384)
#define sprite_8_a	(spr_fixed + 448)
#define sprite_9_a	(spr_fixed + 512)
#define sprite_10_a	(spr_fixed + 576)
#define sprite_11_a	(spr_fixed + 640)
#define sprite_12_a	(spr_fixed + 704)
#define sprite_13_a	(spr_fixed + 768)
#define sprite_14_a	(spr_fixed + 832)
#define sprite_15_a	(spr_fixed + 896)
#define sprite_16_a	(spr_fixed + 960)
#define sprite_17_a	(spr_fixed + 1024)
#define sprite_18_a	(spr_fixed + 1088)
#define sprite_19_a	(spr_fixed + 1152)
#define sprite_20_a	(spr_fixed + 1216)
#define sprite_21_a	(spr_fixed + 1280)
#define sprite_22_a	(spr_fixed + 1344)
