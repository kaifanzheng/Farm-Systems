#include <Arduino.h>
#include <arduino.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
#define PB 4
#define PM A0
#define LS A3
#define BZ 5
#define LED 6

//after testing, if light<3 it means the sensor is covered
// void testLightSensor(){
//   int raw_light = analogRead(LS);
//   int light = map(raw_light, 0, 1023, 0, 100);
//   Serial.println(light);
//   delay(1000);
// }

//helper for the task management
boolean elevatorDirection = false; //true means go up, false means go down
int currentFloor = 1; // record the current floor
int Tasks[5] = {-1, -1, -1, -1, -1};
//return -1 when there's no space: impossible. 
//return 0 when already exist, return 1 when sucessfully added
int addTask(int floorNum){
  //first make sure there's no overlap
  for(int i=0;i<5;i++){
    if(Tasks[i] == floorNum){
      return 0;
    }
  }
  for(int i=0;i<5;i++){
    if(Tasks[i] == -1){
      Tasks[i] = floorNum;
      return 1;
    }
  }
  return -1;
}
//return -1 if there's no next task find
int findNextTask(){
  if(elevatorDirection == true){
    for(int i=0;i<5;i++){
      if(Tasks[i] > currentFloor && Tasks[i] != -1){
        return Tasks[i];
      }
    }
  }else{
    for(int i=0;i<5;i++){
      if(Tasks[i] < currentFloor && Tasks[i] != -1){
        return Tasks[i];
      }
    }
  }
  return -1;
}
//remove a compelete task from task list
int removeTask(int floorNum){
  for(int i=0;i<5;i++){
    if(Tasks[i] == floorNum){
      Tasks[i] = -1;
      return 1;
    }
  }
  return -1;
}

int isInTasks(int floorNum){
  for(int i=0;i<5;i++){
    if(Tasks[i] == floorNum){
      return 1;
    }
  }
  return 0;
}
//##################impliment the logic of button panel###################
//bit map for control pannel display
uint8_t bitMapG[8] = {0b00111100, 0b01100110, 0b11000000, 0b11001110, 0b11000110, 0b01100110, 0b00111100, 0b00000000};
uint8_t bitMap2[8] = {0b00111100, 0b01100110, 0b00000110, 0b00001100, 0b00110000, 0b01100000, 0b01111110, 0b00000000};
uint8_t bitMap3[8] = {0b00111100, 0b01100110, 0b00000110, 0b00011100, 0b00000110, 0b01100110, 0b00111100, 0b00000000};
uint8_t bitMap4[8] = {0b00001100, 0b00011100, 0b00110100, 0b01100100, 0b01111110, 0b00000100, 0b00000100, 0b00000000};
uint8_t bitMap5[8] = {0b01111110, 0b01100000, 0b01111100, 0b00000110, 0b00000110, 0b01100110, 0b00111100, 0b00000000};

uint8_t bitMapG_h[8] = {0b11000011, 0b10011001, 0b00111111, 0b00110001, 0b00111001, 0b10011001, 0b11000011, 0b11111111};
uint8_t bitMap2_h[8] = {0b11000011, 0b10011001, 0b11111001, 0b11110011, 0b11001111, 0b10011111, 0b10000001, 0b11111111};
uint8_t bitMap3_h[8] = {0b11000011, 0b10011001, 0b11111001, 0b11100011, 0b11111001, 0b10011001, 0b11000011, 0b11111111};
uint8_t bitMap4_h[8] = {0b11110011, 0b11100011, 0b11001011, 0b10011011, 0b10000001, 0b11111011, 0b11111011, 0b11111111};
uint8_t bitMap5_h[8] = {0b10000001, 0b10011111, 0b10000011, 0b11111001, 0b11111001, 0b10011001, 0b11000011, 0b11111111};


uint8_t* characters[5] = {bitMapG, bitMap2, bitMap3, bitMap4, bitMap5};
uint8_t* characters_h[5] = {bitMapG_h, bitMap2_h, bitMap3_h, bitMap4_h, bitMap5_h};

int positions[5][2] = {
    {0, 4},//Position for 'G'
    {0, 3},//Position for '2'
    {0, 2},//Position for '3'
    {0, 1},//Position for '4'
    {0, 0} //Position for '5'
};
enum ButtonPanelState {
  IDLE,
  ButtonPressed
};

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

int currentNumberPosition = 0;
void ButtonPanel(){
  //display the button panel
  for(int i=0;i<5;i++){
    if(currentNumberPosition == i){
      u8x8.drawTile(positions[i][0],positions[i][1],1,characters_h[i]);
    }else if(isInTasks(i+1) == 0){
      u8x8.drawTile(positions[i][0],positions[i][1],1,characters[i]);
    }
  }
  for(int i=0;i<5;i++){
    if(Tasks[i]!= -1){
      u8x8.drawTile(positions[Tasks[i]-1][0],positions[Tasks[i]-1][1],1,characters_h[Tasks[i]-1]);
    }
  }
  int potValue = analogRead(PM);//Read the potentiometer value
  currentNumberPosition = map(potValue, 0, 1023, 0, 4);//Map to currentNumberPosition
  
  if(buttonDetect() == 1 && currentNumberPosition+1 != currentFloor){
    addTask(currentNumberPosition + 1);
  }
}

//controller instructions:
boolean doorClosed_C = true;
int targetFloor = 1;//instruction to the next floor
boolean door_sig = false;//if the door_sig is false, closing signal, true is opening sigal
boolean ring_sig = false;//if the ring_sig is true, ring the ring tone
boolean ring_once = false;//else ring the ring tone twice
boolean doorOpened = true;
unsigned long doorOpenTime = 0;
enum ControllerState {
  WAITING_C,
  MOVING_C,
  DOOR_OPENING_C,
  DOOR_CLOSING_C
};

ControllerState controllerState = WAITING_C;//initial state
void Controller() {
  int moreTasks;
  switch (controllerState ) {
    case WAITING_C:
      moreTasks = findNextTask();
      //Serial.println(targetFloor);
      if(moreTasks == -1){
        elevatorDirection = !elevatorDirection;//flip the direction of the elevator for more tasks
      }
      if (moreTasks != -1 && doorClosed_C == true) {
        targetFloor = moreTasks;
        Serial.println("waiting for tasks");
        controllerState  = MOVING_C;
        // Serial.print("found next floor: ");
        // Serial.println(targetFloor);
        // Serial.print("currentFloor: ");
        // Serial.println(currentFloor);
      }
      break;
    
    case MOVING_C:
      if (currentFloor == targetFloor) {
        doorClosed_C = false;
        controllerState = DOOR_OPENING_C;
        door_sig = true;
        ring_sig = true;
        ring_once = elevatorDirection;
        doorOpenTime = millis();//record the time when the door starts opening
        Serial.println("reached desired floor");
      }
      break;
    
    case DOOR_OPENING_C:
      if (millis() - doorOpenTime >= 8000) {//assume door takes 6 seconds to open
        door_sig = false;
        doorOpened = true;
        removeTask(currentFloor);
        currentFloor = targetFloor;
        controllerState  = DOOR_CLOSING_C;
      }
      break;
    
    case DOOR_CLOSING_C:
      //assuming the door closes immediately after opening for simplicity
      doorOpened = false;
      controllerState = WAITING_C;
      break;
  }
}

enum ChimeState {
  IDLE_R,
  RING_R
};
ChimeState chimeState = IDLE_R;
const int note1 = 440; // A4
const int note2 = 262; // C4
const int duration = 100;
unsigned long chimeTime = 0;
void Chime(){
  switch (chimeState)
  {
  case IDLE_R://do nothing in idle
    if(ring_sig == true){
      chimeState = RING_R;
      chimeTime = millis();
    }
    break;
  case RING_R:
    if(elevatorDirection == true){
      tone(BZ, note1, duration);
      chimeState = IDLE_R;//return to idle after ring once
      ring_sig = false;
      break;
    }else{
      tone(BZ, note2, duration);
      if(millis() - chimeTime >= 200){
        tone(BZ, note1, duration);
        chimeState = IDLE_R;//time the program, return to idle after
        ring_sig = false;
        break;
      }
    }
  }

}

unsigned long previousMillis_led = 0; 
bool ledState = false; 
void LEDBlink(unsigned int interval){
  //Check if it's time to blink the LED
  unsigned long currentMillis = millis();//get the current time
  if (currentMillis - previousMillis_led >= interval) {
    //save the last time we blinked the LED
    previousMillis_led = currentMillis;
    //if the LED is off, turn it on. If it's on, turn it off.
    ledState = !ledState;
    digitalWrite(LED, ledState);
  }
}

//######doorSystem#######
unsigned long openingTime = 0;
unsigned long closingTime = 0;
boolean openingDoor = true;
boolean closingDoor = true;
boolean doorClosed = true;

enum DoorState {
  DOOR_OPENING,
  DOOR_OPENED,
  DOOR_CLOSING,
  DOOR_CLOSED
};

DoorState doorState = DOOR_CLOSED;//initial state

void doorSystem() {
  int raw_light;
  int light;
  switch (doorState) {
    case DOOR_OPENING:
      u8x8.drawString(3, 0, "opening");
      LEDBlink(200);
      if (millis() - openingTime >= 3000) {
        u8x8.drawString(3, 0, "opened!");
        LEDBlink(200);
        doorState = DOOR_OPENED;
      }
      break;

    case DOOR_OPENED:
      digitalWrite(LED,HIGH);
      //wait for a signal to start closing
      if (door_sig == false) {
        doorState = DOOR_CLOSING;
        closingTime = millis();
      }
      break;

    case DOOR_CLOSING:
      doorClosed_C = false;
      u8x8.drawString(3, 0, "closing");
      LEDBlink(200);
      raw_light = analogRead(LS);
      light = map(raw_light, 0, 1023, 0, 100);
      u8x8.drawString(3, 1, "obstacle: ");
      if (light < 5) {
        u8x8.drawString(3, 2, "True!");
        closingTime = millis();//reset the closing time if there's an obstacle
      } else {
        u8x8.drawString(3, 2, "False");
        if (millis() - closingTime >= 3000) {
          u8x8.drawString(3, 0, "closed!");
          doorState = DOOR_CLOSED;
          
        }
      }
      break;

    case DOOR_CLOSED:
      doorClosed_C = true;
      digitalWrite(LED,LOW);
      //wait for a signal to start opening
      if (door_sig == true) {
        doorState = DOOR_OPENING;
        openingTime = millis();
      }
      break;
  }
}


enum ElevatorState {
  STOPPED,
  MOVING
};

ElevatorState elevatorState = STOPPED;
unsigned long moveStartTime;
unsigned long moveDuration;

void ElevatorCar(){
  //calculate the difference in floors
  int floorDifference = targetFloor - currentFloor;
  moveDuration = abs(floorDifference) * 2000;//duration in milliseconds

  switch(elevatorState) {
    case STOPPED:
      if(floorDifference != 0 && doorClosed_C == true) {
        moveStartTime = millis();
        elevatorState = MOVING;
        if(elevatorDirection == true) {
          u8x8.drawString(3,3,"MoveUp↑");
        } else {
          u8x8.drawString(3,3,"GoDown↓");
        }
      }
      break;
    case MOVING:
      if(millis() - moveStartTime >= moveDuration) {
        //movement finished
        currentFloor = targetFloor;
        u8x8.drawString(3,3,"Stopped");
        elevatorState = STOPPED;
      }
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PB, INPUT_PULLUP);
  pinMode(LED,OUTPUT);
  u8x8.begin();//to start using the screen
	u8x8.setFlipMode(1);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0);//defining the starting point for the cursor
  u8x8.clear();
}

void loop() {
  ButtonPanel();
  Controller();
  doorSystem();
  ElevatorCar();
  Chime();
  char floor[50];
  sprintf(floor, "currFloor: %d", currentFloor);
  u8x8.drawString(3,4,floor);
}