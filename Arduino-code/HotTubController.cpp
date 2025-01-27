#include <Arduino.h>
#include <arduino.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
#define PB 4
#define PM A0

enum State {
  HOT,
  HEATING,
  COOLING,
  IDLE,
  SETTINGONE,
  SETTINGTWO
};
//####logic for push button detection####
unsigned long pressTime = 0;
unsigned long releaseTime = 0;
boolean isPressing = false;
int cutoffInterval = 2000;

//return 0 is nothing is detected, 1 for short, 2 for long
int buttonDetect(){
  int buttonState = digitalRead(PB);
  //if button being pressed
  if(buttonState == HIGH && isPressing == false){
    isPressing = true;
    pressTime = millis();
  }

  if(buttonState == LOW && isPressing == true){
    isPressing = false;
    releaseTime = millis();
    if(releaseTime - pressTime >= 2000){
      return 2;
    }else{
      return 1;
    }
  }
  return 0;
}
//behavioral model for temperature change
int currentTemp = 25;
unsigned long previousTime_T;//initilize whenever using this model
void temp_model(boolean isHeating){
  unsigned long currentTime = millis();
  if(currentTime - previousTime_T >= 5000){
    previousTime_T = currentTime;
    if(isHeating){
      currentTemp = currentTemp + 1;
    }else{
      currentTemp = currentTemp - 1;
    }
  }
}

unsigned long previousTime = 0;
State currentState = IDLE;
State previousState = IDLE;
const int ambientTemp = 25;
boolean jet = false;
int tempSet = 0;

//method that display for each state, according to the current state
void displayIDLE(){
  u8x8.drawString(0,0,"IDLE");
  char tempIDLE[50];
  sprintf(tempIDLE, "Temp: %d *C", ambientTemp);
  u8x8.drawString(0,1,tempIDLE);
  u8x8.drawString(0,2,"JET: OFF");//assume jet is always off during the IDLE state
}
void displayHot(){
  u8x8.drawString(0,0,"HOT");
  char tempHOT[50];
  sprintf(tempHOT, "Temp: %d *C", currentTemp);
  u8x8.drawString(0,1,tempHOT);
  if(jet == true){
    u8x8.drawString(0,2,"JET: ON!");
  }else{
    u8x8.drawString(0,2,"JET: OFF");
  }
}
void displayHeating(){
  u8x8.drawString(0,0,"HEATING");
  char tempHEAT[50];
  sprintf(tempHEAT, "Temp: %d *C", currentTemp);
  u8x8.drawString(0,1,tempHEAT);
  char timeHEAT[50];
  sprintf(timeHEAT, "Time: %d S", (tempSet-currentTemp)*5);
  u8x8.drawString(0,2,timeHEAT);
}
void displayCooling(){
  u8x8.drawString(0,0,"COOLING");
  char tempCOOL[50];
  sprintf(tempCOOL, "Temp: %d *C", currentTemp);
  u8x8.drawString(0,1,tempCOOL);
  char timeCOOL[50];
  sprintf(timeCOOL, "Time: %d S", (currentTemp-ambientTemp)*5);
  u8x8.drawString(0,2,timeCOOL);
}
void displaySettingOne(){
  u8x8.drawString(0,0,"SETTING1");
  u8x8.drawString(0,3,"short press");
}

//return 1 if the tempSetting is changing
int displaySettingTwo(){
  u8x8.drawString(0,0,"SETTING2");
  u8x8.drawString(0,1,"Select Temp:");
  int pmValue = analogRead(PM);
  int tempSetting = map(pmValue, 0, 1023, 25, 40);
  char tempString[2];
  sprintf(tempString, "%d", tempSetting);
  u8x8.drawString(0,2,tempString);
  u8x8.drawString(0,3,"short press");
  if(tempSet != tempSetting){
    tempSet = tempSetting;
    return 1;//setting two need to detect potential meter and it's change. if the potential meter don't change, timeout
  }
  return 0;
}

//function that determine the output while in that state.
void IDLEOutput(){
  jet = false;
  displayIDLE();
}
int settingTwoOutput(){
  return displaySettingTwo();
}
void settingOneOutput(){
  displaySettingOne();
}
void HeatingOutput(){
  temp_model(true);
  displayHeating();
}
void HotOutput(){
  displayHot();
}
void CoolingOutput(){
  temp_model(false);
  displayCooling();
}

//state_machine method
void state_machine(){
  unsigned long currentTime = millis();
  int pbHot = 0;
  switch(currentState){
  case IDLE:
    //output of IDLE
    IDLEOutput();
    //guard to change the state
    if(buttonDetect() == 2){
      currentState = SETTINGTWO;
      previousState = IDLE;
      previousTime = millis();
      u8x8.clear();
    }
    break;
  case HOT:
    HotOutput();
    pbHot = buttonDetect();
    if(pbHot  == 1){
      jet = !jet;
    }
    //guard to change the state
    if(pbHot == 2){
      currentState = SETTINGONE;
      previousState = HOT;
      previousTime = millis();
      u8x8.clear();
    }
    break;
  case COOLING:
    CoolingOutput();
    //guard to change the state
    if(ambientTemp-currentTemp>=0){
      currentState = IDLE;
      previousState = COOLING;
      u8x8.clear();
    }
    //guard to change the state
    if(buttonDetect() == 2){
      currentState = SETTINGTWO;
      previousState = COOLING;
      previousTime = millis();
      u8x8.clear();
    }
    break;
  case HEATING:
    HeatingOutput();
    //guard to change the state
    if(tempSet-currentTemp<=0){
      currentState = HOT;
      previousState = HEATING;
      u8x8.clear();
    }
    //guard to change the state
    if(buttonDetect() == 2){
      currentState = SETTINGONE;
      previousState = HEATING;
      previousTime = millis();
      u8x8.clear();
    }
    break;
  case SETTINGONE:
    settingOneOutput();
    if(buttonDetect() == 1){
      currentState = COOLING;
      previousState = SETTINGONE;
      previousTime_T = millis();
      u8x8.clear();
    }
    //guard to change the state
    if(currentTime - previousTime >= 5000){
      currentState = previousState;
      previousState = SETTINGONE;
      u8x8.clear();
    }
    break;
  case SETTINGTWO:
    if(settingTwoOutput() == 1){
      previousTime = currentTime;//if potential meter value change, update the previous time to stop timeout
    }
    //guard to change the state
    if(buttonDetect() == 1){
      currentState = HEATING;
      previousState = SETTINGTWO;
      previousTime_T = millis();
      u8x8.clear();
    }
    //guard to change the state
    if(currentTime - previousTime >= 5000){
      currentState = previousState;
      previousState = SETTINGTWO;
      u8x8.clear();
    }
    break;
  }

}

void setup() {
  Serial.begin(9600);
  pinMode(PB, INPUT_PULLUP);
  u8x8.begin(); // to start using the screen
	u8x8.setFlipMode(1);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0); //defining the starting point for the cursor
}

void loop() {
  state_machine();

}