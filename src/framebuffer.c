#include "framebuffer.h"
#include "font.h"       // Our 8x8 font data
#include "string.h"     // For memcpy and memset
#include <stddef.h>     // For NULL

// --- Framebuffer State ---
static struct limine_framebuffer *fb;
static uint32_t *fb_addr;
static uint64_t pitch;

// --- Screen & Font Geometry ---
static int fb_width, fb_height;     // Screen dimensions in pixels
static int char_width, char_height; // Character dimensions in pixels (scaled)
static int cols, rows;              // Screen dimensions in characters
static int cursor_x, cursor_y;      // Cursor position in characters (e.g., 0,0)
static uint32_t color = 0xFFFFFFFF; // Default to white
static uint32_t scale = 1;          // Default to 1x scale

/**
 * @brief Plots a single pixel. (This is the only function that touches fb_addr).
 */
static void fb_plot_pixel(int x, int y, uint32_t c) {
    // Check boundaries
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height) {
        return;
    }
    // Calculate the pixel's address
    fb_addr[y * (pitch / 4) + x] = c;
}

/**
 * @brief Recalculates screen geometry based on the current scale.
 */
static void fb_update_metrics(void) {
    char_width = 128 * scale;
    char_height = 16 * scale;
    cols = fb_width / char_width;
    rows = fb_height / char_height;
}

/**
 * @brief Scrolls the entire screen up by one character row.
 */
static void fb_scroll(void) {
    // Calculate size of one character row in bytes
    size_t line_size = char_height * pitch;
    
    // Calculate total size to move (all rows except the top one)
    size_t move_size = (rows - 1) * line_size;
    
    // Destination is the top of the screen
    void *dest = (void *)fb_addr;
    // Source is the second row
    void *src = (void *)(fb_addr + char_height * (pitch / 4));
    
    // Copy the data up
    memcpy(dest, src, move_size);
    
    // Find the address of the last (newly blank) row
    void *last_line = (void *)(fb_addr + (rows - 1) * line_size / 4);
    
    // Clear the last row
    memset(last_line, 0, line_size);
    
    // Reset cursor to the beginning of the new blank line
    cursor_y = rows - 1;
    cursor_x = 0;
}


// --- Public Functions ---

void fb_init(struct limine_framebuffer *fb_info) {
    if (fb_info == NULL) {
        return; // No framebuffer
    }
    
    fb = fb_info;
    fb_addr = fb->address;
    pitch = fb->pitch;
    fb_width = fb->width;
    fb_height = fb->height;
    
    scale = 1; // Default scale
    fb_update_metrics();
    
    fb_clear();
}

void fb_clear(void) {
    // Fill the entire framebuffer with 0 (black)
    memset(fb_addr, 0, fb_height * pitch);
    cursor_x = 0;
    cursor_y = 0;
}

void fb_set_color(uint32_t c) {
    // Color is stored in 0xAARRGGBB, but we only care about 0xRRGGBB
    color = c;
}

/**
 * @brief Prints a single character at the cursor (and scrolls).
 */
void fb_putchar(char c) {
    if (c == '\n') {
        // Handle newline
        cursor_x = 0;
        cursor_y++;
    } else if (c >= 0 && c < 128) { // Only print valid ASCII
        // Handle printable character
        
        // Get the glyph data from our font array
        const unsigned char *glyph = vga_font_8x16[(int)c];
        
        // Calculate the character's top-left pixel coordinate
        int pixel_x = cursor_x * char_width;
        int pixel_y = cursor_y * char_height;
        
        // Draw the character (scaled)
        for (int i = 0; i < 16; i++) { // Glyph row (0-7)
            for (int j = 0; j < 128; j++) { // Glyph col (0-7)
                // Check if the bit is set in the glyph
                if ((glyph[i] & (1 << (128 - 1 - j)))) {
                    // It is set, draw a scale x scale block
                    for (uint32_t dy = 0; dy < scale; dy++) {
                        for (uint32_t dx = 0; dx < scale; dx++) {
                            fb_plot_pixel(pixel_x + (j * scale) + dx,
                                          pixel_y + (i * scale) + dy,
                                          color);
                        }
                    }
                }
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

/**
 * @brief Prints a null-terminated string.
 */
void fb_print(const char *s) {
    if (fb == NULL) return; // Don't print if not initialized
    
    while (*s) {
        fb_putchar(*s++);
    }
}

