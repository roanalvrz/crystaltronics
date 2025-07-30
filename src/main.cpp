/*
 * This is an example sketch that shows how to toggle the display
 * on and off at runtime to avoid screen burn-in.
 * 
 * The sketch also demonstrates how to erase a previous value by re-drawing the 
 * older value in the screen background color prior to writing a new value in
 * the same location. This avoids the need to call fillScreen() to erase the
 * entire screen followed by a complete redraw of screen contents.
 * 
 * Originally written by Phill Kelley. BSD license.
 * Adapted for ST77xx by Melissa LeBlanc-Williams
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Adafruit_AHTX0.h> // Hardware-specific library for AHT20/AHT21/AHT30/AHT31
#include <SPI.h>

#define TFT_CS         5
#define TFT_RST        4 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         2

#define SerialDebugging true

// For 1.44" and 1.8" TFT with ST7735 (including HalloWing) use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_AHTX0 aht;

// connect a push button between ground and...
const uint8_t   Button_pin              = 15;

//Liquid Level Sensor Pin
const uint8_t   LevelSensor = 13;

// color definitions
const uint16_t  Display_Color_Black        = 0x0000;
const uint16_t  Display_Color_Blue         = 0x001F;
const uint16_t  Display_Color_Red          = 0xF800;
const uint16_t  Display_Color_Green        = 0x07E0;
const uint16_t  Display_Color_Cyan         = 0x07FF;
const uint16_t  Display_Color_Magenta      = 0xF81F;
const uint16_t  Display_Color_Yellow       = 0xFFE0;
const uint16_t  Display_Color_White        = 0xFFFF;

// The colors we actually want to use
uint16_t        Display_Text_Color         = Display_Color_White;
uint16_t        Display_Backround_Color    = Display_Color_Black;

// assume the display is off until configured in setup()
bool            isDisplayVisible        = false;

// declare size of working string buffers. Basic strlen("d hh:mm:ss") = 10
const size_t    MaxString               = 16;

// the string being displayed on the SSD1331 (initially empty)
char oldTimeString[MaxString]           = { 0 };

// the interrupt service routine affects this
volatile bool   isButtonPressed         = false;

int liquidLevel = 0;

// interrupt service routine
void senseButtonPressed() {
    if (!isButtonPressed) {
        isButtonPressed = true;
    }
}

void setup() {
    if (aht.begin()) {
      Serial.println("Found AHT20");
    } else {
      Serial.println("Didn't find AHT20");
    }  
    // button press pulls pin LOW so configure HIGH
    pinMode(Button_pin,INPUT_PULLUP);
    pinMode(LevelSensor, INPUT);

    // use an interrupt to sense when the button is pressed
    attachInterrupt(digitalPinToInterrupt(Button_pin), senseButtonPressed, FALLING);

    #if (SerialDebugging)
    Serial.begin(9600); while (!Serial); Serial.println();
    #endif

    // settling time
    delay(250);

    // ignore any power-on-reboot garbage
    isButtonPressed = false;

    #ifdef ADAFRUIT_HALLOWING
      // HalloWing is a special case. It uses a ST7735R display just like the
      // breakout board, but the orientation and backlight control are different.
      tft.initR(INITR_HALLOWING);        // Initialize HalloWing-oriented screen
      pinMode(TFT_BACKLIGHT, OUTPUT);
      digitalWrite(TFT_BACKLIGHT, HIGH); // Backlight on
    #else
      // Use this initializer if using a 1.8" TFT screen:
      tft.initR(INITR_GREENTAB);      // Init ST7735S chip, black tab tft.initR(INITR_BLACKTAB);
      tft.setRotation(ST7735_MADCTL_BGR); // !!! DOES NOT RESOLVE INVERTED COLORS

    #endif

    // initialise the display
    tft.setFont();
    tft.fillScreen(Display_Backround_Color);
    tft.setTextColor(Display_Text_Color);
    tft.setTextSize(1);

    // the display is now on
    isDisplayVisible = true;
}

//if water level is 0 = OK, if water level is 1 = LOW

void loop() {
    tft.enableDisplay(isDisplayVisible);
        sensors_event_t humidity, temp;
        liquidLevel = digitalRead(LevelSensor);
          if  (liquidLevel == 0) {
            tft.setCursor(5,100);
            tft.print("Liquid Level : OK!");
          } 
          else {
            tft.setTextColor(ST7735_BLUE);
            tft.setCursor(5,100);
            tft.print("Liquid Level: LOW");
            tft.setCursor(5,110);
            tft.print("PLEASE CHECK TANK!");
          }
        
        aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
        tft.setCursor(5,20);
        tft.print("Liquid Sensor");
        tft.setCursor(5,30);
        tft.print("& AHT20 Demo");
        tft.setCursor(5,60);
        tft.print("Temp: "); tft.print(temp.temperature); tft.println(" C");
        tft.setCursor(5,80);
        tft.print("Hum: "); tft.print(humidity.relative_humidity); tft.println(" %");
        //tft.setCursor(5,80);
        //tft.print("Water Level: "); tft.println(liquidLevel, DEC);

        Serial.print("Temperature: ");Serial.print(temp.temperature);Serial.println(" degrees C");
        Serial.print("Pressure: ");Serial.print(humidity.relative_humidity);Serial.println(" RH %");
        Serial.print("liquidLevel= "); Serial.println(liquidLevel, DEC);

        yield();
        delay(1000);
        tft.fillScreen(Display_Backround_Color);
        tft.setTextColor(Display_Text_Color);

    }

/*  
void displayUpTime() {
    // calculate seconds, truncated to the nearest whole second
    unsigned long upSeconds = millis() / 1000;

    // calculate days, truncated to nearest whole day
    unsigned long days = upSeconds / 86400;

    // the remaining hhmmss are
    upSeconds = upSeconds % 86400;

    // calculate hours, truncated to the nearest whole hour
    unsigned long hours = upSeconds / 3600;

    // the remaining mmss are
    upSeconds = upSeconds % 3600;

    // calculate minutes, truncated to the nearest whole minute
    unsigned long minutes = upSeconds / 60;

    // the remaining ss are
    upSeconds = upSeconds % 60;

    // allocate a buffer
    char newTimeString[MaxString] = { 0 };

    // construct the string representation
    sprintf(
        newTimeString,
        "%lu %02lu:%02lu:%02lu",
        days, hours, minutes, upSeconds
    );

    // has the time string changed since the last tft update?
    if (strcmp(newTimeString,oldTimeString) != 0) {

        // yes! home the cursor
        tft.setCursor(0,0);

        // change the text color to the background color
        tft.setTextColor(Display_Backround_Color);

        // redraw the old value to erase
        tft.print(oldTimeString);

        // home the cursor
        tft.setCursor(0,0);
        
        // change the text color to foreground color
        tft.setTextColor(Display_Text_Color);
    
        // draw the new time value
        tft.print(newTimeString);
    
        // and remember the new value
        strcpy(oldTimeString,newTimeString);
    }
}    
    // unconditional display, regardless of whether display is visible
    //displayUpTime();

    // has the button been pressed?
    if (isButtonPressed) {
        
        // yes! toggle display visibility
        isDisplayVisible = !isDisplayVisible;

        // apply
        #if (SerialDebugging)
        Serial.print("button pressed @ ");
        Serial.print(millis());
        Serial.print(", display is now ");
        Serial.println((isDisplayVisible ? "ON" : "OFF"));
        #endif

        // confirm button handled
        isButtonPressed = false;
        
    }

    // no need to be in too much of a hurry
    delay(100);*/
  