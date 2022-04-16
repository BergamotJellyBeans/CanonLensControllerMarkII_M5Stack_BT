
// iniFiles

/*
	iniFiles.cpp
    The INI file format is an informal standard for configuration files for some computing platforms or software.
    INI files are simple text files with a basic structure composed of sections, properties, and values.

	Copyright (C) 2022 by bergamot-jellybeans.

  Date-written. Feb 16,2019.
  Last-modify.  Apr 09,2021.
  mailto:		bergamot.jellybeans@icloud.com

  -Overview of the functions
  This class mimics the Windows INI file.
*/
#include <stdarg.h>
#include <stdio.h>
#include "IniFiles.h"
#define LINES_MAX 100

// IniFiles class constructor.
IniFiles::IniFiles()
{
}

// IniFiles class constructor with argument.
IniFiles::IniFiles( int maxLines )
{
  maxlines = maxLines;
}

// IniFiles class destructor.
IniFiles::~IniFiles()
{
  maxlines = LINES_MAX;
}

// Add a new line.
bool IniFiles::addLines( String newData )
{
  if ( count < maxlines ) {
    lines[count++] = newData;
    return true; 
  }
  return false;
}

// Read one line.
String IniFiles::readLine( void )
{
  char buff[256], *p;
  String stream;
  p = buff;
  *p = '\0';
  while( file.available() ) {
    uint8_t data = file.read();
    if ( data == 0x0d ) continue;
    if ( data == 0x0a ) {
      break;
    }
    *p++ = data;
  }
  *p = '\0';
  stream = String( buff );
  return stream;
}

// write one line.
bool IniFiles::writeLine( String data )
{
  return file.println( data );
}

// Open the INI file specified by the <path> argument.
bool IniFiles::open( fs::FS &fs, char *path )
{
  filepath = path;
  count = 0;
  lines = new String[maxlines];

  if ( fs.exists( filepath ) ) {
//    Serial.printf( "IniFiles %s is exists\n", filepath );
    file = fs.open( filepath, FILE_READ );
    for ( int i = 0; i < maxlines; i++ ) {
      String stream = readLine();
//      Serial.printf( "%d %s\n", i, stream.c_str() );
      if ( stream == "" ) break;
      lines[count++] = stream;
    }
    file.close();
  }
  if ( count == 0 ) {
    modified = true;
    return false;
  }
  modified = false;
  return true;
}

// Open the INI file.
void IniFiles::close( fs::FS &fs )
{
  if ( modified ) {
	// I'll write if it's modified.
//    Serial.println( "IniFiles::close write..");
    file = fs.open( filepath, FILE_WRITE );
    if ( file ) {
//      Serial.printf( "IniFiles %s is open\n", filepath );
      for ( int n = 0; n < count; n++ ) {
        String wdata = lines[n];
        file.println( wdata );
      }
      file.close();
    } else {
//      Serial.printf( "IniFiles %s is not create\n", filepath );
    }
  }
  delete [] lines;
}

bool IniFiles::isExists( String key )
{
  String keys = key + "=";
  for ( int nk = 0; nk < count; nk++ ) {
    String dline = lines[nk];
    int pos = dline.indexOf( keys );
    if ( pos >= 0 ) return true;
  }
  return false;
}

// Reads the numerical integer data of the argument <key>.
// The <defaultval> argument is the default value to use if the file cannot be read.
int IniFiles::readInteger( String key, int defaultval )
{
  String keys = key + "=";
  for ( int nk = 0; nk < count; nk++ ) {
    String dline = lines[nk];
    int pos = dline.indexOf( keys );
//Serial.printf( "dline %s\n", dline.c_str() );
    if ( pos >= 0 ) {
      pos += keys.length();
      String sval = dline.substring( pos );
      int ival = sval.toInt();
      return ival;
    }
  }
  return defaultval;
}

// Reads the character string of the argument <key>.
// The <defaultval> argument is the default value to use if the file cannot be read.
String IniFiles::readString( String key, String defaultval )
{
  String keys = key + "=";
  for ( int nk = 0; nk < count; nk++ ) {
    String dline = lines[nk];
    int pos = dline.indexOf( keys );
    if ( pos >= 0 ) {
      pos += keys.length();
      String sval = dline.substring( pos );
      return sval;
    }
  }
  return defaultval;
}

// Reads the delimited character string of the argument <key>.
// The <delimiter> argument is delimiter, The <stringList> argument is string array to store delimited strings.
// and <listSize> argument is string array size of storage.
int IniFiles::readDelimitedString( String key, char delimiter, int listSize, String *stringList )
{
  String inString = readString( key, "" );
  int n = inString.length();
  if ( n == 0 ) return 0;
  
  int index = 0;
  for ( int i = 0; i < listSize; i++ ) {
    stringList[i] = "";
  }
  for ( int i = 0; i < n; i++ ) {
    char c = inString.charAt( i );
    if ( c == delimiter ) {
      index++;
      if ( index >= listSize ) return listSize;
    } else {
      stringList[index] += c;
    }
  }
  return index + 1;	 // Returns the number of delimited strings stored.
}

// Reads the numerical floating point data(as double) of the argument <key>.
// The <defaultval> argument is the default value to use if the file cannot be read.
double IniFiles::readFloat( String key, double defaultval )
{
  String keys = key + "=";
  for ( int nk = 0; nk < count; nk++ ) {
    String dline = lines[nk];
    int pos = dline.indexOf( keys );
    if ( pos >= 0 ) {
      pos += keys.length();
      String sval = dline.substring( pos );
      double dval = sval.toDouble();
      return dval;
    }
  }
  return defaultval;
}

// Writes the numerical integer data of the argument <key>.
// The <writeval> argument is the write data.
bool IniFiles::writeInteger( String key, int writeval )
{
  String keys = key + "=";
  String sval = keys + String( writeval );
  modified = true;
  for ( int nk = 0; nk < count; nk++ ) {
    String dline = lines[nk];
    int pos = dline.indexOf( keys );
    if ( pos >= 0 ) {
      lines[nk] = sval;
      return true;
    }
  }
  return addLines( sval ); 
}

// Writes the character string of the argument <key>.
// The <writeval> argument is the write data.
bool IniFiles::writeString( String key, String writeval )
{
  String keys = key + "=";
  String sval = keys + writeval;
  modified = true;
  for ( int nk = 0; nk < count; nk++ ) {
    String dline = lines[nk];
    int pos = dline.indexOf( keys );
    if ( pos >= 0 ) {
      lines[nk] = sval;
      return true;
    }
  }
  return addLines( sval ); 
}

// Writes the numerical floating point(as double) data of the argument <key>.
// The <writeval> argument is the write data.
bool IniFiles::writeFloat( String key, double writeval )
{
  String keys = key + "=";
  String sval = keys + String( writeval );
  modified = true;
  for ( int nk = 0; nk < count; nk++ ) {
    String dline = lines[nk];
    int pos = dline.indexOf( keys );
    if ( pos >= 0 ) {
      lines[nk] = sval;
      return true;
    }
  }
  return addLines( sval ); 
}

String cmds[3] = {"\0"}; // Array to store split strings. 












// he following [section] support is not yet complete.
int IniFiles::readInteger( String section, String key, int defaultval )
{
  String sections = "[" + section + "]";
  for ( int ns = 0; ns < count; ns++ ) {
    if ( lines[ns] == sections ) {
      String keys = key + "=";
      for ( int nk = ns + 1; nk < count; nk++ ) {
        String dline = lines[nk];
        int pos = dline.indexOf( keys );
        if ( pos >= 0 ) {
          String sval = dline.substring( pos );
          int ival = sval.toInt();
          return ival;
        }
      }
    }
  }
  return defaultval;
}

String IniFiles::readString( String section, String key, String defaultval )
{
  String sections = "[" + section + "]";
  for ( int ns = 0; ns < count; ns++ ) {
    if ( lines[ns] == sections ) {
      String keys = key + "=";
      for ( int nk = ns + 1; nk < count; nk++ ) {
        String dline = lines[nk];
        int pos = dline.indexOf( keys );
        if ( pos >= 0 ) {
          String sval = dline.substring( pos );
          return sval;
        }
      }
    }
  }
  return defaultval;
}

double IniFiles::readFloat( String section, String key, double defaultval )
{
  String sections = "[" + section + "]";
  for ( int ns = 0; ns < count; ns++ ) {
    if ( lines[ns] == sections ) {
      String keys = key + "=";
      for ( int nk = ns + 1; nk < count; nk++ ) {
        String dline = lines[nk];
        int pos = dline.indexOf( keys );
        if ( pos >= 0 ) {
          String sval = dline.substring( pos );
          double dval = sval.toDouble();
          return dval;
        }
      }
    }
  }
  return defaultval;
}

void IniFiles::writeInteger( String section, String key, int writeval )
{
  String sections = "[" + section + "]";
  for ( int ns = 0; ns < count; ns++ ) {
    if ( lines[ns] == sections ) {
      String keys = key + "=";
      for ( int nk = ns + 1; nk < count; nk++ ) {
        String dline = lines[nk];
        int pos = dline.indexOf( keys );
        if ( pos >= 0 ) {
          String sval = keys + String( writeval );
          lines[nk] = sval;
          modified = true;
          return;
        }
      }
    }
  }
}

void IniFiles::writeString( String section, String key, String writeval )
{
  String sections = "[" + section + "]";
  for ( int ns = 0; ns < count; ns++ ) {
    if ( lines[ns] == sections ) {
      String keys = key + "=";
      for ( int nk = ns + 1; nk < count; nk++ ) {
        String dline = lines[nk];
        int pos = dline.indexOf( keys );
        if ( pos >= 0 ) {
          String sval = keys + writeval;
          lines[nk] = sval;
          modified = true;
          return;
        }
      }
    }
  }
}

void IniFiles::writeFloat( String section, String key, double writeval )
{
  String sections = "[" + section + "]";
  for ( int ns = 0; ns < count; ns++ ) {
    if ( lines[ns] == sections ) {
      String keys = key + "=";
      for ( int nk = ns + 1; nk < count; nk++ ) {
        String dline = lines[nk];
        int pos = dline.indexOf( keys );
        if ( pos >= 0 ) {
          String sval = keys + String( writeval );
          lines[nk] = sval;
          modified = true;
          return;
        }
      }
    }
  }
}
