// Punchy frame numbers, generated once by tools/pn_assets.py

// 16x32 frames (spr_tall.png), facing right
#define F_P_IDLE 0
#define F_P_PREWALK 1
#define F_P_WALK1 2
#define F_P_WALK2 3
#define F_P_WALK3 4
#define F_P_WALK4 5
#define F_P_UP 6
#define F_P_DOWN 7
#define F_P_PUNCH 8
#define F_P_KICK 9
#define F_P_CLIMB1 10
#define F_P_CLIMB2 11
#define F_P_CLIMB3 12
#define F_PU_WALK1 13
#define F_PU_WALK2 14
#define F_PU_HIT 15
#define F_PU_PUNCH 16
#define F_SH_WALK1 17
#define F_SH_WALK2 18
#define F_SH_HIT 19
#define F_SH_SHOOT 20
#define F_LI_WALK1 21
#define F_LI_WALK2 22
#define F_WA_0 23
#define F_WA_1 24
#define F_WA_2 25
#define F_WA_3 26
#define PN_TALL_FRAMES 27

// 8x16 pieces (spr_piece.png): the part of a 24 px wide NES frame right
// of its 16 px body; PD_ = rows from the top of the 32 px frame
#define PC_P_PUNCH 0
#define PD_P_PUNCH 0
#define PC_P_KICK 1
#define PD_P_KICK 5
#define PC_PU_PUNCH 2
#define PD_PU_PUNCH 10
#define PN_PIECE_FRAMES 3

// world x of the body of a frame relative to gpx, facing right
#define DX_P_PUNCH 2
#define DX_P_KICK -2

// CPC inks (RGB, for reference): background [(0, 255, 255), (0, 0, 0), (128, 0, 0), (255, 128, 128)], sprites [(0, 0, 128), (255, 128, 128), (255, 255, 255)]
