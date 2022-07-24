/*
This is the core graphics library for all our displays, providing a common
set of graphics primitives (points, lines, circles, etc.).  It needs to be
paired with a hardware-specific library for each display device we carry
(to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please
support Adafruit & open-source hardware by purchasing products from Adafruit!

Copyright (c) 2013 Adafruit Industries.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
 */
#include "gfx.h"
#include "glcdfont.c"

// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

// Pointers are a peculiar case...typically 16-bit on AVR boards,
// 32 bits elsewhere.  Try to accommodate both...

#if !defined(__INT_MAX__) || (__INT_MAX__ > 0xFFFF)
#define pgm_read_pointer(addr) ((void *)pgm_read_dword(addr))
#else
#define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#endif

inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
#ifdef __AVR__
  return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
#else
  // expression in __AVR__ section may generate "dereferencing type-punned
  // pointer will break strict-aliasing rules" warning In fact, on other
  // platforms (such as STM32) there is no need to do this pointer magic as
  // program memory may be read in a usual way So expression may be simplified
  return gfxFont->glyph + c;
#endif //__AVR__
}

inline uint8_t *pgm_read_bitmap_ptr(const GFXfont *gfxFont) {
#ifdef __AVR__
  return (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);
#else
  // expression in __AVR__ section generates "dereferencing type-punned pointer
  // will break strict-aliasing rules" warning In fact, on other platforms (such
  // as STM32) there is no need to do this pointer magic as program memory may
  // be read in a usual way So expression may be simplified
  return gfxFont->bitmap;
#endif //__AVR__
}

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

/**
 * getcolour
 *
 * Simply converts RGB to Y1CbY2Cr format
 *
 * I got this from a pastebin, so thanks to whoever originally wrote it!
 */

unsigned int getColor(u8 r1, u8 g1, u8 b1)
{
	int y1, cb1, cr1, y2, cb2, cr2, cb, cr;
	u8 r2, g2, b2;

	r2 = r1;
	g2 = g1;
	b2 = b1;

	y1 = (299 * r1 + 587 * g1 + 114 * b1) / 1000;
	cb1 = (-16874 * r1 - 33126 * g1 + 50000 * b1 + 12800000) / 100000;
	cr1 = (50000 * r1 - 41869 * g1 - 8131 * b1 + 12800000) / 100000;

	y2 = (299 * r2 + 587 * g2 + 114 * b2) / 1000;
	cb2 = (-16874 * r2 - 33126 * g2 + 50000 * b2 + 12800000) / 100000;
	cr2 = (50000 * r2 - 41869 * g2 - 8131 * b2 + 12800000) / 100000;

	cb = (cb1 + cb2) >> 1;
	cr = (cr1 + cr2) >> 1;

	return ((y1 << 24) | (cb << 16) | (y2 << 8) | cr);
}

/****************************************************************************
* ClearScreen
****************************************************************************/
void ClearScreen (){
  VIDEO_ClearFrameBuffer (rmode, __xfb, COL_BLACK);
}

/****************************************************************************
* ShowScreen
****************************************************************************/
void ShowScreen ()
{
	VIDEO_SetNextFramebuffer (__xfb);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
}

/**************************************************************************/
/*!
   @brief    Write a line.  Bresenham's algorithm - thx wikpedia
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int color) {

  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      writePixel(y0, x0, color);
    } else {
      writePixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**************************************************************************/
/*!
   @brief    Write a pixel, overwrite in subclasses if startWrite is defined!
    @param   x   x coordinate
    @param   y   y coordinate
   @param    color 32-bit Y1CbY2Cr
*/
/**************************************************************************/
void writePixel(int16_t x, int16_t y, int color) {
	int val_x = map(x, 0, 640, 0, rmode->fbWidth/2);
	u32 *tmpfb = (u32 *) __xfb;
	tmpfb[val_x + ((rmode->fbWidth/2) * y)] = color;
}

/**************************************************************************/
/*!
   @brief    Write a pixel, overwrite in subclasses if startWrite is defined!
    @param   x   x coordinate
    @param   y   y coordinate
   @param    color 32-bit Y1CbY2Cr
*/
/**************************************************************************/
void writeNativePixel(int16_t x, int16_t y, int color) {
	u32 *tmpfb = (u32 *)  __xfb;
	tmpfb[x + rmode->fbWidth * y ] = color;
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly vertical line (this is often optimized in a
   subclass!)
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void drawFastVLine(int16_t x, int16_t y, int16_t h,
                                 int color) {
  writeLine(x, y, x, y + h - 1, color);
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly horizontal line (this is often optimized in a
   subclass!)
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void drawFastHLine(int16_t x, int16_t y, int16_t w,
                                 int color) {
  writeLine(x, y, x + w - 1, y, color);
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if
   desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            int color) {
  for (int16_t i = x; i < x + w; i++) {
    drawFastVLine(i, y, h, color);
  }
}

/**************************************************************************/
/*!
   @brief    Fill the screen completely with one color. Update in subclasses if
   desired!
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillScreen(int color) {
  fillRect(0, 0, 640, 480, color);
}

/**************************************************************************/
/*!
   @brief    Draw a circle outline
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate

    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void drawCircle(int16_t x0, int16_t y0, int16_t r,
                              int color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  writePixel(x0, y0 + r, color);
  writePixel(x0, y0 - r, color);
  writePixel(x0 + r, y0, color);
  writePixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    writePixel(x0 + x, y0 + y, color);
    writePixel(x0 - x, y0 + y, color);
    writePixel(x0 + x, y0 - y, color);
    writePixel(x0 - x, y0 - y, color);
    writePixel(x0 + y, y0 + x, color);
    writePixel(x0 - y, y0 + x, color);
    writePixel(x0 + y, y0 - x, color);
    writePixel(x0 - y, y0 - x, color);
  }
}

/**************************************************************************/
/*!
    @brief    Quarter-circle drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    cornername  Mask bit #1 or bit #2 to indicate which quarters of
   the circle we're doing
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
                                    uint8_t cornername, int color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      writePixel(x0 + x, y0 + y, color);
      writePixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      writePixel(x0 + x, y0 - y, color);
      writePixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      writePixel(x0 - y, y0 + x, color);
      writePixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      writePixel(x0 - y, y0 - x, color);
      writePixel(x0 - x, y0 - y, color);
    }
  }
}

/**************************************************************************/
/*!
   @brief    Draw a circle with filled color
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillCircle(int16_t x0, int16_t y0, int16_t r,
                              int color) {
  drawFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

/**************************************************************************/
/*!
    @brief  Quarter-circle drawer with fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
                                    uint8_t corners, int16_t delta,
                                    int color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

/**************************************************************************/
/*!
   @brief   Draw a rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            int color) {
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, int color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  startWrite();
  drawFastHLine(x + r, y, w - 2 * r, color);         // Top
  drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
  drawFastVLine(x, y + r, h - 2 * r, color);         // Left
  drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
  // draw four corners
  drawCircleHelper(x + r, y + r, r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw/fill with
*/
/**************************************************************************/
void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, int color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  startWrite();
  fillRect(x + r, y, w - 2 * r, h, color);
  // draw four corners
  fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief   Draw a triangle with no fill color
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, int color) {
  writeLine(x0, y0, x1, y1, color);
  writeLine(x1, y1, x2, y2, color);
  writeLine(x2, y2, x0, y0, color);
}

/**************************************************************************/
/*!
   @brief     Draw a triangle with color-fill
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to fill/draw with
*/
/**************************************************************************/
void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, int color) {

  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }
  if (y1 > y2) {
    _swap_int16_t(y2, y1);
    _swap_int16_t(x2, x1);
  }
  if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
  }

  startWrite();
  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)
      a = x1;
    else if (x1 > b)
      b = x1;
    if (x2 < a)
      a = x2;
    else if (x2 > b)
      b = x2;
    drawFastHLine(a, y0, b - a + 1, color);
    endWrite();
    return;
  }

  int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
          dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2)
    last = y1; // Include y1 scanline
  else
    last = y1 - 1; // Skip it

  for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = (int32_t)dx12 * (y - y1);
  sb = (int32_t)dx02 * (y - y0);
  for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
      _swap_int16_t(a, b);
    drawFastHLine(a, y, b - a + 1, color);
  }
  endWrite();
}

// BITMAP / XBITMAP / GRAYSCALE / RGB BITMAP FUNCTIONS ---------------------

/**************************************************************************/
/*!
   @brief      Draw a PROGMEM-resident 1-bit image at the specified (x,y)
   position, using the specified foreground color (unset bits are transparent).
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                              int16_t w, int16_t h, int color) {

  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t b = 0;

  for (int16_t j = 0; j < h; j++, y++) {
    for (int16_t i = 0; i < w; i++) {
      if (i & 7)
        b <<= 1;
      else
        b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
      if (b & 0x80) {
    	  writePixel(x + i*2, y, color);
      	  writePixel(x + i*2 + (x%2? -1 : 1), y, color);
      }
    }
  }
}

///**************************************************************************/
///*!
//   @brief      Draw a PROGMEM-resident 1-bit image at the specified (x,y)
//   position, using the specified foreground (for set bits) and background (unset
//   bits) colors.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with monochrome bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//    @param    color 16-bit 5-6-5 Color to draw pixels with
//    @param    bg 16-bit 5-6-5 Color to draw background with
//*/
///**************************************************************************/
//void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
//                              int16_t w, int16_t h, int color,
//                              uint16_t bg) {
//
//  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
//  uint8_t b = 0;
//
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b <<= 1;
//      else
//        b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
//      writePixel(x + i, y, (b & 0x80) ? color : bg);
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position,
//   using the specified foreground color (unset bits are transparent).
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with monochrome bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//    @param    color 16-bit 5-6-5 Color to draw with
//*/
///**************************************************************************/
//void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w,
//                              int16_t h, int color) {
//
//  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
//  uint8_t b = 0;
//
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b <<= 1;
//      else
//        b = bitmap[j * byteWidth + i / 8];
//      if (b & 0x80)
//        writePixel(x + i, y, color);
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief      Draw a RAM-resident 1-bit image at the specified (x,y) position,
//   using the specified foreground (for set bits) and background (unset bits)
//   colors.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with monochrome bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//    @param    color 16-bit 5-6-5 Color to draw pixels with
//    @param    bg 16-bit 5-6-5 Color to draw background with
//*/
///**************************************************************************/
//void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w,
//                              int16_t h, int color, uint16_t bg) {
//
//  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
//  uint8_t b = 0;
//
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b <<= 1;
//      else
//        b = bitmap[j * byteWidth + i / 8];
//      writePixel(x + i, y, (b & 0x80) ? color : bg);
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief      Draw PROGMEM-resident XBitMap Files (*.xbm), exported from GIMP.
//   Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
//   C Array can be directly used with this function.
//   There is no RAM-resident version of this function; if generating bitmaps
//   in RAM, use the format defined by drawBitmap() and call that instead.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with monochrome bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//    @param    color 16-bit 5-6-5 Color to draw pixels with
//*/
///**************************************************************************/
//void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
//                               int16_t w, int16_t h, int color) {
//
//  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
//  uint8_t b = 0;
//
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b >>= 1;
//      else
//        b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
//      // Nearly identical to drawBitmap(), only the bit order
//      // is reversed here (left-to-right = LSB to MSB):
//      if (b & 0x01)
//        writePixel(x + i, y, color);
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a PROGMEM-resident 8-bit image (grayscale) at the specified
//   (x,y) pos. Specifically for 8-bit display devices such as IS31FL3731; no
//   color reduction/expansion is performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with grayscale bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawGrayscaleBitmap(int16_t x, int16_t y,
//                                       const uint8_t bitmap[], int16_t w,
//                                       int16_t h) {
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      writePixel(x + i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a RAM-resident 8-bit image (grayscale) at the specified (x,y)
//   pos. Specifically for 8-bit display devices such as IS31FL3731; no color
//   reduction/expansion is performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with grayscale bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap,
//                                       int16_t w, int16_t h) {
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      writePixel(x + i, y, bitmap[j * w + i]);
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a PROGMEM-resident 8-bit image (grayscale) with a 1-bit mask
//   (set bits = opaque, unset bits = clear) at the specified (x,y) position.
//   BOTH buffers (grayscale and mask) must be PROGMEM-resident.
//   Specifically for 8-bit display devices such as IS31FL3731; no color
//   reduction/expansion is performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with grayscale bitmap
//    @param    mask  byte array with mask bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawGrayscaleBitmap(int16_t x, int16_t y,
//                                       const uint8_t bitmap[],
//                                       const uint8_t mask[], int16_t w,
//                                       int16_t h) {
//  int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
//  uint8_t b = 0;
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b <<= 1;
//      else
//        b = pgm_read_byte(&mask[j * bw + i / 8]);
//      if (b & 0x80) {
//        writePixel(x + i, y, (uint8_t)pgm_read_byte(&bitmap[j * w + i]));
//      }
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a RAM-resident 8-bit image (grayscale) with a 1-bit mask
//   (set bits = opaque, unset bits = clear) at the specified (x,y) position.
//   BOTH buffers (grayscale and mask) must be RAM-residentt, no mix-and-match
//   Specifically for 8-bit display devices such as IS31FL3731; no color
//   reduction/expansion is performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with grayscale bitmap
//    @param    mask  byte array with mask bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap,
//                                       uint8_t *mask, int16_t w, int16_t h) {
//  int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
//  uint8_t b = 0;
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b <<= 1;
//      else
//        b = mask[j * bw + i / 8];
//      if (b & 0x80) {
//        writePixel(x + i, y, bitmap[j * w + i]);
//      }
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) at the specified
//   (x,y) position. For 16-bit display devices; no color reduction performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with 16-bit color bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[],
//                                 int16_t w, int16_t h) {
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      writePixel(x + i, y, pgm_read_word(&bitmap[j * w + i]));
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) at the specified (x,y)
//   position. For 16-bit display devices; no color reduction performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with 16-bit color bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap,
//                                 int16_t w, int16_t h) {
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      writePixel(x + i, y, bitmap[j * w + i]);
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a PROGMEM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask
//   (set bits = opaque, unset bits = clear) at the specified (x,y) position. BOTH
//   buffers (color and mask) must be PROGMEM-resident. For 16-bit display
//   devices; no color reduction performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with 16-bit color bitmap
//    @param    mask  byte array with monochrome mask bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[],
//                                 const uint8_t mask[], int16_t w, int16_t h) {
//  int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
//  uint8_t b = 0;
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b <<= 1;
//      else
//        b = pgm_read_byte(&mask[j * bw + i / 8]);
//      if (b & 0x80) {
//        writePixel(x + i, y, pgm_read_word(&bitmap[j * w + i]));
//      }
//    }
//  }
//  endWrite();
//}
//
///**************************************************************************/
///*!
//   @brief   Draw a RAM-resident 16-bit image (RGB 5/6/5) with a 1-bit mask (set
//   bits = opaque, unset bits = clear) at the specified (x,y) position. BOTH
//   buffers (color and mask) must be RAM-resident. For 16-bit display devices; no
//   color reduction performed.
//    @param    x   Top left corner x coordinate
//    @param    y   Top left corner y coordinate
//    @param    bitmap  byte array with 16-bit color bitmap
//    @param    mask  byte array with monochrome mask bitmap
//    @param    w   Width of bitmap in pixels
//    @param    h   Height of bitmap in pixels
//*/
///**************************************************************************/
//void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap,
//                                 uint8_t *mask, int16_t w, int16_t h) {
//  int16_t bw = (w + 7) / 8; // Bitmask scanline pad = whole byte
//  uint8_t b = 0;
//  startWrite();
//  for (int16_t j = 0; j < h; j++, y++) {
//    for (int16_t i = 0; i < w; i++) {
//      if (i & 7)
//        b <<= 1;
//      else
//        b = mask[j * bw + i / 8];
//      if (b & 0x80) {
//        writePixel(x + i, y, bitmap[j * w + i]);
//      }
//    }
//  }
//  endWrite();
//}
//
// TEXT- AND CHARACTER-HANDLING FUNCTIONS ----------------------------------

//// Draw a character
///**************************************************************************/
///*!
//   @brief   Draw a single character
//    @param    x   Bottom left corner x coordinate
//    @param    y   Bottom left corner y coordinate
//    @param    c   The 8-bit font-indexed character (likely ascii)
//    @param    color 16-bit 5-6-5 Color to draw chraracter with
//    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color,
//   no background)
//    @param    size  Font magnification level, 1 is 'original' size
//*/
///**************************************************************************/
//void drawChar(int16_t x, int16_t y, unsigned char c,
//                            int color, uint16_t bg, uint8_t size) {
//  drawChar(x, y, c, color, bg, size, size);
//}

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color,
   no background)
    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
/**************************************************************************/
void drawChar(int16_t x, int16_t y, unsigned char c,
                            int color, int bg, uint8_t size_x,
                            uint8_t size_y) {

    if ((x >= 640) ||              // Clip right
        (y >= 480) ||             // Clip bottom
        ((x + 6 * size_x - 1) < 0) || // Clip left
        ((y + 8 * size_y - 1) < 0))   // Clip top
      return;

    if ((c >= 176))
      c++; // Handle 'classic' charset behavior

    for (int8_t i = 0; i < 5; i++) { // Char bitmap = 5 columns
      uint8_t line = pgm_read_byte(&font[c * 5 + i]);
      for (int8_t j = 0; j < 8; j++, line >>= 1) {
        if (line & 1) {
          if (size_x == 1 && size_y == 1)
            writePixel(x + i, y + j, color);
          else
            fillRect(x + i * size_x, y + j * size_y, size_x, size_y,
                          color);
        } else if (bg != color) {
          if (size_x == 1 && size_y == 1)
            writePixel(x + i, y + j, bg);
          else
            fillRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
        }
      }
    }
    if (bg != color) { // If opaque, draw vertical line for last column
      if (size_x == 1 && size_y == 1)
        drawFastVLine(x + 5, y, 8, bg);
      else
        fillRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
    }
}

void drawString(int16_t x, int16_t y, unsigned char * str,
        int color, int bg, uint8_t size_x,
        uint8_t size_y) {
	while(*str) {
		if(*str==0x10) y+=8+size_y;
		else
		drawChar(x, y, *str++, color, bg, size_x, size_y);
		x+=6*size_x;
	}
}
///**************************************************************************/
///*!
//    @brief  Print one byte/character of data, used to support print()
//    @param  c  The 8-bit ascii character to write
//*/
///**************************************************************************/
//size_t write(uint8_t c) {
//  if (!gfxFont) { // 'Classic' built-in font
//
//    if (c == '\n') {              // Newline?
//      cursor_x = 0;               // Reset x to zero,
//      cursor_y += textsize_y * 8; // advance y one line
//    } else if (c != '\r') {       // Ignore carriage returns
//      if (wrap && ((cursor_x + textsize_x * 6) > _width)) { // Off right?
//        cursor_x = 0;                                       // Reset x to zero,
//        cursor_y += textsize_y * 8; // advance y one line
//      }
//      drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,
//               textsize_y);
//      cursor_x += textsize_x * 6; // Advance x one char
//    }
//
//  } else { // Custom font
//
//    if (c == '\n') {
//      cursor_x = 0;
//      cursor_y +=
//          (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
//    } else if (c != '\r') {
//      uint8_t first = pgm_read_byte(&gfxFont->first);
//      if ((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
//        GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
//        uint8_t w = pgm_read_byte(&glyph->width),
//                h = pgm_read_byte(&glyph->height);
//        if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
//          int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
//          if (wrap && ((cursor_x + textsize_x * (xo + w)) > _width)) {
//            cursor_x = 0;
//            cursor_y += (int16_t)textsize_y *
//                        (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
//          }
//          drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,
//                   textsize_y);
//        }
//        cursor_x +=
//            (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize_x;
//      }
//    }
//  }
//  return 1;
//}
//
///**************************************************************************/
///*!
//    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel
//   that much bigger.
//    @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
//*/
///**************************************************************************/
//void setTextSize(uint8_t s) { setTextSize(s, s); }

/**************************************************************************/
///*!
//    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel
//   that much bigger.
//    @param  s_x  Desired text width magnification level in X-axis. 1 is default
//    @param  s_y  Desired text width magnification level in Y-axis. 1 is default
//*/
///**************************************************************************/
//void setTextSize(uint8_t s_x, uint8_t s_y) {
//  textsize_x = (s_x > 0) ? s_x : 1;
//  textsize_y = (s_y > 0) ? s_y : 1;
//}
//
///**************************************************************************/
///*!
//    @brief      Set rotation setting for display
//    @param  x   0 thru 3 corresponding to 4 cardinal rotations
//*/
///**************************************************************************/
//void setRotation(uint8_t x) {
//  rotation = (x & 3);
//  switch (rotation) {
//  case 0:
//  case 2:
//    _width = WIDTH;
//    _height = HEIGHT;
//    break;
//  case 1:
//  case 3:
//    _width = HEIGHT;
//    _height = WIDTH;
//    break;
//  }
//}
//
///**************************************************************************/
///*!
//    @brief Set the font to display when print()ing, either custom or default
//    @param  f  The GFXfont object, if NULL use built in 6x8 font
//*/
///**************************************************************************/
//void setFont(const GFXfont *f) {
//  if (f) {          // Font struct pointer passed in?
//    if (!gfxFont) { // And no current font struct?
//      // Switching from classic to new font behavior.
//      // Move cursor pos down 6 pixels so it's on baseline.
//      cursor_y += 6;
//    }
//  } else if (gfxFont) { // NULL passed.  Current font struct defined?
//    // Switching from new to classic font behavior.
//    // Move cursor pos up 6 pixels so it's at top-left of char.
//    cursor_y -= 6;
//  }
//  gfxFont = (GFXfont *)f;
//}
//
///**************************************************************************/
///*!
//    @brief  Helper to determine size of a character with current font/size.
//            Broke this out as it's used by both the PROGMEM- and RAM-resident
//            getTextBounds() functions.
//    @param  c     The ASCII character in question
//    @param  x     Pointer to x location of character. Value is modified by
//                  this function to advance to next character.
//    @param  y     Pointer to y location of character. Value is modified by
//                  this function to advance to next character.
//    @param  minx  Pointer to minimum X coordinate, passed in to AND returned
//                  by this function -- this is used to incrementally build a
//                  bounding rectangle for a string.
//    @param  miny  Pointer to minimum Y coord, passed in AND returned.
//    @param  maxx  Pointer to maximum X coord, passed in AND returned.
//    @param  maxy  Pointer to maximum Y coord, passed in AND returned.
//*/
///**************************************************************************/
//void charBounds(unsigned char c, int16_t *x, int16_t *y,
//                              int16_t *minx, int16_t *miny, int16_t *maxx,
//                              int16_t *maxy) {
//
//  if (gfxFont) {
//
//    if (c == '\n') { // Newline?
//      *x = 0;        // Reset x to zero, advance y by one line
//      *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
//    } else if (c != '\r') { // Not a carriage return; is normal char
//      uint8_t first = pgm_read_byte(&gfxFont->first),
//              last = pgm_read_byte(&gfxFont->last);
//      if ((c >= first) && (c <= last)) { // Char present in this font?
//        GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
//        uint8_t gw = pgm_read_byte(&glyph->width),
//                gh = pgm_read_byte(&glyph->height),
//                xa = pgm_read_byte(&glyph->xAdvance);
//        int8_t xo = pgm_read_byte(&glyph->xOffset),
//               yo = pgm_read_byte(&glyph->yOffset);
//        if (wrap && ((*x + (((int16_t)xo + gw) * textsize_x)) > _width)) {
//          *x = 0; // Reset x to zero, advance y by one line
//          *y += textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
//        }
//        int16_t tsx = (int16_t)textsize_x, tsy = (int16_t)textsize_y,
//                x1 = *x + xo * tsx, y1 = *y + yo * tsy, x2 = x1 + gw * tsx - 1,
//                y2 = y1 + gh * tsy - 1;
//        if (x1 < *minx)
//          *minx = x1;
//        if (y1 < *miny)
//          *miny = y1;
//        if (x2 > *maxx)
//          *maxx = x2;
//        if (y2 > *maxy)
//          *maxy = y2;
//        *x += xa * tsx;
//      }
//    }
//
//  } else { // Default font
//
//    if (c == '\n') {        // Newline?
//      *x = 0;               // Reset x to zero,
//      *y += textsize_y * 8; // advance y one line
//      // min/max x/y unchaged -- that waits for next 'normal' character
//    } else if (c != '\r') { // Normal char; ignore carriage returns
//      if (wrap && ((*x + textsize_x * 6) > _width)) { // Off right?
//        *x = 0;                                       // Reset x to zero,
//        *y += textsize_y * 8;                         // advance y one line
//      }
//      int x2 = *x + textsize_x * 6 - 1, // Lower-right pixel of char
//          y2 = *y + textsize_y * 8 - 1;
//      if (x2 > *maxx)
//        *maxx = x2; // Track max x, y
//      if (y2 > *maxy)
//        *maxy = y2;
//      if (*x < *minx)
//        *minx = *x; // Track min x, y
//      if (*y < *miny)
//        *miny = *y;
//      *x += textsize_x * 6; // Advance x one char
//    }
//  }
//}
//
///**************************************************************************/
///*!
//    @brief  Helper to determine size of a string with current font/size.
//            Pass string and a cursor position, returns UL corner and W,H.
//    @param  str  The ASCII string to measure
//    @param  x    The current cursor X
//    @param  y    The current cursor Y
//    @param  x1   The boundary X coordinate, returned by function
//    @param  y1   The boundary Y coordinate, returned by function
//    @param  w    The boundary width, returned by function
//    @param  h    The boundary height, returned by function
//*/
///**************************************************************************/
//void getTextBounds(const char *str, int16_t x, int16_t y,
//                                 int16_t *x1, int16_t *y1, uint16_t *w,
//                                 uint16_t *h) {
//
//  uint8_t c; // Current character
//  int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1; // Bound rect
//  // Bound rect is intentionally initialized inverted, so 1st char sets it
//
//  *x1 = x; // Initial position is value passed in
//  *y1 = y;
//  *w = *h = 0; // Initial size is zero
//
//  while ((c = *str++)) {
//    // charBounds() modifies x/y to advance for each character,
//    // and min/max x/y are updated to incrementally build bounding rect.
//    charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
//  }
//
//  if (maxx >= minx) {     // If legit string bounds were found...
//    *x1 = minx;           // Update x1 to least X coord,
//    *w = maxx - minx + 1; // And w to bound rect width
//  }
//  if (maxy >= miny) { // Same for height
//    *y1 = miny;
//    *h = maxy - miny + 1;
//  }
//}
//
///**************************************************************************/
///*!
//    @brief    Helper to determine size of a string with current font/size. Pass
//   string and a cursor position, returns UL corner and W,H.
//    @param    str    The ascii string to measure (as an arduino String() class)
//    @param    x      The current cursor X
//    @param    y      The current cursor Y
//    @param    x1     The boundary X coordinate, set by function
//    @param    y1     The boundary Y coordinate, set by function
//    @param    w      The boundary width, set by function
//    @param    h      The boundary height, set by function
//*/
///**************************************************************************/
//void getTextBounds(const String &str, int16_t x, int16_t y,
//                                 int16_t *x1, int16_t *y1, uint16_t *w,
//                                 uint16_t *h) {
//  if (str.length() != 0) {
//    getTextBounds(const_cast<char *>(str.c_str()), x, y, x1, y1, w, h);
//  }
//}
//
///**************************************************************************/
///*!
//    @brief    Helper to determine size of a PROGMEM string with current
//   font/size. Pass string and a cursor position, returns UL corner and W,H.
//    @param    str     The flash-memory ascii string to measure
//    @param    x       The current cursor X
//    @param    y       The current cursor Y
//    @param    x1      The boundary X coordinate, set by function
//    @param    y1      The boundary Y coordinate, set by function
//    @param    w      The boundary width, set by function
//    @param    h      The boundary height, set by function
//*/
///**************************************************************************/
//void getTextBounds(const __FlashStringHelper *str, int16_t x,
//                                 int16_t y, int16_t *x1, int16_t *y1,
//                                 uint16_t *w, uint16_t *h) {
//  uint8_t *s = (uint8_t *)str, c;
//
//  *x1 = x;
//  *y1 = y;
//  *w = *h = 0;
//
//  int16_t minx = _width, miny = _height, maxx = -1, maxy = -1;
//
//  while ((c = pgm_read_byte(s++)))
//    charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy);
//
//  if (maxx >= minx) {
//    *x1 = minx;
//    *w = maxx - minx + 1;
//  }
//  if (maxy >= miny) {
//    *y1 = miny;
//    *h = maxy - miny + 1;
//  }
//}
//
///**************************************************************************/
///*!
//    @brief      Invert the display (ideally using built-in hardware command)
//    @param   i  True if you want to invert, false to make 'normal'
//*/
///**************************************************************************/
//void invertDisplay(bool i) {
//  // Do nothing, must be subclassed if supported by hardware
//  (void)i; // disable -Wunused-parameter warning
//}
//
///***************************************************************************/
//
///**************************************************************************/
///*!
//   @brief    Create a simple drawn button UI element
//*/
///**************************************************************************/
//Adafruit_GFX_Button(void) { _gfx = 0; }
//
///**************************************************************************/
///*!
//   @brief    Initialize button with our desired color/size/settings
//   @param    gfx     Pointer to our display so we can draw to it!
//   @param    x       The X coordinate of the center of the button
//   @param    y       The Y coordinate of the center of the button
//   @param    w       Width of the buttton
//   @param    h       Height of the buttton
//   @param    outline  Color of the outline (16-bit 5-6-5 standard)
//   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
//   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
//   @param    label  Ascii string of the text inside the button
//   @param    textsize The font magnification of the label text
//*/
///**************************************************************************/
//// Classic initButton() function: pass center & size
//void initButton(Adafruit_GFX *gfx, int16_t x, int16_t y,
//                                     uint16_t w, uint16_t h, uint16_t outline,
//                                     uint16_t fill, uint16_t textcolor,
//                                     char *label, uint8_t textsize) {
//  // Tweak arguments and pass to the newer initButtonUL() function...
//  initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill, textcolor,
//               label, textsize);
//}
//
///**************************************************************************/
///*!
//   @brief    Initialize button with our desired color/size/settings
//   @param    gfx     Pointer to our display so we can draw to it!
//   @param    x       The X coordinate of the center of the button
//   @param    y       The Y coordinate of the center of the button
//   @param    w       Width of the buttton
//   @param    h       Height of the buttton
//   @param    outline  Color of the outline (16-bit 5-6-5 standard)
//   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
//   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
//   @param    label  Ascii string of the text inside the button
//   @param    textsize_x The font magnification in X-axis of the label text
//   @param    textsize_y The font magnification in Y-axis of the label text
//*/
///**************************************************************************/
//// Classic initButton() function: pass center & size
//void initButton(Adafruit_GFX *gfx, int16_t x, int16_t y,
//                                     uint16_t w, uint16_t h, uint16_t outline,
//                                     uint16_t fill, uint16_t textcolor,
//                                     char *label, uint8_t textsize_x,
//                                     uint8_t textsize_y) {
//  // Tweak arguments and pass to the newer initButtonUL() function...
//  initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill, textcolor,
//               label, textsize_x, textsize_y);
//}
//
///**************************************************************************/
///*!
//   @brief    Initialize button with our desired color/size/settings, with
//   upper-left coordinates
//   @param    gfx     Pointer to our display so we can draw to it!
//   @param    x1       The X coordinate of the Upper-Left corner of the button
//   @param    y1       The Y coordinate of the Upper-Left corner of the button
//   @param    w       Width of the buttton
//   @param    h       Height of the buttton
//   @param    outline  Color of the outline (16-bit 5-6-5 standard)
//   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
//   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
//   @param    label  Ascii string of the text inside the button
//   @param    textsize The font magnification of the label text
//*/
///**************************************************************************/
//void initButtonUL(Adafruit_GFX *gfx, int16_t x1,
//                                       int16_t y1, uint16_t w, uint16_t h,
//                                       uint16_t outline, uint16_t fill,
//                                       uint16_t textcolor, char *label,
//                                       uint8_t textsize) {
//  initButtonUL(gfx, x1, y1, w, h, outline, fill, textcolor, label, textsize,
//               textsize);
//}
//
///**************************************************************************/
///*!
//   @brief    Initialize button with our desired color/size/settings, with
//   upper-left coordinates
//   @param    gfx     Pointer to our display so we can draw to it!
//   @param    x1       The X coordinate of the Upper-Left corner of the button
//   @param    y1       The Y coordinate of the Upper-Left corner of the button
//   @param    w       Width of the buttton
//   @param    h       Height of the buttton
//   @param    outline  Color of the outline (16-bit 5-6-5 standard)
//   @param    fill  Color of the button fill (16-bit 5-6-5 standard)
//   @param    textcolor  Color of the button label (16-bit 5-6-5 standard)
//   @param    label  Ascii string of the text inside the button
//   @param    textsize_x The font magnification in X-axis of the label text
//   @param    textsize_y The font magnification in Y-axis of the label text
//*/
///**************************************************************************/
//void initButtonUL(Adafruit_GFX *gfx, int16_t x1,
//                                       int16_t y1, uint16_t w, uint16_t h,
//                                       uint16_t outline, uint16_t fill,
//                                       uint16_t textcolor, char *label,
//                                       uint8_t textsize_x, uint8_t textsize_y) {
//  _x1 = x1;
//  _y1 = y1;
//  _w = w;
//  _h = h;
//  _outlinecolor = outline;
//  _fillcolor = fill;
//  _textcolor = textcolor;
//  _textsize_x = textsize_x;
//  _textsize_y = textsize_y;
//  _gfx = gfx;
//  strncpy(_label, label, 9);
//  _label[9] = 0; // strncpy does not place a null at the end.
//                 // When 'label' is >9 characters, _label is not terminated.
//}
//
///**************************************************************************/
///*!
//   @brief    Draw the button on the screen
//   @param    inverted Whether to draw with fill/text swapped to indicate
//   'pressed'
//*/
///**************************************************************************/
//void drawButton(bool inverted) {
//  uint16_t fill, outline, text;
//
//  if (!inverted) {
//    fill = _fillcolor;
//    outline = _outlinecolor;
//    text = _textcolor;
//  } else {
//    fill = _textcolor;
//    outline = _outlinecolor;
//    text = _fillcolor;
//  }
//
//  uint8_t r = min(_w, _h) / 4; // Corner radius
//  _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
//  _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);
//
//  _gfx->setCursor(_x1 + (_w / 2) - (strlen(_label) * 3 * _textsize_x),
//                  _y1 + (_h / 2) - (4 * _textsize_y));
//  _gfx->setTextColor(text);
//  _gfx->setTextSize(_textsize_x, _textsize_y);
//  _gfx->print(_label);
//}
//
///**************************************************************************/
///*!
//    @brief    Helper to let us know if a coordinate is within the bounds of the
//   button
//    @param    x       The X coordinate to check
//    @param    y       The Y coordinate to check
//    @returns  True if within button graphics outline
//*/
///**************************************************************************/
//bool contains(int16_t x, int16_t y) {
//  return ((x >= _x1) && (x < (int16_t)(_x1 + _w)) && (y >= _y1) &&
//          (y < (int16_t)(_y1 + _h)));
//}
//
///**************************************************************************/
///*!
//   @brief    Query whether the button was pressed since we last checked state
//   @returns  True if was not-pressed before, now is.
//*/
///**************************************************************************/
//bool justPressed() { return (currstate && !laststate); }
//
///**************************************************************************/
///*!
//   @brief    Query whether the button was released since we last checked state
//   @returns  True if was pressed before, now is not.
//*/
///**************************************************************************/
//bool justReleased() { return (!currstate && laststate); }
//
//// -------------------------------------------------------------------------
//
//// GFXcanvas1, GFXcanvas8 and GFXcanvas16 (currently a WIP, don't get too
//// comfy with the implementation) provide 1-, 8- and 16-bit offscreen
//// canvases, the address of which can be passed to drawBitmap() or
//// pushColors() (the latter appears only in a couple of GFX-subclassed TFT
//// libraries at this time).  This is here mostly to help with the recently-
//// added proportionally-spaced fonts; adds a way to refresh a section of the
//// screen without a massive flickering clear-and-redraw...but maybe you'll
//// find other uses too.  VERY RAM-intensive, since the buffer is in MCU
//// memory and not the display driver...GXFcanvas1 might be minimally useful
//// on an Uno-class board, but this and the others are much more likely to
//// require at least a Mega or various recent ARM-type boards (recommended,
//// as the text+bitmap draw can be pokey).  GFXcanvas1 requires 1 bit per
//// pixel (rounded up to nearest byte per scanline), GFXcanvas8 is 1 byte
//// per pixel (no scanline pad), and GFXcanvas16 uses 2 bytes per pixel (no
//// scanline pad).
//// NOT EXTENSIVELY TESTED YET.  MAY CONTAIN WORST BUGS KNOWN TO HUMANKIND.
//
//#ifdef __AVR__
//// Bitmask tables of 0x80>>X and ~(0x80>>X), because X>>Y is slow on AVR
//const uint8_t PROGMEM GFXcanvas1::GFXsetBit[] = {0x80, 0x40, 0x20, 0x10,
//                                                 0x08, 0x04, 0x02, 0x01};
//const uint8_t PROGMEM GFXcanvas1::GFXclrBit[] = {0x7F, 0xBF, 0xDF, 0xEF,
//                                                 0xF7, 0xFB, 0xFD, 0xFE};
//#endif
//
///**************************************************************************/
///*!
//   @brief    Instatiate a GFX 1-bit canvas context for graphics
//   @param    w   Display width, in pixels
//   @param    h   Display height, in pixels
//*/
///**************************************************************************/
//GFXcanvas1::GFXcanvas1(uint16_t w, uint16_t h) : Adafruit_GFX(w, h) {
//  uint16_t bytes = ((w + 7) / 8) * h;
//  if ((buffer = (uint8_t *)malloc(bytes))) {
//    memset(buffer, 0, bytes);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Delete the canvas, free memory
//*/
///**************************************************************************/
//GFXcanvas1::~GFXcanvas1(void) {
//  if (buffer)
//    free(buffer);
//}
//
///**************************************************************************/
///*!
//    @brief  Draw a pixel to the canvas framebuffer
//    @param  x     x coordinate
//    @param  y     y coordinate
//    @param  color Binary (on or off) color to fill with
//*/
///**************************************************************************/
//void GFXcanvas1::drawPixel(int16_t x, int16_t y, int color) {
//  if (buffer) {
//    if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
//      return;
//
//    int16_t t;
//    switch (rotation) {
//    case 1:
//      t = x;
//      x = WIDTH - 1 - y;
//      y = t;
//      break;
//    case 2:
//      x = WIDTH - 1 - x;
//      y = HEIGHT - 1 - y;
//      break;
//    case 3:
//      t = x;
//      x = y;
//      y = HEIGHT - 1 - t;
//      break;
//    }
//
//    uint8_t *ptr = &buffer[(x / 8) + y * ((WIDTH + 7) / 8)];
//#ifdef __AVR__
//    if (color)
//      *ptr |= pgm_read_byte(&GFXsetBit[x & 7]);
//    else
//      *ptr &= pgm_read_byte(&GFXclrBit[x & 7]);
//#else
//    if (color)
//      *ptr |= 0x80 >> (x & 7);
//    else
//      *ptr &= ~(0x80 >> (x & 7));
//#endif
//  }
//}
//
///**********************************************************************/
///*!
//        @brief    Get the pixel color value at a given coordinate
//        @param    x   x coordinate
//        @param    y   y coordinate
//        @returns  The desired pixel's binary color value, either 0x1 (on) or 0x0
//   (off)
//*/
///**********************************************************************/
//bool GFXcanvas1::getPixel(int16_t x, int16_t y) const {
//  int16_t t;
//  switch (rotation) {
//  case 1:
//    t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    break;
//  case 2:
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//    break;
//  case 3:
//    t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    break;
//  }
//  return getRawPixel(x, y);
//}
//
///**********************************************************************/
///*!
//        @brief    Get the pixel color value at a given, unrotated coordinate.
//              This method is intended for hardware drivers to get pixel value
//              in physical coordinates.
//        @param    x   x coordinate
//        @param    y   y coordinate
//        @returns  The desired pixel's binary color value, either 0x1 (on) or 0x0
//   (off)
//*/
///**********************************************************************/
//bool GFXcanvas1::getRawPixel(int16_t x, int16_t y) const {
//  if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
//    return 0;
//  if (buffer) {
//    uint8_t *ptr = &buffer[(x / 8) + y * ((WIDTH + 7) / 8)];
//
//#ifdef __AVR__
//    return ((*ptr) & pgm_read_byte(&GFXsetBit[x & 7])) != 0;
//#else
//    return ((*ptr) & (0x80 >> (x & 7))) != 0;
//#endif
//  }
//  return 0;
//}
//
///**************************************************************************/
///*!
//    @brief  Fill the framebuffer completely with one color
//    @param  color Binary (on or off) color to fill with
//*/
///**************************************************************************/
//void GFXcanvas1::fillScreen(int color) {
//  if (buffer) {
//    uint16_t bytes = ((WIDTH + 7) / 8) * HEIGHT;
//    memset(buffer, color ? 0xFF : 0x00, bytes);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief  Speed optimized vertical line drawing
//   @param  x      Line horizontal start point
//   @param  y      Line vertical start point
//   @param  h      Length of vertical line to be drawn, including first point
//   @param  color  Color to fill with
//*/
///**************************************************************************/
//void GFXcanvas1::drawFastVLine(int16_t x, int16_t y, int16_t h,
//                               int color) {
//
//  if (h < 0) { // Convert negative heights to positive equivalent
//    h *= -1;
//    y -= h - 1;
//    if (y < 0) {
//      h += y;
//      y = 0;
//    }
//  }
//
//  // Edge rejection (no-draw if totally off canvas)
//  if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
//    return;
//  }
//
//  if (y < 0) { // Clip top
//    h += y;
//    y = 0;
//  }
//  if (y + h > height()) { // Clip bottom
//    h = height() - y;
//  }
//
//  if (getRotation() == 0) {
//    drawFastRawVLine(x, y, h, color);
//  } else if (getRotation() == 1) {
//    int16_t t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    x -= h - 1;
//    drawFastRawHLine(x, y, h, color);
//  } else if (getRotation() == 2) {
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//
//    y -= h - 1;
//    drawFastRawVLine(x, y, h, color);
//  } else if (getRotation() == 3) {
//    int16_t t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    drawFastRawHLine(x, y, h, color);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief  Speed optimized horizontal line drawing
//   @param  x      Line horizontal start point
//   @param  y      Line vertical start point
//   @param  w      Length of horizontal line to be drawn, including first point
//   @param  color  Color to fill with
//*/
///**************************************************************************/
//void GFXcanvas1::drawFastHLine(int16_t x, int16_t y, int16_t w,
//                               int color) {
//  if (w < 0) { // Convert negative widths to positive equivalent
//    w *= -1;
//    x -= w - 1;
//    if (x < 0) {
//      w += x;
//      x = 0;
//    }
//  }
//
//  // Edge rejection (no-draw if totally off canvas)
//  if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
//    return;
//  }
//
//  if (x < 0) { // Clip left
//    w += x;
//    x = 0;
//  }
//  if (x + w >= width()) { // Clip right
//    w = width() - x;
//  }
//
//  if (getRotation() == 0) {
//    drawFastRawHLine(x, y, w, color);
//  } else if (getRotation() == 1) {
//    int16_t t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    drawFastRawVLine(x, y, w, color);
//  } else if (getRotation() == 2) {
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//
//    x -= w - 1;
//    drawFastRawHLine(x, y, w, color);
//  } else if (getRotation() == 3) {
//    int16_t t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    y -= w - 1;
//    drawFastRawVLine(x, y, w, color);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Speed optimized vertical line drawing into the raw canvas buffer
//   @param    x   Line horizontal start point
//   @param    y   Line vertical start point
//   @param    h   length of vertical line to be drawn, including first point
//   @param    color   Binary (on or off) color to fill with
//*/
///**************************************************************************/
//void GFXcanvas1::drawFastRawVLine(int16_t x, int16_t y, int16_t h,
//                                  int color) {
//  // x & y already in raw (rotation 0) coordinates, no need to transform.
//  int16_t row_bytes = ((WIDTH + 7) / 8);
//  uint8_t *ptr = &buffer[(x / 8) + y * row_bytes];
//
//  if (color > 0) {
//#ifdef __AVR__
//    uint8_t bit_mask = pgm_read_byte(&GFXsetBit[x & 7]);
//#else
//    uint8_t bit_mask = (0x80 >> (x & 7));
//#endif
//    for (int16_t i = 0; i < h; i++) {
//      *ptr |= bit_mask;
//      ptr += row_bytes;
//    }
//  } else {
//#ifdef __AVR__
//    uint8_t bit_mask = pgm_read_byte(&GFXclrBit[x & 7]);
//#else
//    uint8_t bit_mask = ~(0x80 >> (x & 7));
//#endif
//    for (int16_t i = 0; i < h; i++) {
//      *ptr &= bit_mask;
//      ptr += row_bytes;
//    }
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Speed optimized horizontal line drawing into the raw canvas buffer
//   @param    x   Line horizontal start point
//   @param    y   Line vertical start point
//   @param    w   length of horizontal line to be drawn, including first point
//   @param    color   Binary (on or off) color to fill with
//*/
///**************************************************************************/
//void GFXcanvas1::drawFastRawHLine(int16_t x, int16_t y, int16_t w,
//                                  int color) {
//  // x & y already in raw (rotation 0) coordinates, no need to transform.
//  int16_t rowBytes = ((WIDTH + 7) / 8);
//  uint8_t *ptr = &buffer[(x / 8) + y * rowBytes];
//  size_t remainingWidthBits = w;
//
//  // check to see if first byte needs to be partially filled
//  if ((x & 7) > 0) {
//    // create bit mask for first byte
//    uint8_t startByteBitMask = 0x00;
//    for (int8_t i = (x & 7); ((i < 8) && (remainingWidthBits > 0)); i++) {
//#ifdef __AVR__
//      startByteBitMask |= pgm_read_byte(&GFXsetBit[i]);
//#else
//      startByteBitMask |= (0x80 >> i);
//#endif
//      remainingWidthBits--;
//    }
//    if (color > 0) {
//      *ptr |= startByteBitMask;
//    } else {
//      *ptr &= ~startByteBitMask;
//    }
//
//    ptr++;
//  }
//
//  // do the next remainingWidthBits bits
//  if (remainingWidthBits > 0) {
//    size_t remainingWholeBytes = remainingWidthBits / 8;
//    size_t lastByteBits = remainingWidthBits % 8;
//    uint8_t wholeByteColor = color > 0 ? 0xFF : 0x00;
//
//    memset(ptr, wholeByteColor, remainingWholeBytes);
//
//    if (lastByteBits > 0) {
//      uint8_t lastByteBitMask = 0x00;
//      for (size_t i = 0; i < lastByteBits; i++) {
//#ifdef __AVR__
//        lastByteBitMask |= pgm_read_byte(&GFXsetBit[i]);
//#else
//        lastByteBitMask |= (0x80 >> i);
//#endif
//      }
//      ptr += remainingWholeBytes;
//
//      if (color > 0) {
//        *ptr |= lastByteBitMask;
//      } else {
//        *ptr &= ~lastByteBitMask;
//      }
//    }
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Instatiate a GFX 8-bit canvas context for graphics
//   @param    w   Display width, in pixels
//   @param    h   Display height, in pixels
//*/
///**************************************************************************/
//GFXcanvas8::GFXcanvas8(uint16_t w, uint16_t h) : Adafruit_GFX(w, h) {
//  uint32_t bytes = w * h;
//  if ((buffer = (uint8_t *)malloc(bytes))) {
//    memset(buffer, 0, bytes);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Delete the canvas, free memory
//*/
///**************************************************************************/
//GFXcanvas8::~GFXcanvas8(void) {
//  if (buffer)
//    free(buffer);
//}
//
///**************************************************************************/
///*!
//    @brief  Draw a pixel to the canvas framebuffer
//    @param  x   x coordinate
//    @param  y   y coordinate
//    @param  color 8-bit Color to fill with. Only lower byte of uint16_t is used.
//*/
///**************************************************************************/
//void GFXcanvas8::drawPixel(int16_t x, int16_t y, int color) {
//  if (buffer) {
//    if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
//      return;
//
//    int16_t t;
//    switch (rotation) {
//    case 1:
//      t = x;
//      x = WIDTH - 1 - y;
//      y = t;
//      break;
//    case 2:
//      x = WIDTH - 1 - x;
//      y = HEIGHT - 1 - y;
//      break;
//    case 3:
//      t = x;
//      x = y;
//      y = HEIGHT - 1 - t;
//      break;
//    }
//
//    buffer[x + y * WIDTH] = color;
//  }
//}
//
///**********************************************************************/
///*!
//        @brief    Get the pixel color value at a given coordinate
//        @param    x   x coordinate
//        @param    y   y coordinate
//        @returns  The desired pixel's 8-bit color value
//*/
///**********************************************************************/
//uint8_t GFXcanvas8::getPixel(int16_t x, int16_t y) const {
//  int16_t t;
//  switch (rotation) {
//  case 1:
//    t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    break;
//  case 2:
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//    break;
//  case 3:
//    t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    break;
//  }
//  return getRawPixel(x, y);
//}
//
///**********************************************************************/
///*!
//        @brief    Get the pixel color value at a given, unrotated coordinate.
//              This method is intended for hardware drivers to get pixel value
//              in physical coordinates.
//        @param    x   x coordinate
//        @param    y   y coordinate
//        @returns  The desired pixel's 8-bit color value
//*/
///**********************************************************************/
//uint8_t GFXcanvas8::getRawPixel(int16_t x, int16_t y) const {
//  if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
//    return 0;
//  if (buffer) {
//    return buffer[x + y * WIDTH];
//  }
//  return 0;
//}
//
///**************************************************************************/
///*!
//    @brief  Fill the framebuffer completely with one color
//    @param  color 8-bit Color to fill with. Only lower byte of uint16_t is used.
//*/
///**************************************************************************/
//void GFXcanvas8::fillScreen(int color) {
//  if (buffer) {
//    memset(buffer, color, WIDTH * HEIGHT);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief  Speed optimized vertical line drawing
//   @param  x      Line horizontal start point
//   @param  y      Line vertical start point
//   @param  h      Length of vertical line to be drawn, including first point
//   @param  color  8-bit Color to fill with. Only lower byte of uint16_t is
//                  used.
//*/
///**************************************************************************/
//void GFXcanvas8::drawFastVLine(int16_t x, int16_t y, int16_t h,
//                               int color) {
//  if (h < 0) { // Convert negative heights to positive equivalent
//    h *= -1;
//    y -= h - 1;
//    if (y < 0) {
//      h += y;
//      y = 0;
//    }
//  }
//
//  // Edge rejection (no-draw if totally off canvas)
//  if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
//    return;
//  }
//
//  if (y < 0) { // Clip top
//    h += y;
//    y = 0;
//  }
//  if (y + h > height()) { // Clip bottom
//    h = height() - y;
//  }
//
//  if (getRotation() == 0) {
//    drawFastRawVLine(x, y, h, color);
//  } else if (getRotation() == 1) {
//    int16_t t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    x -= h - 1;
//    drawFastRawHLine(x, y, h, color);
//  } else if (getRotation() == 2) {
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//
//    y -= h - 1;
//    drawFastRawVLine(x, y, h, color);
//  } else if (getRotation() == 3) {
//    int16_t t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    drawFastRawHLine(x, y, h, color);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief  Speed optimized horizontal line drawing
//   @param  x      Line horizontal start point
//   @param  y      Line vertical start point
//   @param  w      Length of horizontal line to be drawn, including 1st point
//   @param  color  8-bit Color to fill with. Only lower byte of uint16_t is
//                  used.
//*/
///**************************************************************************/
//void GFXcanvas8::drawFastHLine(int16_t x, int16_t y, int16_t w,
//                               int color) {
//
//  if (w < 0) { // Convert negative widths to positive equivalent
//    w *= -1;
//    x -= w - 1;
//    if (x < 0) {
//      w += x;
//      x = 0;
//    }
//  }
//
//  // Edge rejection (no-draw if totally off canvas)
//  if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
//    return;
//  }
//
//  if (x < 0) { // Clip left
//    w += x;
//    x = 0;
//  }
//  if (x + w >= width()) { // Clip right
//    w = width() - x;
//  }
//
//  if (getRotation() == 0) {
//    drawFastRawHLine(x, y, w, color);
//  } else if (getRotation() == 1) {
//    int16_t t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    drawFastRawVLine(x, y, w, color);
//  } else if (getRotation() == 2) {
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//
//    x -= w - 1;
//    drawFastRawHLine(x, y, w, color);
//  } else if (getRotation() == 3) {
//    int16_t t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    y -= w - 1;
//    drawFastRawVLine(x, y, w, color);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Speed optimized vertical line drawing into the raw canvas buffer
//   @param    x   Line horizontal start point
//   @param    y   Line vertical start point
//   @param    h   length of vertical line to be drawn, including first point
//   @param    color   8-bit Color to fill with. Only lower byte of uint16_t is
//   used.
//*/
///**************************************************************************/
//void GFXcanvas8::drawFastRawVLine(int16_t x, int16_t y, int16_t h,
//                                  int color) {
//  // x & y already in raw (rotation 0) coordinates, no need to transform.
//  uint8_t *buffer_ptr = buffer + y * WIDTH + x;
//  for (int16_t i = 0; i < h; i++) {
//    (*buffer_ptr) = color;
//    buffer_ptr += WIDTH;
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Speed optimized horizontal line drawing into the raw canvas buffer
//   @param    x   Line horizontal start point
//   @param    y   Line vertical start point
//   @param    w   length of horizontal line to be drawn, including first point
//   @param    color   8-bit Color to fill with. Only lower byte of uint16_t is
//   used.
//*/
///**************************************************************************/
//void GFXcanvas8::drawFastRawHLine(int16_t x, int16_t y, int16_t w,
//                                  int color) {
//  // x & y already in raw (rotation 0) coordinates, no need to transform.
//  memset(buffer + y * WIDTH + x, color, w);
//}
//
///**************************************************************************/
///*!
//   @brief    Instatiate a GFX 16-bit canvas context for graphics
//   @param    w   Display width, in pixels
//   @param    h   Display height, in pixels
//*/
///**************************************************************************/
//GFXcanvas16::GFXcanvas16(uint16_t w, uint16_t h) : Adafruit_GFX(w, h) {
//  uint32_t bytes = w * h * 2;
//  if ((buffer = (uint16_t *)malloc(bytes))) {
//    memset(buffer, 0, bytes);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Delete the canvas, free memory
//*/
///**************************************************************************/
//GFXcanvas16::~GFXcanvas16(void) {
//  if (buffer)
//    free(buffer);
//}
//
///**************************************************************************/
///*!
//    @brief  Draw a pixel to the canvas framebuffer
//    @param  x   x coordinate
//    @param  y   y coordinate
//    @param  color 16-bit 5-6-5 Color to fill with
//*/
///**************************************************************************/
//drawPixel(int16_t x, int16_t y, int color) {
//  if (buffer) {
//    if ((x < 0) || (y < 0) || (x >= _width) || (y >= _height))
//      return;
//
//    int16_t t;
//    switch (rotation) {
//    case 1:
//      t = x;
//      x = WIDTH - 1 - y;
//      y = t;
//      break;
//    case 2:
//      x = WIDTH - 1 - x;
//      y = HEIGHT - 1 - y;
//      break;
//    case 3:
//      t = x;
//      x = y;
//      y = HEIGHT - 1 - t;
//      break;
//    }
//
//    buffer[x + y * WIDTH] = color;
//  }
//}
//
///**********************************************************************/
///*!
//        @brief    Get the pixel color value at a given coordinate
//        @param    x   x coordinate
//        @param    y   y coordinate
//        @returns  The desired pixel's 16-bit 5-6-5 color value
//*/
///**********************************************************************/
//uint16_t GFXcanvas16::getPixel(int16_t x, int16_t y) const {
//  int16_t t;
//  switch (rotation) {
//  case 1:
//    t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    break;
//  case 2:
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//    break;
//  case 3:
//    t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    break;
//  }
//  return getRawPixel(x, y);
//}
//
///**********************************************************************/
///*!
//        @brief    Get the pixel color value at a given, unrotated coordinate.
//              This method is intended for hardware drivers to get pixel value
//              in physical coordinates.
//        @param    x   x coordinate
//        @param    y   y coordinate
//        @returns  The desired pixel's 16-bit 5-6-5 color value
//*/
///**********************************************************************/
//uint16_t GFXcanvas16::getRawPixel(int16_t x, int16_t y) const {
//  if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
//    return 0;
//  if (buffer) {
//    return buffer[x + y * WIDTH];
//  }
//  return 0;
//}
//
///**************************************************************************/
///*!
//    @brief  Fill the framebuffer completely with one color
//    @param  color 16-bit 5-6-5 Color to fill with
//*/
///**************************************************************************/
//fillScreen(int color) {
//  if (buffer) {
//    uint8_t hi = color >> 8, lo = color & 0xFF;
//    if (hi == lo) {
//      memset(buffer, lo, WIDTH * HEIGHT * 2);
//    } else {
//      uint32_t i, pixels = WIDTH * HEIGHT;
//      for (i = 0; i < pixels; i++)
//        buffer[i] = color;
//    }
//  }
//}
//
///**************************************************************************/
///*!
//    @brief  Reverses the "endian-ness" of each 16-bit pixel within the
//            canvas; little-endian to big-endian, or big-endian to little.
//            Most microcontrollers (such as SAMD) are little-endian, while
//            most displays tend toward big-endianness. All the drawing
//            functions (including RGB bitmap drawing) take care of this
//            automatically, but some specialized code (usually involving
//            DMA) can benefit from having pixel data already in the
//            display-native order. Note that this does NOT convert to a
//            SPECIFIC endian-ness, it just flips the bytes within each word.
//*/
///**************************************************************************/
//byteSwap(void) {
//  if (buffer) {
//    uint32_t i, pixels = WIDTH * HEIGHT;
//    for (i = 0; i < pixels; i++)
//      buffer[i] = __builtin_bswap16(buffer[i]);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Speed optimized vertical line drawing
//   @param    x   Line horizontal start point
//   @param    y   Line vertical start point
//   @param    h   length of vertical line to be drawn, including first point
//   @param    color   color 16-bit 5-6-5 Color to draw line with
//*/
///**************************************************************************/
//drawFastVLine(int16_t x, int16_t y, int16_t h,
//                                int color) {
//  if (h < 0) { // Convert negative heights to positive equivalent
//    h *= -1;
//    y -= h - 1;
//    if (y < 0) {
//      h += y;
//      y = 0;
//    }
//  }
//
//  // Edge rejection (no-draw if totally off canvas)
//  if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
//    return;
//  }
//
//  if (y < 0) { // Clip top
//    h += y;
//    y = 0;
//  }
//  if (y + h > height()) { // Clip bottom
//    h = height() - y;
//  }
//
//  if (getRotation() == 0) {
//    drawFastRawVLine(x, y, h, color);
//  } else if (getRotation() == 1) {
//    int16_t t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    x -= h - 1;
//    drawFastRawHLine(x, y, h, color);
//  } else if (getRotation() == 2) {
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//
//    y -= h - 1;
//    drawFastRawVLine(x, y, h, color);
//  } else if (getRotation() == 3) {
//    int16_t t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    drawFastRawHLine(x, y, h, color);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief  Speed optimized horizontal line drawing
//   @param  x      Line horizontal start point
//   @param  y      Line vertical start point
//   @param  w      Length of horizontal line to be drawn, including 1st point
//   @param  color  Color 16-bit 5-6-5 Color to draw line with
//*/
///**************************************************************************/
//drawFastHLine(int16_t x, int16_t y, int16_t w,
//                                int color) {
//  if (w < 0) { // Convert negative widths to positive equivalent
//    w *= -1;
//    x -= w - 1;
//    if (x < 0) {
//      w += x;
//      x = 0;
//    }
//  }
//
//  // Edge rejection (no-draw if totally off canvas)
//  if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
//    return;
//  }
//
//  if (x < 0) { // Clip left
//    w += x;
//    x = 0;
//  }
//  if (x + w >= width()) { // Clip right
//    w = width() - x;
//  }
//
//  if (getRotation() == 0) {
//    drawFastRawHLine(x, y, w, color);
//  } else if (getRotation() == 1) {
//    int16_t t = x;
//    x = WIDTH - 1 - y;
//    y = t;
//    drawFastRawVLine(x, y, w, color);
//  } else if (getRotation() == 2) {
//    x = WIDTH - 1 - x;
//    y = HEIGHT - 1 - y;
//
//    x -= w - 1;
//    drawFastRawHLine(x, y, w, color);
//  } else if (getRotation() == 3) {
//    int16_t t = x;
//    x = y;
//    y = HEIGHT - 1 - t;
//    y -= w - 1;
//    drawFastRawVLine(x, y, w, color);
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Speed optimized vertical line drawing into the raw canvas buffer
//   @param    x   Line horizontal start point
//   @param    y   Line vertical start point
//   @param    h   length of vertical line to be drawn, including first point
//   @param    color   color 16-bit 5-6-5 Color to draw line with
//*/
///**************************************************************************/
//drawFastRawVLine(int16_t x, int16_t y, int16_t h,
//                                   int color) {
//  // x & y already in raw (rotation 0) coordinates, no need to transform.
//  uint16_t *buffer_ptr = buffer + y * WIDTH + x;
//  for (int16_t i = 0; i < h; i++) {
//    (*buffer_ptr) = color;
//    buffer_ptr += WIDTH;
//  }
//}
//
///**************************************************************************/
///*!
//   @brief    Speed optimized horizontal line drawing into the raw canvas buffer
//   @param    x   Line horizontal start point
//   @param    y   Line vertical start point
//   @param    w   length of horizontal line to be drawn, including first point
//   @param    color   color 16-bit 5-6-5 Color to draw line with
//*/
///**************************************************************************/
//drawFastRawHLine(int16_t x, int16_t y, int16_t w,
//                                   int color) {
//  // x & y already in raw (rotation 0) coordinates, no need to transform.
//  uint32_t buffer_index = y * WIDTH + x;
//  for (uint32_t i = buffer_index; i < buffer_index + w; i++) {
//    buffer[i] = color;
//  }
//}
