// Yun v5 tables resampled to the ZX / CPC frame rate. Hand-written now (made once from Yun v5 precalcs.h:
// fbl_incr_x / _y, fanty_incs, pezon_incs, and the times and speeds in its definitions.h).
//   CPC: 16.7 Hz, NES time: 60 Hz -> 16.67 Hz (3.6 NES frames per frame)
//   ZX:  25 Hz at 75% of NES time (1.8 NES frames per frame)
// Tables: the NES displacements added up, sampled every 3.6 / 1.8 NES frames.

#ifdef CPC

	#define BUB_LIFE			56		// NES 200 frames
	#define BUB_BLINK			14		// NES 50
	#define BUB_DX_MAX			17
	#define BUB_DY_MAX			15
	#define BUB_BOB_MAX			9
	unsigned char bub_dx [] = { 9, 8, 8, 8, 7, 6, 6, 6, 4, 4, 4, 3, 3, 2, 1, 1, 1, 0 };
	unsigned char bub_dy [] = { 1, 0, 1, 2, 2, 2, 2, 4, 2, 5, 4, 4, 6, 5, 6, 7 };
	unsigned char bub_bob [] = { 3, 3, 3, 1, 2, 2, 1, 0, 1 };
	#define BUB_COOLDOWN 		2		// frames between bubbles
	#define BUB_FALL 			4		// px/frame a trapped enemy falls

	// fanty (NES fanty_incs { 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1 })
	#define FZ_MAX				5
	unsigned char fz_incs [] = { 0, 1, 2, 2, 3, 3 };

	// monococo: hidden, appearing, out, hiding (frames; NES 150, 50, 50, 50, fires at 25)
	unsigned char mc_times [] = { 42, 14, 14, 14 };
	#define MC_FIRE_AT			7
	#define COCO_V				86		// 1/16 px per frame (NES 24)

	// pezon (zurully) jump: y offset from y1 per frame (NES pezon_incs, 49 frames -> 14),
	// then PZ_WAIT frames at y1 (NES PEZON_WAIT + (0xA << 3) = 130 frames)
	#define PZ_N				14
	signed char pz_y [] = { -22, -35, -50, -58, -66, -69, -69, -66, -60, -50, -35, -23, -7, -3 };
	#define PZ_WAIT				36

	// gyrosaw: 32 px a side (NES 1 px/frame -> 4)
	#define GYRO_V				4

	// cannon: frames per NES second (the shooting interval is in seconds)
	#define CANNON_SECOND		16

	// conveyors (NES CONVEYOR_SPEED 0.5 px/frame), 1/64 px per frame
	#define CONVEYOR_V			115

	// animation: bits of maincounter that flip the frames of bubbles, gyrosaws, trapped enemies
	#define YA_BUB				2
	#define YA_GYRO				1
	#define YA_UD				4

	// dark rooms: (rand () & DARK_RAND) == 1 starts a lightning
	#define DARK_RAND			31

#else

	#define BUB_LIFE			111
	#define BUB_BLINK			28
	#define BUB_DX_MAX			34
	#define BUB_DY_MAX			29
	#define BUB_BOB_MAX			18
	unsigned char bub_dx [] = {
		4, 5, 4, 4, 4, 4, 4, 4, 3, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 1, 1, 1, 0,
		1, 0, 1, 0, 0
	};
	unsigned char bub_dy [] = {
		1, 0, 0, 0, 1, 0, 1, 1, 0, 2, 1, 1, 1, 1, 2, 2, 1, 1, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 3, 3,
		4
	};
	unsigned char bub_bob [] = { 1, 2, 1, 2, 1, 2, 1, 0, 2, 0, 1, 1, 0, 1, 0, 0, 1, 0 };
	#define BUB_COOLDOWN 		4
	#define BUB_FALL 			2

	#define FZ_MAX				10
	unsigned char fz_incs [] = { 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2 };

	unsigned char mc_times [] = { 83, 28, 28, 28 };
	#define MC_FIRE_AT			14
	#define COCO_V				43

	#define PZ_N				27
	signed char pz_y [] = {
		-11, -20, -29, -36, -43, -49, -55, -59, -62, -66, -68, -69, -69, -69, -68, -66, -63, -59,
		-55, -50, -44, -37, -29, -22, -15, -8, -3
	};
	#define PZ_WAIT				72

	#define GYRO_V				2

	#define CANNON_SECOND		33

	#define CONVEYOR_V			58

	#define YA_BUB				4
	#define YA_GYRO				2
	#define YA_UD				8

	#define DARK_RAND			63

#endif
