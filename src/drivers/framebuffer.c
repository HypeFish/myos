#include "framebuffer.h"
#include "font.h"       // Includes font_8x16.h and defines FONT_WIDTH/HEIGHT
#include "string.h"     // For memcpy and memset
#include <stddef.h>     // For NULL

// --- Framebuffer State ---
static struct limine_framebuffer* fb;
static uint32_t* fb_addr;
static uint64_t pitch;

// --- Screen & Font Geometry ---
static int fb_width, fb_height;     // Screen dimensions in pixels
static int char_width, char_height; // Character dimensions in pixels (now 8x16)
static int cols, rows;              // Screen dimensions in characters
static int cursor_x, cursor_y;      // Cursor position in characters (e.g., 0,0)
static uint32_t color = 0xFFFFFFFF; // Default to white
static uint32_t font_scale = 1;     // Store the current font scale
#define TAB_WIDTH 4           

/**
 * @brief Plots a single pixel.
 */
static void fb_plot_pixel(int x, int y, uint32_t c) {
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) {
        return;
    }
    fb_addr[y * (pitch / 4) + x] = c;
}

/**
 * @brief Draws a solid black rectangle at a given character position.
 */
static void fb_clear_char_at(int cx, int cy) {
    int pixel_x = cx * char_width;
    int pixel_y = cy * char_height;

    // Loop for the scaled height and width of the character
    for (int y = 0; y < char_height; y++) {
        for (int x = 0; x < char_width; x++) {
            fb_plot_pixel(pixel_x + x, pixel_y + y, 0x00000000); // 0 = black
        }
    }
}

/**
 * @brief Recalculates screen geometry based on the font size and scale.
 */
static void fb_update_metrics(void) {
    char_width = FONT_WIDTH * font_scale;
    char_height = FONT_HEIGHT * font_scale;
    cols = fb_width / char_width;
    rows = fb_height / char_height;
}

/**
 * @brief Scrolls the entire screen up by one character row.
 */
static void fb_scroll(void) {
    // Calculate the size of one scaled character line in bytes
    size_t line_size = char_height * pitch;
    // Calculate the size of the block to move (all lines except the first)
    size_t move_size = (rows - 1) * line_size;

    // Source: The beginning of the second line
    void* src = (void*)(fb_addr + char_height * (pitch / 4));
    // Destination: The beginning of the framebuffer
    void* dest = (void*)fb_addr;

    memcpy(dest, src, move_size);

    // Pointer to the beginning of the last line
    void* last_line = (void*)(fb_addr + (rows - 1) * (pitch / 4) * char_height);
    // Clear the last line to black
    memset(last_line, 0, line_size);

    cursor_y = rows - 1;
    cursor_x = 0;
}


// --- Public Functions ---

void fb_init(struct limine_framebuffer* fb_info) {
    if (fb_info == NULL) {
        return;
    }

    fb = fb_info;
    fb_addr = fb->address;
    pitch = fb->pitch;
    fb_width = fb->width;
    fb_height = fb->height;

    font_scale = 2; // Default to 1x scale on init
    fb_update_metrics();
    fb_clear();
}

void fb_clear(void) {
    memset(fb_addr, 0, fb_height * pitch);
    cursor_x = 0;
    cursor_y = 0;
}

void fb_set_color(uint32_t c) {
    color = c;
}


/**
 * @brief Prints a single character at the cursor (and scrolls).
 */
void fb_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\b') { // <-- ADD THIS ELSE-IF BLOCK
        // Handle backspace
        if (cursor_x > 0) {
            cursor_x--;
            fb_clear_char_at(cursor_x, cursor_y);

            // This is optional, but nice:
            // Clear the character at the new cursor position
            // (We just re-draw a space)
            // Note: This is a bit of a hack. A better way would be
            // to have a fb_draw_char_at() function.
            // For now, let's just move the cursor.

            // To properly "erase", you'd re-draw the character
            // at the new (cursor_x, cursor_y) with a black background.
            // For a transparent background, just moving back is fine.
        }
    }
    else if (c == '\t') { // <-- ADD THIS ELSE-IF BLOCK
        // Handle tab
        cursor_x = (cursor_x + TAB_WIDTH) & ~(TAB_WIDTH - 1);
        // This is a bitwise trick equivalent to:
        // cursor_x = cursor_x + (TAB_WIDTH - (cursor_x % TAB_WIDTH));
    }
    else if (c >= 32 && c < 128) { // Only print valid ASCII

        // Get the glyph data from the FONT_DATA array
        // Each char is FONT_HEIGHT (16) bytes
        const unsigned char* glyph = FONT_DATA + ((int)c * FONT_HEIGHT);

        // Calculate the character's top-left pixel coordinate
        int pixel_x = cursor_x * char_width;
        int pixel_y = cursor_y * char_height;

        // Draw the character with scaling
        for (int i = 0; i < FONT_HEIGHT; i++) { // Glyph row (i = 0..15)
            for (int j = 0; j < FONT_WIDTH; j++) { // Glyph col (j = 0..7)

                // Check if the font's pixel bit is set
                // (1 << (FONT_WIDTH - 1 - j)) checks bits from left-to-right
                if ((glyph[i] & (1 << (FONT_WIDTH - 1 - j)))) {

                    // It is set, so draw a scaled rectangle
                    for (uint32_t dy = 0; dy < font_scale; dy++) {
                        for (uint32_t dx = 0; dx < font_scale; dx++) {
                            fb_plot_pixel(pixel_x + (j * font_scale) + dx,
                                pixel_y + (i * font_scale) + dy,
                                color);
                        }
                    }
                }
                // If bit is 0, we do nothing (transparent background)
            }
        }

        // Advance the character cursor
        cursor_x++;
    }

    // Handle line wrapping
    if (cursor_x >= cols) {
        cursor_x = 0;
        cursor_y++;
    }

    // Handle scrolling
    if (cursor_y >= rows) {
        fb_scroll();
    }
}

void fb_set_scale(uint32_t new_scale) {
    if (new_scale == 0) {
        return; // Invalid scale
    }
    font_scale = new_scale;
    // Recalculate all screen metrics based on the new scale
    fb_update_metrics();
    cursor_x = 0;
    cursor_y = 0;
}

/**
 * @brief Prints a null-terminated string.
 */
void fb_print(const char* s) {
    if (fb == NULL) return;

    while (*s) {
        fb_putchar(*s++);
    }
}

/**
 * @brief Sets the cursor position.
 */
void fb_set_cursor(uint32_t x, uint32_t y) {
    if (x < (uint32_t)cols && y < (uint32_t)rows) {
        cursor_x = x;
        cursor_y = y;
    }
}

int fb_get_cursor_x(void) {
    return cursor_x;
}

int fb_get_cursor_y(void) {
    return cursor_y;
}