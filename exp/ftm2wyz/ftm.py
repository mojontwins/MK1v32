"""FamiTracker module (.ftm) reader.

Reads the binary format written by FamiTracker 0.4.x (file version 0x0400-0x0440): a "FamiTracker Module"
signature, a u32 version, then blocks (16 byte name, u32 version, u32 size, data) ending with "END".
Only what is needed to convert the music is kept: songs ("tracks"), frames, patterns, instruments and
sequences.  Every block parser checks that it consumes exactly its block, which is how the layouts below
were verified against real files.
"""
import struct

NOTE_NONE, NOTE_HALT, NOTE_RELEASE = 0, 13, 14
NO_INSTR, NO_VOL = 0x40, 0x10

SEQ_VOLUME, SEQ_ARP, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTY = range(5)
CHANNEL_NAMES = {0: 'Pulse 1', 1: 'Pulse 2', 2: 'Triangle', 3: 'Noise', 4: 'DPCM'}


class Reader:
    def __init__(self, data):
        self.d, self.p = data, 0

    def u8(self):
        v = self.d[self.p]
        self.p += 1
        return v

    def s8(self):
        v = self.u8()
        return v - 256 if v > 127 else v

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]
        self.p += 4
        return v

    def s32(self):
        v = struct.unpack_from('<i', self.d, self.p)[0]
        self.p += 4
        return v

    def raw(self, n):
        v = self.d[self.p:self.p + n]
        self.p += n
        return v

    def cstr(self):
        e = self.d.index(b'\0', self.p)
        v = self.d[self.p:e].decode('latin-1')
        self.p = e + 1
        return v

    def done(self):
        return self.p >= len(self.d)


class Sequence:
    def __init__(self, index, kind, loop, release, setting, values):
        self.index, self.kind, self.loop, self.release = index, kind, loop, release
        self.setting, self.values = setting, values


class Instrument:
    def __init__(self, index, kind, name):
        self.index, self.kind, self.name = index, kind, name
        self.seq = {}          # sequence type -> sequence index (only the enabled ones)


class Row:
    __slots__ = ('note', 'octave', 'instr', 'vol', 'fx')

    def __init__(self, note, octave, instr, vol, fx):
        self.note, self.octave, self.instr, self.vol, self.fx = note, octave, instr, vol, fx


class Song:
    def __init__(self, name):
        self.name = name
        self.speed = 6
        self.tempo = 150
        self.rows = 64
        self.frames = []       # frames[f][channel] = pattern index
        self.fx_columns = []   # per channel
        self.patterns = {}     # (channel, pattern) -> {row: Row}


class Module:
    pass


def parse(path):
    data = open(path, 'rb').read()
    if not data.startswith(b'FamiTracker Module'):
        raise ValueError('not a FamiTracker module')
    m = Module()
    m.version = struct.unpack_from('<I', data, 18)[0]
    if m.version < 0x0400:
        raise ValueError('module version 0x%04X is older than 0x0400 (open and re-save it in FamiTracker 0.4.x)'
                         % m.version)
    m.title = m.author = m.copyright = ''
    m.instruments, m.sequences, m.songs, m.channel_types = {}, {}, [], []
    m.expansion, m.machine, m.engine_speed = 0, 0, 0
    p = 22
    while p < len(data):
        name = data[p:p + 16].split(b'\0')[0].decode()
        if name == 'END' or p + 24 > len(data):
            break
        ver, size = struct.unpack_from('<II', data, p + 16)
        r = Reader(data[p + 24:p + 24 + size])
        {'PARAMS': _params, 'INFO': _info, 'HEADER': _header, 'INSTRUMENTS': _instruments,
         'SEQUENCES': _sequences, 'FRAMES': _frames, 'PATTERNS': _patterns}.get(name, _skip)(m, r, ver)
        if name in ('PARAMS', 'INFO', 'HEADER', 'INSTRUMENTS', 'SEQUENCES', 'FRAMES', 'PATTERNS') and not r.done():
            raise ValueError('block %s v%d: %d bytes left unread' % (name, ver, len(r.d) - r.p))
        p += 24 + size
    return m


def _skip(m, r, ver):
    r.p = len(r.d)


def _params(m, r, ver):
    m.expansion = r.u8()
    m.channels = r.u32()
    m.machine = r.u32()            # 0 NTSC, 1 PAL
    m.engine_speed = r.u32()       # 0 = default (60 / 50 Hz)
    m.vibrato = r.u32()
    m.hl_a, m.hl_b = r.u32(), r.u32()
    if ver >= 6:
        m.split = r.u32()          # speed / tempo split point


def _info(m, r, ver):
    m.title = r.raw(32).split(b'\0')[0].decode('latin-1')
    m.author = r.raw(32).split(b'\0')[0].decode('latin-1')
    m.copyright = r.raw(32).split(b'\0')[0].decode('latin-1')


def _header(m, r, ver):
    n = r.u8() + 1
    names = [r.cstr() for _ in range(n)]
    m.channel_types = []
    cols = []
    for _ in range(m.channels):
        m.channel_types.append(r.u8())
        cols.append([r.u8() for _ in range(n)])
    for t in range(n):
        s = Song(names[t])
        s.fx_columns = [cols[c][t] + 1 for c in range(m.channels)]
        m.songs.append(s)


def _instruments(m, r, ver):
    for _ in range(r.u32()):
        index = r.u32()
        kind = r.u8()
        if kind != 1:
            raise ValueError('instrument %d is of an unsupported (expansion chip) type %d' % (index, kind))
        seqs = {}
        for i in range(r.u32()):
            enabled, sidx = r.u8(), r.u8()
            if enabled:
                seqs[i] = sidx
        r.raw(96 * 3)              # DPCM sample map: sample, pitch, delta per note
        name = r.raw(r.u32()).decode('latin-1')
        ins = Instrument(index, kind, name)
        ins.seq = seqs
        m.instruments[index] = ins


def _sequences(m, r, ver):
    # block v6: every sequence is (index u32, type u32, length u8, loop point s32, values s8...); the release
    # points and arpeggio/duty "settings" of all of them follow as a (release s32, setting u32) table
    items = []
    for _ in range(r.u32()):
        index, kind = r.u32(), r.u32()
        count = r.u8()
        loop = r.s32()
        items.append((index, kind, loop, [r.s8() for _ in range(count)]))
    for index, kind, loop, values in items:
        release, setting = r.s32(), r.u32()
        m.sequences[(kind, index)] = Sequence(index, kind, loop, release, setting, values)


def _frames(m, r, ver):
    for s in m.songs:
        n = r.u32()
        s.speed, s.tempo, s.rows = r.u32(), r.u32(), r.u32()
        s.frames = [[r.u8() for _ in range(m.channels)] for _ in range(n)]


def _patterns(m, r, ver):
    while not r.done():
        track, channel, pat, items = r.u32(), r.u32(), r.u32(), r.u32()
        song = m.songs[track]
        rows = {}
        for _ in range(items):
            row = r.u32()
            note, octave, instr, vol = r.u8(), r.u8(), r.u8(), r.u8()
            fx = []
            for _c in range(song.fx_columns[channel]):
                fx.append((r.u8(), r.u8()))
            rows[row] = Row(note, octave, instr, vol, fx)
        song.patterns[(channel, pat)] = rows


# effect numbers as stored in the file (FamiTracker's effect_t enum) -> the letter shown in the editor
FX_LETTERS = ['', 'F', 'B', 'D', 'C', 'E', '3', '', 'H', 'I', '0', '4', '7', 'P', 'G', 'Z', '1', '2', 'V', 'Y',
              'Q', 'R', 'A', 'S', 'X', 'M', 'T', 'W', 'N', 'O', 'L', 'J', 'K']
FX_SPEED, FX_JUMP, FX_SKIP, FX_HALT = 1, 2, 3, 4


def fx_name(code):
    return FX_LETTERS[code] if code < len(FX_LETTERS) and FX_LETTERS[code] else ('#%d' % code if code else '')


def seq_values(m, ins, kind):
    s = m.sequences.get((kind, ins.seq.get(kind, -1)))
    return s


if __name__ == '__main__':
    import sys
    m = parse(sys.argv[1])
    print('version 0x%04X  title %r  author %r  copyright %r' % (m.version, m.title, m.author, m.copyright))
    print('expansion %d channels %d machine %s engine speed %d' % (m.expansion, m.channels,
                                                                   'PAL' if m.machine else 'NTSC', m.engine_speed))
    print('channel types', [CHANNEL_NAMES.get(t, t) for t in m.channel_types])
    print('\ninstruments:')
    for i in sorted(m.instruments):
        ins = m.instruments[i]
        print('  %2d %-14s' % (i, ins.name), {('vol', 'arp', 'pitch', 'hipitch', 'duty')[k]: v for k, v in ins.seq.items()})
    print('\nsequences:')
    for (k, i), s in sorted(m.sequences.items()):
        print('  %s #%d loop %d release %d setting %d: %s' % (('vol', 'arp', 'pitch', 'hipitch', 'duty')[k], i, s.loop,
                                                          s.release, s.setting, s.values))
    for n, s in enumerate(m.songs):
        print('\nsong %d %r: speed %d tempo %d rows/pattern %d frames %d fx columns %s' %
              (n, s.name, s.speed, s.tempo, s.rows, len(s.frames), s.fx_columns))
        print('  order', [f for f in s.frames][:40])
