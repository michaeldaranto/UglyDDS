/*
 * UglyDDS_LCD16x2 Double Conversion - Alpha Version (Nop2021)
 * VFO & BFO frequency only
 *  
 * Just "copy & paste" from others. What have I done? :) 
 * Michael Daranto - YD3BRB 
 * 
 * Encoder  - MD_REncoder - https://github.com/MajicDesigns/MD_REncoder
 * Si5351   - Etherkit - Jason Milldrum
 * http://ak2b.blogspot.com/
 * 
 * The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 4
 * LCD D5 pin to digital pin 5
 * LCD D6 pin to digital pin 6
 * LCD D7 pin to digital pin 7
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
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

#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


#define F_MIN        6950000UL      // change to suit         
#define F_MAX        7250000UL


#include "si5351.h"
#include "Wire.h"

#include <MD_REncoder.h>
#define pinA 2
#define pinB 3
int s= 8; //Switch button - change to suit

// Define proper RST_PIN if required.
#define RST_PIN -1

Si5351 si5351;

MD_REncoder R = MD_REncoder(pinA, pinB);

/*********************************/
/*  IF1 = 21.4MHz & IF2 = 10MHz  */
/*  Change to suit               */
/*********************************/

long IF1 = 21400000; //21.4MHz xtal, change to suit
long bfo=9998800; //10MHz xtal, change to suit


long vfo = 7000000  ; //start freq - change to suit
volatile uint32_t radix = 10;  //start step size - change to suit
boolean changed_f = 0;


void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  lcd.clear();
  Wire.begin();

  Wire.setClock(400000L);
  
  R.begin(); //Encoder init

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
  int32_t cal_factor = 0; //change to suit
  si5351.set_correction(cal_factor, SI5351_PLL_INPUT_XO);

/*************************/
/*CLK0 - PLLA - VFO/LO   */
/*************************/
  si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA);//change to suit (2MA/4MA/6MA/8MA)  
  si5351.set_freq((IF1-vfo)*100ULL,SI5351_CLK0);

/*************************/
/*CLK1 - PLLA - LO2   */
/*************************/

  si5351.set_ms_source(SI5351_CLK1, SI5351_PLLA);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_6MA);//change to suit (2MA/4MA/6MA/8MA)  
  si5351.set_freq((IF1-bfo)*100ULL,SI5351_CLK1);


/*************************/
/*CLK2 - PLLB - BFO      */
/*************************/ 
  si5351.set_ms_source(SI5351_CLK2, SI5351_PLLB);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);//change to suit (2MA/4MA/6MA/8MA)
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

void loop() {
  // put your main code here, to run repeatedly:


  if (changed_f==1) // Update the display if the frequency has been changed
  {
    display_frequency();
 // si5351.set_freq(vfo*100ULL,SI5351_CLK0); //for calibration
    si5351.set_freq((IF1-vfo)*100ULL,SI5351_CLK0);
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
  lcd.setCursor(0, 0);
 
  f = vfo / 1000000;  //variable is now vfo instead of 'frequency'
  if (f < 10)
    lcd.print(" ");
    lcd.print(f);
    lcd.print(".");
    f = (vfo % 1000000) / 1000;
  if (f < 100)
    lcd.print("0");
  if (f < 10)
    lcd.print("0");
    lcd.print(f);
    lcd.print(".");
    f = vfo % 1000;
  if (f < 100)
    lcd.print("0");
  if (f < 10)
    lcd.print("0");
    lcd.print(f);
    lcd.setCursor(0,1);
  
}
/**************************************/
/* Displays the frequency change step */
/**************************************/
void display_radix()
{
  
  lcd.setCursor(9,1); 

  
   switch (radix)
  {
    case 1:
      lcd.print("    1");
      break;
    case 10:
      lcd.print("   10");
      break;
    case 100:
      lcd.print("  100");
      break;
    case 1000:
      lcd.print("   1k");
      break;
    case 10000:
      lcd.print("  10k");
      break;
    case 100000:
      lcd.print(" 100k");
      break;
 }
 
     lcd.print("Hz");
 
}
