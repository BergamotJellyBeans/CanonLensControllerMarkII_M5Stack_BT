
// buttonEx

/*
	buttonEx.h
    This module is a button and label class for M5Stack.

	Copyright (C) 2022 by bergamot-jellybeans.

  Date-written. Feb 16,2019.
  Last-modify.  Feb 28,2022.
  mailto:			bergamot.jellybeans@icloud.com
*/



#ifndef _BUTTONEX_H_
#define _BUTTONEX_H_

#include <M5Stack.h>

#define taLeftJustify   0	// Text alignment Left-aligned
#define taCenter        1	// Text alignment center-aligned
#define taRightJustify  2	// Text alignment Right-aligned

#define btnLeftCenter   54		// M5Stack A button center position
#define btnCenterCenter 160		// M5Stack B button center position
#define btnRightCenter  266		// M5Stack C button center position

class ButtonEx {
  public:
    ButtonEx( uint16_t tag_, uint16_t x_, uint16_t y_, uint16_t w_, uint16_t h_, const char* name_ = "" );
    ButtonEx( uint16_t tag_, uint16_t x_, uint16_t y_, uint16_t w_, uint16_t h_, uint16_t a_, const char* name_ = "" );
    ~ButtonEx();
    void frameRect( uint16_t frameColor, uint16_t fillColor );
    void frameRect( uint16_t frameColor, uint16_t fillColor, int16_t radius );
    void frameCircle( uint16_t frameColor, uint16_t fillColor );
    void setCaption( uint8_t textSize, char* fmt, ... );
    void setCaption( uint8_t textSize, String caption );
    void setEnabled( bool flag );
    void setVisible( bool flag );
    bool alternate;
    int16_t tag;
    int32_t value;
  private:
    int16_t bx, by, bw, bh;
    int16_t cx, cy;
    uint8_t captionTextSize;
    bool enabled;
    bool visible;
    char captionStr[32];
};

class LabelEx {
  public:
    LabelEx( uint16_t x_, uint16_t y_, uint16_t w_, uint16_t h_ );
    ~LabelEx();
    void frameRect( uint16_t frameColor, uint16_t fillColor );
    void frameRect( uint16_t frameColor, uint16_t fillColor, int16_t radius );
    void caption( uint16_t textColor, char* fmt, ... );
    void caption( uint16_t textColor, String captionStr );
    int16_t alignment;
    int16_t tag;
    int16_t textBaseOffset;
    int8_t textSize;
  private:
    int16_t bx, by, bw, bh;
    int16_t cx, cy;
    int16_t bFillColor;
    int16_t bRadius;
    uint8_t captionTextSize;
    char captionStr[32];
};

#endif
