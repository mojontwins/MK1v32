// Yun v5 tables resampled to the ZX / CPC frame rate. Hand-written now (made once from Yun v5 precalcs.h).
// Yun v5 tables, 60 Hz -> 16.67 Hz (x3.60)

#define BUB_LIFE			56
#define BUB_BLINK			14
#define BUB_DX_MAX			17
#define BUB_DY_MAX			15
#define BUB_BOB_MAX			9
unsigned char bub_dx [] = { 9, 8, 8, 8, 7, 6, 6, 6, 4, 4, 4, 3, 3, 2, 1, 1, 1, 0 };
unsigned char bub_dy [] = { 1, 0, 1, 2, 2, 2, 2, 4, 2, 5, 4, 4, 6, 5, 6, 7 };
unsigned char bub_bob [] = { 3, 3, 3, 1, 2, 2, 1, 0, 1 };

// fanty at the ZX rate (NES fanty_incs { 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1 })
#define FZ_MAX				5
unsigned char fz_incs [] = { 0, 1, 2, 2, 3, 3 };

// monococo: hidden, appearing, out, hiding (frames)
unsigned char mc_times [] = { 42, 14, 14, 14 };
#define MC_FIRE_AT			7
#define COCO_V				86		// 1/16 px per frame
