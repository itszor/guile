# Static recompilation of Guile: the program (gen/guile) and the three
# modules !Run loads (gen/audio1, gen/audio2, gen/acefont), from superspider
# gen via ../tools/images.py, and game.c, linked with libroger (the RISC OS
# runtime).
ROGER   ?= $(HOME)/stuff/libroger
CC      ?= gcc
CFLAGS  ?= -O2 -g -w -fno-strict-aliasing $(if $(TRACE),-DRECOMP_TRACE)
GEN     := $(wildcard gen/*/code*.c) $(wildcard gen/*/entries.c)
OBJ     := $(GEN:.c=.o) game.o
LIBS    := $(shell pkg-config --libs sdl3) -ljpeg -lm -lpthread
INC     := -I$(ROGER)/include

guile: $(OBJ) $(ROGER)/libroger.a
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(ROGER)/libroger.a $(LIBS)

$(ROGER)/libroger.a: FORCE
	$(MAKE) -C $(ROGER) libroger.a

gen/%.o: gen/%.c $(ROGER)/include/roger_cpu.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

game.o: game.c $(ROGER)/include/roger.h
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# Coverage build (~/stuff/superspider/phase0): "make cov" builds guile-cov
# from separate *.cov.o objects; it writes $$RECOMP_COV (default cov.out).
COVDIR  ?= $(HOME)/stuff/superspider/phase0/cov
COVOBJ  := $(OBJ:.o=.cov.o) cov.cov.o

cov: guile-cov

guile-cov: $(COVOBJ) FORCE
	$(MAKE) -C $(ROGER) cov COV=$(COVDIR)
	$(CC) $(CFLAGS) -o $@ $(COVOBJ) $(ROGER)/libroger-cov.a $(LIBS)

%.cov.o: %.c $(ROGER)/include/roger_cpu.h $(COVDIR)/cov.h
	$(CC) $(CFLAGS) -DRECOMP_COV $(INC) -I$(COVDIR) -c -o $@ $<

cov.cov.o: $(COVDIR)/cov.c $(COVDIR)/cov.h
	$(CC) $(CFLAGS) -c -o $@ $<

# ../game/!Guile: the game's files, decompressed from the disc's CFS
# (../extracted, adf_extract.py's tree of it) by ../tools/unpack
game:
	python3 -I ../tools/images.py game

regen:
	python3 -I ../tools/images.py analyse
	python3 -I ../tools/images.py gen

# "make roots" merges the gap entries a --trace-gaps run logged
# (untranslated.log, or LOG=...) into ../analysis/<image>.roots, so the next
# "make regen" translates them.
LOG ?= untranslated.log

roots:
	python3 -I ../tools/images.py roots $(LOG)

clean:
	rm -f gen/*/*.o game.o guile guile-cov *.cov.o gen/*/*.cov.o

.PHONY: FORCE cov game regen roots clean
