/*
File:     SerialParse.ino
Origin:   01-Oct-2022
Author:   Tip Partridge

Description:
  Part of DeskClock.ino.  This file parses commands from USB-Serial

Revisions:
  09-Oct-2022 (TEP) Split from WallClock.ino.
*/

void CommandParse()
{
// Don't declare variables inside a switch or bad things happen!!!
int iTemp;                    // integer from getStr
float fTemp;                  // float from getStr
char cstr[17];                // String from getStr
int ier;
int i;
int yy, nn, dd, hh, mm, ss;   // temp time variable for setting time.
char c;                       // Serial command character
gotIO = false;
if (isUSB)
  {
  c = Serial.read();
  }
else  // assume isI2C
  {
  c = I2C_rcv[0];
  GotI2C = false;
  }

if (isCtrl) c = toupper(c) - 64;  // handling '^' preface as control character
isCtrl = false;

switch (c)
  {
  case 'A':                     // toggle between am/pm and military time
  case 'a': if (p.hourStyle == ampm) p.hourStyle = military;
            else p.hourStyle = ampm;
            prtAmPmMil(p.hourStyle);
            break;
  case 'B':                     // display ambient brightness ADC
  case 'b': iTemp = analogRead(opto);
            prt(F("ADC Now(0..1023)= "));
            prt(iTemp);
            prt(F("    ADC(0.."));
            prt(int(p.adcClip));
            prt(F(")= "));
            prt1(g_ADC);
            prt(F("    Display= "));
            prtln(b_brite);
            break;
  case 'E':                     // display EEPROM parameters
  case 'e': showEPs();
            newln();
            break;
  case 'F':                     // toggle font
  case 'f': p.fontN += 1;
            if (p.fontN >= nFonts || p.fontN < 0) p.fontN = 0;
            loadFont();
            s0 = -1;                                  // force display update
            prtFont(p.fontN);
            break;
  case 'G':                     // Set brightness ADC Gain
  case 'g': prt(F("Brightness ADC gain")); Default(p.adcGain);
            ier = getFloat(& fTemp, 5000);
            newln();
            if (ier > 0)
              {
              p.adcGain = fTemp;
              }
            prt(F("Gain = "));
            prtln((p.adcGain));
            break;
  case 'H': h += 1;             // hour up
            if (h > 23) h = 0;
            if (h == timeToUpdateFromUTP) timeUpdatedToday = true;  // skip daily check
            rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
            s0 = -1;                                  // force display update
            prtln(h);
            break;
  case 'h': h -= 1;             // hour down
            if (h < 0) h = 23;
            if (h == timeToUpdateFromUTP) timeUpdatedToday = true;  // skip daily check
            rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
            s0 = -1;                                  // force display update
            prtln(h);
            break;
  case 'I':                     // Identity report
  case 'i': prt(F("Name: "));
            prt(p.Name);
            prt(F("    Location: "));
            prt(p.Location);
            prt(F("    Version: "));
            prtln(verstr);
            break;
  case 'L':                     // get clock Location
  case 'l': prt(F("Clock Location")); Default(p.Location);
            ier = getStr( cstr, 16, 0, 10000);
            if (ier > 0 && cstr[0] != 0) {strcpy(p.Location, cstr);}
            break;
  case 'M': m += 1;             // minute up
            if (m > 59) m = 0;
            rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
            s0 = -1;                                  // force display update
            prtln(m);
            break;
  case 'm': m -= 1;             // minute down
            if (m < 0) m = 59;
            rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
            s0 = -1;                                  // force display update
            prtln(m);
            break;
  case 'N':                     // get clock Name
  case 'n': prt(F("Clock Name")); Default(p.Name);
            ier = getStr( cstr, 16, 0, 10000);
            if (ier > 0 && cstr[0] != 0) {strcpy(p.Name, cstr);}
            break;
  case 'P':                     // display runtime parameters
  case 'p': showGPs();
            newln();
            break;
  case 'T':                      // display date, time
  case 't':
            showTime();
            break;
  case 'U':                      // display date, time from NTP
  case 'u':
            yy=y; nn=n; dd=d; hh=h; mm=m; ss=s;
            ier = getI2TimeStr();
            if (ier == 1) { showTime(); }
            else {prt(F("Error: ")); prtln(ier);}
            y=yy; n=nn; d=dd; h=hh; m=mm; s=ss;
            break;
  case 'V':                     // display global variables
  case 'v': shoGlob();
            newln();
            break;
  case '^': isCtrl = true;         // '^' treats next character as control character
            break;
  case '0':                     // zero seconds
  case ')': s = 0;
            if (h < 0) h = 23;
            rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
            prtln(F("sec set to 0"));
            showTime();
            break;
  case 2:   // ^B                  // Set brightness fit b parameter
            prt(F("Brightness offset (0..16)")); Default(p.adcOffset);
            ier = getFloat(& fTemp, 5000);
            newln();
            if (ier > 0)
              {
              p.adcOffset = fTemp;
              }
            prt(F("fit b = "));
            prtln((p.adcOffset));
            break;
  case 3:   // ^C                   // Load global parameters from EEPROM
            prt(F("Loading global parameters from EEPROM..."));
            getE2P();
            prt("done.");
            showGPs();
            break;
  case 4:   // ^D                   // Load global parameters with defaults
            prt(F("Loading default global parameters..."));
            setG2D( & p);
            prt("done.");
            showGPs();
            break;
  case 5:   // ^E                   // Load parameters to EEPROM
            prt(F("Writing parameters to EEPROM..."));
            putP2E();
            prt("done.");
            showEPs();
            break;
  case 6:   // ^F                   // Load parameters to EEPROM
            prt(F("Writing default parameters to EEPROM..."));
            putD2E();
            prt(F("done."));
            showEPs();
            break;
  case 7:   // ^G                    // display default parameters
            showDOs();
            newln();
            break;
  case 9:   // ^I':                  // init display
            mx.begin();
            prtln(F("Display init."));
            break;
  case 10:  // Ignore <LF> and <CR> = 10, 13 = ^J, ^M
  case 13:  break;
  case 18:  // ^R                // Toggle periodic brightness report
            gBriteFlg = !gBriteFlg;
            if (gBriteFlg)
              {
              prtln(F("On"));
              if (gotRTC)
                {
                now = rtc.now();
                Min0 = now.minute();
                if (now.second() >= 57) Min0 += 1;
                if (Min0 >= 60) Min0 = Min0 - 60;
                }
              else Min0 = -1;
              }
            else prtln(F("Off"));
            break;
  case 19:  // ^S                  // set brightness ADC Saturation
            prt(F("Brightness ADC Saturation (0..1023)")); Default(p.adcClip);
            ier = getInt(& iTemp, 5000);
            newln();
            if (ier > 0)
              {
              p.adcClip = iTemp;
              }
            prt(F("ADC Sat = "));
            prtln((p.adcClip));
            break;
  case 20:  // ^T                // set date, time
            yy=y; nn=n; dd=d; hh=h; mm=m; ss=s;
            newln();
            prtln(F("Set Time:"));
// Year
            prt(F("Year")); Default(yy);
            ier = getInt(& iTemp, 10000);
            if (ier > 0 && iTemp >= 2023 && iTemp <= 2099)
              {
              yy = iTemp;
              updateRTC = true;
              }
// Month
            prt(F("Month")); Default(nn);
            ier = getInt(& iTemp, 10000);
            if (ier > 0 && iTemp >= 1 && iTemp <= 12)
              {
              nn = iTemp;
              updateRTC = true;
              }
// Day
            prt(F("Day")); Default(dd);
            ier = getInt(& iTemp, 10000);
            if (ier > 0 && iTemp >= 1 && iTemp <= 31)
              {
              dd = iTemp;
              updateRTC = true;
              }
// Hour
            prt(F("Hour")); Default(hh);
            ier = getInt(& iTemp, 10000);
            if (ier > 0 && iTemp >= 0 && iTemp <= 23)
              {
              hh = iTemp;
              updateRTC = true;
              }
// Min
            prt(F("Minute")); Default(mm);
            ier = getInt(& iTemp, 10000);
            if (ier > 0 && iTemp >= 0 && iTemp <= 59)
              {
              mm = iTemp;
              updateRTC = true;
              }
// Sec
            prt(F("Second")); Default(ss);
            ier = getInt(& iTemp, 10000);
            if (ier > 0 && iTemp >= 0 && iTemp <= 59)
              {
              ss = iTemp;
              updateRTC = true;
              }
//
            if (updateRTC)
              {
              prt(F("Set this time?   "));
              prt(yy);prt(F("/"));prt(nn);prt(F("/"));prt(dd);prt(F(" "));
              prt(hh);prt(F(":"));prt(mm);prt(F(":"));prt(ss);
              prt(F("  [Y]/N "));
              ier = getYN(10000);
              if (ier<= 0) updateRTC = false;
              }
            if (updateRTC)
              {
              y=yy; n=nn; d=dd; h=hh; m=mm; s=ss;
              if (h == timeToUpdateFromUTP) timeUpdatedToday = true;  // skip daily check
              prt(F("Time updated.\r\n"));
              }
            else
               prt(F("Time not changed.\r\n"));
            break;
  case 21:  // ^U                 // Update time & date via NTP
            ier = getI2TimeStr();
            if (ier == 1) 
              {
              rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
              s0 = -1;                                  // force display update
              showTime();
              }
            else {prt(F("Error: ")); prtln(ier);}
            break;
  case 24:  // ^X               // wifi status, expect 3 bytes: bool isConnected, clockStarted, telnetStarted
            ier = getI2CmdString( 0xfe);
            if (ier != 1) {prtln(F("error"));}
            else
              {
              //if (I2_pnt == 3)  // got valid enough bytes
              prt(F("Wifi connected: ")); prtln(byte(I2C_rcv[0]));
              prt(F("Clock started:  ")); prtln(byte(I2C_rcv[1]));
              prt(F("Telnet started: ")); prtln(byte(I2C_rcv[2]));
              }
            break;
  case 26:  // ^Z               // wifi version, expect verstr, 20 char max plus null terminator
            ier = getI2CmdString( 0xfd);
            if (ier != 1) prtln(F("error"));
            else
              {
              if (I2_pnt > 1)  // got at least 1 character
                for (ier=0; ier<(I2_pnt-1); ier++) prtc(I2C_rcv[ier]);
              newln();
              }
            break;
  case '?':                     // display Menu  (76 char wide)
  case '/': newln();
            prt(F(verstr)); prt(F("     ")); prt(p.Name); prt(F("     ")); prtln(p.Location);
            prtln(F("===TIME============================================="));
            prtln(F("H/h  Hour up/dn  T  show date/time   U  display NTP"));
            prtln(F("M/m  Min up/dn  ^T  set date/Time   ^U  set via NTP"));
            prtln(F(" 0   zero secs   A  AM/PM/Military   F  toggle font"));
            prtln(F("===BRIGHTNESS======================================="));
            prtln(F(" G  Gain     ^B  Offset     ^S  Clip     B  show"));
            prtln(F("===PARAMETERS======================================="));
            prtln(F(" P  show Params   E  show EEPROM   ^G  show defaults"));
            prtln(F("^E  params to EEPROM        ^F  deFaults to EEPROM"));
            prtln(F("^D  params from Defaults    ^C  params from EEPROM"));
            prtln(F(" N  Name          L  Location       I  show Identity"));
            prtln(F("===DEBUG============================================"));
            prtln(F(" V  show Variables          ^V  dump Frame buffer"));
            prtln(F("^A  toggle briteness report ^Y  toggle loop timer"));
            prtln(F("^X  Wifi status             ^Z  Wifi version"));
/*
            prtln(F("===TIME====================================================================="));
            prtln(F("H/h  increment Hour     A   AM/PM/Military   N   Set Name   L   Set Location"));
            prtln(F("M/m  increment Minute   T   display date & Time   U   Display time from NTP"));
            prtln(F(" 0    zero seconds     ^T   set date & Time      ^U   Update time via NTP"));
            prtln(F("===DISPLAY=================================================================="));
            prtln(F("^B   set brightness base level            B   display ambient Brightness ADC"));
            prtln(F("^S   set brightness ADC Saturation        F   toggle Font"));
            prtln(F("===PARAMETERS==============================================================="));
            prtln(F(" P   display runtime Parameters           E   display EEPROM params"));
            prtln(F("^C   load runtime params from EEPROM     ^E   write runtime params to EEPROM"));
            prtln(F("^D   load runtime params with Defaults   ^F   write deFault params to EEPROM"));
            prtln(F("^G   display default parameters           I   Identity report"));
            prtln(F("===DEBUG===================================================================="));
            prtln(F(" V   display global Variables            ^I   Initialize display"));
            prtln(F("^R   Toggle periodic brightness report"));
*/
            break;
  default:  prt(int(c)); prtln(F(" ?"));
            break;
  } // end switch
Bail2:
//
// Change RTC time
if (updateRTC)
  {
  rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
  s0 = -1;                                  // force display update
  updateRTC = false;
  }
}  // CommandParse
