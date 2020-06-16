// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  11
#define SDCARD_MISO_PIN  12
#define SDCARD_SCK_PIN   13

#define FLASH_CS_PIN    6
#define FLASH_MOSI_PIN  11
#define FLASH_MISO_PIN  12
#define FLASH_SCK_PIN   13

#define A_PIN           5 //A Base Button
#define B_PIN           2 //B Base Button
#define ENTER_PIN       3 //Enter(Start) Button
#define ESC_PIN         4 //ESC (Cancel) Button
#define SIRENE_PIN     22 //Output for Sirene

#define BAT_VOLT_PIN   14 //Lipo battery voltage

#include <Arduino.h> 
#include <Audio.h> //Teensy Audio Library
#include <Wire.h> //wire for SPI
#include <SPI.h> //SPI library
#include <SD.h> //SD card access
#include <SerialFlash.h> //Serial Flash memory access
#include "sml.hpp" //Boost:SML StateMachine
#include <Metro.h> //Teensy Metronom library
#include <OLED.h> // the OLED library
#include <ADC.h> //Teensy ADC library
#include <ADC_util.h> //Teensy ADC library
#include <Bounce.h> //Teensy Bounde library


void soundDoubleDigit(uint8_t digit);
void soundTime(u_int32_t time);
void sirene(uint32_t timeDelay);
void flashPlay(const char *filename);
void sdPlay(const char *filename);
void sdStop();
void flashPlayDigit(uint8_t digit);
void splashScreen();
void batVoltage();
void audVolume(float vol);

byte Gbat100[] ={0x0E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
byte Gbat80[] = {0x0E, 0x1B, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
byte Gbat60[] = {0x0E, 0x1B, 0x11, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
byte Gbat40[] = {0x0E, 0x1B, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x00};
byte Gbat20[] = {0x0E, 0x1B, 0x11, 0x11, 0x11, 0x1F, 0x1F, 0x00};
byte Gbat00[] = {0x0E, 0x1B, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x00};


// Enumeration of HW buttons.
// TODO: WIND is a virtual button when wind conditions are irregullar -
enum Buttons_enum {NONE, BASE_A, BASE_B, ESC, ENTER, WIND};

namespace sml = boost::sml;

namespace
{
  OLED oled(OLED_V2, 24, 25, 32, 29, 28, 31, 30); //@args:OLED ver, RS , R/W, Enable, D4, D5, D6, D7

  uint8_t race_round = 0;
  uint32_t timer_start = 0;
  uint32_t timer_end = 0;
  uint32_t timer_tmp = 0;
  Metro metronom =  Metro(1000); // ticks every 1000ms
  Metro batMeasure = Metro(30000); //each 30s measurement
  uint8_t timer_to_elaps = 30;
  bool elapsed_check = false;
  bool bat_check = true;
  float audioVolume = 0.6;

  //Events
  struct enter_button {};
  struct esc_button {};
  struct menu_button {}; //virtual button to trigger bat measurement
  struct A_base_button {};
  struct B_base_button {}; 
  struct race_finished {}; 
  struct timer_elapsed {}; //30 second timer elapsed event

  //Actions
  const auto action_menu = [] 
  {
    oled.setCharMode();
    bat_check = true;
    batVoltage();
    oled.setCursor(0, 1);
    oled.print("NAST.   PRAC.CAS");
#ifdef DEBUG
    batVoltage();
    Serial.println("NAST.   PRAC.CAS");
#endif
  };

  const auto action_setup = [] 
  {
    oled.setCursor(0, 1);
    oled.printf("VOL+ %4.1f   MENU", audioVolume*10);
#ifdef DEBUG
    Serial.println("Setup");
#endif
  };
  
  const auto action_prep = [] 
  {
    metronom.reset();
    elapsed_check = true;
    bat_check = false;
    timer_to_elaps = 30;
    oled.setCharMode();
    batVoltage();
    oled.setCursor(0, 1);
    oled.print("PRAC.CAS     30s");
    flashPlay("TIME");     
#ifdef DEBUG
    Serial.println("PRAC.CAS     30s");
#endif
  };
  
  const auto action_competition = [] 
  {
    metronom.reset();
    elapsed_check = true;
    timer_to_elaps = 30;
    oled.setCursor(0, 1);
    oled.print("START        30s");
    flashPlay("START");     
#ifdef DEBUG
    Serial.println("START        30s");
#endif
  };
  
  const auto action_in_A_base = [] 
  {
    sirene(400);
    oled.setCursor(0, 1);
    oled.print("V BAZI A");     
#ifdef DEBUG
    Serial.println("V BAZI A");
#endif
  };
  
  const auto action_race_started = [] 
  {
    elapsed_check = false;
    race_round = 0;
    timer_start = millis();
    sirene(400);
    oled.setCursor(0, 1);
    oled.print("  MERENY USEK   ");
#ifdef DEBUG
    Serial.println("  MERENY USEK  ");
#endif
  };
  
  const auto action_race = [] 
  {
    race_round++;
    timer_tmp = millis() - timer_start;
    if(race_round < 10)
    {
      sirene(400);
      soundDoubleDigit(race_round);
      oled.setCursor(0, 1);
      oled.printf("KOLO:%d %6.2fs  ",race_round, (float)timer_tmp/1000 );
    }
#ifdef DEBUG
      Serial.printf("KOLO:%d %6.2fs  \n",race_round, (float)timer_tmp/1000 );
#endif
  };
  
  const auto action_race_finished = [] 
  {
    timer_end = millis();
    uint32_t timer_race = timer_end - timer_start;
    oled.setCursor(0, 1);
    oled.printf("KOLO:10%6.2fs  ",(float)timer_race/1000);
#ifdef DEBUG
    Serial.printf("Cas celkovy %5.2f \n", (float)timer_race / 1000);
#endif
    sirene(1000);
    oled.setGraphicMode();
    oled.clear();
    if (timer_race > 100000) oled.drawDigit(1,1);
    oled.drawDigit(timer_race/10000%10,2);
    oled.drawDigit(timer_race/1000%10,3);
    oled.drawDot(4);
    oled.drawDigit(timer_race/100%10,5);
    oled.drawDigit(timer_race/10%10,6);
    soundTime(timer_race);
    bat_check = true;
    batMeasure.reset();
  };
  
  const auto action_canceled = []
  {
    elapsed_check = false;
#ifdef DEBUG
    Serial.println("  LET   ZRUSEN  ");
#endif
    oled.setCursor(0, 1);
    oled.print("  LET   ZRUSEN  ");
    sirene(400);
    delay(100);
    sirene(400);
    delay(100);
    sirene(400);
    delay(100);
    bat_check = true;
  };
  
  const auto action_race_anulled = [] 
  {
    elapsed_check = false;
#ifdef DEBUG
    Serial.println("  LET ANULOVAN  ");
#endif
    oled.setCursor(0, 1);
    oled.print("  LET ANULOVAN  ");
    flashPlay("NULL");
    bat_check = true;
  };

  const auto action_vol = []
  {
    audioVolume += 0.05;
    if (audioVolume > 0.81)
      audioVolume = 0.2;
    audVolume(audioVolume);
    oled.setCursor(0, 1);
    oled.printf("VOL+ %4.1f   MENU", audioVolume*10);
  };
    
  struct F3F_StateMachine  
  {
    auto operator()() const
    {
      using namespace sml;
      return make_transition_table(
        
        *"entry"_s          + event<enter_button>   / action_menu           = "menu"_s,
          
          "menu"_s           + event<esc_button>     / action_setup          = "setup"_s,
          "menu"_s           + event<enter_button>   / action_prep           = "prep"_s,
          "menu"_s           + event<menu_button>    / action_menu           = "menu"_s,
          
          "prep"_s           + event<enter_button>   / action_competition    = "competition"_s,
          "prep"_s           + event<timer_elapsed>  / action_race_anulled   = "race_null"_s,
          "prep"_s           + event<esc_button>     / action_canceled       = "canceled"_s,
          
          "competition"_s    + event<A_base_button>  / action_in_A_base      = "prep_A_base"_s,
          "competition"_s    + event<timer_elapsed>  / action_race_started   = "race_to_B"_s,
          "competition"_s    + event<esc_button>     / action_canceled       = "canceled"_s,
          
          "prep_A_base"_s    + event<A_base_button>  / action_race_started   = "race_to_B"_s,
          "prep_A_base"_s    + event<timer_elapsed>  / action_race_started   = "race_to_B"_s,
          "prep_A_base"_s    + event<esc_button>     / action_canceled       = "canceled"_s,
          
          "race_to_A"_s      + event<A_base_button>  / action_race           = "race_to_B"_s,
          "race_to_A"_s      + event<esc_button>     / action_canceled       = "canceled"_s,
          
          "race_to_B"_s      + event<B_base_button>  / action_race           = "race_to_A"_s,
          "race_to_B"_s      + event<race_finished>  / action_race_finished  = "time_eval"_s,
          "race_to_B"_s      + event<esc_button>     / action_canceled       = "canceled"_s,
      
          "time_eval"_s      + event<enter_button>   / action_prep           = "prep"_s,
          "time_eval"_s      + event<esc_button>     / action_menu           = "menu"_s,
          "time_eval"_s      + event<menu_button>    / action_menu           = "menu"_s,
          
          "canceled"_s       + event<enter_button>   / action_prep           = "prep"_s,
          "canceled"_s       + event<esc_button>     / action_menu           = "menu"_s,
          
          "race_null"_s      + event<enter_button>  / action_prep            = "prep"_s,
          "race_null"_s      + event<esc_button>    / action_menu            = "menu"_s,
          
          "setup"_s          + event<esc_button>    / action_vol,
          "setup"_s          + sml::on_entry<_>     / [] {sdPlay("MUSIC");},
          "setup"_s          + sml::on_exit<_>      / [] {sdStop();},
          "setup"_s          + event<enter_button>  / action_menu            = "menu"_s

      );
    }
  };
}