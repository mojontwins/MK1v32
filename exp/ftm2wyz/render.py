#!/usr/bin/env python3
"""Render a .wyz to a WAV with a small AY-3-8910 model that follows WYZTracker's player rules (preview/check only).
Usage: render.py song.wyz out.wav [loops=1]"""
import sys, wave
import numpy as np
import check

SR = 44100
LETTERS = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}


def play(w, fx_channel, loops=1):
    """-> per-tick list of 3 x (tone_period, volume, noise_reg)"""
    tempo = w['tempo']
    order = w['order'] * 1
    ticks = []
    st = [dict(note=None, ins=None, pos=0, pitchacc=0) for _ in range(3)]
    fxs = None
    for _ in range(loops):
        for pi in order:
            for ns, fx in w['pats'][pi]:
                if fx != -2147483648:
                    fxs = [w['fx'][fx], 0]
                for ch, (o, note, semi, ins) in enumerate(ns):
                    s = st[ch]
                    if ins is not None:
                        s['ins'] = ins
                    if note == 80:
                        s['note'] = None
                    elif o is not None:
                        s['note'] = 12 * (o - 2) + LETTERS[chr(note)] + (1 if semi else 0) + 2
                        s['pos'] = 0
                        s['pitchacc'] = 0
                    elif ins is not None:
                        s['pos'] = 0
                for t in range(tempo):
                    frame = []
                    for ch in range(3):
                        s = st[ch]
                        per, vol, nz = 0, 0, 0
                        if s['note'] is not None and s['ins'] in w['ins']:
                            vols, pitch, looped, ls = w['ins'][s['ins']]
                            p = s['pos']
                            if p >= len(vols):
                                p = ls if looped else None
                            if p is not None:
                                vb = vols[p]
                                vol = vb & 15
                                idx = s['note']
                                if pitch is not None and vb & 0x10:
                                    idx += pitch[p]
                                elif pitch is not None:
                                    s['pitchacc'] += pitch[p]
                                idx = max(2, min(61, idx))
                                per = w['freq'][idx] + (s['pitchacc'] if not vb & 0x10 else 0)
                                s['pos'] = p + 1
                        frame.append([per, vol, nz])
                    if fxs is not None:
                        vols, nzs, frs = fxs[0]
                        k = fxs[1]
                        if k < len(vols):
                            frame[fx_channel] = [frs[k], vols[k] & 15, nzs[k] & 0x7F]
                            fxs[1] += 1
                        else:
                            fxs = None
                    ticks.append(frame)
    return ticks


def render(ticks, chip):
    spt = SR // 50
    out = np.zeros(len(ticks) * spt)
    rng = np.random.RandomState(1)
    amp = [0] + [0.5 ** ((15 - v) / 2.0) for v in range(1, 16)]
    phase = np.zeros(3)
    lfsr, noise_ph = 1, 0.0
    for ti, frame in enumerate(ticks):
        seg = np.zeros(spt)
        n = np.arange(spt)
        # noise: one LFSR step per AY noise tick (chip/16/reg) -> sample-and-hold white noise
        nzreg = max([c[2] for c in frame] + [0])
        if nzreg:
            nf = chip / 16.0 / nzreg
            steps = int(spt * nf / SR) + 2
            vals = rng.randint(0, 2, steps) * 2 - 1
            nz = vals[(n * nf / SR).astype(int)]
        else:
            nz = None
        for ch, (per, vol, nreg) in enumerate(frame):
            if vol == 0:
                continue
            a = amp[vol]
            if per > 0:
                f = chip / 16.0 / per
                ph = phase[ch] + n * f / SR
                seg += a * (np.where((ph % 1.0) < 0.5, 1.0, -1.0))
                phase[ch] = (phase[ch] + spt * f / SR) % 1.0
            if nreg and nz is not None:
                seg += a * nz
        out[ti * spt:(ti + 1) * spt] = seg
    out /= max(1.0, np.abs(out).max())
    return (out * 0.8 * 32767).astype(np.int16)


if __name__ == '__main__':
    import re, gzip
    path = sys.argv[1]
    w = check.load(path)
    import xml.etree.ElementTree as ET
    r = ET.fromstring(gzip.open(path).read())
    fxc = int(r.findtext('FxChannel'))
    ticks = play(w, fxc, int(sys.argv[3]) if len(sys.argv) > 3 else 1)
    pcm = render(ticks, w['chip'])
    with wave.open(sys.argv[2], 'wb') as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(SR); f.writeframes(pcm.tobytes())
    print('%s: %.1f s' % (sys.argv[2], len(ticks) / 50.0))
