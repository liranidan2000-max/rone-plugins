"""Renders the Plugins Center's dry/wet previews (Center 2.0).

Each plugin plays the same dry loop - Liran's kick + bass from the tutorials,
142 BPM, 8 bars - once untouched and once through the INSTALLED VST3, with
the knob moves below drawn as automation. Both files get the same tail and
the same gain, so DRY/WET in the Center is a fair switch.

    python render_previews.py [--only RoneThrow] [--out <folder>]

Writes <id>-dry.mp3 / <id>-wet.mp3 (+ .wav) and a levels report. Nothing is
published from here: the clips go to roneaudio.com/media/previews/ and into
versions.json ("preview": {"dry", "wet"}) only after Liran has listened.
"""
import argparse, json, os, subprocess, sys, wave, array, math

HERE = os.path.dirname(os.path.abspath(__file__))
RENDER = os.path.join(HERE, 'build', 'RonePreviewRender_artefacts', 'Release', 'RonePreviewRender.exe')
VST3 = r'C:\Program Files\Common Files\VST3\RONE'
DRY = r'D:\RONE PLUGINS\WEBSITE RONE AUDIO\videos\_kit\tutorial\kickbass-142-8bars.wav'
BPM = 142
TAIL = 4.0


def ramp(name, start, end, v0, v1, step=0.5):
    """Automation points from v0 at `start` to v1 at `end` (beats)."""
    pts, b = [], start
    while b < end - 1e-9:
        pts.append((name, b, v0 + (v1 - v0) * (b - start) / (end - start)))
        b += step
    pts.append((name, end, v1))
    return pts


PLUGINS = {
    # THROW opens on the last beat of every second bar and closes on the next downbeat
    'RoneThrow': dict(file='RONE Throw', set={},
                      auto=[('Throw', 0, 0)] + [p for bar in (2, 4, 6, 8) for p in (('Throw', bar * 4 - 1, 1.0), ('Throw', bar * 4, 0.0))]),
    # Not here, on purpose: RONE Stutter and Reverse Reverb play a file or a
    # captured sample rather than the signal passing through (a dry/wet pair of
    # the same loop says nothing about them), Iron is an instrument, the
    # Analyzer makes no sound. Their pages say a preview is on its way.
    # the knob drawn up over bar 4 and bar 8: the loop rolls up into the next bar
    'RoneStucker': dict(file='RONE Stucker', set={},
                        auto=[('Amount', 0, 0)] + ramp('Amount', 12, 15.75, 0.0, 1.0, 0.25) + [('Amount', 16, 0)]
                             + ramp('Amount', 28, 31.75, 0.0, 1.0, 0.25) + [('Amount', 32, 0)]),
    # INTENSITY climbs for seven bars and snaps to zero on the drop (bar 8)
    'RoneRise': dict(file='RONE Rise', set={},
                     auto=ramp('Intensity', 0, 27.5, 0.0, 1.0) + [('Intensity', 28, 0.0)]),
    # INFINITE barber-pole rise, half wet
    'RoneFlanger': dict(file='RONE Flanger', set={'Infinite': 1.0, 'Dry/Wet': 0.55, 'Feedback': 0.8}, auto=[]),
    # pushed into the clipper (+12 dB in), level matched by nothing: louder is the point
    'RoneClipper': dict(file='RONE Clipper', set={'Input': 0.75, 'Clip': 1.0, 'Low Cut': 0.0}, auto=[]),
    # a big space behind the loop
    'RoneAfterspace': dict(file='RONE AFTERSPACE', set={'Mix': 0.45, 'Size': 0.85, 'Decay': 0.62, 'Echo': 0.25}, auto=[]),
}


def read_wav(path):
    w = wave.open(path)
    n, ch, sw, sr = w.getnframes(), w.getnchannels(), w.getsampwidth(), w.getframerate()
    raw = w.readframes(n)
    w.close()
    if sw == 3:
        vals = [int.from_bytes(raw[i:i + 3], 'little', signed=True) / 8388608.0 for i in range(0, len(raw), 3)]
    elif sw == 2:
        a = array.array('h', raw); vals = [x / 32768.0 for x in a]
    else:
        raise SystemExit(f'unsupported sample width {sw}')
    return vals, ch, sr


def levels(path):
    vals, ch, sr = read_wav(path)
    peak = max(abs(v) for v in vals) or 1e-9
    rms = math.sqrt(sum(v * v for v in vals) / len(vals)) or 1e-9
    return 20 * math.log10(peak), 20 * math.log10(rms)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only')
    ap.add_argument('--out', default=os.path.join(HERE, 'out'))
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    # the dry reference with the same tail: a pass through nothing
    dry_wav = os.path.join(args.out, '_dry-with-tail.wav')
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', DRY, '-af', f'apad=pad_dur={TAIL}', '-c:a', 'pcm_s24le', dry_wav], check=True)

    report = {}
    for pid, cfg in PLUGINS.items():
        if args.only and pid != args.only:
            continue
        wet_wav = os.path.join(args.out, f'{pid}-wet.wav')
        cmd = [RENDER, '--plugin', os.path.join(VST3, cfg['file'] + '.vst3'), '--in', DRY, '--out', wet_wav,
               '--bpm', str(BPM), '--tail', str(TAIL),
               '--set', ';'.join(f'{k}={v}' for k, v in cfg['set'].items()),
               '--auto', ';'.join(f'{n}@{b}={v}' for n, b, v in cfg['auto'])]
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        if r.returncode != 0:
            print(pid, 'FAILED', r.stdout, r.stderr)
            continue

        # One gain for both files: the louder of the two peaks lands at -1 dBFS.
        dp, dr = levels(dry_wav)
        wp, wr = levels(wet_wav)
        gain = -1.0 - max(dp, wp)
        for src, name in ((dry_wav, f'{pid}-dry'), (wet_wav, f'{pid}-wet')):
            subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', src, '-af', f'volume={gain:.2f}dB',
                            '-c:a', 'libmp3lame', '-b:a', '192k', os.path.join(args.out, name + '.mp3')], check=True)
        report[pid] = dict(dry_peak=round(dp, 1), dry_rms=round(dr, 1), wet_peak=round(wp, 1), wet_rms=round(wr, 1), gain_db=round(gain, 1))
        print(f'{pid:16s} dry {dp:6.1f}/{dr:6.1f}  wet {wp:6.1f}/{wr:6.1f} dBFS (peak/rms)  gain {gain:+.1f} dB')

    with open(os.path.join(args.out, 'levels.json'), 'w') as f:
        json.dump(report, f, indent=1)


if __name__ == '__main__':
    main()
