/*
File:     Commands.ino
Origin:   01-Oct-2022
Author:   Tip Partridge

Description:
  Part of DeskClock.ino.  This file contains various user command routines called from CommandParse

Revisions:
  09-Oct-2022 (TEP) Split from WallClock.ino.
*/
//********************************************************
// shoGlob - Display global runtime variables
//********************************************************
void shoGlob()
  {
  newln();
  prtln(F("Global Variables:"));
  prt(F("g_ADC =      "));
  prtln(g_ADC);
  prt(F("b_brite =    "));
  prtln(b_brite);
  }

//********************************************************
// setG2D - Set runtime Parameters to default values
//********************************************************
void setG2D( Parameters * gg)
  {
  gg->sBrite = d_sBrite;
  gg->bBrite = d_bBrite;
  gg->hourStyle = d_hourStyle;
  gg->fontN = d_fontN;
  strcpy(gg->Name, d_Name);
  strcpy(gg->Location, d_Location);
  }

//********************************************************
// getE2P - Copy parameters from EEPROM to global variables
//********************************************************
void getE2P()
  {
  EEPROM.get( parametersAddr, p);
  }

//**********************************************************
// putP2E - Write runtime Parameters to EEPROM
//**********************************************************
void putP2E()
  {
  EEPROM.put( EEInitAddr, EEInitFlag);
  EEPROM.put( parametersAddr, p);
  }

//**********************************************************
// putD2E - Set EEPROM parameters to default values
//**********************************************************
void putD2E()
  {
  Parameters gg;
  setG2D( & gg);
  EEPROM.put( EEInitAddr, EEInitFlag);
  EEPROM.put( parametersAddr, gg);
  }

//**********************************************************
// showGPs - Display runtime Parameters
//**********************************************************
void showGPs()
  {
  newln();
  prtln(F("Runtime Parameters:"));
  printParams( &p, "p");
  }

//**********************************************************
// showDOs - Display default Parameters
//**********************************************************
void showDOs()
  {
  Parameters gg;
  setG2D( & gg);
  newln();
  prtln(F("Default Parameters:"));
  printParams( &gg, "d");
  }

//**********************************************************
// showEPs - Display parameters in EEPROM
//**********************************************************
void showEPs()
  {
  Parameters ee;
  Ini ini;
  newln();
  prtln(F("EEPROM Parameters:"));
  EEPROM.get( EEInitAddr, ini);
  if (ini.iniflg == EEInitFlag) prt(F("EEPROM is initialized:  "));
  else prt(F("EEPROM not initialized!  "));
  for (int cc = 0; cc<4; cc++) prtc( ini.inistr[cc]);
  prt( "  "); prt( ini.iniflg);
  newln();
  EEPROM.get(parametersAddr, ee);
  printParams( &ee, "ee");
 }

//**********************************************************
// printParams - Display parameters
//**********************************************************

void printParams(Parameters *gg, const char pp[])
  {
  prt(pp); prt(F(".sBrite   ^S = ")); prtln( gg->sBrite);
  prt(pp); prt(F(".bBrite   ^B = ")); prtln( gg->bBrite);
  prt(pp); prt(F(".hourStyle A = ")); prtln( gg->hourStyle);
  prt(pp); prt(F(".fontN     F = ")); prtln( gg->fontN);
  prt(pp); prt(F(".Name      N = ")); prtln(gg->Name);
  prt(pp); prt(F(".Location  L = ")); prtln(gg->Location);
  }

//**********************************************************
// showTime - Display current date and time
//**********************************************************

void showTime()
  {
  prt(y);prt(F("/"));prt(n);prt(F("/"));prt(d);prt(F(" "));
  prt(h);prt(F(":"));prt(m);prt(F(":"));prtln(s);
  }
