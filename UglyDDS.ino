/* UglyDDS - Alpha Version 3
 * VFO & BFO frequency only
 *  
 * Just "copy & paste" from others. What have I done? :) 
 * Michael Daranto - YD3BRB 
 * 
 * Encoder  - MD_REncoder - https://github.com/MajicDesigns/MD_REncoder
 * Si5351   - Etherkit - Jason Milldrum
 * Oled     - SSD1306Ascii - Bill Greiman
 * http://ak2b.blogspot.com/
 * 
 * This sketch  is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License.
 * If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#define F_MIN        6950000UL      // change to suit         
#define F_MAX        7250000UL

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#include "si5351.h"
#include "Wire.h"

#include <MD_REncoder.h>
#define pinA 2
#define pinB 3
int s= 7; //Switch button

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

// Define proper RST_PIN if required.
#define RST_PIN -1

Si5351 si5351;
SSD1306AsciiWire oled;
MD_REncoder R = MD_REncoder(pinA, pinB);

long bfo;

long IF = 12000000; //IF freq - change to suit
long sb = 1500; // side band - change to suit

long vfo = 7030000  ; //start freq - change to suit
volatile uint32_t radix = 100;  //start step size - change to suit
boolean changed_f = 0;

/******************************************/
/*  Setup everything for first time       */
/******************************************/  
void setup() {
  Wire.begin();
  Wire.setClock(400000L);
  
  R.begin(); //Encoder init
  
/********************/
/* LSB / USB        */
/********************/

bfo=IF-sb ; //LSB or USB. Just change "-" with "+"

/********************/
/* OLED 128x64      */
/********************/
#if RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS, RST_PIN);
#else // RST_PIN >= 0
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
#endif // RST_PIN >= 0

  oled.setFont(Adafruit5x7);

  oled.setCursor(37,7);
  oled.print("UglyDDS"); // change to suit :)
  oled.set2X();

  display_frequency();  // Update the display
  display_radix();
 
/********************/
/*  Si5351          */
/********************/
 
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0); //Activate Si5351
 
/**************************************************************/
/*Calibration.                                                */  
/*Increase/decrease cal_factor value to change output freq    */
/*Default = 0                                                 */  
/**************************************************************/ 
  int32_t cal_factor = 0;
  si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);

/*************************/
/*CLK0 - PLLA - VFO      */
/*************************/
  si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA); //change to suit (2MA/4MA/6MA/8MA)  
  si5351.set_freq((IF-vfo)*100ULL,SI5351_CLK0);

/*************************/
/*CLK2 - PLLB - BFO      */
/*************************/ 
  si5351.set_ms_source(SI5351_CLK2, SI5351_PLLB);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA); //change to suit (2MA/4MA/6MA/8MA)
  si5351.set_freq(bfo*100ULL, SI5351_CLK2);

/************************************************/
/*Interupt Pin for Encoder & s (switch button)  */
/*                                              */
/*D2 & D3 for Encoder, & D7 for s.              */
/*                                              */  
/************************************************/ 

  pinMode(s, INPUT_PULLUP); // We need input pin with internal pull up for switch
  PCICR |= (1 << PCIE2);   // Enable pin change interrupt for the encoder
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
}
/**************************/
/*    Main Program        */
/*                        */
/**************************/
void loop() 
{
 if (changed_f==1) // Update the display if the frequency has been changed
  {
    display_frequency();
 // si5351.set_freq(vfo*100ULL,SI5351_CLK0);
    si5351.set_freq((IF-vfo)*100ULL,SI5351_CLK0);
    changed_f = 0;
  }
  
 if (get_button()==0) // Button press changes the frequency change steps 
  {
    delay(170); // change to suit
    switch (radix)
    {
      case 1:
        radix = 10;
        break;
      case 10:
        radix = 100;
        break;
      case 100:
        radix = 1000;
        break;
      case 1000:
        radix = 10000;
        break;
      case 10000:
        radix = 100000;
        break;
      case 100000:
        radix = 1;
        break;
    }
    display_radix();
  }  
}

/**************************************/
/* Interrupt service routine for      */
/* encoder frequency change           */
/**************************************/
ISR(PCINT2_vect) 
{
  uint8_t x = R.read();

  if (x == DIR_CW)
    set_frequency(1);
  if (x == DIR_CCW)
    set_frequency(-1);
}

/**************************************/
/* Change the frequency               */
/* dir = 1    Increment               */
/* dir = -1   Decrement               */
/**************************************/
void set_frequency(short dir)
{
  if (dir == 1)
        vfo += radix;
  if (dir == -1)
        vfo -= radix;
        
  if (vfo > F_MAX)
        vfo = F_MAX;
  if (vfo < F_MIN)
        vfo = F_MIN;

  changed_f = 1;
}

/**************************************/
/* Read the button with debouncing    */
/**************************************/
boolean get_button()
{
  if (digitalRead(s)==0)
  {
    delay(50); //change to suit
    return 0;
   }
   delay(50); //change to suit
   return 1;
}

/**************************************/
/* Displays the frequency             */
/**************************************/
void display_frequency()
{
  
  uint16_t f, g;
  
  oled.setCursor(0,0);
  oled.set2X();
  f = vfo / 1000000;  //variable is now vfo instead of 'frequency'
  if (f < 10)
    oled.print(" ");
    oled.print(f);
    oled.print(".");
    f = (vfo % 1000000) / 1000;
  if (f < 100)
    oled.print("0");
  if (f < 10)
    oled.print("0");
    oled.print(f);
    oled.print(".");
    f = vfo % 1000;
  if (f < 100)
    oled.print("0");
  if (f < 10)
    oled.print("0");
    oled.print(f);
    oled.setCursor(0,1);
  
}
/**************************************/
/* Displays the frequency change step */
/**************************************/
void display_radix()
{
  
  oled.setCursor(75,2); //1x fonts
  oled.set1X();
  
   switch (radix)
  {
    case 1:
      oled.print("    1");
      break;
    case 10:
      oled.print("   10");
      break;
    case 100:
      oled.print("  100");
      break;
    case 1000:
      oled.print("   1k");
      break;
    case 10000:
      oled.print("  10k");
      break;
    case 100000:
      oled.print(" 100k");
      break;
 }
 
     oled.print("Hz");
 
}
