/*
ESP Remote
By: Dominic Pompilio







*/




#include <Arduino.h>
//LCD and UI
#include "DFRobot_UI.h"
#include "DFRobot_GDL.h"
#include "DFRobot_Picdecoder_SD.h"
#include "DFRobot_Touch.h"
//
#include <credentials.h>
#include "WiFi.h"
#include "time.h"
//SD Card
#include "SD.h"
#include "SPI.h"
//Buttons
#include "Adafruit_NeoTrellis.h"
//IR
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>
//Accelerometer
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>



#define LCD_DC    D2
#define LCD_CS    D6
#define LCD_SD    D7
#define LCD_RST   D3
#define LCD_BL    D13
#define TOUCH_RST D7
#define TOUCH_INT D11 //d11
#define buttonPin GPIO_NUM_4 //A0
#define BATT_VOLT A2
#define IR_LED GPIO_NUM_43 //TX
#define IR_REC D14

Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);

/** Global variable to determine which interrupt(s) are enabled on the ADXL343. */
int_config g_int_config_enabled = { 0 };

/** Global variables to determine which INT pin interrupt(s) are mapped to on the ADXL343. */
int_config g_int_config_map = { 0 };


DFRobot_Picdecoder_SD decoder;

const char* ssid = WIFISSID;
const char* password = WIFIPSWD;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000; // Eastern Time (UTC-5)
const int daylightOffset_sec = 3600; // DST offset

Adafruit_NeoTrellis trellis;

IRsend irsend(IR_LED);

uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}




DFRobot_Touch_GT911_IPS touch(0X5D,TOUCH_RST,TOUCH_INT);

/**
   @brief Constructor When the screen uses hardware SPI communication, the driver IC is ST7365P, and the screen resolution is 320x480, this constructor can be called
   @param dc Command/data line pin for SPI communication
   @param cs Chip select pin for SPI communication
   @param rst Reset pin of the screen
*/
DFRobot_ST7365P_320x480_HW_SPI screen(/*dc=*/LCD_DC,/*cs=*/LCD_CS,/*rst=*/LCD_RST,/*bl=*/LCD_BL);

/**
   @brief Constructor
   @param gdl Screen object
   @param touch Touch object
*/
DFRobot_UI ui(&screen, &touch);
void screenDrawPixel(int16_t x, int16_t y, uint16_t color)
{
  //Draw a point on the screen
  screen.writePixel(x,y,color);
}

void updateWifiSig() {
  int rssi = WiFi.RSSI();
  if (rssi > -58) { //strong
    decoder.drawPicture(/*filename=*/"/images/wifi_H.bmp",/*sx=*/5,/*sy=*/10,/*ex=*/70,/*ey=*/28,/*screenDrawPixel=*/screenDrawPixel);
    }
  else if (rssi > -68) {  //medium
    decoder.drawPicture(/*filename=*/"/images/wifi_M.bmp",/*sx=*/5,/*sy=*/10,/*ex=*/70,/*ey=*/28,/*screenDrawPixel=*/screenDrawPixel);
    }
  else {  //low
    decoder.drawPicture(/*filename=*/"/images/wifi_L.bmp",/*sx=*/5,/*sy=*/10,/*ex=*/70,/*ey=*/28,/*screenDrawPixel=*/screenDrawPixel);
    }
  }

void updateBattPercent() {
  int battValue = analogRead(BATT_VOLT);
  Serial.println(battValue);
  int f = battValue - 2512;
  float h = (f / 1000.0);
  int battPercent = h*100;
  Serial.println(battPercent);
  if (battPercent > 75) { //76-100%
    decoder.drawPicture(/*filename=*/"/images/battery100percent.bmp",/*sx=*/10,/*sy=*/205,/*ex=*/104,/*ey=*/50,/*screenDrawPixel=*/screenDrawPixel);
    }
  else if (battPercent > 50) {  //51-75%
    decoder.drawPicture(/*filename=*/"/images/battery75percent.bmp",/*sx=*/200,/*sy=*/5,/*ex=*/52,/*ey=*/25,/*screenDrawPixel=*/screenDrawPixel);
    }
  else if (battPercent > 25) {  //26-50%
    decoder.drawPicture(/*filename=*/"/images/battery50percent.bmp",/*sx=*/200,/*sy=*/5,/*ex=*/52,/*ey=*/25,/*screenDrawPixel=*/screenDrawPixel);
    }
  else {  //25-0%
    decoder.drawPicture(/*filename=*/"/images/battery25percent.bmp",/*sx=*/200,/*sy=*/5,/*ex=*/52,/*ey=*/25,/*screenDrawPixel=*/screenDrawPixel);
    }
}

TrellisCallback blink(keyEvent evt){
  // Check is the pad pressed?
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255))); //on rising
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
  // or is the pad released?
    trellis.pixels.setPixelColor(evt.bit.NUM, 0); //off falling
  }

  // Turn on/off the neopixels!
  trellis.pixels.show();

  return 0;
}



void printLocalDateTime() {
  struct tm timeInfo; // store the current time
  if (!getLocalTime(&timeInfo)) { // get the current time
    Serial.println("Failed to obtain time");
  }

char timeStringBuff[64];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%I:%M %p", &timeInfo);
  ui.drawString(/*x=*/screen.width()/3,/*y=*/10,timeStringBuff,COLOR_RGB565_WHITE,ui.bgColor,/*fontsize =*/2,/*Invert=*/0);
}


//buttons

void slpBtnCallback(DFRobot_UI::sButton_t &btn,DFRobot_UI::sTextBox_t &obj) {
  esp_deep_sleep_start();
}
DFRobot_UI::sButton_t & menuBackBtn = ui.creatButton();
DFRobot_UI::sSlider_t &screenBrightness = ui.creatSlider();


void changeBrightness(DFRobot_UI::sSlider_t &slider, DFRobot_UI::sTextBox_t &textBox) {
  int i(slider.value);
  Serial.println(i);
  analogWrite(13, i);
  
}

void menuBtnCallback(DFRobot_UI::sButton_t &btn,DFRobot_UI::sTextBox_t &obj) {
  //Set the name of the button
  ui.clear();
  //menuBackBtn.setCallback(menuBtnCallback);
  //Each button has a text box, its parameter needs to be set by yourself.
  ui.draw(&menuBackBtn,/**x=*/5,/**y=*/60,/*width*/screen.width()/4,/*height*/screen.width()/1);
  //Initialize the slider control, initialize and assign the parameters of the slider
  
  
  //  slider.bgColor = COLOR_RGB565_GREEN;
  //Set the output text box of the slider
  //Draw a slider at a specified position
  ui.draw(&screenBrightness,/*x = */(screen.width() - screenBrightness.width) / 2,/*y = */100);

  
  //ui.refresh();
}

void menuBackBtnCallback(DFRobot_UI::sButton_t &btn,DFRobot_UI::sTextBox_t &obj) {
  esp_deep_sleep_start();
}



void int1_isr(void)
{
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, GPIO_INTR_HIGH_LEVEL); //A0 High

    /* By default, this sketch routes the OVERRUN interrupt to INT1. */
    /*Serial.println("_______");
    Serial.println("INT");
    int ADXL = analogRead(buttonPin);
    Serial.println(ADXL);
    Serial.println("_______");*/

    /* TODO: Toggle an LED! */
}
/** Configures the HW interrupts on the ADXL343 and the target MCU. */
void config_interrupts(void)
{
  /* NOTE: Once an interrupt fires on the ADXL you can read a register
   *  to know the source of the interrupt, but since this would likely
   *  happen in the 'interrupt context' performing an I2C read is a bad
   *  idea since it will block the device from handling other interrupts
   *  in a timely manner.
   *
   *  The best approach is to try to make use of only two interrupts on
   *  two different interrupt pins, so that when an interrupt fires, based
   *  on the 'isr' function that is called, you already know the int source.
   */

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1); //A0 High
  /* Attach interrupt inputs on the MCU. */
  pinMode(GPIO_NUM_4, INPUT);
  //attachInterrupt(digitalPinToInterrupt(buttonPin), int1_isr, RISING);

  /* Enable interrupts on the accelerometer. */

  //g_int_config_enabled.bits.inactivity = false;
  g_int_config_enabled.bits.activity = true;


  /* Map specific interrupts to one of the two INT pins. */

 // g_int_config_map.bits.inactivity = ADXL343_INT1;
  g_int_config_map.bits.activity = ADXL343_INT1;
  /* Initialise the sensor */

  if(!accel.mapInterrupts(g_int_config_map))
  {
    /* There was a problem detecting the ADXL343 ... check your connections */
    Serial.println("Ooops, no map detected ... Check your wiring!");
    while(1);
  }

    if(!accel.enableInterrupts(g_int_config_enabled))
  {
    /* There was a problem detecting the ADXL343 ... check your connections */
    Serial.println("Ooops, no enable detected ... Check your wiring!");
    while(1);
  }
  
  
}

void setup()
{
  Serial.begin(9600);
  Serial.print("--Serial Comms Up--");
  irsend.begin();
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_SD, OUTPUT);
  analogWrite(D13, 120); //screen brightness
  //gpio_wakeup_enable(buttonPin, GPIO_INTR_HIGH_LEVEL);
  



  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL343 ... check your connections */
    Serial.println("Ooops, no ADXL343 detected ... Check your wiring!");
    while(1);
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL343_RANGE_2_G);

  /* Configure the HW interrupts. */
  config_interrupts();




  //Initialize UI
  ui.begin();
  SD.begin(LCD_SD);
  ui.setTheme(DFRobot_UI::MODERN);
  ui.setBgColor(0x4bb2);


  menuBackBtn.setText("Back");
  menuBackBtn.bgColor = COLOR_RGB565_DGRAY;
  menuBackBtn.setCallback(menuBackBtnCallback);
  screenBrightness.setCallback(changeBrightness);



  //decoder.drawPicture(/*filename=*/"/images/battery100percent.bmp",/*sx=*/0,/*sy=*/0,/*ex=*/130,/*ey=*/63,/*screenDrawPixel=*/screenDrawPixel);
/*
    //battery level
  DFRobot_UI::sBar_t &bar1 = ui.creatBar();
  /** User-defined progress bar parameters */
 // bar1.setStyle(DFRobot_UI::BAR);
  //bar1.fgColor = COLOR_RGB565_GREEN;
 // bar1.width = 75;
 // bar1.height = 5;
  //bar1.setCallback(barCallback1);
  //ui.draw(&bar1,/*x=*/220,/*y=*/10);

  //Sleep button
  DFRobot_UI::sButton_t & slpBtn = ui.creatButton();
  slpBtn.setText("Sleep");
  slpBtn.bgColor = COLOR_RGB565_SKYBLUE;
  slpBtn.setCallback(slpBtnCallback);
  ui.draw(&slpBtn,/**x=*/235,/**y=*/60,/*width*/screen.width()/4,/*height*/screen.width()/10);

  //Menu button
  DFRobot_UI::sButton_t & menuBtn = ui.creatButton();
  //Set the name of the button
  menuBtn.setText("Menu");
  //menuBtn.draw()
  menuBtn.bgColor = COLOR_RGB565_DGRAY;
  menuBtn.setCallback(menuBtnCallback);
  ui.draw(&menuBtn,/**x=*/5,/**y=*/60,/*width*/screen.width()/4,/*height*/screen.width()/10);





  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) { // indicate that the ESP32 is trying to connect
    decoder.drawPicture(/*filename=*/"/images/wifi_N.bmp",/*sx=*/5,/*sy=*/10,/*ex=*/70,/*ey=*/28,/*screenDrawPixel=*/screenDrawPixel);
    //ui.drawString(/*x=*/screen.width()/3,/*y=*/10,"Connect",COLOR_RGB565_WHITE,ui.bgColor,/*fontsize =*/2,/*Invert=*/0);
    printLocalDateTime();
    ui.refresh();
  }
  Serial.println("Connected!");
  Serial.println(WiFi.RSSI());
  updateWifiSig();
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // configure time settings
  printLocalDateTime();



  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while(1) delay(1);
  } else {
    Serial.println("NeoPixel Trellis started");
  }

 

  //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, Wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
    trellis.pixels.setBrightness(20);
    trellis.pixels.show();
    delay(50);
  }
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }


  
}


void loop()
{
    //screen.setFont(&FreeSans12pt7b);
  updateBattPercent();
  updateWifiSig();
  //refresh
  ui.refresh();
  //decoder.drawPicture(/*filename=*/"/images/wifi_M.bmp",/*sx=*/0,/*sy=*/0,/*ex=*/100,/*ey=*/30,/*screenDrawPixel=*/screenDrawPixel);
  printLocalDateTime();
  trellis.read();

  //troubleshooting gyro

  sensors_event_t event;
  accel.getEvent(&event);
  accel.getEvent(&event);
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
  Serial.println("");
}

