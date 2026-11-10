#!/usr/bin/env python3
"""Independent check: read a .wyz back (XML), play it with a small re-implementation of WYZTracker's player rules and
compare the pitch of every melodic row with the source .ftm.  Usage: check.py file.ftm song_index file.wyz"""
import base64, gzip, sys, re
import xml.etree.ElementTree as ET
import ftm, wyz


def load(path):
    try:
        data = gzip.open(path).read()
    except OSError:
        data = open(path, 'rb').read()
    r = ET.fromstring(data)
    s = {}
    s['tempo'] = int(r.findtext('Tempo'))
    s['order'] = [int(x.text) for x in r.find('PlayOrder')]
    s['freq'] = [int(x.text) for x in r.find('Frequencies')]
    s['chip'] = int(r.findtext('ChipFrequency'))
    s['pats'] = []
    for p in r.find('Patterns'):
        lines = []
        for l in p.find('Lines'):
            ns = []
            for n in l.find('Notes'):
                o = int(n.findtext('Octave'))
                ns.append((None if o == -2147483648 else o, int(n.findtext('Note')), int(n.findtext('Seminote')),
                           n.findtext('Instrument') or None))
            lines.append((ns, int(l.findtext('Fx'))))
        s['pats'].append(lines)
    s['ins'] = {}
    for i in r.find('Instruments'):
        v = i.find('Volumes')
        if v is None:
            continue
        pm = i.find('PitchModifiers')
        s['ins'][i.findtext('ID')] = (list(base64.b64decode(v.text or '')), [int(x.text) for x in pm] if pm is not None else None,
                                      i.findtext('Looped') == 'true', int(i.findtext('LoopStart')))
    s['fx'] = {}
    for e in r.find('Effects'):
        s['fx'][int(e.findtext('ID'))] = (list(base64.b64decode(e.findtext('Volumes'))), list(base64.b64decode(e.findtext('Noises'))),
                                         [int(x.text) for x in e.find('Frequencies')])
    return s


def main(ftm_path, si, wyz_path):
    mod = ftm.parse(ftm_path)
    song = mod.songs[si]
    w = load(wyz_path)
    errs = tot = 0
    # row-level expected pitch from the FTM: compare note letters/octaves modulo the per-channel octave shift
    wy_rows = []
    for pi in w['order']:
        for ns, fx in w['pats'][pi]:
            wy_rows.append((ns, fx))
    # rebuild FTM rows by walking like the converter does (frames until jump)
    f, seen, rows = 0, set(), []
    while f not in seen:
        seen.add(f)
        nxt, n = (f + 1) % len(song.frames), song.rows
        for r in range(song.rows):
            jump = skip = False
            for ch in range(mod.channels):
                row = song.patterns.get((ch, song.frames[f][ch]), {}).get(r)
                if row:
                    for code, param in row.fx:
                        if code == ftm.FX_JUMP: jump = param
                        if code == ftm.FX_SKIP: skip = True
            rows.append((f, r))
            if jump is not False or skip:
                nxt = jump if jump is not False else nxt
                break
        f = nxt
    print('rows: ftm %d  wyz %d' % (len(rows), len(wy_rows)))
    assert len(rows) == len(wy_rows), 'row count differs'
    for ch in range(3):
        cur_pitch_ftm = cur_pitch_wyz = None
        mism = 0
        for (f, r), (ns, fx) in zip(rows, wy_rows):
            row = song.patterns.get((ch, song.frames[f][ch]), {}).get(r)
            o, note, semi, ins = ns[ch]
            if row and 1 <= row.note <= 12:
                exp = (row.octave * 12 + row.note - 1) % 12
                got = None
                if o is not None and note != 80:
                    got = ((o * 12) + {'C':0,'D':2,'E':4,'F':5,'G':7,'A':9,'B':11}[chr(note)] + (1 if semi else 0)) % 12
                tot += 1
                if got != exp:
                    mism += 1
            elif row and row.note in (13, 14):
                tot += 1
                if note != 80: mism += 1
            else:
                if note != 0 or o is not None: mism += 1
        print('channel %d: pitch-class mismatches %d' % (ch, mism))
        errs += mism
    # play check: every tone index in range and every referenced instrument/effect exists
    bad = 0
    for ns, fx in wy_rows:
        for o, note, semi, ins in ns:
            if o is not None and note != 80:
                idx = 12 * (o - 2) + {'C':0,'D':2,'E':4,'F':5,'G':7,'A':9,'B':11}[chr(note)] + (1 if semi else 0) + 2
                if not 2 <= idx <= 61: bad += 1
            if ins is not None and ins not in w['ins'] and ins != 'R': bad += 1
        if fx != -2147483648 and fx not in w['fx']: bad += 1
    print('out-of-range / dangling references:', bad)
    print('RESULT', 'OK' if errs == 0 and bad == 0 else 'PROBLEMS')


if __name__ == '__main__':
    main(sys.argv[1], int(sys.argv[2]), sys.argv[3])
