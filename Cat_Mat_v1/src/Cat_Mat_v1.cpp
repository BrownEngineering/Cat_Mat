/* 
 * Project Cat_Mat_v1
 * Author: Benjamin Brown
 * Date: 08052025
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

#include "Particle.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"
#include "credentials.h"
#include "Adafruit_VL53L0X.h"
#include "IoTTimer.h"
#include "Stepper.h"
#include "Colors.h"
#include <neopixel.h>

/************ Global State (you don't need to change this!) ***   ***************/ 
TCPClient TheClient; 

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details. 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

/****************************** Feeds ***************************************/ 
Adafruit_MQTT_Subscribe Cat_Game = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Cat_Game_Picker"); 
Adafruit_MQTT_Publish Cat_Is_Here = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Cat_Present!");

int startPix = 0;
int endPix;
const int PIXELCOUNT = 240;

int width = 12;
int height = 20;
int PIX_GRID = width * height;
int x = 0;
int y = 0;
int next;
//bool evenRow = true;

int pixel_Array[12][20] = {
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 },
  { 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20 },
  { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 },
  { 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60 },
  { 250, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99 },
  {250,118,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100 },
  {120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139 },
  {159,158,157,156,155,154,153,152,151,150,149,148,147,146,145,144,143,142,141,140 },
  {160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179 },
  {199,198,197,196,195,194,193,192,191,190,189,188,187,186,185,184,183,182,181,250 },
  {200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219 },
  {239,238,237,236,235,234,233,232,231,230,229,228,227,226,225,224,223,222,221,220 }
};
int start_x;
int start_y;


Adafruit_NeoPixel pixel(PIXELCOUNT,SPI,WS2812B);

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
VL53L0X_RangingMeasurementData_t measure;
int distance;


int feather_Movement;
int rand_Wiggle;

bool cat_Presence = false;
bool cat_Has_Left_The_Chat = false;
bool kitty_Dont_Play_Flag = false;
bool wiggle_Lock_Out = false;
unsigned int kitty_Time;
int cat_Game_Select;
int cat;
bool cat_Detected = false;
bool what_Cat = true;
bool game_Lock= false;
unsigned int catUpdateTime;

const int IN1 = D7;
const int IN3 = D5;
const int IN2 = D6;
const int IN4 = D4;
const int one_Hour = (((1000*10)*6)*60);
const int twelve_Hour = ((((1000*10)*6)*60)*12);
Stepper myStepper(2048,IN1,IN3,IN2,IN4);

IoTTimer wiggle_Timer, light_Timer;

SYSTEM_MODE(SEMI_AUTOMATIC);

void pixelFill (int stpix, int enpix, int c);
void sleepULP();
void MQTT_connect();
bool MQTT_ping();
void whack_A_Dot();
void laser_Chase_Vertical();
void laser_Chase(int startX,int startY);



void setup() {

  Serial.begin(9600);
  // Serial1.begin(9600);
  // waitFor(Serial.isConnected,10000);

  WiFi.on();
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf("^.^");
  }
  Serial.printf("\n\n");

  pixel.begin();
  pixel.setBrightness(30);
  pixel.show();

  Wire.begin();

  if (!lox.begin()) {
    Serial.printf("Failed to find VL53L0X");
    while (1) delay(10);
  }

  myStepper.setSpeed(15);

  wiggle_Timer.startTimer(100);
  light_Timer.startTimer(100);

  mqtt.subscribe(&Cat_Game);
}

void loop() {
  MQTT_connect();
  MQTT_ping();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &Cat_Game) {
    cat_Game_Select = atoi ((char*)Cat_Game.lastread);
    Serial.printf("Cat game is %i",cat_Game_Select);
    }
  }


////////This is code used for debugging and ranging////// Comment in if testing required. 
  // Serial.print("Reading a measurement...\n ");
  // lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!
  // if (measure.RangeStatus != 4) {  // phase failures have incorrect data
  //   Serial.printf("Distance (mm): %i\n",(measure.RangeMilliMeter)); //Serial.printf(measure.RangeMilliMeter);
  // } else {
  //   Serial.printf(" out of range\n ");
  // }

  lox.rangingTest(&measure,true);

  distance = measure.RangeMilliMeter;
  cat_Has_Left_The_Chat = (distance>500||(measure.RangeStatus == 4));
  Serial.printf("distance = %i\n",distance);

  if(distance >=200){
    cat_Detected=true;
  }
    if(cat_Has_Left_The_Chat){
      if(!kitty_Dont_Play_Flag){
        kitty_Dont_Play_Flag = true;
        kitty_Time = millis();
      }else{
        if(millis()-kitty_Time>=10000){
          cat_Detected=false;
          wiggle_Lock_Out=false;
          kitty_Dont_Play_Flag=false;
        }
      }
    }else{
      kitty_Dont_Play_Flag = false;
      cat_Detected = false;
  }
    if(!cat_Detected && (millis()-kitty_Time>60000)){
      sleepULP();
    }


    cat = cat_Detected;
 if(cat_Detected && (millis()-catUpdateTime>10000)){
    mqtt.Update();
    Cat_Is_Here.publish(cat);
    catUpdateTime = millis();
    Serial.printf("\ncat detected bool = %i\n",cat);

  }else if(!cat_Detected && (millis()-catUpdateTime>10000)){
    mqtt.Update();
    Cat_Is_Here.publish(cat);
    catUpdateTime = millis();
    Serial.printf("\ncat detected bool = %i\n",cat);
  }


  if(distance>300 && distance<500){
    if(!wiggle_Lock_Out){
      rand_Wiggle = random(-2048,2048);
      myStepper.step(rand_Wiggle);
      delay(500);
      myStepper.step(-rand_Wiggle);
      delay(500);
      myStepper.step(rand_Wiggle/2);
      delay(500);
      wiggle_Lock_Out = true;
    }
  }
  if(distance>150 && distance<405){
    if(!game_Lock){
      cat_Game_Select =random(0,3);
      Serial.printf("cat_Game_# = %i\n",cat_Game_Select);
      game_Lock = true;
    }

    switch (cat_Game_Select){
    ///laser_Chase logic
      case 0:      
        start_x = random(0,12);
        start_y = random(0,20);
        laser_Chase(start_x,start_y);

      break;
    
    ///whack_A_Dot

      case 1:
      
        whack_A_Dot();
      break;
    ///laser_Chase_Verticle
    
      case 2:
      
        laser_Chase_Vertical();
      
      break;
    ///mouse_Run
      // case 3:

      // break;

      default:

      pixel.clear();
      pixel.show();
      break;
    }
  }else{
    game_Lock = false;
  }
}




void sleepULP(){
  int feather_Move_Wake_Timer = random (((1000*60)*30),(((1000*60)*60)*2));//wake up randomly -- 30min to 2 hours intervals
  SystemSleepConfiguration config;
  config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(feather_Move_Wake_Timer);
  SystemSleepResult result = System.sleep(config);
  delay(1000);

    if (result.wakeupReason()==SystemSleepWakeupReason::BY_RTC){
      feather_Movement = random (200,2000);
      myStepper.step(feather_Movement);
      delay(100);
      myStepper.step(feather_Movement);
      delay(200);
      myStepper.step(feather_Movement);
      delay(100);
      Serial.printf("wiggle the feather!\n");
    }

}
/////.gpio(DO,RISING)

 void pixelFill (int stpix, int enpix, int c){
  int j;
  for (j=stpix; j<=enpix; j++){
    pixel.setPixelColor(j, c);
  }
   pixel.show();
}

void MQTT_connect() {
  int8_t ret;
 
  // Return if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
 
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("Error Code %s\n",mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds...\n");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds and try again
  }
  Serial.printf("MQTT Connected!\n");
}

bool MQTT_ping() {
  static unsigned int last;
  bool pingStatus;

  if ((millis()-last)>120000) {
      Serial.printf("Pinging MQTT \n");
      pingStatus = mqtt.ping();
      if(!pingStatus) {
        Serial.printf("Disconnecting \n");
        mqtt.disconnect();
      }
      last = millis();
  }
  return pingStatus;
}

void laser_Chase(int startX,int startY){

int x = startX;
int y = startY;
int pix;
int pix1;
int pix2;

int direction =random(8);//0=up, 1=up/right, 2=right, 3 =down/right, ...., 7=up/left
int distance = random(8,19);

pix=pixel_Array[x][y];
pix1 = pix2 = pix;

while(distance>0){ 

  pixel.clear();
  pixel.setPixelColor(pix,255,0,0);
  pixel.setPixelColor(pix1,60,0,0);
  pixel.setPixelColor(pix2,10,0,0);
  pixel.show();
  delay(50);

  pix2=pix1;
  pix1=pix;

switch (direction) {
  case 0: y--; break;      // up
  case 1: y--; x++; break; // up-right
  case 2: x++; break;      // right
  case 3: y++; x++; break; // down-right
  case 4: y++; break;      // down
  case 5: y++; x--; break; // down-left
  case 6: x--; break;      // left
  case 7: y--; x--; break; // up-left
}
// Top edge
if (y == 0 && direction == 7){
   direction = 3;
}
if (y == 0 && direction == 3){
   direction = 1;
}
if (y == 0 && direction == 1){
   direction = 5;
}
// Bottom edge
if (y == 11 && direction == 1){
   direction = 5;
}
if (y == 11 && direction == 5){
   direction = 7;
}
if (y == 11 && direction == 3){
   direction = 7;
}
// Left edge
if (x == 0 && direction == 5){
   direction = 1;
}
if (x == 0 && direction == 7){
   direction = 1;
}
if (x == 0 && direction == 6){
   direction = 2;
}
// Right edge
if (x == 19 && direction == 1){
   direction = 7;
}
if (x == 19 && direction == 3){
   direction = 7;
}
if (x == 19 && direction == 2){
   direction = 6;
}
pix = pixel_Array[x][y];
if(pix<0){
  pix=0;
}
if(pix>=240){
  pix=239;
}

distance --;

}
Serial.printf("x=%i, y=%i, pix=%i, direction=%i\n", x, y, pix, direction);

}

void laser_Chase_Vertical() {
  int width = 12;
  int height = 20;
  int GRID_SIZE = width * height;

  static bool move_Positive = true;
  int r_Pix = random(0, GRID_SIZE);

  if (move_Positive){
    for (int i= r_Pix;i <GRID_SIZE;i += 2){ 
      pixel.clear();
      pixel.setPixelColor(i, green);
      pixel.show();
      delay(15);
    }
  } else {
    for (int i= r_Pix;i >=0;i -= 2){
      pixel.clear();
      pixel.setPixelColor(i, green);
      pixel.show();
      delay(15);
    }
  }

  move_Positive = !move_Positive; 
}

void whack_A_Dot(){
  pixel.clear();

  int rand_Pixel = random(3,237);

    pixel.setPixelColor(rand_Pixel, purple);
    // pixel.setPixelColor(rand_Pixel+1, red);
    // pixel.setPixelColor(rand_Pixel-1, red);

    pixel.show();   
  }
