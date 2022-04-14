#include <M5Stack.h>
#include "facesEncoder.h"

// facesEncoder class constructor.
facesEncoder::facesEncoder()
{
  addr = Faces_Encoder_I2C_ADDR;  
}

// facesEncoder class constructor with argument.
facesEncoder::facesEncoder( uint8_t i2caddr )
{
  addr = i2caddr;  
}

// facesEncoder class destructor.
facesEncoder::~facesEncoder()
{
}

bool facesEncoder::check( void )
{
  currentPosition = 0;
  incrementMultiplier = 1;
  for ( int index = 0; index < Faces_Encoder_RingLight_Count; index++ ) {
    currentRingLight[index].colorRed = 0;
    currentRingLight[index].colorGreen = 0;
    currentRingLight[index].colorBlue = 0;
  }
  return getEncoderValue();
}

void facesEncoder::setEncoderPosition( int16_t position )
{
  currentPosition = position;
}

void facesEncoder::setIncrementMultiplier( int16_t multiplier )
{
  incrementMultiplier = multiplier;
}

bool facesEncoder::getEncoderValue( void )
{
  Wire.requestFrom( (int)addr, 2 );
  if ( Wire.available() == 2 ){
    encoderValue = Wire.read();
    encoderButton = Wire.read();
    return true;
  }
  return false;
}

int16_t facesEncoder::getCurrentPosition( void )
{
  encoderValue = 0;
  if ( getEncoderValue() ) {
    if ( encoderValue > 127 ) { // anti-clockwise
      incrementPosition = (int16_t)encoderValue - 256;
    } else {
      incrementPosition = (int16_t)encoderValue;
    }
    currentPosition += incrementPosition * incrementMultiplier;
  }
  
  return currentPosition;
}

bool facesEncoder::buttonIsPressed( void )
{
  return encoderButton ? false : true;
}

uint8_t facesEncoder::ringLight( int index, uint8_t r, uint8_t g, uint8_t b )
{
  if ( ( currentRingLight[index].colorRed != r ) || ( currentRingLight[index].colorGreen != g ) || ( currentRingLight[index].colorBlue != b ) ) {
    currentRingLight[index].colorRed = r;
    currentRingLight[index].colorGreen = g;
    currentRingLight[index].colorBlue = b;
    delay( 5 );   // Latency for continuous access
    Wire.beginTransmission( addr );
    Wire.write( index );
    Wire.write( r );
    Wire.write( g );
    Wire.write( b );
    return Wire.endTransmission();
  }
  return 0;
}

void facesEncoder::ringLight( uint8_t r, uint8_t g, uint8_t b )
{
  for ( int index = 0; index < Faces_Encoder_RingLight_Count; index++ ) {
    ringLight( index, r, g, b );
  }
}

void facesEncoder::ringLight( uint16_t *patternTable, uint16_t delayTime, uint8_t r, uint8_t g, uint8_t b )
{
  for ( int n = 0; n < 100; n++ ) {
    uint16_t pattern = patternTable[n];
    if ( pattern == RINGLIGHT_BIT_END ) return;

    uint16_t mbit = RINGLIGHT_BIT0;
    for ( int index = 0; index < Faces_Encoder_RingLight_Count; index++ ) {
      if ( pattern & mbit ) {
        ringLight( index, r, g, b );
      } else {
        ringLight( index, 0, 0, 0 );
      }
      mbit <<= 1;
    }
    delay( delayTime );
  }
}

uint8_t facesEncoder::ringLight( int index, ledColorInfo_t ledColor )
{
  return ringLight( index, ledColor.colorRed, ledColor.colorGreen, ledColor.colorBlue );
}

void facesEncoder::ringLight( ledColorInfo_t ledColor )
{
  ringLight( ledColor.colorRed, ledColor.colorGreen, ledColor.colorBlue );
}

void facesEncoder::ringLight( uint16_t *patternTable, uint16_t delayTime, ledColorInfo_t ledColor )
{
  ringLight( patternTable, delayTime, ledColor.colorRed, ledColor.colorGreen, ledColor.colorBlue );
}
