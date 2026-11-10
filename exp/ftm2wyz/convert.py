#!/usr/bin/env python3
"""ftm2wyz - convert FamiTracker (.ftm) modules to WYZTracker (.wyz) songs.

NES 2A03 -> AY-3-8910:   Pulse 1 -> AY A, Pulse 2 -> AY B, Triangle -> AY C, Noise -> WYZ "Effects"
(sound effects that play on one AY channel, the "FX channel").  See README.md for the full mapping.
"""
import argparse, math, os, re, sys
import ftm, wyz

HERE = os.path.dirname(os.path.abspath(__file__))
NES_NOISE = [4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068]
NES_CPU = 1789773
CHIP = {'zx': 1773400, 'cpc': 1000000}
LOW, HIGH = 24, 83            # WYZ playable range in semitones from C0 (C2..B6)
TICK_CAP = 120                # max instrument length in 50 Hz ticks


def lcm(a, b):
    return a * b // math.gcd(a, b)


class Pool:
    """Instruments and drum effects, created on demand. One pool shared by every song converted from a module
    gives all of them the same instrument and effect numbers, so a single player can play all of them."""
    def __init__(self):
        self.wins, self.wi_list = {}, []
        self.drums, self.drum_list = {}, []


class Conv:
    def __init__(self, mod, opts, pool=None):
        self.mod, self.o = mod, opts
        self.pool = pool or Pool()
        self.refresh = 50 if (mod.machine == 1 and not mod.engine_speed) else (mod.engine_speed or 60)
        self.warnings = []

    def warn(self, msg):
        if msg not in self.warnings:
            self.warnings.append(msg)

    # ---------------------------------------------------------------- instruments
    def volmap(self, v):
        return 0 if v <= 0 else max(1, round(15 * (v / 15.0) ** self.o.gamma))

    def timeline(self, ins, min_len=0):
        """Sample an FTM instrument's sequences at the 50 Hz WYZ rate.
        Returns dict(vol, arp, pitch, pre, loop) - lists of per-tick values; the tail from `pre` repeats."""
        mod = self.mod
        seqs = {}
        for kind in (ftm.SEQ_VOLUME, ftm.SEQ_ARP, ftm.SEQ_PITCH):
            s = mod.sequences.get((kind, ins.seq.get(kind, -1))) if kind in ins.seq else None
            if s is not None and s.values:
                seqs[kind] = s
        if ftm.SEQ_HIPITCH in ins.seq:
            self.warn('instrument "%s": hi-pitch sequence ignored' % ins.name)
        if ftm.SEQ_ARP in seqs and seqs[ftm.SEQ_ARP].setting != 0:
            self.warn('instrument "%s": only absolute arpeggio sequences are converted (others treated as absolute)'
                      % ins.name)
        if ftm.SEQ_ARP in seqs and ftm.SEQ_PITCH in seqs:
            self.warn('instrument "%s": arpeggio and pitch sequence together: pitch dropped (one per instrument)'
                      % ins.name)
            del seqs[ftm.SEQ_PITCH]
        pre, per = 0, 1
        for k, s in seqs.items():
            n = len(s.values)
            if 0 <= s.loop < n:
                pre, per = max(pre, s.loop), lcm(per, n - s.loop)
            else:
                pre = max(pre, n - 1)

        def val(kind, f, default):
            s = seqs.get(kind)
            if s is None:
                return default
            n = len(s.values)
            if f >= n:
                if 0 <= s.loop < n:
                    f = s.loop + (f - s.loop) % (n - s.loop)
                else:
                    return 0 if kind == ftm.SEQ_PITCH else s.values[-1]
            return s.values[f]

        R = self.refresh
        pre50 = -(-pre * 50 // R)
        m = R // math.gcd(per * 50, R)
        loop50 = m * per * 50 // R
        if pre50 + loop50 > TICK_CAP:                      # too long: approximate the loop
            self.warn('instrument "%s": loop too long for WYZ, approximated' % ins.name)
            loop50 = max(1, min(loop50, round(per * 50 / R)))
            pre50 = min(pre50, TICK_CAP - loop50)
        total = max(pre50 + loop50, min_len)
        vol, arp, pit, acc, prev = [], [], [], 0, 0
        for k in range(total):
            f = k * R // 50
            vol.append(val(ftm.SEQ_VOLUME, f, 15))
            arp.append(val(ftm.SEQ_ARP, f, 0))
            if ftm.SEQ_PITCH in seqs:
                c = sum(val(ftm.SEQ_PITCH, g, 0) for g in range(f + 1))   # FTM pitch accumulates every frame
                pit.append(c)
            else:
                pit.append(0)
        return dict(vol=vol, arp=arp, pitch=pit, pre=pre50, loop=loop50)

    def make_instrument(self, wid, ins, tri):
        t = self.timeline(ins)
        n = len(t['vol'])
        top = 15 if not tri else self.o.tri_vol
        if tri:      # the NES triangle has no volume control: any non-zero volume plays at full level
            vols = [top if v > 0 else 0 for v in t['vol']]
        else:
            vols = [self.volmap(v) for v in t['vol']]
        orn = any(t['arp'])
        pitch = None
        if orn:
            pitch = list(t['arp'])
            vols = [v | 0x10 for v in vols]
        elif any(t['pitch']):
            scale = CHIP[self.o.target] / NES_CPU * (self.refresh / 50.0) * (2 if tri else 1)
            pitch, last = [], 0
            for c in t['pitch']:
                pitch.append(round((c - last) * scale))
                last = c
        looped, start = True, min(t['pre'], n - 1)
        if t['loop'] == 1 and (vols[-1] & 15) == 0 and not (pitch and not orn and any(pitch)):
            looped = False
            vols = vols[:-1] or [0]
            pitch = pitch[:len(vols)] if pitch else None
            start = 0
        elif t['loop'] == 1 and pitch is not None and not orn:
            pitch[-1] = 0                                    # stop drifting once the pitch sequence is over
        name = ('%02X %s' % (ins.index, ins.name or 'inst'))[:28] + (' (tri)' if tri else '')   # FTM number: names repeat
        return wyz.Instrument(wid, name, vols, pitch, looped, start)

    def make_drum(self, ins, n):
        """Noise instrument + noise note n (0..15) -> WYZ Effect arrays (volumes, noises, tone periods)."""
        t = self.timeline(ins, min_len=1)
        N = len(t['vol'])
        if self.volmap(t['vol'][-1]) > 0:                       # sustained drum: give it some length, then stop
            t = self.timeline(ins, min_len=max(N, 16))
            N = len(t['vol'])
        vols = [self.volmap(v) for v in t['vol']]
        while len(vols) > 1 and vols[-1] == 0:
            vols.pop()
        N = len(vols)
        noises, tones = [], []
        for k in range(N):
            idx = min(15, max(0, n + t['arp'][k]))
            P = NES_NOISE[15 - idx]
            reg = min(31, max(1, round(P / 16.0 * CHIP[self.o.target] / NES_CPU)))
            noises.append(reg)
            tones.append(0)
        pi = 15 - n
        if pi >= self.o.kick_from:                              # low rumble: noise is too hissy on the AY -> sweep a tone
            for k in range(N):
                f = 150 - 105 * k / max(1, N - 1)
                tones[k] = round(CHIP[self.o.target] / (16.0 * f))
                noises[k] = 0
        return vols, noises, tones

    # ---------------------------------------------------------------- song
    def convert(self, si):
        mod, o, song = self.mod, self.o, self.mod.songs[si]
        nfr = len(song.frames)
        T = song.speed * 150.0 / song.tempo * 50.0 / self.refresh
        tint = round(T)
        cyc = abs(T - tint) > 1e-6
        if cyc:
            self.warn('song %d: %.3f ticks/row is not an integer: tempo is approximated with +/-1 tempo modifiers'
                      % (si, T))
        # ---- walk the frames
        order, visited, f = [], {}, 0
        loop_to, halted = 0, False
        while True:
            if f in visited:
                loop_to = visited[f]
                break
            visited[f] = len(order)
            length, jump, skip, halt = song.rows, None, False, False
            for r in range(song.rows):
                jump, skip = None, False
                for ch in range(mod.channels):
                    row = song.patterns.get((ch, song.frames[f][ch]), {}).get(r)
                    if not row:
                        continue
                    for code, param in row.fx:
                        if code == ftm.FX_JUMP:
                            jump = param
                        elif code == ftm.FX_SKIP:
                            skip = True
                            if param:
                                self.warn('Dxx with a non-zero start row is treated as D00')
                        elif code == ftm.FX_HALT:
                            halt = True
                        elif code == ftm.FX_SPEED:
                            self.warn('song %d: F (speed/tempo change) is not converted' % si)
                        elif code:
                            self.warn('effect %s%02X is not converted' % (ftm.fx_name(code), param))
                if halt or jump is not None or skip:
                    length = r + 1
                    break
            order.append((f, length))
            if halt:
                halted = True
                break
            f = jump if jump is not None else (f + 1) % nfr
            if jump is not None and jump >= nfr:
                f = 0
        # ---- per channel octave shift so everything fits C2..B6
        shift = {}
        for ch in range(3):
            semis = [r.octave * 12 + r.note - 1 for (c, p), rows in song.patterns.items() if c == ch
                     for r in rows.values() if 1 <= r.note <= 12]
            lo = min(semis) if semis else LOW
            shift[ch] = 12 * max(0, -(-(LOW - lo) // 12))
            if shift[ch]:
                self.warn('song %d, %s: moved up %d octave(s) (lowest note was below C2)' %
                          (si, ftm.CHANNEL_NAMES[ch], shift[ch] // 12))
        # ---- instruments / drums, created on demand (in the pool, maybe shared with other songs)
        wins, wi_list = self.pool.wins, self.pool.wi_list
        drums, drum_list = self.pool.drums, self.pool.drum_list

        def wyz_instr(fi, tri):
            ins = mod.instruments.get(fi)
            if ins is None:
                fi = min(mod.instruments)
                ins = mod.instruments[fi]
            key = (fi, tri)
            if key not in wins:
                i = self.make_instrument(str(len(wi_list)), ins, tri)
                for e in wi_list:                              # identical instrument already there
                    if (e.volumes, e.pitch, e.looped, e.loop_start) == (i.volumes, i.pitch, i.looped, i.loop_start):
                        wins[key] = e.id
                        break
                else:
                    wi_list.append(i)
                    wins[key] = i.id
            return wins[key]

        def drum_fx(fi, n):
            ins = mod.instruments.get(fi) or mod.instruments[min(mod.instruments)]
            key = (ins.index, n)
            if key not in drums:
                v, nz, tn = self.make_drum(ins, n)
                for e in drum_list:                            # identical effect already there
                    if (e.volumes, e.noises, e.freqs) == (v, nz, tn):
                        drums[key] = e.id
                        break
                else:
                    e = wyz.Effect(len(drum_list), '%02X %s n%X' % (ins.index, (ins.name or 'drum')[:20], n), v, nz, tn)
                    drum_list.append(e)
                    drums[key] = e.id
            return drums[key]

        cur = [None] * 4
        pats, pat_key, play = [], {}, []
        activity, fx_events = [], []            # for choosing the FX channel
        tempo_state = tint
        for oi, (f, length) in enumerate(order):
            chans = [song.patterns.get((ch, song.frames[f][ch]), {}) for ch in range(4)]
            lines = [wyz.Line(3) for _ in range(length)]
            last = [None] * 3
            for r in range(length):
                ln = lines[r]
                act = [None] * 3
                for ch in range(4):
                    row = chans[ch].get(r)
                    if row is None:
                        continue
                    if row.instr != ftm.NO_INSTR:
                        cur[ch] = row.instr
                    if row.vol != ftm.NO_VOL:
                        self.warn('volume column is not converted')
                    if ch == 3:
                        if 1 <= row.note <= 12:
                            ln.fx = drum_fx(cur[3], (row.octave * 12 + row.note - 1) & 15)
                            fx_events.append((len(activity) + r, ln.fx))
                        continue
                    if ch > 3:
                        continue
                    if row.note == ftm.NOTE_HALT or row.note == ftm.NOTE_RELEASE:
                        ln.notes[ch] = wyz.Note(None, wyz.SILENCE)
                    elif 1 <= row.note <= 12:
                        s = row.octave * 12 + row.note - 1 + shift[ch]
                        while s > HIGH:
                            s -= 12
                        while s < LOW:
                            s += 12
                        nv = s % 12
                        sharp = nv in (1, 3, 6, 8, 10)
                        letter = 'CCDDEFFGGAAB'[nv]
                        wid = wyz_instr(cur[ch], ch == 2)
                        ln.notes[ch] = wyz.Note(s // 12, ord(letter), 1 if sharp else 0, wid if wid != last[ch] else None)
                        last[ch] = wid
            # tempo modifiers for non-integer ticks per row
            if cyc:
                for r in range(length):
                    t_r = int((r + 1) * T) - int(r * T)
                    d = t_r - tempo_state
                    lines[r].tempo_mod = max(-1, min(1, d))
                    tempo_state += lines[r].tempo_mod
            sig = (length, tuple(tuple((n.octave, n.note, n.semi, n.instrument) for n in l.notes) + (l.fx, l.tempo_mod)
                                 for l in lines))
            if sig not in pat_key:
                p = wyz.Pattern('P%02d' % len(pats), length, 3)
                p.lines = lines
                pat_key[sig] = len(pats)
                pats.append(p)
            play.append(pat_key[sig])
        # ---- FX channel: the melodic channel that suffers least from drums stealing it
        fxch = o.fx_channel
        if fxch is None:
            rows_active, base = {}, 0
            for oi, (f, length) in enumerate(order):
                chans = [song.patterns.get((ch, song.frames[f][ch]), {}) for ch in range(3)]
                for r in range(length):
                    for ch in range(3):
                        row = chans[ch].get(r)
                        if row is not None:
                            if 1 <= row.note <= 12:
                                rows_active[(base + r, ch)] = True
                            elif row.note in (13, 14):
                                rows_active[(base + r, ch)] = False
                base += length
            sounding, cost = [False] * 3, [0] * 3
            evmap = {}
            for gr, e in fx_events:
                evmap[gr] = e
            efflen = {e.id: max(1, -(-len(e.volumes) // max(1, tint))) for e in drum_list}
            until = -1
            base = 0
            for oi, (f, length) in enumerate(order):
                for r in range(length):
                    g = base + r
                    for ch in range(3):
                        if (g, ch) in rows_active:
                            sounding[ch] = rows_active[(g, ch)]
                    if g in evmap:
                        until = g + efflen[evmap[g]]
                    if g < until:
                        for ch in range(3):
                            if sounding[ch]:
                                cost[ch] += 1
                base += length
            fxch = min((1, 0, 2), key=lambda c: (cost[c], c))
            self.warn('song %d: FX (drum) channel = %s; rows where a melodic note is cut by drums: %s'
                      % (si, 'ABC'[fxch], {'ABC'[c]: cost[c] for c in range(3)}))
        # ---- assemble
        w = wyz.Song(song.name or 'song %d' % si, 3)
        w.patterns, w.play_order = pats, play
        w.tempo = tint
        if cyc:
            w.tempo = int(T * 64) - int(T * 63)          # tempo of the last line of a 64 row pattern
        self.set_pool(w)
        cpc = o.target == 'cpc'
        w.cpc, w.chip_frequency = cpc, CHIP[o.target]
        w.frequencies = wyz.load_freq_table(os.path.join(HERE, 'data', 'NOTAS_CPC.dat' if cpc else 'NOTAS.DAT'))
        w.looped = not halted
        w.loop_to = loop_to
        w.fx_channel = fxch
        return w


    def set_pool(self, w):
        """The song gets the whole pool, used or not (call again once every song sharing it is converted)."""
        w.instruments = list(self.pool.wi_list) + [wyz.Instrument('R', 'Sawtooth', None, None, False, 0)]
        w.effects = list(self.pool.drum_list)


def slug(s):
    s = re.sub(r'[^A-Za-z0-9]+', '_', s).strip('_')
    return s or 'song'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('input', help='FamiTracker module (.ftm, version >= 0.4.0)')
    ap.add_argument('-o', '--out', default='.', help='output directory (one .wyz per song in the module)')
    ap.add_argument('-t', '--target', choices=('zx', 'cpc'), default='zx', help='AY clock / frequency table')
    ap.add_argument('-s', '--song', type=int, action='append', help='only this song index (repeatable)')
    ap.add_argument('--fx-channel', choices='ABC', help='AY channel used for drums (default: automatic)')
    ap.add_argument('--gamma', type=float, default=0.55, help='NES->AY volume curve exponent (default 0.55)')
    ap.add_argument('--tri-vol', type=int, default=13, help='AY volume of the triangle channel (default 13)')
    ap.add_argument('--kick-from', type=int, default=13,
                    help='noise period index (0..15, 15 = lowest) from which drums become a tone sweep (default 13)')
    ap.add_argument('--separate', action='store_true',
                    help='each song with its own instruments and effects (default: one pool shared by all the '
                         'songs converted, same numbers in every .wyz, for a single player)')
    ap.add_argument('--list', action='store_true', help='list the songs and exit')
    o = ap.parse_args()
    o.fx_channel = 'ABC'.index(o.fx_channel) if o.fx_channel else None
    mod = ftm.parse(o.input)
    if o.list:
        for i, s in enumerate(mod.songs):
            print(i, s.name, 'speed', s.speed, 'tempo', s.tempo, 'frames', len(s.frames))
        return
    os.makedirs(o.out, exist_ok=True)
    base = slug(os.path.splitext(os.path.basename(o.input))[0])
    pool = Pool()
    done = []
    for si in (o.song if o.song else range(len(mod.songs))):
        c = Conv(mod, o, None if o.separate else pool)
        done.append((si, c, c.convert(si)))
    for si, c, w in done:
        c.set_pool(w)          # shared pool: now complete, the same in every song
        path = os.path.join(o.out, '%s_%02d_%s_%s.wyz' % (base, si, slug(mod.songs[si].name), o.target))
        wyz.save(w, path)
        print('%s: %d patterns, order %d, %d instruments, %d drum effects, fx channel %s, loop to %d' %
              (path, len(w.patterns), len(w.play_order), len(w.instruments) - 1, len(w.effects),
               'ABC'[w.fx_channel], w.loop_to if w.looped else -1))
        for m in c.warnings:
            print('   note:', m)
    if not o.separate:
        print('shared pool: %d instruments (+ R Sawtooth), %d effects' % (len(pool.wi_list), len(pool.drum_list)))
        for i in pool.wi_list:
            print('   instrument %s: %s' % (i.id, i.name))
        for e in pool.drum_list:
            print('   effect %d: %s' % (e.id, e.name))


if __name__ == '__main__':
    main()
