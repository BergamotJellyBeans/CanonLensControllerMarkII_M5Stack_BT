
// iniFiles

/*
  iniFiles.h
    The INI file format is an informal standard for configuration files for some computing platforms or software.
    INI files are simple text files with a basic structure composed of sections, properties, and values.

	Copyright (C) 2022 by bergamot-jellybeans.

  Date-written. Feb 16,2019.
  Last-modify.  Apr 09,2021.
  mailto:		bergamot.jellybeans@icloud.com

  -Overview of the functions
  This class mimics the Windows INI file.
*/

#ifndef INIFILES_H
#define	INIFILES_H

#include <M5Stack.h>

#ifdef	__cplusplus
extern "C" {
#endif

class IniFiles
{
private:
  char *filepath;
  File file;
  String *lines;
  int count;
  int maxlines;
  bool modified;
  bool addLines( String newData );
  String readLine( void );
  bool writeLine( String data );

public:
  IniFiles();
  IniFiles( int maxLines );
  ~IniFiles();

  bool open( fs::FS &fs, char *path );
  void close( fs::FS &fs );
  int readInteger( String key, int defaultval );
  double readFloat( String key, double defaultval );
  String readString( String key, String defaultval );
  bool writeInteger( String key, int writeval );
  bool writeFloat( String key, double writeval );
  bool writeString( String key, String writeval );
  int readInteger( String section, String key, int defaultval );
  double readFloat( String section, String key, double defaultval );
  String readString( String section, String key, String defaultval );
  int readDelimitedString( String key, char delimiter, int listSize, String *stringList );
  void writeInteger( String section, String key, int writeval );
  void writeFloat( String section, String key, double writeval );
  void writeString( String section, String key, String writeval );
};

#ifdef	__cplusplus
}
#endif

#endif	/* INIFILES_H */
