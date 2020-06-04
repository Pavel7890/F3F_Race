/*
Author V.Urban
Version 0.3 Alpha - OLED + Statemachine Boost::SML +Teensy Audio

 */
#include <Arduino.h>

//#include "StateMachine.h"
#include "main.h"

#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  11
#define SDCARD_SCK_PIN   13

#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Enumeration of HW buttons.
// TIME_ELAPSED is a vitual button when 30s timer elapsed
// WIND is a virtual button when wind conditions are irregullar
enum Buttons_enum {NONE, BASE_A, BASE_B, ESC, ENTER, TIME_ELAPSED, WIND};

// Eunumeration of race condition
enum Race_state {NILL, FROM_A , FROM_B};

#define abuttonPin      5
#define bbuttonPin      2
#define enterbuttonPin  3
#define escbuttonPin    4
#define sirenegpo       22

//#include <OLED.h> // the OLED library 
#include <Metro.h> // Metronom Teensy library
#include <Bounce.h> //Bounce Teensy library


Bounce aBaseButton = Bounce(abuttonPin, 300);
Bounce bBaseButton = Bounce(bbuttonPin, 300);
Bounce enterButton = Bounce(enterbuttonPin, 100);
Bounce escButton = Bounce(escbuttonPin, 100);

AudioPlaySdWav           playSdWav1;
AudioOutputI2S           i2s1;
AudioControlSGTL5000     sgtl5000_1;
AudioConnection          patchCord1(playSdWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playSdWav1, 1, i2s1, 1);
  

sml::sm<F3F_StateMachine> sm;


// Helper function, reads inputs from Serial/keyboard or from Inputs
uint8_t read_button()
{
  uint8_t tmp_button = 0;
  
  if (aBaseButton.update())
    if(aBaseButton.fallingEdge())
      return BASE_A;

  if (bBaseButton.update())
    if(bBaseButton.fallingEdge())
      return BASE_B;

  if (enterButton.update())
    if(enterButton.fallingEdge())
      return ENTER;

  if (escButton.update())
    if(escButton.fallingEdge())
      return ESC;

#ifdef DEBUG
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
#endif

 //serial_input = 0;
  return tmp_button;

}


void setup() 
{
  
  pinMode (abuttonPin, INPUT_PULLUP);
  pinMode (bbuttonPin, INPUT_PULLUP);
  pinMode (enterbuttonPin, INPUT_PULLUP);
  pinMode (escbuttonPin, INPUT_PULLUP);
  pinMode (sirenegpo, OUTPUT);

  oled.begin(16, 2);// Initialize the OLED with 16 characters and 2 lines
  oled.setCharMode(); //In case of soft reset when OLED was in graphics mode
  
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if(!SD.begin(SDCARD_CS_PIN))
    oled.print("SD Error");
  delay(1000);
  
  sdPlay("intro.wav");
    
  splashScreen();
  
  sm.process_event(enter_button{}); //goto initial menu

#ifdef DEBUG
  Serial.begin(9600);
#endif

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

#ifdef DEBUG
    Serial.printf("Zbyvajici cas: %.2d \n",timer_to_elaps);
#endif

    oled.setCursor(13, 1);
    oled.printf("%.2d", timer_to_elaps);

    if (timer_to_elaps == 25 || timer_to_elaps == 20 || timer_to_elaps == 15 || timer_to_elaps <= 10)
      soundDoubleDigit(timer_to_elaps);

    if (timer_to_elaps == 0)
      sm.process_event(timer_elapsed{});

  }

 
}

void soundTime(u_int32_t time)
{
  sdPlay("comp.wav");
  delay(300);

  if (time > 100000) 
    {
      soundDoubleDigit(100);
      time = time - 100000;
    }
    soundDoubleDigit(time/1000%100);
      
    while (playSdWav1.isPlaying())  {}
    sdPlay("point.wav");
        
    soundDoubleDigit(time/10%100);
}

void soundDoubleDigit(uint8_t digit)
{
  
  char data[10];

  if (digit == 100){
    while (playSdWav1.isPlaying())  {}
    sdPlay("100.wav");
  }
    
  if (digit > 20 && digit < 100)
  {
    while (playSdWav1.isPlaying())  {}
    sprintf(data,"%d.wav",digit-(digit%10));
    sdPlay(data);
        
    while (playSdWav1.isPlaying())  {}
    sprintf(data,"%d.wav",digit%10);
    sdPlay(data);
  }
  
  if (digit <= 20)
  {
    while (playSdWav1.isPlaying())  {}
    sprintf(data,"%d.wav",digit);
    sdPlay(data);
  }
}

void sdPlay(const char *filename)
{
  playSdWav1.play(filename);
  delay(10);
}