#ifndef _ADAFRUIT_GFX_H
#define _ADAFRUIT_GFX_H

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfxfont.h"
extern GXRModeObj *rmode;
extern u8 __xfb[];

#define COL_BLACK getColor(0,0,0)
#define COL_YELLOW getColor(255,255,0)
#define COL_HIGHLIGHT getColor(255,255,255)

void ClearScreen ();
void ShowScreen ();

unsigned int getColor(u8 r1, u8 g1, u8 b1);

  /**********************************************************************/
  /*!
    @brief  Draw to the screen/framebuffer/etc.
    Must be overridden in subclass.
    @param  x    X coordinate in pixels
    @param  y    Y coordinate in pixels
    @param color  16-bit pixel color.
  */
  /**********************************************************************/
  void drawPixel(int16_t x, int16_t y, int color);

  // TRANSACTION API / CORE DRAW API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  void startWrite(void);
  void writePixel(int16_t x, int16_t y, int color);
  void writeNativePixel(int16_t x, int16_t y, int color);
  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                             int color);
  void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                         int color);
  void endWrite(void);
//
//  // CONTROL API
//  // These MAY be overridden by the subclass to provide device-specific
//  // optimized code.  Otherwise 'generic' versions are used.
//  void setRotation(uint8_t r);
//  void invertDisplay(bool i);
//
//  // BASIC DRAW API
//  // These MAY be overridden by the subclass to provide device-specific
//  // optimized code.  Otherwise 'generic' versions are used.
//
//  // It's good to implement those, even if using transaction API
  void drawFastVLine(int16_t x, int16_t y, int16_t h, int color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, int color);
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                        int color);
  void fillScreen(int color);
//  // Optional and probably not necessary to change
//  // These exist only with Adafruit_GFX (no subclass overrides)
  void drawCircle(int16_t x0, int16_t y0, int16_t r, int color);
  void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
                        int color);
  void fillCircle(int16_t x0, int16_t y0, int16_t r, int color);
  void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername,
                        int16_t delta, int color);
  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                    int16_t y2, int color);
  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2,
                    int16_t y2, int color);
  void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
                     int16_t radius, int color);
  void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h,
                     int16_t radius, int color);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                  int16_t h, int color);
//  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
//                  int16_t h, int color, uint16_t bg);
//  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h,
//                  int color);
//  void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h,
//                  int color, uint16_t bg);
//  void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
//                   int16_t h, int color);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
//                           int16_t w, int16_t h);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w,
//                           int16_t h);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
//                           const uint8_t mask[], int16_t w, int16_t h);
//  void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, uint8_t *mask,
//                           int16_t w, int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w,
//                     int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w,
//                     int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[],
//                     const uint8_t mask[], int16_t w, int16_t h);
//  void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, uint8_t *mask,
//                     int16_t w, int16_t h);
//  void drawChar(int16_t x, int16_t y, unsigned char c, int color,
//                uint16_t bg, uint8_t size);
  void drawChar(int16_t x, int16_t y, unsigned char c, int color,
		  int bg, uint8_t size_x, uint8_t size_y);
  void drawString(int16_t x, int16_t y, unsigned char * str,
          int color, int bg, uint8_t size_x,
          uint8_t size_y);
//  void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1,
//                     int16_t *y1, uint16_t *w, uint16_t *h);
////  void getTextBounds(const __FlashStringHelper *s, int16_t x, int16_t y,
////                     int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
////  void getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);
//  void setTextSize(uint8_t s);
//  void setTextSize(uint8_t sx, uint8_t sy);
//  void setFont(const GFXfont *f = NULL);
//
//  /**********************************************************************/
//  /*!
//    @brief  Set text cursor location
//    @param  x    X coordinate in pixels
//    @param  y    Y coordinate in pixels
//  */
//  /**********************************************************************/
//  void setCursor(int16_t x, int16_t y) {
//    cursor_x = x;
//    cursor_y = y;
//  }
//
//  /**********************************************************************/
//  /*!
//    @brief   Set text font color with transparant background
//    @param   c   16-bit 5-6-5 Color to draw text with
//    @note    For 'transparent' background, background and foreground
//             are set to same color rather than using a separate flag.
//  */
//  /**********************************************************************/
//  void setTextColor(uint16_t c) { textcolor = textbgcolor = c; }
//
//  /**********************************************************************/
//  /*!
//    @brief   Set text font color with custom background color
//    @param   c   16-bit 5-6-5 Color to draw text with
//    @param   bg  16-bit 5-6-5 Color to draw background/fill with
//  */
//  /**********************************************************************/
//  void setTextColor(uint16_t c, uint16_t bg) {
//    textcolor = c;
//    textbgcolor = bg;
//  }
//
//  /**********************************************************************/
//  /*!
//  @brief  Set whether text that is too long for the screen width should
//          automatically wrap around to the next line (else clip right).
//  @param  w  true for wrapping, false for clipping
//  */
//  /**********************************************************************/
//  void setTextWrap(bool w) { wrap = w; }
//
//  /**********************************************************************/
//  /*!
//    @brief  Enable (or disable) Code Page 437-compatible charset.
//            There was an error in glcdfont.c for the longest time -- one
//            character (#176, the 'light shade' block) was missing -- this
//            threw off the index of every character that followed it.
//            But a TON of code has been written with the erroneous
//            character indices. By default, the library uses the original
//            'wrong' behavior and old sketches will still work. Pass
//            'true' to this function to use correct CP437 character values
//            in your code.
//    @param  x  true = enable (new behavior), false = disable (old behavior)
//  */
//  /**********************************************************************/
//  void cp437(bool x = true) { _cp437 = x; }
//
//  void write(uint8_t);
//
//
//  /************************************************************************/
//  /*!
//    @brief      Get width of the display, accounting for current rotation
//    @returns    Width in pixels
//  */
//  /************************************************************************/
//  const int16_t width(void)  { return _width; };
//
//  /************************************************************************/
//  /*!
//    @brief      Get height of the display, accounting for current rotation
//    @returns    Height in pixels
//  */
//  /************************************************************************/
//  const int16_t height(void)  { return _height; }
//
//  /************************************************************************/
//  /*!
//    @brief      Get rotation setting for display
//    @returns    0 thru 3 corresponding to 4 cardinal rotations
//  */
//  /************************************************************************/
//  const uint8_t getRotation(void)  { return rotation; }
//
//  // get current cursor position (get rotation safe maximum values,
//  // using: width() for x, height() for y)
//  /************************************************************************/
//  /*!
//    @brief  Get text cursor X location
//    @returns    X coordinate in pixels
//  */
//  /************************************************************************/
//  const int16_t getCursorX(void)  { return cursor_x; }
//
//  /************************************************************************/
//  /*!
//    @brief      Get text cursor Y location
//    @returns    Y coordinate in pixels
//  */
//  /************************************************************************/
//  const int16_t getCursorY(void) { return cursor_y; };
//
//  void charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx,
//                  int16_t *miny, int16_t *maxx, int16_t *maxy);
//  int16_t WIDTH;        ///< This is the 'raw' display width - never changes
//  int16_t HEIGHT;       ///< This is the 'raw' display height - never changes
//  int16_t _width;       ///< Display width as modified by current rotation
//  int16_t _height;      ///< Display height as modified by current rotation
//  int16_t cursor_x;     ///< x location to start print()ing text
//  int16_t cursor_y;     ///< y location to start print()ing text
//  uint16_t textcolor;   ///< 16-bit background color for print()
//  uint16_t textbgcolor; ///< 16-bit text color for print()
//  uint8_t textsize_x;   ///< Desired magnification in X-axis of text to print()
//  uint8_t textsize_y;   ///< Desired magnification in Y-axis of text to print()
//  uint8_t rotation;     ///< Display rotation (0 thru 3)
//  bool wrap;            ///< If set, 'wrap' text at right edge of display
//  bool _cp437;          ///< If set, use correct CP437 charset (default is off)
//  GFXfont *gfxFont;     ///< Pointer to special font
//};
//
//  // "Classic" initButton() uses center & size
//  void initButton(int16_t x, int16_t y, uint16_t w,
//                  uint16_t h, uint16_t outline, uint16_t fill,
//                  uint16_t textcolor, char *label, uint8_t textsize);
//  void initButton(int16_t x, int16_t y, uint16_t w,
//                  uint16_t h, uint16_t outline, uint16_t fill,
//                  uint16_t textcolor, char *label, uint8_t textsize_x,
//                  uint8_t textsize_y);
//  // New/alt initButton() uses upper-left corner & size
//  void initButtonUL(int16_t x1, int16_t y1, uint16_t w,
//                    uint16_t h, uint16_t outline, uint16_t fill,
//                    uint16_t textcolor, char *label, uint8_t textsize);
//  void initButtonUL(int16_t x1, int16_t y1, uint16_t w,
//                    uint16_t h, uint16_t outline, uint16_t fill,
//                    uint16_t textcolor, char *label, uint8_t textsize_x,
//                    uint8_t textsize_y);
//  void drawButton(bool inverted = false);
//  bool contains(int16_t x, int16_t y);
//
//  /**********************************************************************/
//  /*!
//    @brief    Sets button state, should be done by some touch function
//    @param    p  True for pressed, false for not.
//  */
//  /**********************************************************************/
//  void press(bool p) {
//    laststate = currstate;
//    currstate = p;
//  }
//
//  bool justPressed();
//  bool justReleased();
//
//  /**********************************************************************/
//  /*!
//    @brief    Query whether the button is currently pressed
//    @returns  True if pressed
//  */
//  /**********************************************************************/
//  bool isPressed(void) { return currstate; };
//
//  int16_t _x1, _y1; // Coordinates of top-left corner
//  uint16_t _w, _h;
//  uint8_t _textsize_x;
//  uint8_t _textsize_y;
//  uint16_t _outlinecolor, _fillcolor, _textcolor;
//  char _label[10];

//
///// A GFX 1-bit canvas context for graphics
//class GFXcanvas1 : public Adafruit_GFX {
//public:
//  GFXcanvas1(uint16_t w, uint16_t h);
//  ~GFXcanvas1(void);
//  void drawPixel(int16_t x, int16_t y, int color);
//  void fillScreen(int color);
//  void drawFastVLine(int16_t x, int16_t y, int16_t h, int color);
//  void drawFastHLine(int16_t x, int16_t y, int16_t w, int color);
//  bool getPixel(int16_t x, int16_t y) const;
//  /**********************************************************************/
//  /*!
//    @brief    Get a pointer to the internal buffer memory
//    @returns  A pointer to the allocated buffer
//  */
//  /**********************************************************************/
//  uint8_t *getBuffer(void) const { return buffer; }
//
//protected:
//  bool getRawPixel(int16_t x, int16_t y) const;
//  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, int color);
//  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, int color);
//
//private:
//  uint8_t *buffer;
//
//#ifdef __AVR__
//  // Bitmask tables of 0x80>>X and ~(0x80>>X), because X>>Y is slow on AVR
//  static const uint8_t PROGMEM GFXsetBit[], GFXclrBit[];
//#endif
//};
//
///// A GFX 8-bit canvas context for graphics
//class GFXcanvas8 : public Adafruit_GFX {
//public:
//  GFXcanvas8(uint16_t w, uint16_t h);
//  ~GFXcanvas8(void);
//  void drawPixel(int16_t x, int16_t y, int color);
//  void fillScreen(int color);
//  void drawFastVLine(int16_t x, int16_t y, int16_t h, int color);
//  void drawFastHLine(int16_t x, int16_t y, int16_t w, int color);
//  uint8_t getPixel(int16_t x, int16_t y) const;
//  /**********************************************************************/
//  /*!
//   @brief    Get a pointer to the internal buffer memory
//   @returns  A pointer to the allocated buffer
//  */
//  /**********************************************************************/
//  uint8_t *getBuffer(void) const { return buffer; }
//
//protected:
//  uint8_t getRawPixel(int16_t x, int16_t y) const;
//  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, int color);
//  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, int color);
//
//private:
//  uint8_t *buffer;
//};
//
/////  A GFX 16-bit canvas context for graphics
//class GFXcanvas16 : public Adafruit_GFX {
//public:
//  GFXcanvas16(uint16_t w, uint16_t h);
//  ~GFXcanvas16(void);
//  void drawPixel(int16_t x, int16_t y, int color);
//  void fillScreen(int color);
//  void byteSwap(void);
//  void drawFastVLine(int16_t x, int16_t y, int16_t h, int color);
//  void drawFastHLine(int16_t x, int16_t y, int16_t w, int color);
//  uint16_t getPixel(int16_t x, int16_t y) const;
//  /**********************************************************************/
//  /*!
//    @brief    Get a pointer to the internal buffer memory
//    @returns  A pointer to the allocated buffer
//  */
//  /**********************************************************************/
//  uint16_t *getBuffer(void) const { return buffer; }
//
//protected:
//  uint16_t getRawPixel(int16_t x, int16_t y) const;
//  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, int color);
//  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, int color);
//
//private:
//  uint16_t *buffer;
//};

#endif // _ADAFRUIT_GFX_H
