#include <Arduino.h>

#include "StateMachine.h"

// Enumeration of HW buttons. ¨
// TIME_ELAPSED is a vitual button when 30s timer elapsed
// WIND is a virtual button when wind conditions are irregullar
enum Buttons_enum {NONE, BASE_A, BASE_B, ESC, ENTER, TIME_ELAPSED, WIND};

// Eunumeration of race condition
enum Race_state {NILL, FROM_A , FROM_B};

/*
Author V.Urban
Version 0.1 Alpha - OLED Display

 */

#include <OLED.h> // the OLED library 
#include <Metro.h> // Metronom Teensy library

OLED oled(OLED_V2, 23, 22, 21, 20, 19, 18, 17); //Version 2(timing), RS, R/W, Enable, D4, D5, D6, D7
sml::sm<F3F_StateMachine> sm;


//-------------------------------------------------------------------------------------------
void splashScreen()
{
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("F3F COMPETITION ");
  oled.setCursor(0, 1);
  oled.print("TIMER v2.0  ");
  
  for (size_t i = 0; i < 3; i++)
  {
    delay(1000);
    oled.noDisplay();
    delay(500);
    oled.display();
  }
  
}

// Helper function
uint8_t read_button()
{
  uint8_t tmp_button = 0;
  // temp char from Serial input
  char serial_input = 0;
  
  //temp process for buttons emulation
  if (Serial.available()) 
  {
    serial_input = Serial.read();
  }

  switch (serial_input)
  {
  case 97: //"a"
    tmp_button = BASE_A;
    break;
  case 100: //"d"
    tmp_button = BASE_B;
    break;  
  case 113: //"q"
    tmp_button = ESC;
    break;  
  case 101: //"e"
    tmp_button = ENTER;
    break;  
  case 119: //"w"
    tmp_button = WIND;
    break;  
  default:
    tmp_button = NILL;
    break;
  }

      
  //serial_input = 0;
  return tmp_button;

}


void setup() 
{
  
  oled.begin(16, 2);// Initialize the OLED with 16 characters and 2 lines
  oled.setCharMode(); //In case of soft reset when OLED was in graphics mode
  splashScreen();
  Serial.begin(9600);
  
}

void loop() 
{
  uint8_t tmp = read_button();
  
  if (tmp == ENTER)
    sm.process_event(enter_button{});
  
  if (tmp == ESC)
    sm.process_event(esc_button{});
  
  if (tmp == BASE_A)
    sm.process_event(A_base_button{});
  
  if (tmp == BASE_B)
    sm.process_event(B_base_button{});
  
  if (race_round == 10)
    sm.process_event(race_finished{});
  
  if ((metronom.check() == 1) && elapsed_check == true )
  {
    timer_to_elaps--;
    Serial.printf("Time to elaps: %d \n",timer_to_elaps);
    if (timer_to_elaps == 0)
      sm.process_event(timer_elapsed{});

  }

  

 
}