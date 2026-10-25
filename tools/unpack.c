/* Computer Concepts' CFS files (filetype &D96, CFSlzw) decompressed by
 * CfsModule's own decoder, run by libroger's ARM2 interpreter: nothing is
 * translated, every instruction is interpreted.
 *
 *   unpack --cfs=MODULE IN=OUT...
 *       decompress each IN (a CFS file) with CfsModule 1.17's decoder,
 *       writing it to OUT,llllllll-eeeeeeee (its load and exec addresses,
 *       as adf_extract.py names files). The disc keeps the whole game
 *       compressed this way, served through the CFS filing system (!Run:
 *       NEWCFS, Guile$Path CFS#...).
 *
 * The decoder (&47FC in the module) reads the file from its first byte -
 * the length, then tagged records: 3 the attributes, load and exec
 * addresses, 1 an LZW block, 0 the end - through two routines whose
 * addresses are in its workspace (R12):
 *   +&20 input:  R0 &FF start, 0 more; returns R1..R2 the bytes there are,
 *                C set when there are none
 *   +&24 output: R0 &FF start (returns R10 where to write, R11 the limit),
 *                0 or 2 R10 is where it has written to, 1 [R10, R11) to be
 *                appended (then R10 where to carry on); flags kept
 * Its caller for a whole file loaded into memory (&309C) passes a pair
 * that do this, reading the file through CFS's own buffering; here they
 * are the hooks below, with the whole file in memory. The decoder's
 * dictionary is in its workspace (+&1C00) and the workspace keeps the
 * length (+&C), attributes (+&44), load and exec (+&48, +&4C). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <roger.h>

#define MODBASE 0x10000                 /* CfsModule's code */
#define STACK   0x3F000                 /* SVC stack, downwards */
#define WS      0x40000                 /* its workspace (R12) */
#define WSSIZE  0x20000
#define INBUF   0x80000                 /* the compressed file */
#define OUTBUF  0x200000                /* the decompressed file */
#define TOP     0x800000
#define DECODE  0x47FC                  /* the decoder, in the module */
#define CB_IN   0x7F00                  /* the two routines: hooks */
#define CB_OUT  0x7F04

static uint8_t nocode[TOP / 32];
static struct roger_image image = {0x8000, TOP, NULL, 0, nocode, 'P'};

static const char *module;
static const char *pairs[256];
static int npairs;
static uint32_t in_end, out_pos;

static void load(const char *path, uint32_t a, uint32_t max, uint32_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    size_t n = fread(MEM + a, 1, max, f);
    if (!feof(f)) { fprintf(stderr, "%s: too big\n", path); exit(1); }
    fclose(f);
    *len = (uint32_t)n;
}

static int hooks(uint32_t a)
{
    if (a == CB_IN) {
        if (R[0] == 0xFF) R[1] = INBUF;
        else if (R[0] == 0) R[1] = in_end;      /* all of it was there at the start */
        else { fprintf(stderr, "input routine: R0 %u\n", R[0]); exit(1); }
        R[2] = in_end;
        NF = 0; ZF = R[1] == R[2]; CF = R[1] >= R[2]; VF = 0;
    } else if (a == CB_OUT) {
        switch (R[0]) {
        case 0xFF: R[10] = out_pos; R[11] = 0xFFFFFFFF; break;
        case 0: case 2: out_pos = R[10]; break;
        case 1:
            memmove(MEM + out_pos, MEM + R[10], R[11] - R[10]);
            out_pos += R[11] - R[10];
            R[10] = out_pos; R[11] = 0xFFFFFFFF;
            break;
        default: fprintf(stderr, "output routine: R0 %u\n", R[0]); exit(1);
        }
        if (out_pos >= TOP) { fprintf(stderr, "output too big\n"); exit(1); }
    } else return 0;
    npc = R[14] & 0x03FFFFFC;
    return 1;
}

static void cfs_all(void)
{
    uint32_t len;
    load(module, MODBASE, WS - MODBASE - 0x1000, &len);
    for (int k = 0; k < npairs; k++) {
        const char *eq = strchr(pairs[k], '=');
        char in[1024], out[1100];
        snprintf(in, sizeof in, "%.*s", (int)(eq - pairs[k]), pairs[k]);
        memset(MEM + INBUF, 0, OUTBUF - INBUF);
        load(in, INBUF, OUTBUF - INBUF - 4, &len);
        memset(MEM + WS, 0, WSSIZE);
        in_end = INBUF + len;
        out_pos = OUTBUF;
        R[12] = WS;
        R[13] = STACK;
        MEM[WS + 0x20] = CB_IN & 0xFF; MEM[WS + 0x21] = CB_IN >> 8;
        MEM[WS + 0x24] = CB_OUT & 0xFF; MEM[WS + 0x25] = CB_OUT >> 8;
        roger_call(MODBASE + DECODE, ROGER_MODE_SVC, ROGER_HANDLER_RET);
        if (VF) { fprintf(stderr, "%s: error %s\n", in, (char *)MEM + R[0] + 4); exit(1); }
        uint32_t want, ld, ex;
        memcpy(&want, MEM + WS + 0xC, 4);
        memcpy(&ld, MEM + WS + 0x48, 4);
        memcpy(&ex, MEM + WS + 0x4C, 4);
        if (out_pos - OUTBUF != want) {
            fprintf(stderr, "%s: %u bytes, the header says %u\n", in, out_pos - OUTBUF, want);
            exit(1);
        }
        snprintf(out, sizeof out, "%s,%08x-%08x", eq + 1, ld, ex);
        FILE *f = fopen(out, "wb");
        if (!f) { perror(out); exit(1); }
        fwrite(MEM + OUTBUF, 1, want, f);
        fclose(f);
        printf("%-60s %7u bytes\n", out, want);
    }
    exit(0);
}

static struct roger_config config = {
    .title = "unpack",
    .image = &image,
    .appdir = ".",
    .initial_mode = 13,
    .wimpslot = TOP - 0x8000,
    .interpret_gaps = 1,
    .call_addr = hooks,
    .init = cfs_all,
    .usage = "  --cfs=MODULE IN=OUT...\n",
};

int main(int argc, char **argv)
{
    char *args[8] = {argv[0], "--headless"};
    for (int k = 1; k < argc; k++) {
        if (!strncmp(argv[k], "--cfs=", 6)) module = argv[k] + 6;
        else if (strchr(argv[k], '=') && npairs < 256) pairs[npairs++] = argv[k];
        else { fprintf(stderr, "unpack --cfs=MODULE IN=OUT...\n"); return 1; }
    }
    if (!module || !npairs) { fprintf(stderr, "unpack --cfs=MODULE IN=OUT...\n"); return 1; }
    args[2] = "/dev/null";
    roger_main(3, args, &config);
}
