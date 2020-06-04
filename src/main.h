// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  11
#define SDCARD_SCK_PIN   13

#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


void soundDoubleDigit(uint8_t digit);
void soundTime(u_int32_t time);
void sdPlay(const char *filename);

#include <Arduino.h>
#include "sml.hpp"
#include <Metro.h>
#include <OLED.h> // the OLED library

namespace sml = boost::sml;

namespace
{
    OLED oled(OLED_V2, 24, 25, 32, 29, 28, 31, 30); //@args:OLED ver, RS , R/W, Enable, D4, D5, D6, D7

    uint8_t race_round = 0;
    uint32_t timer_start = 0;
    uint32_t timer_end = 0;
    uint32_t timer_tmp = 0;
    Metro metronom =  Metro(1000); // ticks every 1000ms
    uint8_t timer_to_elaps = 30;
    bool elapsed_check = false;

    //-------------------------------------------------------------------------------------------
    void splashScreen()
    {
      oled.clear();
      oled.setCursor(0, 0);
      oled.print("F3F Zvoneni v2.0");
      oled.setCursor(0, 1);
      oled.print("Autor - V.Urban");
      
      for (size_t i = 0; i < 3; i++)
      {
        delay(1000);
        oled.noDisplay();
        delay(500);
        oled.display();
      }
      
    }
    void batVoltage()
    {
      oled.setCursor(0, 0);
      oled.print("Lipo 12.1V");
    }

    //Events
    struct enter_button {};
    struct esc_button {};
    struct A_base_button {};
    struct B_base_button {};
    struct race_finished {};
    struct timer_elapsed {};

    //Actions
    const auto action_menu = [] 
    {
#ifdef DEBUG
      Serial.println("Lipo 12.1V");
      Serial.println("NAST.   PRAC.CAS");
#endif
      oled.setCharMode();
      batVoltage();
      oled.setCursor(0, 1);
      oled.print("NAST.   PRAC.CAS");

    };

    const auto action_setup = [] 
    {
#ifdef DEBUG
      Serial.println("Setup");
#endif
    };
    
    const auto action_prep = [] 
    {
      metronom.reset();
      elapsed_check = true;
      timer_to_elaps = 30;
#ifdef DEBUG
      Serial.println("Prac.cas     30s");
#endif
      oled.setCharMode();
      batVoltage();
      oled.setCursor(0, 1);
      oled.print("Prac.cas     30s");
      sdPlay("time.wav");     
    };
    
    const auto action_competition = [] 
    {
      metronom.reset();
      elapsed_check = true;
      timer_to_elaps = 30;
#ifdef DEBUG
      Serial.println("START        30s");
#endif
      oled.setCursor(0, 1);
      oled.print("START        30s");
      sdPlay("start.wav");     
    };
    
    const auto action_in_A_base = [] 
    {
#ifdef DEBUG
      Serial.println("V BAZI A");
#endif
      oled.setCursor(0, 1);
      oled.print("V BAZI A");     
    };
    const auto action_race_started = [] 
    
    {
      elapsed_check = false;
      race_round = 0;
      timer_start = millis();
#ifdef DEBUG
      Serial.println("MERENY USEK     ");
#endif
      oled.setCursor(0, 1);
      oled.print("MERENY USEK     ");
    };
    
    const auto action_race = [] 
    {
      race_round++;
      timer_tmp = millis() - timer_start;
#ifdef DEBUG
      Serial.printf("KOLO:%d CAS:%5.1f\n",race_round, (float)timer_tmp/1000 );
#endif
      oled.setCursor(0, 1);
      oled.printf("KOLO:%d CAS:%5.1f",race_round, (float)timer_tmp/1000 );
      soundDoubleDigit(race_round);
    };
    
    const auto action_race_finished = [] 
    {
      timer_end = millis();
      uint32_t timer_race = timer_end - timer_start;
#ifdef DEBUG
      Serial.printf("Cas celkovy %5.2f \n", (float)timer_race / 1000);
#endif
      oled.setGraphicMode();
      oled.clear();
      
      if (timer_race > 100000) oled.drawDigit(1,1);
      oled.drawDigit(timer_race/10000%10,2);
      oled.drawDigit(timer_race/1000%10,3);
      oled.drawDot(4);
      oled.drawDigit(timer_race/100%10,5);
      oled.drawDigit(timer_race/10%10,6);

      soundTime(timer_race);
            
    };

    
    const auto action_canceled = []
    {
      elapsed_check = false;
#ifdef DEBUG
      Serial.println("  LET   ZRUSEN  ");
#endif
      oled.setCursor(0, 1);
      oled.print("  LET   ZRUSEN  ");
      
    };
    
    const auto action_race_anulled = [] 
    {
      elapsed_check = false;
#ifdef DEBUG
      Serial.println("  LET ANULOVAN  ");
#endif
      oled.setCursor(0, 1);
      oled.print("  LET ANULOVAN  ");
      sdPlay("null.wav");
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
                 
                 "canceled"_s       + event<enter_button>   / action_prep           = "prep"_s,
                 "canceled"_s       + event<esc_button>     / action_menu           = "menu"_s,
                 
                 "race_null"_s       + event<enter_button>  / action_prep           = "prep"_s,
                 "race_null"_s       + event<esc_button>    / action_menu           = "menu"_s

           );
        }
    };
}