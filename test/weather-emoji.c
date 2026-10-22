/* cc $(pkg-config --cflags fcft pixman-1) test/weather-emoji.c \
 *    -o /tmp/opencode/weather-emoji $(pkg-config --libs fcft pixman-1)
 * Requires the configured Apple Color Emoji font. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <fcft/fcft.h>
#include <pixman.h>

int main(void) {
    fcft_init(FCFT_LOG_COLORIZE_AUTO, false, FCFT_LOG_CLASS_ERROR);
    const char *names[] = {"Ttyp0 OTB:regular:size=11:antialias=false", "Apple Color Emoji:size=11"};
    struct fcft_font_options *options = fcft_font_options_create();
    if (!options) return 1;
    options->emoji_presentation = FCFT_EMOJI_PRESENTATION_EMOJI;
    const uint32_t symbols[] = {0x2600, 0x2601, 0x26c5, 0x26c8, 0x26a1, 0x2744,
        0x1f327, 0x1f328, 0x1f329, 0x1f32b, 0x1f32c, 0x1f506};
    int failed = 0;
    for (unsigned scale = 1; scale <= 2; scale++) {
        char attrs[32];
        snprintf(attrs, sizeof attrs, "dpi=%u", 96 * scale);
        struct fcft_font *font = fcft_from_name2(2, names, attrs, options);
        if (!font) { failed = 1; break; }
        for (size_t i = 0; i < sizeof symbols / sizeof *symbols; i++) {
            const struct fcft_glyph *g = fcft_rasterize_char_utf32(font, symbols[i], FCFT_SUBPIXEL_NONE);
            bool color = g && pixman_image_get_format(g->pix) == PIXMAN_a8r8g8b8;
            printf("%s scale=%u U+%04X color emoji\n", color ? "PASS" : "FAIL", scale, symbols[i]);
            if (!color) failed = 1;
        }
        fcft_destroy(font);
    }
    fcft_font_options_destroy(options);
    fcft_fini();
    return failed;
}
