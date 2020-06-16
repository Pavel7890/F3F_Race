/*
Author V.Urban
Version 0.4 Beta:
 - OLED library with own implementation of graphics digits - current display 16x2 RS0010 driver 4bit connection with busy check (fast)
 - Statemachine based on Boost::SML - greatly simplifies the code and has very small memory footprint
 - Teensy Audio Library and Shield https://www.pjrc.com/store/teensy3_audio.html
 - Internal Flash as storage of audio files in RAW format (44.1KHz Mono 16bit)
 - SD card as storage for music or other audio files
 - DO for sirene with external MOS transistor
 - Battery Voltage Measurements of LiPo 3 cells as main power source, < 10V triggers message
 - Seup menu added with audio output Volume settings
 - TODO: Wind measurement Direction and speed
 - TODO: Wireless support for remote bases A/B
 */
#include <Arduino.h>
#include "main.h"

Bounce aBaseButton = Bounce(A_PIN, 300);
Bounce bBaseButton = Bounce(B_PIN, 300);
Bounce enterButton = Bounce(ENTER_PIN, 100);
Bounce escButton = Bounce(ESC_PIN, 100);

ADC *adc = new ADC();

AudioPlaySerialflashRaw  playFlashFile;
AudioPlaySdRaw           playSDFile;
AudioSynthWaveform       playSynthWave;
AudioMixer4              mixer;
AudioOutputI2S           i2s1;
AudioControlSGTL5000     sgtl5000;
AudioConnection          patchCord1(playFlashFile, 0, mixer, 0);
AudioConnection          patchCord2(playSDFile, 0, mixer, 1);
AudioConnection          patchCord3(playSynthWave, 0, mixer, 2);
AudioConnection          patchCord4(mixer, 0, i2s1, 0);
AudioConnection          patchCord5(mixer, 0, i2s1, 1);
  
int batVolt = 0;

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
    tmp_button = NONE;
    break;
  }
#endif

 //serial_input = 0;
  return tmp_button;

}


void setup() 
{
  
  pinMode (A_PIN, INPUT_PULLUP);
  pinMode (B_PIN, INPUT_PULLUP);
  pinMode (ENTER_PIN, INPUT_PULLUP);
  pinMode (ESC_PIN, INPUT_PULLUP);
  pinMode (SIRENE_PIN, OUTPUT);
  
  //pinMode (BAT_VOLT_PIN, INPUT);

  oled.begin(16, 2);// Initialize the OLED with 16 characters and 2 lines
  oled.createChar(5,Gbat100);
  oled.createChar(4,Gbat80);
  oled.createChar(3,Gbat60);
  oled.createChar(2,Gbat40);
  oled.createChar(1,Gbat20);
  oled.createChar(0,Gbat00);
  

  oled.setCharMode(); //In case of soft reset when OLED was in graphics mode
  oled.setCursor(0, 0);
  oled.clear();
  
  adc->adc0->setAveraging(16);
  adc->adc0->setResolution(16);
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);
  
  AudioMemory(8);
  SPI.setMOSI(FLASH_MOSI_PIN);
  SPI.setMISO(FLASH_MISO_PIN);
  SPI.setSCK(FLASH_SCK_PIN);
  playSynthWave.begin(0.0, 4000, WAVEFORM_SAWTOOTH);
  
  
  sgtl5000.enable();
  sgtl5000.volume(audioVolume);
  
  if(!SD.begin(SDCARD_CS_PIN))
    oled.print("SD Error        ");
  delay(1000); //Wait to initilize SD card */
  
  if (!SerialFlash.begin(FLASH_CS_PIN)) {
    oled.print("Error Flash chip");
    delay(2000);
  }

  flashPlay("INTRO");
  splashScreen();
  
  sm.process_event(enter_button{}); //goto initial menu
  
  batMeasure.reset();

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
  
  if ((metronom.check() == 1) and elapsed_check)
  {
    timer_to_elaps--;

#ifdef DEBUG
    Serial.printf("Zbyvajici cas: %2d \n",timer_to_elaps);
#endif

    oled.setCursor(13, 1);
    oled.printf("%2d", timer_to_elaps);

    if (timer_to_elaps == 25 or timer_to_elaps == 20 or timer_to_elaps == 15 or timer_to_elaps <= 10)
      soundDoubleDigit(timer_to_elaps);

    if (timer_to_elaps == 0)
      sm.process_event(timer_elapsed{});
  }

  if (batMeasure.check() == 1 and bat_check)
    sm.process_event(menu_button{});
       
}

void soundTime(u_int32_t time)
{
  flashPlay("COMP");
  delay(300);

  if (time > 100000) 
    {
      soundDoubleDigit(100);
      time = time - 100000;
    }
  soundDoubleDigit(time/1000%100);
    
  while (playFlashFile.isPlaying())  {}
  flashPlay("POINT");
      
  soundDoubleDigit(time/10%100);
}

void soundDoubleDigit(uint8_t digit)
{
  
  if (digit == 100){
    while (playFlashFile.isPlaying())  {}
    flashPlayDigit(100);
  }
  
  if (digit == 25){
    while (playFlashFile.isPlaying())  {}
    flashPlayDigit(25);
  }
    
  if (digit > 20 and digit < 100 and !(digit == 25))
  {
    while (playFlashFile.isPlaying())  {}
    flashPlayDigit(digit-(digit%10));
    
        
    while (playFlashFile.isPlaying())  {}
    flashPlayDigit(digit%10);
  }
  
  if (digit <= 20)
  {
    while (playFlashFile.isPlaying())  {}
    flashPlayDigit(digit);
  }
}

void flashPlay(const char *filename)
{
  char buff[10];
  sprintf(buff,"%s.RAW",filename);
#ifdef DEBUG
    Serial.printf("Buff: %s \n",buff);
#endif
  
  playFlashFile.play(buff);
}


void flashPlayDigit(uint8_t digit)
{
  char buff[10];
  sprintf(buff,"%d.RAW",digit);
#ifdef DEBUG
    Serial.printf("Buff: %s \n",buff);
#endif

  playFlashFile.play(buff);
}

void sdPlay(const char *filename)
{
  char buff[10];
  sprintf(buff,"%s.RAW",filename);
#ifdef DEBUG
    Serial.printf("Buff: %s \n",buff);
#endif
  
  playSDFile.play(buff);
}

void sdStop()
{
  playSDFile.stop();
}

void sirene(uint32_t timeDelay)
{
  digitalWrite(SIRENE_PIN, HIGH);
  delay(timeDelay);
  digitalWrite(SIRENE_PIN, LOW);
}

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
  batVolt = adc->adc0->analogRead(BAT_VOLT_PIN);
  float tmpAdc = batVolt*3.3*6.626/adc->adc0->getMaxValue(); //3.3V Ref, 5.217 Resisitive Divider constant
  oled.setCursor(0, 0);
  if (tmpAdc < 10.0)
  {
    playSynthWave.amplitude(1.0);
    oled.printf("Z%4.1fV    POZOR!",tmpAdc);
    delay(1000);
    playSynthWave.amplitude(0.0);
  }
  else
  {
    //oled.print("Z               ");                 //preparation for graphic cursor
    oled.printf("Z%4.1fV          ",tmpAdc);
    //oled.setCursor(1, 0);                           //preparation for graphic cursor
    //oled.write((uint8_t)((tmpAdc-10)/0.52));        //preparation for graphic cursor
  }
#ifdef DEBUG
    Serial.printf("ADC Value: %d\n", batVolt);
    Serial.printf("BAT%3.1fV\n",batVolt*3.3*6.626/adc->adc0->getMaxValue());
#endif
}

void audVolume(float vol)
{
  sgtl5000.volume(vol);
}