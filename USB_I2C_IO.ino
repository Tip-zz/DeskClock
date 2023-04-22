/*
File:     Wall_IO.ino
Origin:   04-Feb-2023
Author:   Tip Partridge

Description:
  Part of DeskClock.ino.  This file contains USB / I2C output routines.
  Characters sent to USB or I2C depending on state of global flags isUSB and isI2C,
Revisions:
  22-Apr-2023 (TEP) Add protocol description to header.
*/

// ********************************************************************

void newln()
  {
  if (isUSB) Serial.println();
  if (isI2C) outI2C( "\r\n");
  }

// ********************************************************************

void prtc( char chr)
  {
  if (isUSB) Serial.write( chr);
  if (isI2C) outI2C( chr);
  }

// ********************************************************************

void prt( char * str)
  {
  if (isUSB) Serial.print( str);
  if (isI2C) outI2C( str);
  }

void prtln( char * str)
  {
  if (isUSB) Serial.print( str);
  if (isI2C) outI2C( str);
  newln();
  }

// ********************************************************************

void prt( const char * str)
  {
  if (isUSB) Serial.print( str);
  if (isI2C) outI2C( str);
  }

void prtln( const char * str)
  {
  if (isUSB) Serial.print( str);
  if (isI2C) outI2C( str);
  newln();
  }

// ********************************************************************

void prt( const __FlashStringHelper* str)
  {
  char astr[40];
  PGM_P p = reinterpret_cast<PGM_P>(str);
  size_t n = 0;
  while (1) 
    {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    astr[n++] = c;
    }
  astr[n]=0;
  if (isUSB) Serial.print( astr);
  if (isI2C) outI2C( astr);
  }

void prtln( const __FlashStringHelper* str)
  {
  char astr[80];
  PGM_P p = reinterpret_cast<PGM_P>(str);
  size_t n = 0;
  while (1) 
    {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    astr[n++] = c;
    }
  astr[n]=0;
  if (isUSB) Serial.print( astr);
  if (isI2C) outI2C( astr);
  newln();
  }

// ********************************************************************

void prt( float flt)
  {
  char str[10];
  sprintf( str, "%1.3f", flt);
  prt( str);
  }

void prt1( float flt)
  {
  char str[10];
  sprintf( str, "%1.1f", flt);
  prt( str);
  }

void prtln( float flt)
  {
  char str[10];
  sprintf( str, "%1.3f", flt);
  prt( str);
  newln();
  }

// ********************************************************************

void prt( int num)
  {
  char str[10];
  sprintf( str, "%d", num);
  prt( str);
  }

void prtln( int num)
  {
  char str[10];
  sprintf( str, "%d", num);
  prt( str);
  newln();
  }

// ********************************************************************

void prt( unsigned long num)
  {
  char str[10];
  sprintf( str, "%lu", num);
  prt( str);
  }

// ********************************************************************

void prtln( unsigned long num)
  {
  char str[10];
  sprintf( str, "%lu", num);
  prt( str);
  newln();
  }

// ********************************************************************
