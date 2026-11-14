// Sir Ababol 2 CPC - extra frames. Hand-written.

// The "hidden" frame: cpcdirty never draws it, only compares its address.
extern unsigned char spr_empty [];
#asm
	._spr_empty
		defs 4, 0
#endasm
