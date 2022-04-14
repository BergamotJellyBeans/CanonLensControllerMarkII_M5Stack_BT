// encoder

/*
  facesEncoder.h
    ENCODER is compatible with FACE Kit. You can have it replace the key-card panel inside the FACE kit.
    It is designed for rotary encoder control, integrated Mega328 microprocessor inside and LEDs around the encoder.
    The series communication protocol between M5 core and ENCODER is I2C (The default I2c address is: 0x5E)

  Copyright (C) 2022 by bergamot-jellybeans.

  Date-written. Apr 12,2022.
  Last-modify.  Apr 12,20221.
  mailto:   bergamot.jellybeans@icloud.com

  -Overview of the functions
*/

#ifndef FACESENCODER_H
#define  FACESENCODER_H

//#include <M5Stack.h>
#define Faces_Encoder_I2C_ADDR  0x5E
#define Faces_Encoder_RingLight_Count 12
#define RINGLIGHT_BIT0  0x0001
#define RINGLIGHT_BIT1  0x0002
#define RINGLIGHT_BIT2  0x0004
#define RINGLIGHT_BIT3  0x0008
#define RINGLIGHT_BIT4  0x0010
#define RINGLIGHT_BIT5  0x0020
#define RINGLIGHT_BIT6  0x0040
#define RINGLIGHT_BIT7  0x0080
#define RINGLIGHT_BIT8  0x0100
#define RINGLIGHT_BIT9  0x0200
#define RINGLIGHT_BIT10 0x0400
#define RINGLIGHT_BIT11 0x0800
#define RINGLIGHT_BIT_END 0xFFFF

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t colorRed;
  uint8_t colorGreen;
  uint8_t colorBlue;
} ledColorInfo_t;

class facesEncoder
{
private:
  uint8_t addr;
  uint8_t encoderValue;
  uint8_t encoderButton;
  int16_t incrementPosition;
  int16_t incrementMultiplier;
  int16_t currentPosition;
  ledColorInfo_t currentRingLight[Faces_Encoder_RingLight_Count];

  bool getEncoderValue( void );
public:
  facesEncoder();
  facesEncoder( uint8_t i2caddr );
  ~facesEncoder();

  bool check( void );
  void setEncoderPosition( int16_t position );
  void setIncrementMultiplier( int16_t multiplier );
  int16_t getCurrentPosition( void );
  bool buttonIsPressed( void );
  uint8_t ringLight( int index, uint8_t r, uint8_t g, uint8_t b );
  void ringLight( uint8_t r, uint8_t g, uint8_t b );
  void ringLight( uint16_t *patternTable, uint16_t delayTime, uint8_t r, uint8_t g, uint8_t b );
  uint8_t ringLight( int index, ledColorInfo_t ledColor );
  void ringLight( ledColorInfo_t ledColor );
  void ringLight( uint16_t *patternTable, uint16_t delayTime, ledColorInfo_t ledColor );

};

#ifdef  __cplusplus
}
#endif

#endif  /* FACESENCODER_H */
