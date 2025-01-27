#include <Arduino.h>
#include <arduino.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
#define PB 4
unsigned long previousTime = 0;
const long interval = 100;
unsigned long pulseTime = 0;
boolean isWatchRunning = true;
unsigned long timeBeforeStop = 0;
int buttonDebounceDelay = 0;


const int debounceDelay = 50;
int buttonState;
int previousButtonState = LOW;
unsigned long lastDebounceTime = 0;
//detect the button state, return 1 if the button is pressed
//reference: https://docs.arduino.cc/built-in-examples/digital/Debounce/
int buttonDetect(){
  int reading = digitalRead(PB);
  if (reading != previousButtonState) {
    //reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      //return if the button is pressed
      if (buttonState == HIGH) {
        return 1;
      }
    }
  }
  previousButtonState = reading;
  return 0;
}

// ######preLab########
void stopWatch(){
  unsigned long currentTime = millis() - pulseTime;//substract time passed
  //if the button is pressed, keep record how many time has been passed
  if(buttonDetect() == 1){
    //Serial.println(isWatchRunning);
    isWatchRunning = !isWatchRunning;//taggle the watch state
    if(isWatchRunning == false){
      timeBeforeStop = currentTime;
    }else{
      previousTime = currentTime;
    }
  }

  if(isWatchRunning == false){
    pulseTime = pulseTime + (currentTime - timeBeforeStop);
  }
  //if time passed 1ss, update the time display
  if(currentTime - previousTime>= interval && isWatchRunning == true){
    previousTime = currentTime;
    //calculate the times
    unsigned long totalSeconds = currentTime / 1000;
    unsigned int minutes = totalSeconds / 60;
    unsigned int seconds = totalSeconds % 60;
    unsigned int tenths = (currentTime % 1000) / 100;

    //first test with serial before print to oled
    // Serial.print(minutes);
    // Serial.print(":");
    // if (seconds < 10) {
    //   Serial.print("0");
    // }
    // Serial.print(seconds);
    // Serial.print(".");
    // Serial.println(tenths);
    //print with oled

    char timeString[10];
    sprintf(timeString, "%02u:%02u.%1u", minutes, seconds, tenths);
	  u8x8.drawString(0,0,timeString);
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
  stopWatch();
}