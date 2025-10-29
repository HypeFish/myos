#ifndef __FONT_H__
#define __FONT_H__

// This includes the actual font data array (vga_font_8x16)
#include "font_8x16.h"

// Define the font's dimensions for the rest of the kernel
#define FONT_WIDTH  8
#define FONT_HEIGHT 16
#define FONT_DATA   fontGlyphBuffer 

#endif // __FONT_H__
