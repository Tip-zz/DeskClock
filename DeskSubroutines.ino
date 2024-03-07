/*
File:     DeskSubroutines.ino
Origin:   01-Oct-2022
Author:   Tip Partridge

Description:
  Part of DeskClock.ino. Miscellaneous subroutines.

Revisions:
  14-Dec-2022 (TEP) Split from WallClock.ino.
*/
//**********************************************************
// shodig - Write character to display
//**********************************************************
void shodig( int dig, int posn, int columns)
// Write character to display buffer
// dig is index into font. 0 = '0'
// posn is display column to start. First (left) column is 0.
// columns is number of colums to write.
// if posn < 0 then shift left by -posn.  This for thin colon with hour = 1
// if posn < 0 and columns < 0 then shift right by -posn.  This for sans with minute = 1*.
  {
  int cc, pp;    // cc = col, pp = pointer into frame buffer
  if (posn < 0)
    if (columns < 0)
      {
      pp = didx[p.fontN][didxidx][-posn] + 1;  // shift column right
      columns = -columns;
      }
    else
      pp = didx[p.fontN][didxidx][-posn] - 1;  // shift column left
  else
    pp = didx[p.fontN][didxidx][posn];
  for (cc=0; cc<columns; cc++)  // write columns to display buffer
    {
    buf.b1x32[pp+cc] = font[dig][cc];
    }
  }

//**********************************************************
// Print strings
//**********************************************************

void prtAmPmMil(hour_style hourStyle)
  {
  if (hourStyle==ampm) prtln(F("AM/PM"));
  else prtln(F("Military"));
  }

void prtFont(int font)
  {
  prt(font);
  prt(F(" = "));
  switch(font)
    {
    case 0:  prtln(F(font0));
             break;
    case 1:  prtln(F(font1));
             break;
    case 2:  prtln(F(font2));
             break;
    case 3:  prtln(F(font3));
             break;
    case 4:  prtln(F(font4));
             break;
    case 5:  prtln(F(font5));
             break;
    default: prtln(F("unknown"));
             break;
    }
  }
 
//**********************************************************
// Print formatted default value
//**********************************************************
void Default( int dflt)
  {
  prt(F(" ["));
  prt(dflt);
  prt(F("]: "));
  }
void Default( float dflt)
  {
  prt(F(" ["));
  prt(dflt);
  prt(F("]: "));
  }
void Default( uint8_t dflt)
  {
  prt(F(" ["));
  prt(dflt);
  prt(F("]: "));
  }
void Default( char * dflt)
  {
  prt(F(" ["));
  prt(dflt);
  prt(F("]: "));
  }

//**********************************************************
// Load selected font from Flash to RAM
//**********************************************************
void loadFont()
  {
  int rr, cc, ii;
  ii = p.fontN * 75;
  for (rr = 0; rr < 15; rr++)
    for (cc = 0; cc < 5; cc++)
      font[rr][cc] = pgm_read_byte_near(fontP + ii++);
  }
