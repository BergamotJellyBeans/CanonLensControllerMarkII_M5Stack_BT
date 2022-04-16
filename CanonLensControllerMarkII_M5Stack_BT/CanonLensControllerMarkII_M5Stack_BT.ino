/*
  ASCOM Canon EF Lens Controller by ASTROMECHANICS

Protocol description
  User level software communicates with ASCOM Lens Controller via a simple commands transfer protocol.
  Commands transmit as a specially formed text units.
  You can use this protocol autonomously or embed it in your software products.
  You can use a development environment that allows you to interact with COM ports.

  Now you can try to type some commands. All commands terminate by # symbol.
  List of available commands and their syntax is shown in the table.

  1.P#
    Get current focus position (P# 4800#)

  2.Mxxxx#
    Move the focus position to xxxx absolute value (M5200#).
    This command doesn't send a reply.

  3.Axx#
    Set the aperture value. NOTE: xx value is a difference between the aperture value you want and maximum aperture value (xx=0) available for your lens.
    For example: f1.8 -> f2.8 with Canon EF 50 mm f1.8 - A04#. A00# return aperture value to f/1.8.
    This is easy to understand if you look at AV series for your lens.
    Canon EF 50 mm f/1.8 | 1.8 2.0 2.2 2.5 2.8 3.2 3.5 4.0 4.5 5.0 5.6 6.3 7.1 8 9 10 11 13 14 16 18 20 22
    This command doesn't send a reply.


	M5Stack version

	This program was created by Bergamot for the M5Stack.
	and It requires an HSB host module in addition to the M5Stack CPU module.

	Copyright (C) 2022 by bergamot-jellybeans.

	Date-written.	Jan 06,2022.
	Last-modify.	Apr 14,2022.
	mailto:			bergamot.jellybeans@icloud.com

  Revision history
  Feb 16,2022
    Add battery indicator of one second interval.
  Feb 17,2022
    Button "UP" and Button "DOWN" are swapped.
  Mar 03,2022
    Changed the method of get the battery charge level.
  Apr 10,2022
    Added Bluetooth remote control function.
    The program name has been changed.    
  Apr 12,2022
    Added Faces encoder allows for focus control.
  Apr 16,2022
    Added backlight control.
    
*/

// ----------------------------------------------------------------------------------------------------------
// FIRMWARE CODE START
#include <M5Stack.h>
#include "stringQueue.h"	//  By Steven de Salas
#include <cdcftdi.h>
#include <usbhub.h>
#include "IniFiles.h"
#include "ButtonEx.h"
#include "BluetoothSerial.h"
#include "facesEncoder.h"

int baud = 38400;   // for ASCOM Canon EF Lens Controller

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

#define MYINIFILENAME       "/canonLens.ini"
#define LENSINFOFILENAME    "/Lens.txt"
#define MAX_LENS        15      // number of lens list
#define MAX_APERTURE    32      // number of aperture list
#define QUEUELENGTH     10      // number of commands that can be saved in the serial queue
#define RECVLINES       32

// State machine phase
#define PHASE_WAIT_USB_CONNECT  0   // Waiting for the lens controller to be connected.
#define PHASE_LENS              1   // Lens selection in progress.
#define PHASE_APERTURE          2   // Aperture selection in progress.
#define PHASE_FOCUS             3   // Adjusting the focus position of the lens.
#define PHASE_WAIT_BT_CONNECT   10  // .

#define BATTERYUPDATETIMEMS 2500
#define RGB(r,g,b) (int16_t)( b + (g << 5 ) + ( r << 11 ) )

typedef struct {
  String lines;
  String lensName;
  int numberOfAperture;
  String aperture[MAX_APERTURE];
} lensInfo_t;

typedef struct {
  int phase;
  int lensIndex;
  int apertureIndex;
  int focusPosition;
  int remoconMode;
  String macBTString;
  uint8_t macBT[6];
} systemParameter_t;

typedef struct {
  uint8_t currentBrightness;
  uint8_t wakeupBrightness;
  uint8_t sleepBrightness;
  unsigned long secondsToDim;
  unsigned long dimmerCounter;
  bool inSpeeping;
} backLightControl_t;

// FTDI Async class
class FTDIAsync : public FTDIAsyncOper {
  public:
    bool flagOnInit;
    uint8_t OnInit( FTDI *pftdi );
};

uint8_t FTDIAsync::OnInit( FTDI *pftdi ) {
  uint8_t rcode = 0;

  rcode = pftdi->SetBaudRate( baud );  // for ASCOM Canon EF Lens Controller

  if ( rcode ) {
    Serial.printf( "rode=%d\n", rcode );
    ErrorMessage<uint8_t>( PSTR( "SetBaudRate" ), rcode );
    return rcode;
  }
  rcode = pftdi->SetFlowControl( FTDI_SIO_DISABLE_FLOW_CTRL );

  if (rcode){
    Serial.printf( "rode=%d\n", rcode );
    ErrorMessage<uint8_t>( PSTR( "SetFlowControl" ), rcode );
  }

  Serial.println( "OnInit" );
  flagOnInit = true;
  return rcode;
}

// USB settings
USB              Usb;
FTDIAsync        FtdiAsync;
FTDI             Ftdi( &Usb, &FtdiAsync );
StringQueue queueUSB( QUEUELENGTH );       // receive serial queue of commands
int recvLineUSBIndex;
char recvLineUSB[RECVLINES];

// BluetoothSerial
BluetoothSerial SerialBT;
StringQueue queueBT( QUEUELENGTH );       // receive serial queue of commands
int recvLineBTIndex;
char recvLineBT[RECVLINES];

// Faces Encoder
facesEncoder encoder;
bool useEncoder;
int16_t latestEncoderPosition;
bool latestButtonStatus;
int16_t incremet;
int16_t currentLightIndicator;
ledColorInfo_t ledColorConnect;
ledColorInfo_t ledColorIndicator1;
ledColorInfo_t ledColorIndicator10;
ledColorInfo_t ledColorIndicatorSleep1;
ledColorInfo_t ledColorIndicatorSleep10;
backLightControl_t backLight;

unsigned long batteryUpdateTime;
int lastBatteryLevel;
int numberOfLens;
int connectBT;
String myMacBTString;
systemParameter_t systemParam;
systemParameter_t compareParam;
lensInfo_t *selectlensInfo;
uint8_t virtualKeyMap[3];

lensInfo_t lensInfo[MAX_LENS];
ButtonEx* buttonScan;
LabelEx* labelStatus;
LabelEx* labelLensNameTitle;
LabelEx* labelApertureTitle;
LabelEx* labelFocusTitle;
LabelEx* labelLensName;
LabelEx* labelAperture;
LabelEx* labelFocus;
LabelEx* labelBtnA;
LabelEx* labelBtnB;
LabelEx* labelBtnC;
LabelEx* labelMacBT;

const uint16_t connectRingLitPattern[] = {
  RINGLIGHT_BIT0 | RINGLIGHT_BIT11,
  RINGLIGHT_BIT1 | RINGLIGHT_BIT10,
  RINGLIGHT_BIT2 | RINGLIGHT_BIT9,
  RINGLIGHT_BIT3 | RINGLIGHT_BIT8,
  RINGLIGHT_BIT4 | RINGLIGHT_BIT7,
  RINGLIGHT_BIT5 | RINGLIGHT_BIT6,
  RINGLIGHT_BIT4 | RINGLIGHT_BIT7,
  RINGLIGHT_BIT3 | RINGLIGHT_BIT8,
  RINGLIGHT_BIT2 | RINGLIGHT_BIT9,
  RINGLIGHT_BIT1 | RINGLIGHT_BIT10,
  RINGLIGHT_BIT0 | RINGLIGHT_BIT11,
  0,
  RINGLIGHT_BIT_END,
};
       
void perserUSB( void );
void perserBT( void );

// ---------------------------------------------------------------------------------------------------------
// DO NOT CHANGE
//#define DEBUG     1
//#define DEBUGHPSW 1

// ---------------------------------------------------------------------------------------------------------
// CODE START

void software_Reboot()
{
//  asm volatile ( "jmp 0");        // jump to the start of the program
}

int argumentSeparatorString( String inString, String *StringList, int separator, int maxElements )
{
  int n = 0;
  int s1;
  
  // Parse an inString.
  while ( inString.length() > 0 ) {
    s1 = inString.indexOf( separator );  // Find the separator in the string.
    if ( s1 > 0 ) {
      StringList[n++] = inString.substring( 0, s1 );
      inString.remove( 0, s1 + 1 );
      if ( n >= maxElements ) return n;
    } else {
      StringList[n++] = inString;
      break;          
    }
  }
  return n;
}

uint8_t atox1( const char *p )
{
  uint8_t c, d;

  c = toupper( *p );
  d = ( isdigit( c ) ) ? c - '0' : c - 'A' + 10;
  return d;
}

uint8_t atox2( const char *p )
{
  uint8_t d;

  d  = atox1( p++ ) << 4;
  d += atox1( p );
  return d;
}

// 01234567890123456
// XX:XX:XX:XX:XX:XX
void decodeMacAddressString( String macAddrStr, uint8_t macaddr[6] )
{
  const char *p = macAddrStr.c_str();
  macaddr[0] = atox2( &p[0] );
  macaddr[1] = atox2( &p[3] );
  macaddr[2] = atox2( &p[6] );
  macaddr[3] = atox2( &p[9] );
  macaddr[4] = atox2( &p[12] );
  macaddr[5] = atox2( &p[15] );
}

// Load the lens information list from the micro SD card.
bool readLensInfoFile( void )
{
  IniFiles ini( MAX_LENS );
  bool validFile = ini.open( SD, LENSINFOFILENAME );

  Serial.printf( "Open : %s %d\n", LENSINFOFILENAME, validFile );

  numberOfLens = 0;
  for ( int i = 0; i < MAX_LENS; i++ ) {
    int s1;
    String key = "lens" + String( i+1 );
    String lines = ini.readString( key, "" );
    lensInfo_t *lensp = &lensInfo[i];
    s1 = lines.indexOf( '|' );  // Find the separator between the lens name and the aperture string.
//    Serial.printf( "Line(%s) : %s\n", key.c_str(), lines.c_str() );

    // Parse the lens name and aperture string.
    if ( s1 > 0 ) {
      String aLines = lines.substring( 0, s1 ); // cut a lens name of lines.
      lines.remove( 0, s1 + 1 ); // remove a lens name of lines.
      aLines.trim();
      lensp->lensName = aLines;
      lines.trim();
      numberOfLens++;
      Serial.printf( "LensName%d = %s\n", i, lensp->lensName.c_str() );
      Serial.printf( "Aperture%d = %s\n", i, lines.c_str() );

      // Aperture
      lensp->numberOfAperture = argumentSeparatorString( lines, lensp->aperture, ' ', MAX_APERTURE );
      Serial.printf( "numberOfAperture%d = %d\n", i, lensp->numberOfAperture );
    }
  }

  ini.close( SD );
  return true;
}

// Display lens name on the labelLensName.
void lensSelect( void )
{
  selectlensInfo = &lensInfo[systemParam.lensIndex];
  int clFill = ( systemParam.phase == PHASE_LENS ) ? TFT_BLUE : TFT_BLACK;
  labelLensName->frameRect( TFT_WHITE, clFill, 4 );
  labelLensName->caption( TFT_WHITE, selectlensInfo->lensName );
}

// Set the lens to the index of the argument <sel>
void lensSelect( int sel )
{
  systemParam.lensIndex = sel;
  lensSelect();
}

// Move the lens list to the next index.
void lensSelectNext( void )
{
  int nsel = systemParam.lensIndex + 1;
  if ( nsel >= numberOfLens ) {
    nsel = 0;
  }
  lensSelect( nsel );
}

// Move the lens list to the previous index.
void lensSelectPrev( void )
{
  int nsel = systemParam.lensIndex - 1;
  if ( nsel < 0 ) {
    nsel = numberOfLens - 1;
  }
  lensSelect( nsel );
}

// Display aperture name on the labelAperture.
void apertureSelect( void )
{
  int clFill = ( systemParam.phase == PHASE_APERTURE ) ? TFT_RED : TFT_BLACK;
  labelAperture->frameRect( TFT_WHITE, clFill, 4 );
  labelAperture->caption( TFT_WHITE, selectlensInfo->aperture[systemParam.apertureIndex] );
}

// Set the aperture to the index of the argument <sel>
void apertureSelect( int sel )
{
  systemParam.apertureIndex = sel;
  apertureSelect();
  setApertureValue( sel );
}

// Move the aperture to the next index.
void apertureSelectNext( void )
{
  int nsel = systemParam.apertureIndex + 1;
  if ( nsel >= selectlensInfo->numberOfAperture ) {
    nsel = 0;
  }
  apertureSelect( nsel );
}

// Move the aperture to the previous index.
void apertureSelectPrev( void )
{
  int nsel = systemParam.apertureIndex - 1;
  if ( nsel < 0 ) {
    nsel = selectlensInfo->numberOfAperture - 1;
  }
  apertureSelect( nsel );
}

// Display focus position on the labelFocus.
void focusPosition( void )
{
  int clFill = ( systemParam.phase == PHASE_FOCUS ) ? TFT_RED : TFT_BLACK;
  labelFocus->frameRect( TFT_WHITE, clFill, 4 );
  labelFocus->caption( TFT_WHITE, "%d", systemParam.focusPosition );
}

// Set the focus to the position of the argument <sel>
void focusPosition( int sel )
{
  systemParam.focusPosition = sel;
  focusPosition();
  setFocusPosition( sel );
}

// Move the focus position of the lens by the incremental argument <incstep>.
void focusPositionIncrease( int incstep )
{
  int nsel = systemParam.focusPosition + incstep;
  focusPosition( nsel );
}

// Send aperture setting commands to the lens controller.
uint8_t setApertureValue( int index )
{
  uint8_t  rcode;
  char buff[16];

  sprintf( buff, "A%02d#", index );  
  Serial.printf( ">%s\n", buff );
  rcode = Ftdi.SndData( strlen( buff ), (uint8_t*)buff );
  return rcode;
}

// Send focus position setting commands to the lens controller.
uint8_t setFocusPosition( int position )
{
  uint8_t  rcode;
  char buff[16];

  sprintf( buff, "M%d#", position );  
  Serial.printf( ">%s\n", buff );
  rcode = Ftdi.SndData( strlen( buff ), (uint8_t*)buff );
  return rcode;
}

// Save the system settings to the micro SD card.
bool writeSystemFile( void )
{
  IniFiles ini( 16 );
  bool validFile = ini.open( SD, MYINIFILENAME );
  ini.writeInteger( "LensIndex", systemParam.lensIndex );
//  ini.writeInteger( "ApertureIndex", systemParam.apertureIndex );
  ini.close( SD );
  return validFile;
}

// Load the system settings from the micro SD card.
bool readSystemFile( void )
{
  IniFiles ini( 16 );
  bool validFile = ini.open( SD, MYINIFILENAME );
  systemParam.lensIndex = ini.readInteger( "LensIndex", 0 );
  systemParam.apertureIndex = ini.readInteger( "ApertureIndex", 0 );

  // If you want to run as a remote control, please write the mac address of the connection destination.
  // macBT=XX:XX:XX:XX:XX:XX
  systemParam.macBTString = ini.readString( "macBT", "" );
  Serial.printf( "macBTString = %s\n", systemParam.macBTString.c_str() );
  if ( systemParam.macBTString != "" ) {
    decodeMacAddressString( systemParam.macBTString, systemParam.macBT );
    systemParam.remoconMode = 1;
  } else {
    systemParam.remoconMode = 0;
  }

  String paramString;
  String paramStringList[3];
  int nParam;
  
  paramString = ini.readString( "ledColorConnect", "0 0 255" );
  nParam = argumentSeparatorString( paramString, paramStringList, ' ', 3 );
  ledColorConnect.colorRed = paramStringList[0].toInt();
  ledColorConnect.colorGreen = paramStringList[1].toInt();
  ledColorConnect.colorBlue = paramStringList[2].toInt();

  paramString = ini.readString( "ledColorIndicator1", "0 64 0" );
  nParam = argumentSeparatorString( paramString, paramStringList, ' ', 3 );
  ledColorIndicator1.colorRed = paramStringList[0].toInt();
  ledColorIndicator1.colorGreen = paramStringList[1].toInt();
  ledColorIndicator1.colorBlue = paramStringList[2].toInt();
  ledColorIndicatorSleep1.colorRed = ledColorIndicator1.colorRed / 2;
  ledColorIndicatorSleep1.colorGreen = ledColorIndicator1.colorGreen / 2;
  ledColorIndicatorSleep1.colorBlue = ledColorIndicator1.colorBlue / 2;

  paramString = ini.readString( "ledColorIndicator10", "64 0 0" );
  nParam = argumentSeparatorString( paramString, paramStringList, ' ', 3 );
  ledColorIndicator10.colorRed = paramStringList[0].toInt();
  ledColorIndicator10.colorGreen = paramStringList[1].toInt();
  ledColorIndicator10.colorBlue = paramStringList[2].toInt();
  ledColorIndicatorSleep10.colorRed = ledColorIndicator10.colorRed / 2;
  ledColorIndicatorSleep10.colorGreen = ledColorIndicator10.colorGreen / 2;
  ledColorIndicatorSleep10.colorBlue = ledColorIndicator10.colorBlue / 2;

  backLight.wakeupBrightness = ini.readInteger( "backLightWakeupBrightness", 128 );
  if ( !ini.isExists( "backLightWakeupBrightness" ) ) {
    Serial.printf( "backLightWakeupBrightness = %d\n", backLight.wakeupBrightness );
    ini.writeInteger( "backLightWakeupBrightness", backLight.wakeupBrightness );
  }
  backLight.sleepBrightness = ini.readInteger( "backLightSleepBrightness", 16 );
  if ( !ini.isExists( "backLightSleepBrightness" ) ) {
    ini.writeInteger( "backLightSleepBrightness", backLight.sleepBrightness );
  }
  backLight.secondsToDim = ini.readInteger( "backLightsecondsToDim", 30 );
  if ( !ini.isExists( "backLightsecondsToDim" ) ) {
    ini.writeInteger( "backLightsecondsToDim", backLight.secondsToDim );
  }

  ini.close( SD );

  return validFile;
}

void indicateBatteryLevel( int batteryLevel )
{
  const int xBase = 256;
  const int yBase = 0;
  M5.Lcd.fillRect( xBase, yBase, 56, 21, TFT_WHITE );
  M5.Lcd.fillRect( xBase + 56, yBase + 4, 4, 13, TFT_WHITE );
  M5.Lcd.fillRect( xBase + 2, yBase + 2, 52, 17, TFT_BLACK );
  if ( batteryLevel <= 25 ) {
    M5.Lcd.fillRect( xBase + 3, yBase + 3, batteryLevel / 2, 15, TFT_RED ); // for low level alert.
  } else {
    M5.Lcd.fillRect( xBase + 3, yBase + 3, batteryLevel / 2, 15, TFT_GREEN ); // for normal state.
  }
}

void selectLensDisplay( void )
{
  labelMacBT->caption( TFT_WHITE, "" );
  labelApertureTitle->frameRect( TFT_BLACK, TFT_BLACK );
  labelApertureTitle->alignment = taLeftJustify;
  labelApertureTitle->textSize = 2;
  labelApertureTitle->caption( TFT_WHITE, "Aperture" );
  labelFocusTitle->frameRect( TFT_BLACK, TFT_BLACK );
  labelFocusTitle->alignment = taLeftJustify;
  labelFocusTitle->textSize = 2;
  labelFocusTitle->caption( TFT_WHITE, "Focus" );
  labelLensNameTitle->caption( TFT_WHITE, "Lens" );
  labelApertureTitle->caption( TFT_GREEN, "Aperture" );
  labelAperture->frameRect( TFT_WHITE, TFT_RED, 4 );
  labelAperture->alignment = taCenter;
  labelAperture->textSize = 6;
  labelAperture->textBaseOffset = -4;
  labelFocus->frameRect( TFT_WHITE, TFT_RED, 4 );
  labelFocus->alignment = taRightJustify;
  labelFocus->textSize = 6;
  labelFocus->textBaseOffset = -4;
}

void lightIndicator( void )
{
  if ( currentLightIndicator >= 0 ) {
    if ( incremet == 1 ) {
      if ( !backLight.inSpeeping ) {
        encoder.ringLight( currentLightIndicator, ledColorIndicator1 );
      } else {
        encoder.ringLight( currentLightIndicator, ledColorIndicatorSleep1 );
      }
    } else {
      if ( !backLight.inSpeeping ) {
        encoder.ringLight( currentLightIndicator, ledColorIndicator10 );
      } else {
        encoder.ringLight( currentLightIndicator, ledColorIndicatorSleep10 );
      }
    }
  }
}

void setBackLight( uint8_t brightness )
{
  backLight.currentBrightness = brightness;
  M5.Lcd.setBrightness( backLight.currentBrightness );
}

void sleepBackLight( void )
{
  backLight.inSpeeping = true;
  setBackLight( backLight.sleepBrightness );
}

void wakeupBackLight( void )
{
  backLight.inSpeeping = false;
  backLight.dimmerCounter = millis() + ( backLight.secondsToDim * 1000 );  // goto sleep time
  setBackLight( backLight.wakeupBrightness );
}

bool shallWeGoToSleep( void )
{
  if ( !backLight.inSpeeping ) {
    if ( backLight.dimmerCounter < millis() ) {
      sleepBackLight();
      if ( useEncoder ) {
        lightIndicator();
      }
    }
  }
  return backLight.inSpeeping;
}

bool shallWeWakeup( void )
{
  if ( backLight.inSpeeping ) {
    wakeupBackLight();    
    if ( useEncoder ) {
      lightIndicator();
    }
  }
  return backLight.inSpeeping;
}

// --- Functions that are no longer used
// Get the battery information of the M5Stack.
#if 0
int8_t getBatteryLevel( void )
{
  Wire.beginTransmission( 0x75 ); // start I2C transmission.
  Wire.write( 0x78 ); // slave address.
  if ( Wire.endTransmission( false ) == 0 && Wire.requestFrom( 0x75, 1 ) ) {
    switch ( Wire.read() & 0xF0 ) {
    case 0xE0: return 25;   // battery level is about 25%.
    case 0xC0: return 50;   // battery level is about 50%.
    case 0x80: return 75;   // battery level is about 75%.
    case 0x00: return 100;  // battery level is full.
    default: return 0;      // battery level is empty.
    }
  }
  return -1;  // Battery information could not be obtained.
}
#endif

// Setup
/*************************************************************************
 * NAME  setup - 
 * SYNOPSIS
 *
 *    void setup( void )
 *
 * DESCRIPTION
 *  
 *************************************************************************/
void setup()
{
  // initialize the M5Stack object
  M5.begin( true, true, true, true );
  M5.Power.begin();
  Wire.begin();

  useEncoder = encoder.check();
  if ( useEncoder ) {
    Serial.printf( "Faces encoder connected.\n" );
  } else {
    Serial.printf( "Faces encoder not recognized.\n" );
  }
  
  currentLightIndicator = -1;
  latestEncoderPosition = 0;
  latestButtonStatus = false;
  batteryUpdateTime = 0;
  lastBatteryLevel = 0;
  recvLineUSBIndex = 0;
  recvLineBTIndex = 0;
  connectBT = 0;
  virtualKeyMap[0] = virtualKeyMap[1] = virtualKeyMap[2] = 0;

  FtdiAsync.flagOnInit = false;
  Serial.printf( "Start\n" );
  if ( M5.Power.canControl() ) {
    Serial.printf( "BatteryLevel = %d\n", M5.Power.getBatteryLevel() );
  } else {
    Serial.printf( "Battery control is not supported.\n" );
  }
    

  // Display base images
  M5.Lcd.fillScreen( TFT_BLACK );
  M5.Lcd.setTextSize( 1 );
  M5.Lcd.setTextColor( TFT_WHITE, TFT_BLACK );
  M5.Lcd.drawString( "ASCOM Canon EF Lens Controller", 0, 0, 2 );

  labelLensNameTitle = new LabelEx( 0, 32, 32, 16 );
  labelApertureTitle = new LabelEx( 0, 80, 64, 16 );
  labelFocusTitle = new LabelEx( 0, 132, 64, 16 );

  labelLensName = new LabelEx( 40, 24, 260, 32 );
  labelAperture = new LabelEx( 64, 64, 100, 48 );
  labelFocus = new LabelEx( 64, 120, 150, 48 );

  labelLensNameTitle->frameRect( TFT_BLACK, TFT_BLACK );
  labelLensNameTitle->alignment = taLeftJustify;
  labelLensNameTitle->textSize = 2;
  labelLensNameTitle->caption( TFT_WHITE, "Lens" );

  labelLensName->frameRect( TFT_WHITE, TFT_BLUE, 4 );
  labelLensName->alignment = taCenter;
  labelLensName->textSize = 2;

  labelStatus = new LabelEx( 0, 176, 319, 40 );
  labelStatus->frameRect( TFT_BLACK, TFT_BLACK, 4 );
  labelStatus->alignment = taLeftJustify;

  labelMacBT = new LabelEx( 0, 132, 319, 40 );
  labelMacBT->frameRect( TFT_BLACK, TFT_BLACK, 4 );
  labelMacBT->alignment = taLeftJustify;

  labelBtnA = new LabelEx( 26, 220, 76, 16 );
  labelBtnA->frameRect( TFT_WHITE, TFT_BLACK, 4 );
  labelBtnA->alignment = taCenter;
  labelBtnA->textSize = 2;

  labelBtnB = new LabelEx( 122, 220, 76, 16 );
  labelBtnB->frameRect( TFT_WHITE, TFT_BLACK, 4 );
  labelBtnB->alignment = taCenter;
  labelBtnB->textSize = 2;

  labelBtnC = new LabelEx( 218, 220, 76, 16 );
  labelBtnC->frameRect( TFT_WHITE, TFT_BLACK, 4 );
  labelBtnC->alignment = taCenter;
  labelBtnC->textSize = 2;

  labelBtnA->caption( TFT_WHITE, "SEL" );
  labelBtnB->caption( TFT_WHITE, "DOWN" );
  labelBtnC->caption( TFT_WHITE, "UP" );

  delay( 300 );

  readLensInfoFile();
  readSystemFile();

  wakeupBackLight();
  labelLensNameTitle->caption( TFT_GREEN, "Lens" );
  lensSelect();
  
  uint8_t macBT[6];
  char macBTbuff[32];
  esp_read_mac( macBT, ESP_MAC_BT );
  sprintf( macBTbuff, "%02X:%02X:%02X:%02X:%02X:%02X", macBT[0], macBT[1], macBT[2], macBT[3], macBT[4], macBT[5] );
  myMacBTString = String( macBTbuff );
  if ( systemParam.remoconMode ) {
    SerialBT.begin( "M5StackCLC", true ); // I am Host. Bluetooth device name
    labelStatus->caption( TFT_WHITE, "Attempting connect to controller %s", systemParam.macBTString.c_str() );
    systemParam.phase = PHASE_WAIT_BT_CONNECT;
  } else {
    labelMacBT->caption( TFT_WHITE, "macBT %s", myMacBTString.c_str() );
    SerialBT.begin( "M5StackCLC" ); // I am Devuce. Bluetooth device name
    String USB_STATUS;
    if ( Usb.Init() == -1 ) {
      Serial.println( "OSC did not start." );
      USB_STATUS = "OSC did not start.";
    }else{
      Serial.println( "OSC started." );
      USB_STATUS = "Waiting for the lens controller to be connected.";
    }
    labelStatus->caption( TFT_YELLOW, USB_STATUS );
    systemParam.phase = PHASE_WAIT_USB_CONNECT;
  }
  delay( 300 );

}

// Main Loop
/*************************************************************************
 * NAME  loop - 
 *
 * SYNOPSIS
 *
 *    void loop( void )
 *
 * DESCRIPTION
 *  
 *************************************************************************/
void loop( void )
{
  Usb.Task();
  M5.update();

  switch ( systemParam.phase ) {
  case PHASE_WAIT_USB_CONNECT:  // // Waiting for the lens controller to be connected.
    if ( FtdiAsync.flagOnInit ) {
      String USB_STATUS;
      USB_STATUS = "USB FTDI CDC Baud Rate:" + String( baud ) + "bps";
      labelStatus->caption( TFT_YELLOW, USB_STATUS );
      Ftdi.SndData( 2, (uint8_t*)"P#" );
      systemParam.phase = PHASE_LENS;
    }
    break;

  case PHASE_WAIT_BT_CONNECT:  // // Waiting for the Bluetooth serial to be connected.
    connectBT = SerialBT.connect( systemParam.macBT );
    Serial.printf( "connectBT=%d\n", connectBT );
    if ( connectBT ) {
      labelStatus->caption( TFT_YELLOW, "Connected to controller %s", systemParam.macBTString.c_str() );
      if ( useEncoder ) {
        encoder.ringLight( (uint16_t *)connectRingLitPattern, 20, ledColorConnect );
      }
      incremet = 1;
      currentLightIndicator = 0;
      lightIndicator();
      SerialBT.printf( "Q%s#", myMacBTString.c_str() );
      systemParam.phase = PHASE_LENS;
    }
    break;
  }

  if ( !systemParam.remoconMode ) {
    switch ( systemParam.phase ) {
    case PHASE_LENS:    // Lens selection in progress.
      if ( M5.BtnA.wasPressed() || ( virtualKeyMap[0] == 'A' ) ) {
        if ( virtualKeyMap[0] != 'A' ) {
          wakeupBackLight();
        }
        systemParam.phase = PHASE_APERTURE;
        selectLensDisplay();
        lensSelect();
        apertureSelect( 0 );
        focusPosition();
        writeSystemFile();
        SerialBT.printf( "P%d %d %d %d#", systemParam.phase, systemParam.lensIndex, systemParam.apertureIndex, systemParam.focusPosition );
      }
      if ( M5.BtnC.wasPressed() || ( virtualKeyMap[0] == 'C' ) ) {
        if ( virtualKeyMap[0] != 'C' ) {
          wakeupBackLight();
        }
        lensSelectNext();
        if ( connectBT ) {
          SerialBT.printf( "L%d %d#", systemParam.phase, systemParam.lensIndex );
        }
      }
      if ( M5.BtnB.wasPressed() || ( virtualKeyMap[0] == 'B' ) ) {
        if ( virtualKeyMap[0] != 'B' ) {
          wakeupBackLight();
        }
        lensSelectPrev();
        if ( connectBT ) {
          SerialBT.printf( "L%d %d#", systemParam.phase, systemParam.lensIndex );
        }
      }
      break;
    case PHASE_APERTURE:  // Aperture selection in progress.
      if ( M5.BtnA.wasPressed() || ( virtualKeyMap[0] == 'A' ) ) {
        if ( virtualKeyMap[0] != 'A' ) {
          wakeupBackLight();
        }
        systemParam.phase = PHASE_FOCUS;
        labelApertureTitle->caption( TFT_WHITE, "Aperture" );
        labelFocusTitle->caption( TFT_GREEN, "Focus" );
        apertureSelect();
        focusPosition();
        if ( connectBT ) {
          SerialBT.printf( "F%d %d#", systemParam.phase, systemParam.focusPosition );
        }
      }
      if ( M5.BtnC.wasPressed() || ( virtualKeyMap[0] == 'C' ) ) {
        if ( virtualKeyMap[0] != 'C' ) {
          wakeupBackLight();
        }
        apertureSelectNext();
        if ( connectBT ) {
          SerialBT.printf( "A%d %d#", systemParam.phase, systemParam.apertureIndex );
        }
      }
      if ( M5.BtnB.wasPressed() || ( virtualKeyMap[0] == 'B' ) ) {
        if ( virtualKeyMap[0] != 'B' ) {
          wakeupBackLight();
        }
        apertureSelectPrev();
        if ( connectBT ) {
          SerialBT.printf( "A%d %d#", systemParam.phase, systemParam.apertureIndex );
        }
      }
      break;
    case PHASE_FOCUS:   // Adjusting the focus position of the lens.
      if ( M5.BtnA.wasPressed() || ( virtualKeyMap[0] == 'A' ) ) {
        if ( virtualKeyMap[0] != 'A' ) {
          wakeupBackLight();
        }
        if ( M5.BtnC.isPressed() || ( virtualKeyMap[1] == 'C' ) ) { 
          focusPositionIncrease( +10 );
          if ( connectBT ) {
          SerialBT.printf( "F%d %d#", systemParam.phase, systemParam.focusPosition );
          }
        } else if ( M5.BtnB.isPressed() || ( virtualKeyMap[1] == 'B' ) ) {
          focusPositionIncrease( -10 );
          if ( connectBT ) {
          SerialBT.printf( "F%d %d#", systemParam.phase, systemParam.focusPosition );
          }
        } else {
          systemParam.phase = PHASE_APERTURE;
          labelFocusTitle->caption( TFT_WHITE, "focus" );
          labelApertureTitle->caption( TFT_GREEN, "Aperture" );
          focusPosition();
          apertureSelect();
          if ( connectBT ) {
          SerialBT.printf( "A%d %d#", systemParam.phase, systemParam.apertureIndex );
          }
        }
      }
      if ( M5.BtnC.wasPressed() || ( virtualKeyMap[0] == 'C' ) ) {
        if ( virtualKeyMap[0] != 'C' ) {
          wakeupBackLight();
        }
        focusPositionIncrease( +1 );
        if ( connectBT ) {
          SerialBT.printf( "F%d %d#", systemParam.phase, systemParam.focusPosition );
        }
      }
      if ( M5.BtnB.wasPressed() || ( virtualKeyMap[0] == 'B' ) ) {
        if ( virtualKeyMap[0] != 'B' ) {
          wakeupBackLight();
        }
        focusPositionIncrease( -1 );
        if ( connectBT ) {
          SerialBT.printf( "F%d %d#", systemParam.phase, systemParam.focusPosition );
        }
      }
      break;
    }
    virtualKeyMap[0] = virtualKeyMap[1] = virtualKeyMap[2] = 0;
  }
    
  if ( systemParam.remoconMode && connectBT ) {
    if ( M5.BtnA.wasPressed() ) {
      wakeupBackLight();
      if ( M5.BtnC.isPressed() ) { 
        SerialBT.printf( "BAC#" );
      } else if ( M5.BtnB.isPressed() ) {
        SerialBT.printf( "BAB#" );
      } else {
        SerialBT.printf( "BA #" );
      }
    } else if ( M5.BtnC.wasPressed() ) {
      wakeupBackLight();
      SerialBT.printf( "BC #" );
    } else if ( M5.BtnB.wasPressed() ) {
      wakeupBackLight();
      SerialBT.printf( "BB #" );
    }
    if ( useEncoder ) {
     // Rotate the encoder clockwise and the focus will be farther away.
     // Rotate the encoder counter-clockwise brings the focus closer.
      switch ( systemParam.phase ) {
      case PHASE_APERTURE:  // Aperture selection in progress.
      case PHASE_FOCUS:   // Adjusting the focus position of the lens.
        int16_t position = encoder.getCurrentPosition();
        int16_t diff = position - latestEncoderPosition;
        if ( diff != 0 ) {
          wakeupBackLight();
          SerialBT.printf( "f%d#", position );
          latestEncoderPosition = position;
          encoder.ringLight( currentLightIndicator, 0, 0, 0 );
          diff /= incremet;
          currentLightIndicator += diff;
          if ( currentLightIndicator >= Faces_Encoder_RingLight_Count ) {
            currentLightIndicator %= Faces_Encoder_RingLight_Count;
          } else if ( currentLightIndicator < 0 ) {
            currentLightIndicator += Faces_Encoder_RingLight_Count;
          }
          lightIndicator();
//          Serial.printf( "currentLightIndicator%d\n", currentLightIndicator );
        }
        bool buttonStatus = encoder.buttonIsPressed(); 
        if ( latestButtonStatus != buttonStatus ) {
          wakeupBackLight();
          if ( buttonStatus ) {
            incremet = ( incremet == 1 ) ? 10 : 1;
            encoder.setIncrementMultiplier( incremet );
            lightIndicator();
          }
          latestButtonStatus = buttonStatus;
        }
        break;
      }
    }
  }

  // Battery indicator of one second interval.
  // refererd by ProgramResource.net. Thanks ねふぁさん
  if ( !systemParam.remoconMode ) {
    if ( batteryUpdateTime < millis() ) {
      batteryUpdateTime = millis() + BATTERYUPDATETIMEMS;
      if ( M5.Power.canControl() ) {
        int batteryLevel = M5.Power.getBatteryLevel();
        if ( lastBatteryLevel != batteryLevel ) {
          // Rewrite only when the battery state changes.
          indicateBatteryLevel( batteryLevel );
          lastBatteryLevel = batteryLevel;
          if ( connectBT ) {
            SerialBT.printf( "V%d#", batteryLevel );
          }
        }
      }
    }
  }

  // USB data processing
  if ( !systemParam.remoconMode ) {
    perserUSB();
  }

  // Bluetooth serial data processing
  perserBT();

  // Sleep
  shallWeGoToSleep();

}

/*************************************************************************
 * NAME  perserUSB - 
 *
 * SYNOPSIS
 *
 *    void perserUSB( void )
 *
 * DESCRIPTION
 *  
 *************************************************************************/
void perserUSB( void )
{
  if ( queueUSB.count() > 0 ) {  // Check for serial command
    String replystr;
    replystr = queueUSB.pop();   // Take out receive data
    Serial.println( replystr );
    systemParam.focusPosition = replystr.toInt(); // Set current focus position
    setApertureValue( 0 );  // Set the aperture wide open.
  }

  // USB data receive
  if ( Usb.getUsbTaskState() == USB_STATE_RUNNING ) {
    uint8_t rcode;
    uint8_t buff[64];

    memset( buff, 0, 64 );
    uint16_t rcvd = 64;
    rcode = Ftdi.RcvData( &rcvd, buff );

    if ( rcode && rcode != hrNAK ) {
      ErrorMessage<uint8_t>( PSTR("Ret"), rcode );
    }
    // The device reserves the first two bytes of data
    //   to contain the current values of the modem and line status registers.
    if ( rcvd > 2 ) {
      for ( int i = 2; i < rcvd; i++ ) {
        char inChar = buff[i];
        if ( inChar == '#' ) {
          recvLineUSBIndex = 0;
          queueUSB.push( String( recvLineUSB ) );
          memset( recvLineUSB, 0, RECVLINES );
        } else {
          recvLineUSB[recvLineUSBIndex++] = inChar;
          if ( recvLineUSBIndex >= RECVLINES ) {
            recvLineUSBIndex = RECVLINES - 1;
          }
        }
      }
    }
  }
}

/*************************************************************************
 * NAME  perserBT - 
 *
 * SYNOPSIS
 *
 *    void perserBT( void )
 *
 * DESCRIPTION
 *  
 *************************************************************************/
void perserBT( void )
{
  String paramStringList[8];
  int nParam;
  if ( queueBT.count() > 0 ) {  // Check for serial command
    String replystr;
    replystr = queueBT.pop();   // Take out receive data
    Serial.println( replystr );
    int cmd = replystr.charAt( 0 );
    String param = replystr.substring( 1 );
    switch ( cmd ) {
    case 'Q':
      wakeupBackLight();
      labelStatus->caption( TFT_YELLOW, "Connected from remote controller %s", param.c_str() );
      connectBT = 1;
      SerialBT.printf( "P%d %d %d %d#", systemParam.phase, systemParam.lensIndex, systemParam.apertureIndex, systemParam.focusPosition );
      SerialBT.printf( "V%d#", M5.Power.getBatteryLevel() );
      break;
    case 'B':
      virtualKeyMap[0] = param.charAt( 0 );
      virtualKeyMap[1] = param.charAt( 1 );
      break;
    case 'V':
      indicateBatteryLevel( param.toInt() );
      break;
    case 'f':
      focusPosition( param.toInt() );
      SerialBT.printf( "F%d %d#", systemParam.phase, systemParam.focusPosition );
      break;
    case 'L':
      nParam = argumentSeparatorString( param, paramStringList, ' ', 8 );
      systemParam.phase = paramStringList[0].toInt(); 
      systemParam.lensIndex = paramStringList[1].toInt(); 
      lensSelect();
      break;
    case 'A':
      labelApertureTitle->caption( TFT_GREEN, "Aperture" );
      labelFocusTitle->caption( TFT_WHITE, "Focus" );
      nParam = argumentSeparatorString( param, paramStringList, ' ', 8 );
      systemParam.phase = paramStringList[0].toInt(); 
      systemParam.apertureIndex = paramStringList[1].toInt(); 
      apertureSelect();
      focusPosition();
      break;
    case 'F':
      labelApertureTitle->caption( TFT_WHITE, "Aperture" );
      labelFocusTitle->caption( TFT_GREEN, "Focus" );
      nParam = argumentSeparatorString( param, paramStringList, ' ', 8 );
      systemParam.phase = paramStringList[0].toInt(); 
      systemParam.focusPosition = paramStringList[1].toInt(); 
      apertureSelect();
      focusPosition();
      break;
    case 'P':
      nParam = argumentSeparatorString( param, paramStringList, ' ', 8 );
      systemParam.phase = paramStringList[0].toInt(); 
      systemParam.lensIndex = paramStringList[1].toInt(); 
      systemParam.apertureIndex = paramStringList[2].toInt(); 
      systemParam.focusPosition = paramStringList[3].toInt(); 
      latestEncoderPosition = systemParam.focusPosition;
      switch ( systemParam.phase ) {
      case PHASE_LENS:    // Lens selection in progress.
        lensSelect();
        break;
      case PHASE_APERTURE:  // Aperture selection in progress.
        selectLensDisplay();
        labelApertureTitle->caption( TFT_GREEN, "Aperture" );
        labelFocusTitle->caption( TFT_WHITE, "Focus" );
        lensSelect();
        apertureSelect();
        focusPosition();
        if ( useEncoder ) {
          encoder.setEncoderPosition( latestEncoderPosition );
        }
        break;
      case PHASE_FOCUS:   // Adjusting the focus position of the lens.
        selectLensDisplay();
        labelApertureTitle->caption( TFT_WHITE, "Aperture" );
        labelFocusTitle->caption( TFT_GREEN, "Focus" );
        lensSelect();
        apertureSelect();
        focusPosition();
        if ( useEncoder ) {
          encoder.setEncoderPosition( latestEncoderPosition );
        }
        break;
      }
//      SerialBT.printf( "P%d %d %d %d#", phase, systemParam.lensIndex, systemParam.apertureIndex, systemParam.focusPosition );
      break;
    }
    compareParam = systemParam;
  }

  while ( SerialBT.available() ) {
    char inChar = SerialBT.read();
    if ( inChar == '#' ) {
      recvLineBTIndex = 0;
      queueBT.push( String( recvLineBT ) );
      memset( recvLineBT, 0, RECVLINES );
    } else {
      recvLineBT[recvLineBTIndex++] = inChar;
      if ( recvLineBTIndex >= RECVLINES ) {
        recvLineBTIndex = RECVLINES - 1;
      }
    }
  }
}
