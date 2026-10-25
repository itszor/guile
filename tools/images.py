#!/usr/bin/env python3
"""Guile's files, from the disc.

    images.py game     extracted/disc (the disc's files, as weave.py extracts it) ->
                       game/!Guile, the application as the CFS filing
                       system serves it: each file under $.!Guile.£.!Guile
                       decompressed by tools/unpack (CfsModule's own decoder),
                       named with its load and exec addresses

The disc is a menu disc of the time (Dream Software's): its !Guile only
loads CfsModule, makes the disc a CFS one (*NewCfs) and opens the directory
£ in it. Everything in there is CFS-compressed (filetype &D96); the game is
£.!Guile, the rest (!Spark, !SparkFS, !ArcFSr.w, !FlipTop, !Modoff) are
utilities that came on the disc. CfsModule itself is $.!Cfs.CfsModule.

    images.py analyse  SuperSpider's analysis of each image (below), into
                       analysis/<name>.json; analysis/<name>.roots, if there,
                       adds entry points
    images.py gen      its C translation, into recomp/gen/<name>
    images.py roots LOG...
                       the gap entries --trace-gaps runs logged (libroger's
                       untranslated.log) merged into analysis/<name>.roots

The images: the program (Guile, an absolute at &8000) and the three
modules !Run RMLoads, each translated at the RMA address recomp/game.c
loads it to.
"""
import os
import shutil
import subprocess
import sys

TOP = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXTRACTED = os.path.join(TOP, 'extracted', 'disc')
GAME = os.path.join(TOP, 'game', '!Guile')
UNPACK = os.path.join(TOP, 'tools', 'unpack')
SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))
SSBIN = os.path.join(SS, '_build/default/bin/main.exe')
AN = os.path.join(TOP, 'analysis')

# (name, file in game/!Guile, base, module, C prefix, table prefix, files);
# the modules' bases are game.c's AUDIO1, AUDIO2, ACEFONT
IMAGES = [('guile', 'guile', 0x8000, False, 'F', 'recomp', 8),
          ('audio1', 'DATA/Audio1', 0x1800000, True, 'A', 'recomp_audio1', 1),
          ('audio2', 'DATA/Audio2', 0x1810000, True, 'B', 'recomp_audio2', 1),
          ('acefont', 'DATA/RMAceFont', 0x1820000, True, 'C', 'recomp_acefont', 1)]


def find(directory, name):
    """directory's file name, whatever its ',load-exec' suffix"""
    for f in os.listdir(directory):
        if f.split(',')[0] == name:
            return os.path.join(directory, f)
    raise SystemExit('no %s in %s' % (name, directory))


def game():
    src = os.path.join(EXTRACTED, '!Guile', '£', '!Guile')
    module = find(os.path.join(EXTRACTED, '!Cfs'), 'CfsModule')
    subprocess.run(['make', '-s', '-C', os.path.join(TOP, 'tools')], check=True)
    if os.path.exists(GAME):
        shutil.rmtree(GAME)
    pairs = []
    for root, _, files in os.walk(src):
        out = os.path.join(GAME, os.path.relpath(root, src))
        os.makedirs(out, exist_ok=True)
        for f in sorted(files):
            path = os.path.join(root, f)
            name = f.split(',')[0]
            if f.split(',')[1][3:6] == 'd96':     # CFSlzw
                pairs.append('%s=%s' % (path, os.path.join(out, name)))
            else:                                  # !Sprites: stored as it is
                shutil.copy(path, os.path.join(out, f))
    subprocess.run([UNPACK, '--cfs=' + module] + pairs, check=True)


def image_path(f):
    d, leaf = os.path.split(os.path.join(GAME, f))
    return find(d, leaf)


def run(cmd):
    print('+', ' '.join(cmd), flush=True)
    subprocess.run(cmd, check=True)


def analyse():
    subprocess.run(['dune', 'build'], cwd=SS, check=True)
    os.makedirs(AN, exist_ok=True)
    for name, f, base, module, _, _, _ in IMAGES:
        cmd = [SSBIN, 'analyse', image_path(f), '%x' % base, '--full',
               '-o', os.path.join(AN, name + '.json')]
        rf = os.path.join(AN, name + '.roots')
        if os.path.exists(rf):
            cmd += ['--roots', rf]
        if module:
            cmd += ['--module']
        run(cmd)


def gen():
    for k, (name, f, base, module, prefix, tab, files) in enumerate(IMAGES):
        out = os.path.join(TOP, 'recomp', 'gen', name)
        if os.path.exists(out):
            shutil.rmtree(out)
        run([SSBIN, 'gen', image_path(f), '%x' % base, os.path.join(AN, name + '.json'),
             '--c', out, '--prefix=' + prefix, '--tabprefix=' + tab, '--files=%d' % files]
            + (['--covtag=%s' % chr(ord('A') + k)] if k else []))


def roots(logs):
    for name, f, base, _, _, _, _ in IMAGES:
        run([sys.executable, '-I', os.path.join(SS, 'tools/gap_roots.py'), image_path(f),
             '%x' % base, os.path.join(AN, name + '.roots')] + logs)


if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else ''
    if cmd == 'roots':
        roots(sys.argv[2:] or ['untranslated.log'])
    elif cmd in ('game', 'analyse', 'gen') and len(sys.argv) == 2:
        {'game': game, 'analyse': analyse, 'gen': gen}[cmd]()
    else:
        raise SystemExit(__doc__)
