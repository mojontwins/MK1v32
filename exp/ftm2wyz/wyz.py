"""WYZTracker song model + writer (.wyz = gzip of the .NET XmlSerializer XML of WYZTracker.Song)."""
import base64, gzip, struct
from xml.sax.saxutils import escape

NONE = -2147483648
SILENCE = ord('P')
NOTE_CODES = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}


class Note:
    """octave None + note 0 = nothing; note 'P' = silence; seminote 1 = sharp ('+')."""
    __slots__ = ('octave', 'note', 'semi', 'instrument')

    def __init__(self, octave=None, note=0, semi=0, instrument=None):
        self.octave, self.note, self.semi, self.instrument = octave, note, semi, instrument


class Line:
    def __init__(self, channels):
        self.notes = [Note() for _ in range(channels)]
        self.fx = None
        self.tempo_mod = 0


class Pattern:
    def __init__(self, name, length, channels):
        self.name, self.length = name, length
        self.lines = [Line(channels) for _ in range(length)]


class Instrument:
    def __init__(self, id, name, volumes=None, pitch=None, looped=False, loop_start=0):
        self.id, self.name, self.volumes, self.pitch = str(id), name, volumes, pitch
        self.looped, self.loop_start = looped, loop_start


class Effect:
    def __init__(self, id, name, volumes, noises, freqs):
        self.id, self.name, self.volumes, self.noises, self.freqs = id, name, volumes, noises, freqs


class Song:
    def __init__(self, name='', channels=3):
        self.name, self.channels = name, channels
        self.patterns, self.play_order = [], []
        self.tempo = 5
        self.instruments, self.effects = [], []
        self.frequencies = []
        self.chip_frequency = 1773400
        self.looped, self.loop_to = True, 0
        self.cpc = False
        self.fx_channel = 1


def load_freq_table(path):
    d = open(path, 'rb').read()
    return list(struct.unpack('<%dh' % (len(d) // 2), d))


def b64(vals):
    return base64.b64encode(bytes(v & 255 for v in vals)).decode()


def to_xml(s):
    o = ['<?xml version="1.0"?>',
         '<Song xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">',
         '  <MutedChannels>'] + ['    <boolean>false</boolean>'] * 4 + ['  </MutedChannels>']
    o.append('  <Name>%s</Name>' % escape(s.name))
    o.append('  <Patterns>')
    for p in s.patterns:
        o += ['    <Pattern>', '      <Name>%s</Name>' % escape(p.name), '      <Length>%d</Length>' % p.length,
              '      <Lines>']
        for ln in p.lines:
            o += ['        <ChannelLine>', '          <Notes>']
            for n in ln.notes:
                o.append('            <ChannelNote>')
                o.append('              <Octave>%d</Octave>' % (NONE if n.octave is None else n.octave))
                o.append('              <Note>%d</Note>' % n.note)
                o.append('              <Seminote>%d</Seminote>' % (43 if n.semi else 0))
                o.append('              <Instrument>%s</Instrument>' % n.instrument if n.instrument is not None
                         else '              <Instrument />')
                o.append('            </ChannelNote>')
            o.append('          </Notes>')
            o.append('          <Fx>%d</Fx>' % (NONE if ln.fx is None else ln.fx))
            o.append('          <Channels>%d</Channels>' % s.channels)
            if ln.tempo_mod:
                o.append('          <TempoModifier>%d</TempoModifier>' % ln.tempo_mod)
            o.append('        </ChannelLine>')
        o += ['      </Lines>', '      <Channels>%d</Channels>' % s.channels, '    </Pattern>']
    o.append('  </Patterns>')
    o.append('  <PlayOrder>')
    o += ['    <int>%d</int>' % i for i in s.play_order]
    o.append('  </PlayOrder>')
    o.append('  <Tempo>%d</Tempo>' % s.tempo)
    o.append('  <Channels>%d</Channels>' % s.channels)
    o.append('  <Instruments>')
    for i in s.instruments:
        o += ['    <Instrument>', '      <ID>%s</ID>' % i.id, '      <Name>%s</Name>' % escape(i.name)]
        if i.volumes is not None:
            o.append('      <Volumes>%s</Volumes>' % b64(i.volumes))
        if i.pitch is not None:
            o.append('      <PitchModifiers>')
            o += ['        <int>%d</int>' % v for v in i.pitch]
            o.append('      </PitchModifiers>')
        o += ['      <Looped>%s</Looped>' % ('true' if i.looped else 'false'),
              '      <LoopStart>%d</LoopStart>' % i.loop_start, '    </Instrument>']
    o.append('  </Instruments>')
    o.append('  <Effects>')
    for e in s.effects:
        o += ['    <Effect>', '      <ID>%d</ID>' % e.id, '      <Name>%s</Name>' % escape(e.name),
              '      <Volumes>%s</Volumes>' % b64(e.volumes), '      <Noises>%s</Noises>' % b64(e.noises),
              '      <Frequencies>']
        o += ['        <int>%d</int>' % f for f in e.freqs]
        o += ['      </Frequencies>', '    </Effect>']
    o.append('  </Effects>')
    o.append('  <Frequencies>')
    o += ['    <short>%d</short>' % f for f in s.frequencies]
    o.append('  </Frequencies>')
    o.append('  <ChipFrequency>%d</ChipFrequency>' % s.chip_frequency)
    o.append('  <Looped>%s</Looped>' % ('true' if s.looped else 'false'))
    o.append('  <DefaultMsxFreqs>%s</DefaultMsxFreqs>' % ('false' if s.cpc else 'true'))
    o.append('  <DefaultCpcFreqs>%s</DefaultCpcFreqs>' % ('true' if s.cpc else 'false'))
    o += ['  <CustomFreqs>false</CustomFreqs>', '  <ParameterizedFreqs>false</ParameterizedFreqs>',
          '  <ParameterValue>%d</ParameterValue>' % s.chip_frequency,
          '  <FxChannel>%d</FxChannel>' % s.fx_channel, '  <LoopToPattern>%d</LoopToPattern>' % s.loop_to, '</Song>']
    return '\n'.join(o) + '\n'


def save(s, path):
    with gzip.open(path, 'wb') as f:
        f.write(to_xml(s).encode('utf-8'))
