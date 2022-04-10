
// buttonEx

/*
	buttonEx.cpp
    This module is a button and label class for M5Stack.

	Copyright (C) 2022 by bergamot-jellybeans.

  Date-written. Feb 16,2019.
  Last-modify.  Feb 28,2022.
  mailto:			bergamot.jellybeans@icloud.com
*/

#include <M5Stack.h>
#include <stdarg.h>
#include <stdio.h>
#include "buttonEx.h"

// ButtonEx class constructor with argument.
ButtonEx::ButtonEx( uint16_t tag_, uint16_t x_, uint16_t y_, uint16_t w_, uint16_t h_, const char* name_ /*= ""*/ )
{
  tag = tag_;
  enabled = true;
  visible = true;
  value = 0;
  alternate = false;
  bx = x_;
  by = y_;
  bw = w_;
  bh = h_;
  cx = bx + ( bw / 2 ); 
  cy = by + ( bh / 2 );
}

ButtonEx::ButtonEx( uint16_t tag_, uint16_t x_, uint16_t y_, uint16_t w_, uint16_t h_, uint16_t a_, const char* name_ /*= ""*/ )
{
  tag = tag_;
  enabled = true;
  visible = true;
  value = 0;
  alternate = false;
  
  bw = w_;
  bh = h_;
  switch ( a_ ) {
    case taCenter:
      bx = x_ - ( bw / 2 );
      by = y_;
      break;
    case taRightJustify:
      bx = x_ - bw;
      by = y_;
      break;
    default:
      bx = x_;
      by = y_;
      break;
  }
  cx = bx + ( bw / 2 ); 
  cy = by + ( bh / 2 );
}

ButtonEx::~ButtonEx()
{
}

void ButtonEx::setCaption( uint8_t textSize, char* fmt, ... )
{
  va_list ap;
  char buff[32];

  va_start( ap, fmt );
  vsprintf( buff, fmt, ap );
  va_end( ap );
  strcpy( captionStr, buff );
  captionTextSize = textSize;
}

void ButtonEx::setCaption( uint8_t textSize, String caption )
{
	strcpy( captionStr, caption.c_str() );
	captionTextSize = textSize;
}

void ButtonEx::frameRect( uint16_t frameColor, uint16_t fillColor )
{
	frameRect( frameColor, fillColor, 1 );
}

void ButtonEx::frameRect( uint16_t frameColor, uint16_t fillColor, int16_t radius )
{
  int cy1 = cy - M5.Lcd.fontHeight( captionTextSize ) / 2; 
  M5.Lcd.drawRoundRect( bx, by, bw, bh, radius, frameColor );
  M5.Lcd.fillRoundRect( bx+1, by+1, bw-2, bh-2, radius, fillColor );
  M5.Lcd.setTextColor( frameColor );
  M5.Lcd.drawCentreString( captionStr, cx, cy1, captionTextSize );
}

void ButtonEx::frameCircle( uint16_t frameColor, uint16_t fillColor )
{
  int rx = bw / 2;
  int ry = bh / 2;
  M5.Lcd.drawEllipse( cx, cy, rx, ry, frameColor );
  M5.Lcd.fillEllipse( cx, cy, rx - 1, ry - 1, fillColor );
}

// LabelEx class
LabelEx::LabelEx( uint16_t x_, uint16_t y_, uint16_t w_, uint16_t h_ )
{
  tag = 0;
  alignment = taLeftJustify;
  textSize = 1;
  textBaseOffset = 0;
  bx = x_;
  by = y_;
  bw = w_;
  bh = h_;
  cx = bx + ( bw / 2 ); 
  cy = by + ( bh / 2 );
}
LabelEx::~LabelEx()
{
}

void LabelEx::caption( uint16_t textColor, char* fmt, ... )
{
  va_list ap;
  char buff[256];

  va_start( ap, fmt );
  vsprintf( buff, fmt, ap );
  va_end( ap );
  caption( textColor, String( buff ) );
}

void LabelEx::caption( uint16_t textColor, String captionStr )
{
  int cx1, cy1;
  cy1 = cy - M5.Lcd.fontHeight( textSize ) / 2; 
  cy1 -= textBaseOffset;
  M5.Lcd.fillRoundRect( bx+1, by+1, bw-2, bh-2, bRadius, bFillColor );
  M5.Lcd.setTextColor( textColor );
  switch ( alignment ) {
    case taCenter:
      cx1 = cx;
      M5.Lcd.drawCentreString( captionStr, cx1, cy1, textSize );
      break;
    case taRightJustify:
      cx1 = bx + bw - 1;
      M5.Lcd.drawRightString( captionStr, cx1, cy1, textSize );
      break;
    default:
      cx1 = bx + 2;
      M5.Lcd.drawString( captionStr, cx1, cy1, textSize );
      break;
  }
//  Serial.printf( "fontHeight=%d cx=%d, cy1=%d, %s\n", M5.Lcd.fontHeight( textSize ), cx1, cy1, captionStr );
}

void LabelEx::frameRect( uint16_t frameColor, uint16_t fillColor )
{
  frameRect( frameColor, fillColor, 1 );
}

void LabelEx::frameRect( uint16_t frameColor, uint16_t fillColor, int16_t radius )
{
  int cy1 = cy - M5.Lcd.fontHeight( captionTextSize ) / 2; 
  bFillColor = fillColor;
  bRadius = radius;
  M5.Lcd.drawRoundRect( bx, by, bw, bh, radius, frameColor );
  M5.Lcd.fillRoundRect( bx+1, by+1, bw-2, bh-2, bRadius, bFillColor );
  M5.Lcd.setTextColor( frameColor );
  M5.Lcd.drawCentreString( captionStr, cx, cy1, captionTextSize );
}
