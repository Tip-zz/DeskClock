/*
File:     DeskClock.ino
Origin:   11-Dec-2022
Author:   Tip Partridge

Description:
  Desk clock based on 8x32 MAX7219 based LED panel and a DS3231 RTC from Adafruit.
  Nano host. Clock time control via pushbuttons Hour up, Hour down, Minute up, Minute down, Up/Down X 5, and Second to 0.
  Clock advanced control functions via USB serial (115200 Baud) menu.
  Optionally Communicates via I2C to NodeMCU ESP8266 wi Wifi for timeserver connect and telnet remote control.
  ESP8266 runs ClockWifi.ino sketch.

Revisions:
  Dec-2022 (TEP) create from WallClock v0.55.
  26-Mar-2023 (TEP) v0.26 Works pretty well.
  02-Apr-2023 (TEP) v0.27 Add support to get time over over I2C from WiFi.
  15-Apr-2023 (TEP) v0.28 Add Name and Location.
  22-Apr-2023 (TEP) v0.29 Code cleanup.
  22-Jun-2023 (TEP) v0.31 Add Command U show time from NTP server.
  05-Sep-2023 (TEP) v0.32 Add Button X + Button S updates time from NTP server.
  26-Sep-2023 (TEP) v0.32 Fix ^U display (save values to temp).
  30-Sep-2023 (TEP) v0.33 Remove ^X set to compile time. Add ^X show wifi status, ^Z show wifi version.
  08-Oct-2023 (TEP0 v0.34 Update menu to 53 wide from 76 wide to fit iPhone telnet screen
  18-Nov-2023 (TEP) v0.35 Add daily time update at 3:00 am from NTP.
  28-Dec-2023 (TEP) v0.36 Fix menu text for T, ^T, and A comands.
  02-Mar-2024 (TEP) v0.37 New font replaces Thick plu2 4 more (nFonts = 6).  Add '^' as control character prefix.

18-N0v-2023 DeskClock v0.35
Sketch uses 27412 bytes (89%) of program storage space. Maximum is 30720 bytes.
Global variables use 927 bytes (45%) of dynamic memory, leaving 1121 bytes for local variables. Maximum is 2048 bytes.
*/

#define verstr "DeskClock v0.37"

#include <EEPROM.h>
#include <LibPrintf.h>  // need this for sprintf %f
#include <Wire.h>       // I2C for RTC

/*
// Include these to activate Debn() and Debnln()
//#define DEB0UG  // SETUP
//#define DEB1UG  // USB
//#define DEB2UG  // I2C
//#define DEB3UG  // Parse
//#define DEB4UG  // Button
//#define DEB5UG  // Loop
//#define DEB6UG  // I2C Output
//#define DEB7UG  // update display
//#define DEB8UG  // getI2TimeStr
//#define DEB9UG  //
#include <Debug_Print.h>
*/

// RTC
#include "RTClib.h"
RTC_DS3231 rtc;
DateTime now;
bool gotRTC = false;    // true if RTC found
// timer0 and timerNow for use with no RTC
unsigned long timer0;   // startup time
unsigned long timerNow;

// ******* Display Panel
// Display panel
#include <MD_MAX72xx.h>
#include <SPI.h>
///                                  Type           hwDigRows  hwRevCols  hwRevRows
///                                  ====           =========  =========  =========
///#define HARDWARE_TYPE MD_MAX72XX::DR0CR0RR0_HW // false;     false;     false;
///#define HARDWARE_TYPE MD_MAX72XX::DR0CR0RR1_HW // false;     false;     true;
///#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW   // false;     true;      false;
///#define HARDWARE_TYPE MD_MAX72XX::DR0CR1RR1_HW // false;     true;      true;
///#define HARDWARE_TYPE MD_MAX72XX::FC16_HW      // true;      false;     false;
///#define HARDWARE_TYPE MD_MAX72XX::DR1CR0RR1_HW // true;      false;     true;
///#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW    // true;      true;      false;
#define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW // true;      true;      true;
///
#define MAX_DEVICES 4   // 4 8x8 modules
#define CLK_PIN   13  // SCK
#define DATA_PIN  11  // MOSI
#define CS_PIN    10  // SS
#define MaxBrite  15  // display btightnrss goes from 0..15
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

union buf   // frame buffer
  {
  uint8_t b1x32[32];
  uint8_t b4x8[4][8];
  };

buf buf;    // frame buffer, addressable as 1x32 or 4x8
// *******

// Define pins
const int hUp = 2;
const int hDn = 5;
const int mUp = 4;
const int mDn = 3;
const int xXX = 6;
const int sec0 = 8;
#define opto A7
const int reqPin = 7;   // hardware flag to tell I2C master to request data
const int bsyPin = A0;  // kluge to make LED blink on ESP for reqPin
#define LEDpin  9

// Flags to indicate buttons released
bool hUpBut = false;
bool hDnBut = false;
bool mUpBut = false;
bool mDnBut = false;
bool xXXBut = false;
bool sec0But = false;

// display font: 0..9, :, A, P, M, and ' '
#define Colon 10
#define Aye 11
#define Pee 12
#define Emm 13
#define Space 14

// See Font2Hex.xlsx.
#define font0 "Serif"
#define font1 "Sans"
#define font2 "7 Seg"
#define font3 "Base Serif"
#define font4 "Base Sans"
#define font5 "Thick"
#define nFonts 6
//const uint8_t fontP[nFonts][15][5] PROGMEM = {
const uint8_t fontP[] PROGMEM = {
// font 0
0x7E,   0x81,   0x81,   0x81,   0x7E,   // 0
0x00,   0x82,   0xFF,   0x80,   0x00,   // 1
0xC2,   0xA1,   0x91,   0x89,   0x86,   // 2
0x42,   0x81,   0x89,   0x89,   0x76,   // 3
0x0F,   0x08,   0x08,   0x08,   0xFF,   // 4
0x4F,   0x89,   0x89,   0x89,   0x71,   // 5
0x7E,   0x91,   0x89,   0x89,   0x72,   // 6
0xE1,   0x11,   0x09,   0x05,   0x03,   // 7
0x76,   0x89,   0x89,   0x89,   0x76,   // 8
0x46,   0x89,   0x89,   0x89,   0x7E,   // 9
0x24,   0x00,   0x00,   0x00,   0x00,   // :
0xF0,   0x28,   0x24,   0x28,   0xF0,   // A
0xFC,   0x24,   0x24,   0x24,   0x18,   // P
0xFC,   0x08,   0x10,   0x08,   0xFC,   // M
0x00,   0x00,   0x00,   0x00,   0x00,   // _
// font 1
0x7E,   0x81,   0x81,   0x81,   0x7E,   // 0
0x00,   0x00,   0xFF,   0x00,   0x00,   // 1
0xC1,   0xA1,   0x91,   0x89,   0x86,   // 2
0x81,   0x89,   0x89,   0x89,   0x76,   // 3
0x0F,   0x08,   0x08,   0x08,   0xFF,   // 4
0x8F,   0x89,   0x89,   0x89,   0x71,   // 5
0x7F,   0x88,   0x88,   0x88,   0x70,   // 6
0xE1,   0x11,   0x09,   0x05,   0x03,   // 7
0x76,   0x89,   0x89,   0x89,   0x76,   // 8
0x06,   0x09,   0x09,   0x09,   0xFE,   // 9
0x24,   0x00,   0x00,   0x00,   0x00,   // :
0xF0,   0x28,   0x24,   0x28,   0xF0,   // A
0xFC,   0x24,   0x24,   0x24,   0x18,   // P
0xFC,   0x08,   0x10,   0x08,   0xFC,   // M
0x00,   0x00,   0x00,   0x00,   0x00,   // _
// font 2
0xFF,   0x81,   0x81,   0x81,   0xFF,   // 0
0x00,   0x00,   0xFF,   0x00,   0x00,   // 1
0xF9,   0x89,   0x89,   0x89,   0x8F,   // 2
0x89,   0x89,   0x89,   0x89,   0xFF,   // 3
0x0F,   0x08,   0x08,   0x08,   0xFF,   // 4
0x8F,   0x89,   0x89,   0x89,   0xF9,   // 5
0xFF,   0x89,   0x89,   0x89,   0xF9,   // 6
0x01,   0x01,   0x01,   0x01,   0xFF,   // 7
0xFF,   0x89,   0x89,   0x89,   0xFF,   // 8
0x0F,   0x09,   0x09,   0x09,   0xFF,   // 9
0x24,   0x00,   0x00,   0x00,   0x00,   // :
0xFC,   0x24,   0x24,   0x24,   0xFC,   // A
0xFC,   0x24,   0x24,   0x24,   0x3C,   // P
0xFC,   0x08,   0x10,   0x08,   0xFC,   // M
0x00,   0x00,   0x00,   0x00,   0x00,   // _
// font 3
0x7E,   0xC1,   0xC1,   0xC1,   0x7E,   // 0
0x00,   0xC2,   0xFF,   0xC0,   0x00,   // 1
0xC2,   0xE1,   0xD1,   0xC9,   0xC6,   // 2
0x42,   0xC1,   0xC9,   0xC9,   0x76,   // 3
0x0F,   0x08,   0x08,   0xC8,   0xFF,   // 4
0x4F,   0xC9,   0xC9,   0xC9,   0x71,   // 5
0x7E,   0xD1,   0xC9,   0xC9,   0x72,   // 6
0xC1,   0xF1,   0x09,   0x05,   0x03,   // 7
0x76,   0xC9,   0xC9,   0xC9,   0x76,   // 8
0x46,   0xC9,   0xC9,   0xC9,   0x7E,   // 9
0x24,   0x00,   0x00,   0x00,   0x00,   // :
0xF0,   0xE8,   0x24,   0xE8,   0xF0,   // A
0xFC,   0xE4,   0x24,   0x24,   0x18,   // P
0xFC,   0xC8,   0x10,   0xC8,   0xFC,   // M
0x00,   0x00,   0x00,   0x00,   0x00,   // _
// font 4
0x7E,   0xC1,   0xC1,   0xC1,   0x7E,   // 0
0x00,   0xC0,   0xFF,   0xC0,   0x00,   // 1
0xF1,   0xC9,   0xC9,   0xC9,   0xC6,   // 2
0xC1,   0xC9,   0xC9,   0xC9,   0x76,   // 3
0x0F,   0x08,   0x08,   0xC8,   0xFF,   // 4
0xCF,   0xC9,   0xC9,   0xC9,   0x71,   // 5
0x7E,   0xC9,   0xC9,   0xC9,   0x70,   // 6
0x01,   0x01,   0x01,   0xC1,   0xFF,   // 7
0x76,   0xC9,   0xC9,   0xC9,   0x76,   // 8
0x06,   0x09,   0x09,   0xC9,   0xFE,   // 9
0x24,   0x00,   0x00,   0x00,   0x00,   // :
0xF0,   0xE8,   0x24,   0xE8,   0xF0,   // A
0xFC,   0xE4,   0x24,   0x24,   0x18,   // P
0xFC,   0xC8,   0x10,   0xC8,   0xFC,   // M
0x00,   0x00,   0x00,   0x00,   0x00,   // _
// font 5
0x7E,   0xFF,   0xC3,   0xFF,   0x7E,   // 0
0x00,   0xC6,   0xFF,   0xFF,   0xC0,   // 1
0xE2,   0xF3,   0xDB,   0xCF,   0xC6,   // 2
0x42,   0xC3,   0xDB,   0xFF,   0x66,   // 3
0x1F,   0x1F,   0x18,   0xFF,   0xFF,   // 4
0x5F,   0xDF,   0xDB,   0xFB,   0x73,   // 5
0x7E,   0xFF,   0xDB,   0xFB,   0x72,   // 6
0xE3,   0xF3,   0x1B,   0x0F,   0x07,   // 7
0x76,   0xFF,   0xDB,   0xFF,   0x76,   // 8
0x4E,   0xDF,   0xDB,   0xFF,   0x7E,   // 9
0x66,   0x66,   0x00,   0x00,   0x00,   // :
0xF8,   0xFC,   0x36,   0xFC,   0xF8,   // A
0xFE,   0xFE,   0x36,   0x3E,   0x1C,   // P
0xFE,   0xFC,   0x18,   0xFC,   0xFE,   // M
0x00,   0x00,   0x00,   0x00,   0x00    // _
                      };
uint8_t font[15][5];
int didxidx;
int didx[nFonts][2][6] = {   // digit column, 6 char, 5 char
  {{0, 6, 12, 14, 20, 27}, {3, 3, 9, 11, 17, 24}}, // Font 0
  {{0, 6, 12, 14, 20, 27}, {3, 3, 9, 11, 17, 24}}, // Font 1
  {{0, 6, 12, 14, 20, 27}, {3, 3, 9, 11, 17, 24}}, // Font 2
  {{0, 6, 12, 14, 20, 27}, {3, 3, 9, 11, 17, 24}}, // Font 3
  {{0, 6, 12, 14, 20, 27}, {3, 3, 9, 11, 17, 24}}, // Font 4
  {{0, 6, 12, 15, 21, 27}, {3, 3, 9, 12, 18, 24}}  // Font 5 thick
  };

// Enums
enum hour_style {ampm, military};

// Default paramrters
const float d_adcClip = 1000.0;   // clip ambient ADC here
const float d_adcOffset = 0.0;    // ambient ADC offset
const float d_adcGain = 1.0;      // ambient ADC gain
const hour_style d_hourStyle = ampm;
const int d_fontN = 0;
#define d_Name "Clock\0\0\0\0\0\0\0\0\0\0\0"
#define d_Location "Desk\0\0\0\0\0\0\0\0\0\0\0\0"
// Global parameters
struct Parameters
  {
  float adcClip;
  float adcOffset;
  hour_style hourStyle;
  int fontN;
  char Name[17];
  char Location[17];
  float adcGain;
  };
Parameters p;

// ***** EEPROM **************************************************
const unsigned long EEInitFlag = 1802724676L;  // this 32 bit value flags that EEPROM is initialized = 0x6b 73 65 44  kseD 
union Ini     // EEPROM initialized flag, can be log or array of 4 characters, just for fun
  {
  unsigned long iniflg;
  char inistr[4];
  };

// EEPROM state
Ini g_ini;              // special value indicates EEPROM has been initialized

// EEPROM layout
const int EEInitAddr = 0;   // address in EEPROM of flag that says EEPROM has been initialized
const unsigned int parametersAddr = EEInitAddr + sizeof(g_ini);
// ***** end EEPROM **********************************************

// Miscellaneous global variables and such
int y, n, d, h, m, s;   // current date/time
#define timeToUpdateFromUTP 3 // time for daily update of RTC from UTP
bool timeUpdatedToday;
bool updateRTC;         // flag to update the RTC with y, n, d, h, m, s.
int s0;                 // last second to update display each second
float g_ADC;            // brightness read from optic sensor
int b_brite;            // desired brightness, 0..15
unsigned long bMilli;   // brightness update timer
const unsigned long bMilVal = 500;   // update each 1/2 second

// Periodic brightness report
int Min0 = 0;           // periodic brightness report
bool gBriteFlg;         // flag for periodic brightness report
#define dMinute 10      // report each 10 minutes

// IO
boolean isUSB = false;
boolean isI2C = false;
boolean gotIO = false;
boolean isCtrl = false;   // "^" pressed so treat next as control character

// I2C Stuff
uint8_t ClientID = 1;             // Client I2C address
char i2Str[33];                   // I2C transmit buffer
uint8_t i2Len;                    // number of bytes in I2C transmit buffer
#define i2RcvMax 21               // Size of I2C receive buffer
volatile char I2C_rcv[i2RcvMax];  // For i2C receive ISR, received character
volatile int I2_pnt;              // index used in I2C receive ISR to put characters into I2C_str
volatile bool GotI2C = false;     // For i2C receive ISR, data available flag
volatile bool I2CReq = false;     // For I2C request USR, data requested
volatile bool isLen;              // Flag to requestEvent ISR to send length

// ***** TIMER ***************************************************
// system clock = 16 MHz
// timer prescale = 64;
#define TicksPer100ms 25000   // clock ticks per 100 ms
volatile bool g_timeout = false;           // global flag set when 100ms timer timeout.

// ***************   SSSSS   EEEEEE   TTTTTTTT   UU   UU   PPPPP    *********
// ***************  SS       EE          TT      UU   UU   PP   PP  *********
// Setup *********    SS     EEEEE       TT      UU   UU   PPPPP    *********
// ***************      SS   EE          TT      UU   UU   PP       *********
// ***************  SSSSS    EEEEEE      TT        UUU     PP       *********

void setup()
{
// Say Hi!
  Serial.begin(115200);
  Serial.println(verstr);
  Serial.println(F("  ? for menu"));
// Set up pins
  pinMode( hUp, INPUT_PULLUP);
  pinMode( hDn, INPUT_PULLUP);
  pinMode( mUp, INPUT_PULLUP);
  pinMode( mDn, INPUT_PULLUP);
  pinMode( xXX, INPUT_PULLUP);
  pinMode( sec0, INPUT_PULLUP);
  pinMode( LEDpin, OUTPUT);

// ***** check EEPROM status *************************************
  EEPROM.get( EEInitAddr, g_ini.iniflg);
  if (g_ini.iniflg == EEInitFlag) // EEPROM initialized?
    {
    getE2P();       // load global parameters from EEPROM
    }
  else              // EEPROM blank, use defaults
    {
    Serial.print(F("Initializing EEPROM.  "));
    putD2E();       // Load default parameters to EEPROM
    getE2P();       // load global parameters from EEPROM
    }

// ***** Start RTC ***********************************************
  if (! rtc.begin())
    {
    Serial.println("No RTC!!");   // Oh no!!!
    gotRTC = false;
    delay(500);    // wait a moment 'til we maybe crash. Don't seem to...
    }
  else gotRTC = true;
  updateRTC = false;
  timer0 = millis();
  
// start display ************************************************
  mx.begin();
  mx.clear();

// Load font ************************************************
  loadFont();
// ***** Initialize I2C ************************************************
  Wire.begin(ClientID);
///  Wire.setClock(I2Cclock);
  Wire.onReceive( I2Creceive);  /* register receive event */
  Wire.onRequest(requestEvent); /* register request event */
  digitalWrite( reqPin, LOW);
  pinMode( reqPin, OUTPUT);
  digitalWrite( bsyPin, HIGH);
  pinMode( bsyPin, OUTPUT);
// ***** Set up timer/counter1 (16 bit) ******************************************
// we are using timer/counter1 Output Compare A for something
  TCCR1A = 0;                       // Disconnect Compare1 Output pins
  TCCR1B = 0;                       // Stop clock
  TCCR1C = 0;                       // Nothing forced
  TCNT1 = 0;                        // clear timer counter register
  OCR1A = TicksPer100ms;            // output compare register target
  TIFR1 = 255;                      // Clear flag registers
  TIMSK1 = 0;                       // interrupts off for now...
  TCCR1B |= _BV(CS11) | _BV(CS10);  // set prescale 64
  TIFR1 = 255;                      // clear OCRA flag registers so we don't get immediate interrupt
  TIMSK1 &= !_BV(OCIE1A);           // timer 1 output compare interrupt disabled
  sei();
// ***** Get current time ******************************************
// Get current time
    now = rtc.now();
    y = now.year();
    n = now.month();
    d = now.day();
    h = now.hour();
    m = now.minute();
    s = now.second();
} // end setup()

//*******************  LL       OOOOO     OOOOO    PPPPPP   **************************
//*******************  LL      00   00   00   00   PP   PP  **************************
// Loop *************  LL      00   O0   00   00   PPPPPP   **************************
//*******************  LL      00   OO   00   00   PP       **************************
//*******************  LLLLLL   00O00     OOOOO    PP       **************************

void loop()
{
///goto Bail;
// Periodic update, bMilVal = 500.
  if ( millis()-bMilli >= bMilVal)
    {
// Update brightness
    bMilli = millis();
    g_ADC = float(analogRead(opto));
    for (int ii = 0; ii < 9; ii++) g_ADC = g_ADC + float(analogRead(opto));
    g_ADC = g_ADC / 10.0;         // average 10 ADC readings
    if (gBriteFlg && m == Min0)   // periodic brightness report
      {
      Min0 = Min0 + dMinute;
      if (Min0 >= 60) Min0 = 0;
      prt(h);prt(F(":"));prt(m);prt(F(":"));prt(s);prt(F(" = "));prtln(g_ADC);
      }
    g_ADC *= p.adcGain;     // apply gain
    if (g_ADC > p.adcClip-p.adcOffset) g_ADC = p.adcClip-p.adcOffset;   // ADC saturates at p.adcClip
    b_brite = (g_ADC + p.adcOffset) * 16.0 / p.adcClip;
    if (b_brite < 0) b_brite = 0;
    else if (b_brite > 15) b_brite = 15;
    mx.control(MD_MAX72XX::INTENSITY, b_brite);   // Brightness 0..15

// Get current time
    if (gotRTC)
      {
      now = rtc.now();
      y = now.year();
      n = now.month();
      d = now.day();
      h = now.hour();
      m = now.minute();
      s = now.second();
      }
    else
      {
      timerNow = millis();
      timerNow = timerNow - timer0;
      d = timerNow / 86400000;
      timerNow = timerNow - d * 86400000;
      h = timerNow / 3600000;
      timerNow = timerNow - h * 3600000;
      m = timerNow / 60000;
      timerNow = timerNow - m * 60000;
      s = timerNow / 1000;
      }
    }

// Time to update time from NTP?
  if (h==timeToUpdateFromUTP)
    {
    if (!timeUpdatedToday)
      {
      int ier = getI2TimeStr();   // request time/date from NTP
      if (ier == 1)     // got it
        {
        rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
        s0 = -1;                                  // force display update
        timeUpdatedToday = true;
        }
      }
    }
  else
    timeUpdatedToday = false;

// Update display
  if (s != s0)
    {
    updateDisplay();
    s0 = s;
    digitalWrite( LEDpin, !digitalRead( LEDpin));
    }
//
// Check for commands and parse any
// Check for USB
  if (Serial.available()) // Got command from USB
    {
    isUSB = true;
    isI2C = false;
    gotIO = true;
    }
// Check for I2C
  else if (GotI2C)        // Got command from I2C
    {
    isI2C = true;
    isUSB = false;
    gotIO = true;
    }
// Parse command from Serial or I2C
  if (gotIO) CommandParse();  // (CommandParse sets gotIO to false)
//
// Check for buttons
Bail:
  ButtonParse();
}   // bottom loop

void updateDisplay()
  {
//
// Update display frame buffer
//
  int bb, uu;   // bottom and upper digits
// clear buffer
  for (bb=0; bb<32; bb++) buf.b1x32[bb]=0;
// adjust for am or pm
  bb = h;
  if (p.hourStyle != military && bb>=12) bb=bb-12;
  if (p.hourStyle == ampm && bb == 0) bb = 12;
// hour
  if (bb>9)
    {
    if (didxidx==1)
      {
      didxidx = 0;
      }
    uu=bb/10;
    shodig( uu, 0, 5);
    bb=bb-(10*uu);
    }
  else
    {
    if (didxidx==0)
      {
      didxidx = 1;
      }
    }
  shodig( bb, 1, 5);
// colon
  if (p.fontN!=5 && bb==1 && !(m>9 && m<20))    // special case for colon when lower hour digit == 1 and thin font.
/// if (bb==1 && !(m>9 && m<20))    // special case for colon when lower hour digit == 1.
    shodig( Colon, -2, 2);
  else if((p.fontN==1 || p.fontN==2) && bb!=1 && (m>9 && m<20))
    shodig( Colon, -2, -2);
  else
    shodig( Colon, 2, 2);
// minute
  bb = m;
  bb=bb%60;
  if (bb>9)
    {
    uu=bb/10;
    shodig( uu, 3, 5);
    bb=bb-(10*uu);
    }
  else shodig( 0, 3, 5);
  shodig( bb, 4, 5);
// am or pm or military
  if (p.hourStyle == military) shodig( Emm, 5, 5);
  else
    {
    if (h<12) shodig( Aye, 5, 5);
    else shodig( Pee, 5, 5);
    }
//
// Write to display
//
  mx.setBuffer(7, 8, buf.b4x8[0]);
  mx.setBuffer(15, 8, buf.b4x8[1]);
  mx.setBuffer(23, 8, buf.b4x8[2]);
  mx.setBuffer(31, 8, buf.b4x8[3]);
  }
  
// end loop() ***************************************************
