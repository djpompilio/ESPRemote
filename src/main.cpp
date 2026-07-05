/*

      --ESP Remote--
    By: Dominic Pompilio
          2026







*/

/* 

NOTES & TODO

  Hardware
    - Add light sensor
    - Redesign bottom case to have reset hole and guide up to esp's reset button
    - Clear cover for ir leds

  Software
    - LCD Driver
    - Axcelerometer wake up
    - Auto screen brightness

*/


#include <Arduino.h>
//LCD and UI
#include "DFRobot_UI.h"
#include "DFRobot_GDL.h"
#include "DFRobot_Picdecoder_SD.h"
#include "DFRobot_Touch.h"
#include "TFT_eSPI.h"
#include "lvgl.h"
#include <../ui/ui.h>
//Das WIIFII
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

//ULP
//#include "esp32/ulp.h"
//#include "ulp_main.h"
//#include "ulptool.h"
#include "esp32/ulp.h"
// include ulp header you will create
#include "ulp_main.h"
// must include ulptool helper functions also
#include "ulptool.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");


//not working
//#include "../ui/lv_conf.h"



#define LCD_DC    D2
#define LCD_CS    D6
#define LCD_SD    D7
#define LCD_RST   D3
#define LCD_BL    D13
#define TOUCH_RST D7
#define TOUCH_INT D11 //d11
#define buttonPin GPIO_NUM_4 //A0
#define BATT_VOLT A2
#define IR_LED GPIO_NUM_11 //A5
#define IR_REC D14

//not working
#define LV_CONF_INCLUDE_SIMPLE

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
/** 
//LVGL

static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 320;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

TFT_eSPI lcd = TFT_eSPI(screenWidth, screenHeight);

void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  lcd.startWrite();
  lcd.setAddrWindow(area->x1, area->y1, w, h);
  lcd.pushColors((uint16_t *)&color_p->full, w * h, true);
  lcd.endWrite();

  lv_disp_flush_ready(disp);  // Let LVGL know the flushing is done
}

void touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  uint16_t touchX = 0, touchY = 0;
  bool touched = lcd.getTouch(&touchX, &touchY, 600);

  if (!touched) {
    data->state = LV_INDEV_STATE_REL;  // Not touched
  } else {
    data->state = LV_INDEV_STATE_PR;  // Pressed
    data->point.x = touchX;  // Update X position
    data->point.y = touchY;  // Update Y position
  }
}

*/





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

// Battery Percentage Calculation

int calculateBattPercent() {
  int battValue = analogRead(BATT_VOLT);
  //Serial.print("Analog Battery Value: ");
  //Serial.println(battValue);
  int f = battValue - 2512;
  float h = (f / 1000.0); // clean up these 2 lines by dividing by 10 instead
  int battPercent = h*100; // save to above
  return battPercent;

}

//Change array length to adjust sample size of battery measurements
int battHist[20] = {0};

void updateBattPercent() {

  int battPercent;
  int sum = 0;
  int baSampleSize = sizeof(battHist) / sizeof(battHist[0]);

  for (int i = 0; i <baSampleSize; i++) {
    battPercent = calculateBattPercent();
    battHist[i] = battPercent;
  }
  for (int i = 0; i <baSampleSize; i++) {
    sum += battHist[i];
  }
  
  int battery = sum/baSampleSize;

  Serial.print("Calculated Battery Percentage: ");
  Serial.print(battery);
  Serial.println("%");
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
uint16_t rawData[239] = {1275, 386, 1277, 440, 432, 1222, 1301, 413, 1277, 386, 459, 1246, 431, 1221, 458, 1247, 432, 1220, 459, 1221, 458, 1247, 1278, 7196, 1277, 413, 1249, 441, 431, 1247, 1277, 413, 1278, 386, 459, 1247, 431, 1247, 431, 1221, 458, 1223, 457, 1221, 458, 1219, 1306, 8276, 1277, 388, 1303, 386, 459, 1248, 1277, 413, 1278, 413, 432, 1248, 431, 1221, 458, 1224, 455, 1219, 460, 1247, 432, 1247, 1278, 7192, 1277, 387, 1304, 386, 459, 1220, 1304, 386, 1305, 413, 432, 1247, 432, 1220, 459, 1247, 432, 1221, 459, 1222, 458, 1220, 1306, 8275, 1278, 389, 1302, 385, 460, 1246, 1277, 386, 1304, 413, 431, 1247, 431, 1247, 431, 1219, 459, 1246, 432, 1221, 457, 1247, 1277, 7189, 1277, 386, 1304, 413, 432, 1221, 1303, 412, 1279, 412, 433, 1247, 432, 1246, 432, 1220, 458, 1219, 459, 1247, 431, 1218, 1306, 8281, 1277, 387, 1304, 388, 457, 1247, 1277, 413, 1278, 412, 433, 1247, 432, 1248, 432, 1219, 460, 1248, 432, 1221, 459, 1248, 1278, 7196, 1278, 387, 1303, 387, 458, 1247, 1277, 387, 1304, 387, 458, 1247, 432, 1247, 431, 1247, 432, 1248, 432, 1248, 432, 1248, 1278, 8251, 1301, 413, 1278, 413, 432, 1222, 1302, 388, 1302, 413, 432, 1247, 431, 1220, 458, 1220, 458, 1220, 458, 1247, 431, 1246, 1250, 7197, 1276, 441, 1249, 440, 432, 1222, 1275, 441, 1250, 414, 459, 1221, 459, 1219, 460, 1247, 432, 1247, 432, 1247, 432, 1247, 1277};
uint16_t rawData2[239] = {0, 109, 34, 3, 169, 168, 21, 63, 21, 63, 21, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 63, 21, 63, 21, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 64, 21, 21, 21, 63, 21, 63, 21, 63, 21, 63, 21, 63, 21, 63, 21, 1794, 169, 168, 21, 21, 21, 3694};
uint16_t Samsung_power_toggle[71] = {
    38000, 1,  1,  170, 170, 20, 63, 20, 63, 20, 63,  20, 20, 20, 20,
    20,    20, 20, 20,  20,  20, 20, 63, 20, 63, 20,  63, 20, 20, 20,
    20,    20, 20, 20,  20,  20, 20, 20, 20, 20, 63,  20, 20, 20, 20,
    20,    20, 20, 20,  20,  20, 20, 20, 20, 63, 20,  20, 20, 63, 20,
    63,    20, 63, 20,  63,  20, 63, 20, 63, 20, 1798};
void fnBtnCallback(DFRobot_UI::sButton_t &btn,DFRobot_UI::sTextBox_t &obj) {
  irsend.sendRaw(rawData, 239, 38000);
  //irsend.sendGC(Samsung_power_toggle, 71);

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

static void init_run_ulp(uint32_t usec);

static void init_run_ulp(uint32_t usec) {
  ulp_set_wakeup_period(0, usec);
  esp_err_t err = ulptool_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
  err = ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t));

  if (err) Serial.println("Error Starting ULP Coprocessor");
}


void setup()
{
  Serial.begin(9600);
  Serial.print("--Serial Comms Up--");
  irsend.begin();
  //pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_SD, OUTPUT);
  //analogWrite(D13, 120); //screen brightness
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

  /** 

//LVGL

  lv_init();
  lcd.begin();  // Start the TFT display
  lcd.setRotation(0);  // Set screen orientation
  SD.begin(LCD_SD);
  // Initialize LVGL's draw buffer
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

  // Initialize the display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = disp_flush;  // Attach our flush function
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
*/

 // WiFi.begin(ssid, password);
 // Serial.print("Connecting to WiFi...");
 // while (WiFi.status() != WL_CONNECTED) { // indicate that the ESP32 is trying to connect
 //   decoder.drawPicture(/*filename=*/"/images/wifi_N.bmp",/*sx=*/5,/*sy=*/10,/*ex=*/70,/*ey=*/28,/*screenDrawPixel=*/screenDrawPixel);
    //ui.drawString(/*x=*/screen.width()/3,/*y=*/10,"Connect",COLOR_RGB565_WHITE,ui.bgColor,/*fontsize =*/2,/*Invert=*/0);
 //   printLocalDateTime();
  //  ui.refresh();
 // }
 // Serial.println("Connected!");
 // Serial.println(WiFi.RSSI());
 // updateWifiSig();

  

 // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // configure time settings
 // printLocalDateTime();

  //Initialize UI
  ui.begin();
  
  //ui.setTheme(DFRobot_UI::MODERN);
  //ui.setBgColor(0x4bb2);


  menuBackBtn.setText("Back");
  menuBackBtn.bgColor = COLOR_RGB565_DGRAY;
  menuBackBtn.setCallback(menuBackBtnCallback);
  //screenBrightness.setCallback(changeBrightness);

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


  DFRobot_UI::sButton_t & fnBtn = ui.creatButton();
  fnBtn.setText("Fan");
  fnBtn.bgColor = COLOR_RGB565_PINK;
  fnBtn.setCallback(fnBtnCallback);
  ui.draw(&fnBtn,/**x=*/115,/**y=*/80,/*width*/screen.width()/4,/*height*/screen.width()/10);

  



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

  //ui_init();

  
}


void loop()
{
  updateBattPercent();
 // updateWifiSig();
  //decoder.drawPicture(/*filename=*/"/images/wifi_M.bmp",/*sx=*/0,/*sy=*/0,/*ex=*/100,/*ey=*/30,/*screenDrawPixel=*/screenDrawPixel);
 // printLocalDateTime();
  trellis.read();

  ui.refresh();
  //troubleshooting gyro

  float temp_celsius = temperatureRead();

  Serial.print("Chip Temperature: ");
  Serial.print(temp_celsius);
  Serial.println("°C");

  sensors_event_t event;
  accel.getEvent(&event);
  accel.getEvent(&event);
  Serial.println("");
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");
  Serial.println("m/s^2 ");
  Serial.println("");

  ESP_LOGI(TAG, "ULP Loop Count: %d", (int)ulp_loop_count);

}
