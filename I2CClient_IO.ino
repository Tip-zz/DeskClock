/* 
* File: I2CClient_IO.ino
* Origin: 04-Feb-2023
* Author: Tip Partridge
* Descrition:
*		I2C slave for communication with ESP8266 I2C master.
*   Would prefer this was a master, but ESP I2C slave not compatible
*   Hardware handshake pin reqPin implemented to allow slave to request to send data to master.
*   Slave sets reqPin, master sees reqPin and requests 1 byte from slave.
*   Slave sends number of bytes it wants to send.
*   Master requests this number of bytes and then slave sends them.
*/

// *****************************************************************

void outI2C(char *instr)
// Send string variable to I2C. Deal with 30 byte write buffer limit.
  {
  int siz;
  int ii, jj, cc;
  int chunks;
  int tail;

  siz = strlen(instr);
  chunks = siz / 32;
  tail = siz - (chunks * 32);
  i2Len = 32;
  cc = 0;
  for (ii=1; ii<=chunks; ii++)
    {
    for (jj=0; jj<32; jj++) i2Str[jj] = instr[cc++];
    sendI2Str();
    }
  if (tail)
    {
    i2Len = tail;
    for (jj=0; jj<tail; jj++) i2Str[jj] = instr[cc++];
    sendI2Str();
///    nI2C = sendI2Str();
    }
  }

// *****************************************************************

void outI2C(const char *instr)
  {
  char astr[40];
  for (unsigned int ii = 0; ii <= strlen(instr); ii++)
    astr[ii] = instr[ii];
  outI2C( astr);
  }

// *****************************************************************

void outI2C(char inchr)
// Send char variable to I2C.
  {
  i2Len = 1;
  i2Str[0] = inchr;
  i2Str[1] = 0;
  sendI2Str();
  }

// *****************************************************************

void TOInterruptOn()
// Turn on 100ms timeout ISR
  {
  g_timeout = false;                // turn off global timeout flag
  TCNT1 = 0;                        // clear timer counter register
  TIFR1 = 255;                      // clear OCRA flag registers so we don't get immediate interrupt
  TIMSK1 |= _BV(OCIE1A);            // timer 1 output compare interrupt enable
  }
  
// *****************************************************************

void TOInterruptOff()
// Turn off 100ms timeout ISR
  {
  TIMSK1 &= !_BV(OCIE1A);           // done, turn off interrupt
  g_timeout = false;                // turn off global timeout flag
  }

// *****************************************************************

int sendI2Str()
// send global i2Len bytes from global transmit buffer i2Str to I2C
  {
  isLen = true;
  TOInterruptOn();
  digitalWrite( reqPin, HIGH);    // flag host to request size
  digitalWrite( bsyPin, LOW);     // blink ESP LED
  while (!I2CReq && !g_timeout);  // wait for length request or timeout
  TOInterruptOff();
  digitalWrite( reqPin, LOW);     // reset flag
  digitalWrite( bsyPin, HIGH);    // blink ESP LED
  if (I2CReq)
    {
    I2CReq = false;
    }
  else return -1;   // timeout sending length!
  TOInterruptOn();
  while (!I2CReq && !g_timeout);  // wait for data, or timeout
  TOInterruptOff();
  if (I2CReq)
    {
    I2CReq = false;
    }
  else return -2;   // timeout sending data!
  return i2Len;
  }

// *****************************************************************

int getI2CmdString( byte bCmd)
// send special command and get get string or byte data from I2C into global buffer i2Str
// bCmd is the special command: 0xff = get date/time, 0xfe get status, 0xfd get version
  {
  isLen = true;                   // tell requestEvent() to send 1 byte
  i2Len = bCmd;                   // special code tells master what to send
  TOInterruptOn();
  digitalWrite( reqPin, HIGH);    // flag host to request size
  digitalWrite( bsyPin, LOW);     // blink ESP LED
  while (!I2CReq && !g_timeout);  // wait for length request or timeout
  TOInterruptOff();
  digitalWrite( reqPin, LOW);     // reset flag
  digitalWrite( bsyPin, HIGH);    // blink ESP LED
  if (I2CReq)
    {
    I2CReq = false;
    }
  else 
    {
    return -1;   // timeout sending NTP command!
    }
  TOInterruptOn();
  while (!GotI2C && !g_timeout);  // wait for date-time data; 7 bytes yymdhms
  TOInterruptOff();
  if (GotI2C)
    {
    GotI2C = false;
    return 1;
    }
  else 
    {
    return -3;   // timeout receiving data!
    }
  }

// *****************************************************************

int getI2TimeStr()
// get date/time string from I2C into global buffer i2Str
  {
  int ier;
  ier = getI2CmdString( 0xff);
  if (ier != 1) return( ier);
  if (I2_pnt == 7)  // got valid date&time, update RTC
    {
    y = I2C_rcv[0] * 100 + I2C_rcv[1];
    n = I2C_rcv[2];
    d = I2C_rcv[3];
    h = I2C_rcv[4];
    m = I2C_rcv[5];
    s = I2C_rcv[6];
    return 1;
    }
  else 
    {
    return -4;   // invalid time!
    }
  }

// *****************************************************************
// Interrupt Handlers
// *****************************************************************

//********************
// I2C Receive Interrupt Handler
//********************
void I2Creceive( int nrcv)
  {
  while( Wire.available())    // Put received bytes into I2C_rcv buffer
    {
    if (!GotI2C)
      {
      I2_pnt = 0;
      GotI2C = true;
      }
    if (I2_pnt < i2RcvMax) {I2C_rcv[I2_pnt++] = Wire.read();}
    }
  } //void I2Creceive

// *****************************************************************

//********************
// I2C Request Interrupt Handler
//********************
// Function that executes whenever data is requested by I2C master.
// Uses global i2Len number of bytes set by various outI2C() routines.
// If global isLen flag is set sends i2Len, else sends i2Len bytes from global i2Str buffer.
void requestEvent()
  {
  int nn;
  digitalWrite( reqPin, LOW);   // reset flag
  digitalWrite( bsyPin, HIGH);  // blink ESP LED
  if (isLen)    // send single byte data length
    {
    Wire.write(i2Len);
    isLen = false;
    }
  else          // send data
    {
    for (nn=0; nn<i2Len; nn++)
      Wire.write(i2Str[nn]);
    }
  I2CReq = true;
  }

// *****************************************************************

//********************
// Timer 1 ISR, set global g_timeout = true
//********************

ISR(TIMER1_COMPA_vect)
// Come here every 100ms
{
if (digitalRead(13)) digitalWrite(13,LOW);
else digitalWrite(13,HIGH);
  TCNT1 = 0;    // clear timer counter register
  TIMSK1 &= !_BV(OCIE1A);   // done, turn off interrupt
  g_timeout = true;
}

// *****************************************************************
