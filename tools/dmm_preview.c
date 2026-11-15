// Host preview of the multimeter screen: runs the real UI drawing code into a
// framebuffer and writes one PPM per screen state.
//
//   make dmm-preview      -> build/dmm-preview/*.ppm

#include <stdio.h>
#include <string.h>

#define __asm__
#define volatile(...)
#define dmm_current_mode dmm_wire_mode
#include "../src/dmm.c"
#undef dmm_current_mode
#undef volatile
#undef __asm__
#include "../src/ui.c"
#include "../src/font.c"
#include "../src/gfx.c"
#include "../src/ui_fonts.c"

static uint16_t fb[LCD_WIDTH * LCD_HEIGHT];

uint8_t lcd_collecting(void) { return 0; }
void lcd_mark_dirty(uint16_t x, uint16_t y, uint16_t w, uint16_t h) { (void)x; (void)y; (void)w; (void)h; }
void lcd_pixel(int16_t x, int16_t y, uint16_t c) {
    if (x >= 0 && y >= 0 && x < (int16_t)LCD_WIDTH && y < (int16_t)LCD_HEIGHT) fb[y * LCD_WIDTH + x] = c;
}
void lcd_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
    for (uint32_t j = y; j < (uint32_t)y + h && j < LCD_HEIGHT; ++j)
        for (uint32_t i = x; i < (uint32_t)x + w && i < LCD_WIDTH; ++i) fb[j * LCD_WIDTH + i] = c;
}
void lcd_fill(uint16_t c) { lcd_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, c); }
void lcd_frame(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
    lcd_rect(x, y, w, 1, c); lcd_rect(x, (uint16_t)(y + h - 1), w, 1, c);
    lcd_rect(x, y, 1, h, c); lcd_rect((uint16_t)(x + w - 1), y, 1, h, c);
}
void lcd_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) { gfx_line(x0, y0, x1, y1, 1, c, c); }
void lcd_text(uint16_t x, uint16_t y, const char *t, uint16_t fg, uint16_t bg, uint8_t s) {
    (void)x; (void)y; (void)t; (void)fg; (void)bg; (void)s;
}
void lcd_text_transparent(uint16_t x, uint16_t y, const char *t, uint16_t fg, uint8_t s) {
    (void)x; (void)y; (void)t; (void)fg; (void)s;
}
void lcd_text_center(uint16_t x, uint16_t y, uint16_t w, const char *t, uint16_t fg, uint16_t bg, uint8_t s) {
    (void)x; (void)y; (void)w; (void)t; (void)fg; (void)bg; (void)s;
}
uint16_t lcd_text_width(const char *t, uint8_t s) { return (uint16_t)(strlen(t) * 6u * s); }
void lcd_render(lcd_render_fn_t fn, void *ctx) { fn(ctx); }

static uint8_t batt_pct = 87;
uint8_t battery_percent(void) { return batt_pct; }
uint16_t battery_millivolts(void) { return 3950; }
static uint8_t charging;
uint8_t battery_is_charging(void) { return charging; }
uint8_t usb_cdc_is_configured(void) { return charging; }

uint32_t load_counter_read(void) { return 0; }
uint32_t load_counter_elapsed(uint32_t start, uint32_t end) { return end - start; }
void fw_update_status(fw_update_status_t *status) { memset(status, 0, sizeof(*status)); }
uint8_t arb_file_count(void) { return 0; }
const char *arb_file_name(uint8_t index) { (void)index; return ""; }

static void save(const char *name) {
    char path[128];
    snprintf(path, sizeof(path), "build/dmm-preview/%s.ppm", name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    fprintf(f, "P6\n%u %u\n255\n", LCD_WIDTH, LCD_HEIGHT);
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; ++i) {
        uint16_t c = fb[i];
        uint8_t rgb[3] = {(uint8_t)(((c >> 11) & 31) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63),
                          (uint8_t)((c & 31) * 255 / 31)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

static void feed(uint8_t mode, const char digits[4], unsigned dots, uint8_t prefix, uint8_t fb_flags, uint8_t fc_flags) {
    uint8_t segments[4], frame[9];
    for (unsigned i = 0; i < 4; ++i) {
        const char *p = strchr(dmm_segment_chars, digits[i]);
        segments[i] = dmm_segment_codes[p - dmm_segment_chars];
        if (dots & (1u << i)) segments[i] |= 0x10;
    }
    frame[0] = 0x5A; frame[1] = 0xA5;
    frame[2] = segments[0] & 0xF0;
    frame[3] = (segments[1] & 0xF0) | (segments[0] & 0x0F);
    frame[4] = (segments[2] & 0xF0) | (segments[1] & 0x0F);
    frame[5] = (segments[3] & 0xF0) | (segments[2] & 0x0F);
    frame[6] = (segments[3] & 0x0F) | prefix;
    frame[7] = fb_flags; frame[8] = fc_flags;
    dmm_wire_mode = mode;
    parse_frame(frame);
    dmm_stats_update();
}

static void enter(uint8_t mode) {
    dmm_wire_mode = ui.dmm_mode = mode;
    ui.mode = UI_MODE_DMM;
    ui.overlay = UI_OVERLAY_NONE;
    ui.auto_range = 1;
    dmm_valid = dmm_synthetic_valid = dmm_numeric_valid = 0;
    dmm_hold_active = dmm_rel_active = 0;
    dmm_live_wire_alert = 0;
    dmm_stats_reset();
}

static void shot(const char *name) {
    draw_dmm();
    save(name);
}

int main(void) {
    enter(DMM_MODE_DCV);
    feed(1, "1230", 4, 0, 0, 2); feed(1, "1238", 4, 0, 0, 2); feed(1, "1235", 4, 0, 0, 2);
    shot("01-dc-voltage");

    enter(DMM_MODE_RES);
    feed(3, "0998", 2, 0x40, 0x20, 0);
    dmm_toggle_relative();
    feed(3, "1201", 2, 0x40, 0x20, 0); feed(3, "1210", 2, 0x40, 0x20, 0); feed(3, "1204", 2, 0x40, 0x20, 0);
    shot("02-resistance-rel");

    enter(DMM_MODE_DIODE);
    feed(4, "0004", 8, 0, 0x20, 0);
    charging = 1;
    shot("03-continuity");
    charging = 0;

    enter(DMM_MODE_ACV);
    feed(2, "2298", 8, 0, 0, 2); feed(2, "2331", 8, 0, 0, 2); feed(2, "2317", 8, 0, 0, 2);
    dmm_toggle_hold();
    shot("04-ac-voltage-hold-high");

    enter(DMM_MODE_LIVE);
    ui_set_live_wire_detected(1);
    shot("05-live-wire");

    enter(DMM_MODE_LIVE);
    shot("06-live-idle");

    enter(DMM_MODE_DC_LO_CURR);
    feed(12, "1234", 2, 0, 1, 1);
    batt_pct = 9;
    shot("07-dc-current-low");
    batt_pct = 87;

    enter(DMM_MODE_RES);
    feed(3, "00L0", 4, 0, 0x24, 0);
    shot("08-resistance-open");

    enter(DMM_MODE_DCV);
    shot("09-no-reading");

    ui.overlay = UI_OVERLAY_MODE_MENU;
    for (uint8_t i = 0; i < 4u; ++i) {
        char name[32];
        ui.menu_index = i;
        draw_mode_menu();
        snprintf(name, sizeof(name), "menu-%u", (unsigned)i + 1u);
        save(name);
    }
    ui.overlay = UI_OVERLAY_NONE;
    return 0;
}
