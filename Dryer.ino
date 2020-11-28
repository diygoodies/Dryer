
#include "SevSeg.h"
#include <OneWire.h>
#include <DS18B20.h>
#include <ButtonDebounce.h>
#include <EEPROM.h>

#define ONE_WIRE_BUS 14
#define GET_PERIOD 1000
#define INC_PERIOD 500
#define BLNK_PERIOD 1000
#define TEMP_MIDDLE 215

#define TEMP_MODE       0
#define TEMP_ALARM_MODE 1
#define TOP_LEVEL_MODE  2
#define BOT_LEVEL_MODE  3
#define RELAY_PIN       13

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);
int temp=0;
byte topTempTreshhold=5;
byte botTempTreshhold=5;
unsigned int menuMode=0;
unsigned long incTimer;
SevSeg sevseg; //Instantiate a seven segment controller object

ButtonDebounce buttonL(2, 250);
ButtonDebounce buttonC(3, 250);
ButtonDebounce buttonR(4, 250);


bool checkIncPromt(void)
{
  if (millis() - incTimer >= INC_PERIOD) 
  {
    Serial.println(millis() - incTimer);
    incTimer = millis();
    /*
    Serial.print("topTempTreshhold ");
    Serial.print(topTempTreshhold+TEMP_MIDDLE);
    Serial.print("   botTempTreshhold ");
    Serial.println(TEMP_MIDDLE-botTempTreshhold);*/
    return 1;
  }
 return 0; 
}

void buttonLChanged(int state)
{
  //Serial.println("L Changed: " + String(state));
  if (((menuMode==TEMP_MODE)||(menuMode==TEMP_ALARM_MODE))&&(state==0))
  {
    menuMode=BOT_LEVEL_MODE;
  }
  if ((menuMode==BOT_LEVEL_MODE)&&(state==0))
  {
    if ((botTempTreshhold<255)&&(checkIncPromt()==1))
    {
      botTempTreshhold++;
    }
  }
  if ((menuMode==TOP_LEVEL_MODE)&&(state==0))
  {
    if ((topTempTreshhold>10)&&(checkIncPromt()==1))
    {
      topTempTreshhold--;
    }
  }
}
void buttonCChanged(int state)
{
  //Serial.println("C Changed: " + String(state));
  if (menuMode!=TEMP_MODE)
  {
    EEPROM.write(0, botTempTreshhold);
    EEPROM.write(1, topTempTreshhold);
  }
  menuMode=TEMP_MODE;
}
void buttonRChanged(int state)
{
  //Serial.println("R Changed: " + String(state));
  if (((menuMode==TEMP_MODE)||(menuMode==TEMP_ALARM_MODE))&&(state==0))
  {
    menuMode=TOP_LEVEL_MODE;
  }
  if ((menuMode==BOT_LEVEL_MODE)&&(state==0))
  {
    if ((botTempTreshhold>10)&&(checkIncPromt()==1))
    {
      botTempTreshhold--;
    }
  }
  if ((menuMode==TOP_LEVEL_MODE)&&(state==0))
  {
    if ((topTempTreshhold<255)&&(checkIncPromt()==1))
    {
      topTempTreshhold++;
    }
  }
}

void setup() 
{
  byte numDigits = 3;
  byte digitPins[] = {12, 11, 10};
  byte segmentPins[] = {A2, A5, 8, 6, 5, A1, 9, 7};
  bool resistorsOnSegments = true; // 'false' means resistors are on digit pins
  byte hardwareConfig = NP_COMMON_CATHODE; // See README.md for options
  bool updateWithDelays = false; // Default 'false' is Recommended
  bool leadingZeros = false; // Use 'true' if you'd like to keep the leading zeros
  bool disableDecPoint = false; // Use 'true' if your decimal point doesn't exist or isn't connected

  
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
  updateWithDelays, leadingZeros, disableDecPoint);
  sevseg.setBrightness(90);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  Serial.begin(115200);
  Serial.println(__FILE__);

  sensor.begin();
  sensor.requestTemperatures();
  incTimer = millis();
  botTempTreshhold = EEPROM.read(0);
  topTempTreshhold = EEPROM.read(1);
  Serial.print("topTempTreshhold ");
  Serial.print(topTempTreshhold+TEMP_MIDDLE);
  Serial.print("   botTempTreshhold ");
  Serial.println(TEMP_MIDDLE-botTempTreshhold);
  /*
  buttonL.setCallback(buttonLChanged);
  buttonC.setCallback(buttonCChanged);
  buttonR.setCallback(buttonRChanged);
  */ 
}

void loop() 
{
  static unsigned long getTimer = millis();
  static unsigned long blinkTimer = millis();
  buttonL.update();
  buttonC.update();
  buttonR.update();
  if(buttonL.state() == LOW)
  {
    buttonLChanged(0);
  }
  
  if(buttonC.state() == LOW)
  {
    buttonCChanged(0);
  }
  
  if(buttonR.state() == LOW)
  {
    buttonRChanged(0);
  }
  
  switch(menuMode) 
  {
    case TEMP_MODE:
         sevseg.setNumber(temp, 1);
         digitalWrite(RELAY_PIN, LOW);
    break;
    
    case TEMP_ALARM_MODE:
        digitalWrite(RELAY_PIN, HIGH);
        if (millis() - blinkTimer >= BLNK_PERIOD) 
        {
          sevseg.setNumber(temp, 1);
        }
        else
        {
          sevseg.blank();
        }
        if (millis() - blinkTimer >= BLNK_PERIOD*2) 
        {
         blinkTimer += BLNK_PERIOD*2;
        }
        if (sensor.isConversionComplete()); 
        {
          /*Serial.print("temp: ");
          Serial.println(sensor.gettempC());*/
          temp=(int)sensor.getTempC()*10;
          //sevseg.setNumber(temp, 1);
          sensor.requestTemperatures();
        }
    break;
    
    case TOP_LEVEL_MODE:
        sevseg.setNumber(TEMP_MIDDLE+topTempTreshhold, 1);
    break;
    
    case BOT_LEVEL_MODE:
        sevseg.setNumber(TEMP_MIDDLE-botTempTreshhold, 1);
    break;
    
    default  :
    break;
  }
  if (millis() - getTimer >= GET_PERIOD) 
  {
  getTimer += GET_PERIOD;
    if (sensor.isConversionComplete()); 
    {
      /*Serial.print("temp: ");
      Serial.println(sensor.gettempC());*/
      temp=(int)sensor.getTempC()*10;
      if ((menuMode==TEMP_MODE)&&((temp>topTempTreshhold+TEMP_MIDDLE)||(temp<TEMP_MIDDLE-botTempTreshhold)))
      {
        menuMode=TEMP_ALARM_MODE;
        /*
        Serial.print("topTempTreshhold ");
        Serial.print(topTempTreshhold+TEMP_MIDDLE);
        Serial.print("   botTempTreshhold ");
        Serial.println(TEMP_MIDDLE-botTempTreshhold);
        */
      }
      //sevseg.setNumber(temp, 1);
      sensor.requestTemperatures();
    }
  }
  sevseg.refreshDisplay(); 
}
