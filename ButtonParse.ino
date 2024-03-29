///
/*
File:     ButtonParse.ino
Origin:   01-Oct-2022
Author:   Tip Partridge

Description:
  Part of DeskClock.ino.  This file parses hardware command buttons

Revisions:
  09-Oct-2022 (TEP) Split from WallClock.ino.
*/
void ButtonParse()
{
// Second zero or get time from NTP if X button is pressed
  if(!digitalRead( sec0)) {delay(5);}
  if(!digitalRead( sec0))
    {
    if (!sec0But)
      {
      if(digitalRead( xXX))   // X button not pressed
        s=0;                  // set seconds to zero
      else                    // X button pressed
        getI2TimeStr();       // get time from NTP
      updateRTC = true;
      }
    sec0But = true;
    }
  else sec0But = false;
// Hour up
  if(!digitalRead( hUp)) {delay(5);}
  if(!digitalRead( hUp))
    {
    if (!hUpBut)
      {
      if(digitalRead( xXX)) h += 1;
      else  h += 4;
      if (h > 23) h = 0;
      if (h == timeToUpdateFromUTP) timeUpdatedToday = true;  // skip daily check
      updateRTC = true;
      }
    hUpBut = true;
    }
  else hUpBut = false;
// Hour down
  if(!digitalRead( hDn)) {delay(5);}
  if(!digitalRead( hDn))
    {
    if (!hDnBut)
      {
      if(digitalRead( xXX)) h -= 1;
      else  h -= 4;
      if (h < 0) h = 23;
      if (h == timeToUpdateFromUTP) timeUpdatedToday = true;  // skip daily check
      updateRTC = true;
      }
    hDnBut = true;
    }
  else hDnBut = false;
// Minute up
  if(!digitalRead( mUp)) {delay(5);}
  if(!digitalRead( mUp))
    {
    if (!mUpBut)
      {
      if(digitalRead( xXX)) m += 1;
      else  m += 10;
      if (m > 59) m = 0;
      updateRTC = true;
      }
    mUpBut = true;
    }
  else mUpBut = false;
// Minute down
  if(!digitalRead( mDn)) {delay(5);}
  if(!digitalRead( mDn))
    {
    if (!mDnBut)
      {
      if(digitalRead( xXX)) m -= 1;
      else m -= 10;
      if (m < 0) m = 59;
      updateRTC = true;
      }
    mDnBut = true;
    }
  else mDnBut = false;
//
// Change RTC time
  if (updateRTC)
    {
    s0 = -1;                                  // force display update
    rtc.adjust(DateTime( y, n, d, h, m, s));  // update RTC
    updateRTC = false;
    }
}
