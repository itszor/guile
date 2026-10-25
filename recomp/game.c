/* Guile (Dream Software; by Andrew Martin, a Softword publication): what libroger needs
 * to know about the game.
 *
 * The disc keeps the game compressed in Computer Concepts' CFS; ../game is
 * it decompressed (../tools/images.py game). Its !Run, from the CFS
 * directory:
 *
 *   SetMacro Ace$Path <Obey$Dir>.
 *   RMLoad Ace:DATA.Audio1          the sound modules (John Bell's voice
 *   RMLoad Ace:DATA.Audio2          generators and samples)
 *   RMLoad Ace:DATA.RMAceFont       the font, put in place by *AceFont
 *   Wimpslot -min 320K -max 320K
 *   Run Ace:!Setup                  claims 160K of screen memory
 *   Dir <Obey$Dir>
 *   Run Ace:Guile                   the game, an absolute at &8000
 *
 * The program issues *AceFont itself. All four images are translated
 * (gen/, ../tools/images.py), the modules at the RMA addresses below. */
#include <roger.h>

extern const struct roger_image recomp_image, recomp_audio1_image, recomp_audio2_image,
    recomp_acefont_image;                       /* gen/<image>/entries.c */

static const struct roger_module modules[] = {
    {"../game/!Guile/DATA/Audio1,fffffa41-1934d691", &recomp_audio1_image, 0},      /* &1800000 */
    {"../game/!Guile/DATA/Audio2,fffffa41-19b285c9", &recomp_audio2_image, 0},      /* &1810000 */
    {"../game/!Guile/DATA/RMAceFont,fffffa43-00485a17", &recomp_acefont_image, 0},  /* &1820000 */
    {NULL, NULL, 0}};

/* Headless scenario: the title and the story run on their own; a click
 * starts the game; then keypad 4 and 6 held, and Space (the ship's
 * computer: "No drones"). The game reads the buttons with OS_Mouse, which a scenario
 * click (event 10 only) doesn't reach, so the click is Select added to
 * what OS_Mouse returns, for 2M instructions. */
#define CLICK_FROM 400000000ull
#define CLICK_TO   402000000ull

static const struct roger_key keys[] = {
    {123, 440000000, 470000000},                    /* keypad 4 */
    {27, 480000000, 520000000},                     /* keypad 6 */
    {99, 530000000, 531000000},                     /* Space */
};

static int scenario_click(uint32_t n, int x)
{
    static int busy;
    if ((n & ~0x20000u) != 0x1C || busy || roger_interactive() || icount < CLICK_FROM || icount >= CLICK_TO)
        return 0;
    busy = 1;
    roger_swi(n);                                   /* OS_Mouse */
    busy = 0;
    R[2] |= 4;
    return 1;
}

static const char *const vars[] = {"Ace$Path", "!Guile.", NULL};
static const char *const prefixes[] = {"<Obey$Dir>.", NULL};

static const struct roger_config config = {
    .title = "Guile (recompiled)",
    .image_path = "../game/!Guile/guile,fffff844-43fbdff1",
    .image = &recomp_image,
    .modules = modules,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = vars,
    .directory = "!Guile",
    .command_line = "Ace:Guile",
    .initial_mode = 12,                 /* the desktop's: the game sets its own */
    .wimpslot = 320 * 1024,             /* !Run's Wimpslot -min 320K -max 320K */
    .screen_size = 160 * 1024,          /* !Setup's */
    .swi = scenario_click,
    .keys = keys,
    .nkeys = sizeof keys / sizeof *keys,
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
