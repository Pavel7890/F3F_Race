#include <Arduino.h>

/*
Author V.Urban
Version 0.1 Alpha - OLED Display

 */

#include <OLED.h>//include the OLED library 


byte customChar0[] = { 0x04, 0x0E, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04 };
byte customChar1[] = { 0x04, 0x0E, 0x1F, 0x04, 0x04, 0x1F, 0x0E, 0x04 };
byte customChar2[] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F, 0x0E, 0x04 };

OLED oled(OLED_V2, 23, 22, 21, 20, 19, 18, 17); //Version 2(timing), RS, R/W, Enable, D4, D5, D6, D7


//-------------------------------------------------------------------------------------------
void splashScreen()
{
  

  oled.clear();
  oled.setCursor(0, 0);
  oled.print("F3F COMPETITION ");
  oled.setCursor(0, 1);
  oled.print("TIMER v2.0 ");
  
  // Custom Chars need to be written to CGRAM everytime GR/CH modes been switched
  oled.createChar(0, customChar0);
  oled.createChar(1, customChar1);
  oled.createChar(2, customChar2);
  oled.setCursor(11, 1);
  oled.write(0); // Custom char 0-2
  oled.write(1);
  oled.write(2);
  
  delay(1000);
  oled.noDisplay();
  delay(500);
  oled.display();
  delay(1000);
  oled.noDisplay();
  delay(500);
  oled.display();
  delay(1000);
  oled.noDisplay();
  delay(500);
  oled.display();
  delay(1000);

}
//-------------------------------------------------------------------------------------------
void counterSample()
{
  //this function prints a simple counter that counts to 250
  
  oled.clear();
  oled.home();// set the cursor to column 0, line 1
  oled.print("Counter = ");
  
  for(int i = 0; i <= 250; i++)
  {
    oled.setCursor(10,0);
    oled.print(i, DEC);
    delay(10);
  }
  delay(1000);
}




void randomTime()
{
  
  oled.setGraphicMode();
  oled.clear();
    
  uint32_t randNumber = random(3000,12000);
  if (randNumber > 10000) oled.drawDigit(1,1);
    
  oled.drawDigit(randNumber/1000%10,2);
  oled.drawDigit(randNumber/100%10,3);
  oled.drawDot(4);
  oled.drawDigit(randNumber/10%10,5);
  oled.drawDigit(randNumber%10,6);
  
  delay(3000);
  oled.setCharMode();
  
}

void drawGrDigits()
{
  oled.setGraphicMode();
  oled.clear();

  uint8_t i = 0; 
  do
  {
    oled.drawDigit(i,i);
    i++;
  } while (i < 8);
  
  delay(3000);
  oled.clear();
  oled.drawDigit(8,0);
  oled.drawDigit(9,1);
  oled.drawDot(2);
  
  delay(3000);
  oled.setCharMode();
  
}

void setup() 
{
  
  oled.begin(16, 2);// Initialize the OLED with 16 characters and 2 lines
  oled.setCharMode(); //In case of soft reset when OLED was in graphics mode

  
}

void loop() 
{
  splashScreen();
  
  counterSample();
  
  randomTime();
  
  drawGrDigits();

 
 }