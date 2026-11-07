// Nin Nin - the enemies (ENABLE_CUSTOM_ENEMS). Hand-written, from the NES
// game (engine/enengine.h and enemmods/).
//
// The type's low nibble is the movement, the high nibble what it shoots:
//   1-3  linear (the engine's), 4 moving platform (the engine's)
//   6    fanty: flies after Nin Nin (3 hits)
//   7    pursuer: walks after Nin Nin along its row
//   9    drop: falls from (x1, y1) to (x2, y2), again and again; can't die
//   0x1X shoots left / right when Nin Nin is in its row (and it faces him)
//   0x2X shoots up / down when Nin Nin is in its column
//   0x3X-0x6X shoots up, right, down, left every so often
//   0x7X turns while it shoots: the 8 directions in turn
// The engine moves types 1-4 and draws them all (frames from their base
// type: 6 and 7 use type 3's, the NES FANTIES_CELL / PURSUERS_CELL); here
// the rest moves, shoots, and dies when it touches Nin Nin, as on the NES.
//
// Speeds: the .ene speeds are NES pixels per frame at 60 Hz; custom_level_setup
// doubles mx, my for 25 Hz.

#define NN_COCO_FREQ 		62		// NES 150 frames
unsigned char nn_en_ct [MAX_ENEMS], nn_en_cd [MAX_ENEMS];
unsigned char nn_base, nn_touched;
signed int nn_fvx [MAX_ENEMS], nn_fvy [MAX_ENEMS];

void extra_enems_init (void) {
	nn_en_ct [enit] = NN_COCO_FREQ;
	nn_en_cd [enit] = 0;
	nn_fvx [enit] = nn_fvy [enit] = 0;
	if (_en_t <= 4) return;
	nn_base = _en_t & 15;
	if (nn_base <= 4) enems_en_an_calc (nn_base - 1);
	else if (nn_base == 6 || nn_base == 7) enems_en_an_calc (2);
	else if (nn_base == 9) en_an_next_frame [enit] = sprite_cells [NN_CELL_DROP];
}

void nn_shoot (void) {
	if (nn_en_ct [enit]) { nn_en_ct [enit] --; return; }
	_x = _en_x + 4; _y = _en_y + 4; _t = 0xff;
	switch (_en_t & 0xf0) {
		case 0x10:
			if (gpy + 8 >= _en_y && gpy <= _en_y + 8) {
				if (_en_mx == 0 || (_en_mx > 0 && gpx > _en_x) || (_en_mx < 0 && gpx < _en_x))
					_t = (gpx < _en_x) ? 6 : 2;
			}
			break;
		case 0x20:
			if (gpx + 8 >= _en_x && gpx <= _en_x + 8) {
				if (_en_my == 0 || (_en_my > 0 && gpy > _en_y) || (_en_my < 0 && gpy < _en_y))
					_t = (gpy < _en_y) ? 0 : 4;
			}
			break;
		case 0x30:
		case 0x40:
		case 0x50:
		case 0x60:
			_t = ((_en_t >> 4) - 3) << 1;
			break;
		case 0x70:
			nn_en_cd [enit] = (nn_en_cd [enit] + 1) & 7;
			_t = nn_en_cd [enit];
			break;
	}
	if (_t != 0xff) {
		cocos_add ();
		nn_en_ct [enit] = NN_COCO_FREQ;
	}
}

void extra_enems_move (void) {
	nn_base = _en_t & 15;
	if (_en_t > 4) {
		if (nn_base <= 4) {
			#asm
				call en_lineal_do
			#endasm
		} else if (nn_base == 7) {
			// pursuer: along its row, at its speed
			rda = _en_mx < 0 ? -_en_mx : _en_mx; if (rda == 0) rda = 2;
			if (_en_x > gpx + rda) _en_x -= rda;
			else if (_en_x + rda < gpx) _en_x += rda;
		} else if (nn_base == 6) {
			// fanty: accelerates towards Nin Nin (1/64 px, 8 bit steps)
			if (_en_x < gpx) { if (nn_fvx [enit] < 192) nn_fvx [enit] += 16; }
			else { if (nn_fvx [enit] > -192) nn_fvx [enit] -= 16; }
			if (_en_y < gpy) { if (nn_fvy [enit] < 192) nn_fvy [enit] += 16; }
			else { if (nn_fvy [enit] > -192) nn_fvy [enit] -= 16; }
			_en_x += nn_fvx [enit] >> 6; _en_y += nn_fvy [enit] >> 6;
		} else if (nn_base == 9) {
			// drop
			_en_x += _en_mx; _en_y += _en_my;
			if (_en_y >= _en_y2) { _en_x = _en_x1; _en_y = _en_y1; }
			en_an_next_frame [enit] = sprite_cells [NN_CELL_DROP];
		}
	}
	if (_en_t & 0x70 && _en_t < 0x80) nn_shoot ();
}

void extra_enems_checks (void) {
	// touched Nin Nin (this_enemy_kills) or punched: dies
	if (_en_t == 4 || _en_t >= 128) { nn_touched = 0; return; }
	if (nn_touched == 0 && hit_on && hit_hy) {
		if (hit_hx + 1 >= _en_x && hit_hx <= _en_x + 15 && hit_hy + 1 >= _en_y && hit_hy <= _en_y + 15) nn_touched = 1;
	}
	if (nn_touched) {
		nn_touched = 0;
		if ((_en_t & 15) != 9) enems_kill (1);
	}
}

void extra_enems_killed (void) {
}

unsigned char this_enemy_kills (void) {
	nn_touched = 1;
	return 1;
}
