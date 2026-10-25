Guile (Dream Software; by Andrew Martin, a Softword publication) - static recompilation
=====================================================================================

The input is the TOSEC dump, `Guile (19xx)(Dream Software).zip`: one ADFS
E disc. It is a menu disc: its !Guile loads Computer Concepts' CfsModule
(1.17, in $.!Cfs), makes the disc a CFS one (*NewCfs) and opens the
directory `£` in it, where everything is CFS-compressed (filetype &D96).
The game is `£.!Guile`; the rest of `£` (!Spark, !SparkFS, !ArcFSr.w,
!FlipTop, !Modoff) are utilities that came on the disc and aren't used.
The sound modules are dated August 1988 (John Bell), the font module 1991.

A recipe: this repository holds no game data. It rebuilds the game from
your own copy, recognised by content (recipe.json):

    ./build.sh "Guile (19xx)(Dream Software).zip"
    cd recomp && ./guile

build.sh runs recipe.json's steps with SuperSpider's tools/weave.py
(~/stuff/superspider, or $SUPERSPIDER): it extracts the disc to
extracted/disc, decompresses the game to game/!Guile (libroger's appdir),
then analyses, translates and links. Needs python3, OCaml/dune, gcc and
SDL3.

    tools/unpack.c      CfsModule's own decoder run by libroger's interpreter:
                        each CFS file -> the file, named with the load and exec
                        addresses its header keeps
    tools/images.py     `game`: game/!Guile from extracted/disc (with unpack);
                        `analyse`, `gen`, `roots`: SuperSpider on each image
    analysis/           SuperSpider's output, and roots files if any
    recomp/             make game, make regen (analysis + gen), make, ./guile

What runs
---------

!Run (decompressed) RMLoads Audio1, Audio2 and RMAceFont from DATA, sets
a 320K WimpSlot, runs !Setup (a utility claiming 160K of screen memory)
and runs Guile, an absolute at &8000 (307K, about 40K of it code). game.c
says the same to libroger: the three modules, translated at &1800000,
&1810000 and &1820000; the program translated at &8000; Ace$Path; 160K of
screen memory.

The program's first word branches past a decrypting stub (XOR &4C), whose
bytes are already plaintext: the dump's copy had it decrypted. The
start-up code wipes itself (46 stores into translated code, &8604-&87CC,
never run again), and the game keeps variables among its code
(PC-relative stores: more such notes once playing). Neither is run as
code afterwards: nothing is interpreted.

libroger needed the interval timer (OS_Word 3 and 4): the game's delays
wait for it to move on, and without it the title waited for ever.

The translation has no gaps so far: headless runs through the title, the
story and play (2.4G instructions with random keys) logged nothing
untranslated, and a 700M-instruction scripted run of the recompiled game
is byte-identical (events, every screenshot) to the same run fully
interpreted.

Running
-------

    cd recomp && ./guile

The title and the story run on their own; click to play (keys didn't
start it in the headless tries). The game is driven with the mouse and the
keys it scans: keypad 1-4, 6, 8, Space, A, S, C, D, E, H, T, Q, Ctrl (see
the ship's computer's Controls). One of them quits: a run pressing them at
random ended in OS_Exit; without Q and Ctrl it didn't.

Headless: `./guile --headless` (or --limit=N) plays the scenario in game.c:
a click once the story is up, keypad 4 and 6 held, Space. The game reads
the buttons with OS_Mouse, which libroger's scenario clicks (event 10)
don't reach, so game.c's swi hook adds Select to OS_Mouse's result for a
moment instead. Without `--full-os`, headless runs don't draw VDU output,
so screenshots of play show the story's text left in the 3D view's far
end; with it, they don't.

Known differences and open questions
------------------------------------

- Only headless runs have been tried here: sound, the real mouse and the
  game's whole course (the six planets, trading, the end) haven't been
  played through. Gaps found that way: `./guile --trace-gaps`, then
  `make roots && make regen && make` (PORTING.md, step 6).
- The keys' meanings are inferred from what the game scans, not from its
  documentation (there's none on the disc).
- $.!Guile.£.!Guile.DATA.checkdisc (empty) and !Run's commented-out
  `Run Ace:ZZVCheck` (not on the disc) suggest a disc check the
  compilation removed; nothing checks the disc now.
