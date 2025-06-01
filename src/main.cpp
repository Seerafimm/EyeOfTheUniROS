#include <RtcDS1302.h>
#include <Keypad.h>
#include <GyverOLED.h>
#include <CheapStepper.h>
//git testing line v2

//RTC init
const int rstpin = 25;//rst(ce)
const int datpin = 32;//dat(io)
const int clkpin = 33;//clk(sclk)
ThreeWire wireone(datpin,clkpin,rstpin);//some three wire connection, not i2c.
RtcDS1302<ThreeWire> rtc(wireone);//object real time clock


//display init 
GyverOLED<SSD1306_128x64> oled;
const uint8_t bitmap_32x32[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xC0, 0xE0, 0xF0, 0x70, 0x70, 0x30, 0x30, 0x30, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF0, 0x70, 0x30, 0x30, 0x20, 0x00, 0x00,
  0x00, 0x30, 0x78, 0xFC, 0x7F, 0x3F, 0x0F, 0x0F, 0x1F, 0x3C, 0x78, 0xF0, 0xE0, 0xC0, 0x80, 0x80, 0x80, 0x40, 0xE0, 0xF0, 0xF8, 0xFC, 0xFF, 0x7F, 0x33, 0x13, 0x1E, 0x1C, 0x1C, 0x0E, 0x07, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF9, 0xF7, 0xEF, 0x5F, 0x3F, 0x7F, 0xFE, 0xFD, 0xFB, 0xF1, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x1E, 0x33, 0x33, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x1F, 0x0E, 0x04, 0x00, 0x00, 0x00, 0x00,
};//bitmap for display

//Stepper init
CheapStepper stepper(16,17,18,19);  //4 pins for stepper

//Keypad init
char keys[4][4] = {//keyscale scheme
  {'1','2','3','4'},
  {'5','6','7','8'},
  {'9','0','*','#'},
  {'a','b','c','d'}
};

byte rowpins[4] = {12,13,14,27};//pinout defining
byte colpins[4] = {21,22,23,26};//pinout defining

Keypad keypad44 = Keypad(makeKeymap(keys),rowpins,colpins,4,4);//object keypad with this options

//Variables init
char key = NO_KEY; //keypad key
int window = 0; //0 for logo, 1 for planet select, 2 for data display.//depricated
int selection = 0; //menu selection variable, 1-8 for 8 menu items
///////////////////////////////////////////////////////////
void drawStar(int cx, int cy, int l1, int l2, int l3) {
  // Длинные лучи
  oled.line(cx, cy, cx + l1, cy);
  oled.line(cx, cy, cx - l1, cy);
  oled.line(cx, cy, cx, cy + l1);
  oled.line(cx, cy, cx, cy - l1);

  // Средние лучи
  oled.line(cx, cy, cx + l2 * 0.707, cy + l2 * 0.707);
  oled.line(cx, cy, cx - l2 * 0.707, cy + l2 * 0.707);
  oled.line(cx, cy, cx + l2 * 0.707, cy - l2 * 0.707);
  oled.line(cx, cy, cx - l2 * 0.707, cy - l2 * 0.707);

  // Короткие лучи
  oled.line(cx, cy, cx + l3 * 0.924, cy + l3 * 0.383);
  oled.line(cx, cy, cx - l3 * 0.924, cy + l3 * 0.383);
  oled.line(cx, cy, cx + l3 * 0.924, cy - l3 * 0.383);
  oled.line(cx, cy, cx - l3 * 0.924, cy - l3 * 0.383);
}
void initlogooled(){
  oled.init();
  oled.clear();
  oled.setCursorXY(5,10); //0,0 cursor position
  oled.setScale(2);
  oled.autoPrintln(true);
  oled.print("EYE OF THE ");
  oled.setCursorXY(26,40);
  oled.print("UNIVERSE");
  drawStar(50, 30, 25, 15, 8);
  oled.update();
  oled.clear();
  oled.home();
  oled.setScale(2);
}
void showPlanetInfo(float azimuth, float elevation, const char* planetName) {
  oled.clear();
  oled.setScale(1);
  oled.setCursor(0,0);
  oled.printf("Планета: %s\n", planetName);
  oled.printf("Азимут: %.2f\n", azimuth);
  oled.printf("Высота: %.2f\n", elevation);
  oled.update();
}
void menucheck(char key){
  
  if (key == '1') {
    Serial.printf("Menu 1 selected\n");
    oled.clear();
    oled.home();
    oled.print("Menu 1 Selected");
    selection = 1;
    oled.update();
  } 
  else if (key == '2') {
    Serial.printf("Menu 2 selected\n");
    oled.clear();
    oled.home();
    oled.print("Menu 2 Selected");
    selection = 2;
    oled.update();
  } 
  else if (key == '3') {
    Serial.printf("Menu 3 selected\n");
    oled.clear();
    oled.home();
    oled.print("Menu 3 Selected");
    selection = 3;
    oled.update();
  } 
  else if (key == '4') {
    Serial.printf("Menu 4 selected\n");
    oled.clear();
    oled.home();
    oled.print("Menu 4 Selected");
    selection = 4;
    oled.update();
  }

  if (selection !=0){
    if (key == 'a') {
      oled.print("confirmed");
      oled.update();
    }
    if (key == 'b') {
      oled.clear();
      oled.drawBitmap(0, 0, bitmap_32x32, 32, 32);
      oled.update();
    }
    if (key == 'c'){
      stepper.newMoveToDegree(true,180);
    }
    if (key == 'd'){
      stepper.newMoveToDegree(true,90);
    }
  }
    
  
}


void setup() 
{
  Serial.begin(115200);
  delay(1000);

  //OLED start
  initlogooled();


  //RTC start
  rtc.Begin();
  RtcDateTime currentTime = RtcDateTime(24, 05, 10, 16, 59, 20);
  rtc.SetDateTime(currentTime); 
  //RTC end
  
  //Stepper start
  stepper.setRpm(12); //12 for 5v, 16 for 12v
  //Stepper end
  
  //Keypad start
  

  //Keypad end
}

/////////////////////////////////////////////////////////

void loop() 
{ 
  Serial.printf("%d", stepper.getStepsLeft());

  //stepper shreks
  if (stepper.getStepsLeft() != 0) {
  stepper.run(); //yes, its right what is in loop section.
  }
  else {
    digitalWrite(16, LOW); //turn off stepper power
    digitalWrite(17, LOW);
    digitalWrite(18, LOW);
    digitalWrite(19, LOW);
  }

  //RTC start
  RtcDateTime currentTime = rtc.GetDateTime();
  
  char buf[40];

  snprintf(buf, sizeof(buf), "%d/%d/%d %d:%d:%d\r\n",      
          currentTime.Year(), 
          currentTime.Month(), 
          currentTime.Day(),    
          currentTime.Hour(),  
          currentTime.Minute(),
          currentTime.Second() 
         );

 // Serial.printf(buf); // print time to serial
  memset(buf, 0, sizeof(buf)); // clear buffer completely
  
  //RTC end
  
  //Keypad start
  static unsigned long lastprint = 0;
  static char lastkey = NO_KEY;
  key = keypad44.getKey(); //input registration
  if (key != NO_KEY){lastkey = key;}
  menucheck(key); //oled menu selecting
  if(millis() - lastprint >= 1000){//serail logging
    lastprint = millis();

    if (lastkey == NO_KEY){
      // Serial.printf("key not registered\n");
    }
    else {
      Serial.printf("\nkey %c pressed\n", lastkey);
    }

    lastkey = NO_KEY;
  }
  
  //Keypad end

}
