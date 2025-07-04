#include <RtcDS1302.h>
#include <Keypad.h>
#include <GyverOLED.h>
#include <CheapStepper.h>
//microros includes
#include <Arduino.h>
#include <micro_ros_platformio.h>

#include <geometry_msgs/msg/vector3.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/string.h>

#include <std_msgs/msg/int32.h>


//ros defines 



geometry_msgs__msg__Vector3 recv_msg;
std_msgs__msg__String msg;

rcl_publisher_t publisher;
rcl_subscription_t subscriber;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}


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
String planetname = String("null");
RtcDateTime currentTime; //current time variable, used for publishing 
char planetlist[8][18] = {"солнце", "меркурий", "венера", "марс", "юпитер", "сатурн", "уран", "нептун"}; //array of planet names
float currentAzimuth = 0.0; // Initialize current azimuth
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
  oled.print("UNIros");
  drawStar(50, 30, 25, 15, 8);
  oled.update();
  oled.clear();
  oled.home();
  oled.setScale(2);
}
void showPlanetInfo(float azimuth, float altitude, float distance) {
  oled.clear();
  oled.setScale(1);
  oled.setCursor(0,0);
  oled.printf("Планета: %s\n", planetlist[selection - 1]);
  oled.printf("Азимут: %.2f\n", azimuth);
  oled.printf("Высота: %.2f\n", altitude);
  oled.printf("Расстояние: %.2f\n", distance);
  oled.update();
}
void pushtorasp(int selection) {  
  //Publish message
  static char msg_buffer[64];
  snprintf(msg_buffer, sizeof(msg_buffer), "%d|%04d-%02d-%02d %02d:%02d:%02d",
           selection,
           currentTime.Year(), currentTime.Month(), currentTime.Day(),
           currentTime.Hour(), currentTime.Minute(), currentTime.Second());

  msg.data.data = msg_buffer;
  msg.data.size = strlen(msg_buffer);
  msg.data.capacity = sizeof(msg_buffer);

  RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
  
}
void menucheck(char key){
  
  if (key == '1') {
    Serial.printf("Menu 1 selected\n");
    oled.clear();
    oled.home();
    oled.print("sun");
    selection = 1;
    planetname = "sun";
    oled.update();
  } 
  else if (key == '2') {
    Serial.printf("Menu 2 selected\n");
    oled.clear();
    oled.home();
    oled.print("mercury");
    selection = 2;
    planetname = "mercury";
    oled.update();
  } 
  else if (key == '3') {
    Serial.printf("Menu 3 selected\n");
    oled.clear();
    oled.home();
    oled.print("venus");
    selection = 3;
    planetname = "venus";
    oled.update();
  } 
  else if (key == '4') {
    Serial.printf("Menu 4 selected\n");
    oled.clear();
    oled.home();
    oled.print("mars");
    selection = 4;
    planetname = "mars";
    oled.update();
  }
  else if (key == '5') {
    Serial.printf("Menu 2 selected\n");
    oled.clear();
    oled.home();
    oled.print("jupiter");
    selection = 5;
    planetname = "jupiter";
    oled.update();
  } 
  else if (key == '6') {
    Serial.printf("Menu 2 selected\n");
    oled.clear();
    oled.home();
    oled.print("saturn");
    selection = 6;
    planetname = "saturn";
    oled.update();
  } 
  else if (key == '7') {
    Serial.printf("Menu 2 selected\n");
    oled.clear();
    oled.home();
    oled.print("uranus");
    selection = 7;
    planetname = "uranus";
    oled.update();
  } 
  else if (key == '8') {
    Serial.printf("Menu 2 selected\n");
    oled.clear();
    oled.home();
    oled.print("neptune");
    selection = 8;
    planetname = "neptune";
    oled.update();
  }
  else if (key == '#'){
    oled.print("изменить существующее время?");
    oled.update();
    selection = 2222;
  }

  if (selection !=0){
    if (key == 'a') {
      if (selection != 2222){
      oled.clear();
      oled.setCursor(0,0);
      oled.print("confirmed");
      oled.update();
      pushtorasp(selection); //send selection to ROS
      }
      else {
        RtcDateTime currentTime = RtcDateTime(25, 6, 9, 1, 45, 0);
        rtc.SetDateTime(currentTime); 
        oled.clear();
      }
    
    }

  
    if (key == 'b') {
      oled.clear();
      initlogooled();
    }
    if (key == 'd'){
      stepper.moveDegrees(true,10);
      currentAzimuth = 0.0; // Reset current azimuth to 0 after returning to home position
    }
  }
    
  
}
void rotatetoazim(float targetazim) {

  targetazim = fmod(targetazim, 360.0);
  if (targetazim < 0) targetazim += 360.0;

  float diff = targetazim - currentAzimuth; // Calculate the difference between target azimuth and current azimuth

  // Оптимизация кратчайшего поворота
  if (diff > 180.0) diff -= 360.0;
  if (diff < -180.0) diff += 360.0;
  
  if (diff > 0) {
    stepper.moveDegreesCW(abs(diff));
  } 
  else if (diff < 0) {
    stepper.moveDegreesCCW(abs(diff));
  }

  
  Serial.printf("Rotating to targetazim: %f",  diff);
  
  currentAzimuth = targetazim; // Update current azimuth after rotation
}
void error_loop() {
  while(1) {
    oled.clear();
    oled.setCursor(0,0);
    oled.printf("not connected\n");
    Serial.printf("not connected\n");
    oled.update();
    delay(1000);
  }
}
void subscription_callback(const void * msgin)
{
  const geometry_msgs__msg__Vector3 * msg = (const geometry_msgs__msg__Vector3 *)msgin;

  Serial.printf("Received: x=%.2f, y=%.2f, z=%.2f\n", msg->x, msg->y, msg->z);
  
  // Process the received message
  showPlanetInfo(msg->x, msg->y, msg->z);
  //function for motor rotation todo here
  rotatetoazim(msg->x); // Rotate to the azimuth specified in the message
}

void setup() 
{
  //OLED start
  initlogooled();
  

  Serial.begin(115200);
  set_microros_serial_transports(Serial);
  delay(2000);
  
  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "theeyeesp", "", &support));
  //ROS start
   //Publisher init
  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
    "planet_info"
  ));

  //Subscriber init
  RCCHECK(rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Vector3),
    "/planet_direction"
  ));


  
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator)); //1 second timer
  RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &recv_msg, &subscription_callback, ON_NEW_DATA));



  

  //RTC start
  rtc.Begin();
  
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
  currentTime = rtc.GetDateTime();
  
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
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)); //spin executor to process incoming messages
}

