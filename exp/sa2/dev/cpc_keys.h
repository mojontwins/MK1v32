// Sir Ababol 2 - keys (CPC_CUSTOM_KEYS). The original's: A, D, W, M jumps,
// N swings the sword. Joystick: fire 1 swings the sword, fire 2 jumps.
// pad bits: LEFT, RIGHT, UP, DOWN, FIRE (BUTTON_A: sword), FIRE2 (BUTTON_B: jump)

extern unsigned char def_keys [0];
#asm
	._def_keys
		defw $4820 		; LEFT     A
		defw $4720 		; RIGHT    D
		defw $4708 		; UP       W
		defw $4710 		; DOWN     S

		defw $4540 		; BUTTON_A N (sword)
		defw $4440 		; BUTTON_B M (jump)

		defw $4801 		; KEY_AUX3 1
		defw $4802 		; KEY_AUX4 2

		defw $4880		; KEY_AUX1 Z
		defw $4804 		; KEY_AUX2 3

		defw $4204		; KEY_ENTER
		defw $4804		; KEY_ESC
#endasm

extern unsigned char def_keys_joy [0];
#asm
	._def_keys_joy
		defw 0x4904, 0x4908, 0x4901, 0x4902, 0x4910, 0x4920
		defw 0x4801, 0x4802, 0x4880, 0x4780, 0x4204, 0x4804
#endasm
