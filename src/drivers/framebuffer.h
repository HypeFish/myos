#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include <limine.h> // We need the limine_framebuffer struct
#include <stdint.h> // For uint32_t

/**
 * @brief Initializes the framebuffer console.
 * @param fb_info A pointer to the limine_framebuffer struct.
 */
void fb_init(struct limine_framebuffer* fb_info);

/**
 * @brief Clears the screen to black.
 */
void fb_clear(void);

/**
 * @brief Puts a single character on the screen.
 * @param c The character to print.
 */
void fb_putchar(char c);

/**
 * @brief Prints a null-terminated string to the screen.
 * @param s The string to print.
 */
void fb_print(const char* s);

/**
 * @brief Sets the text color.
 * @param c 32-bit color in 0xRRGGBB format.
 */
void fb_set_color(uint32_t c);

/**
 * @brief Sets the font scaling factor.
 * @param new_scale The multiplier (e.g., 2 for 2x size).
 */
void fb_set_scale(uint32_t new_scale);

/**
 * @brief Sets the cursor position.
 * @param x The x position (in characters).
 * @param y The y position (in characters).
 */
void fb_set_cursor(uint32_t x, uint32_t y);

/**
 * @brief Gets the current cursor X position.
 * @return The cursor X position (in characters).
 */
int fb_get_cursor_x(void);

/**
 * @brief Gets the current cursor Y position.
 * @return The cursor Y position (in characters).
 */
int fb_get_cursor_y(void);

#endif // __FRAMEBUFFER_H__

