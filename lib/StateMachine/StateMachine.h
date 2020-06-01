#include "sml.hpp"
#include <Metro.h>

namespace sml = boost::sml;

namespace
{
    uint8_t race_round = 0;
    uint32_t timer_start = 0;
    uint32_t timer_end = 0;
    Metro metronom =  Metro(1000);
    uint8_t timer_to_elaps = 30;
    bool elapsed_check = false;
    //Events
    struct enter_button {};
    struct esc_button {};
    struct A_base_button {};
    struct B_base_button {};
    struct race_finished {};
    struct timer_elapsed {};

    //Actions
    const auto action_menu = [] {Serial.println("Menu");};
    const auto action_setup = [] {Serial.println("Setup");};
    
    const auto action_prep = [] 
    {
      metronom.reset();
      elapsed_check = true;
      timer_to_elaps = 30;
      Serial.println("Prep");
    };
    
    const auto action_competition = [] 
    {
      metronom.reset();
      elapsed_check = true;
      timer_to_elaps = 30;
      Serial.println("Competition");
    };
    
    const auto action_in_A_base = [] {Serial.println("In A Base");};
    const auto action_race_started = [] 
    
    {
      elapsed_check = false;
      race_round = 0;
      timer_start = millis();
      Serial.println("Race Started");
      
    };
    
    const auto action_race = [] 
    {
      race_round++;
      Serial.printf("Race round: %d \n",race_round);
    };
    
    const auto action_race_finished = [] 
    {
      timer_end = millis();
      uint32_t timer_race = timer_end - timer_start;
      Serial.println("Race Finished");
      Serial.printf("Race Time %f \n", (float)timer_race / 1000);

    };

    
    const auto action_canceled = []
    {
      elapsed_check = false;
      Serial.println("Canceled");
    };
    
    const auto action_race_anulled = [] 
    {
      elapsed_check = false;
      Serial.println("Anulled");
    };
       

    struct F3F_StateMachine  
    {
        auto operator()() const
        {
            using namespace sml;
            return make_transition_table(
                *"menu"_s           + event<esc_button>     / action_setup          = "setup"_s,
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
                 
                 "canceled"_s       + event<enter_button>   / action_prep           = "prep"_s
           );
        }
    };
}