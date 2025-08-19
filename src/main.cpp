/******************************************************************************************/
// CRYSTAL GROWTH MONITORING SYSTEM
// developed by Roan Alvarez on August 2025
// This code is for an ESP32-based system that reads ambient temperature and humidity, 
//  and monitors the liquid level of individual tanks.
/******************************************************************************************/
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SPI.h>
#include <Wire.h>

#define DHTPIN 32     // Digital pin connected to the DHT sensor 
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

#define SerialDebugging true

//TwoWire I2CAHT = TwoWire(0);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
DHT_Unified dht(DHTPIN, DHTTYPE);
//Adafruit_AHTX0 aht;

// connect a push button between ground and...
const uint8_t   Button_pin  = 15;
//Liquid Level Sensor Pin
const uint8_t   LevelSensor = 13;
// the interrupt service routine affects this
volatile bool   isButtonPressed         = false;

int liquidLevel = 0;

uint32_t delayMS;

// interrupt service routine
void senseButtonPressed() {
    if (!isButtonPressed) {
        isButtonPressed = true;
    }
}

void setup() {
    #if (SerialDebugging)
    Serial.begin(9600); while (!Serial); Serial.println();
    #endif

    //I2CAHT.begin(I2C_SDA, I2C_SCL, 400000);bool status;
    lcd.init(); // initialize the lcd 
    lcd.backlight();
    lcd.clear(); // clear the screen

    dht.begin();
    Serial.println(F("Init DHT11 Sensor"));
    // Get temperature sensor details.
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    dht.humidity().getSensor(&sensor);

    // button press pulls pin LOW so configure HIGH
    pinMode(Button_pin,INPUT_PULLUP);
    pinMode(LevelSensor, INPUT);

    // use an interrupt to sense when the button is pressed
    attachInterrupt(digitalPinToInterrupt(Button_pin), senseButtonPressed, FALLING);

    // Set delay between sensor readings based on sensor details.
    delayMS = sensor.min_delay / 1000;

    // ignore any power-on-reboot garbage
    isButtonPressed = false;
}

void loop() {
  // Delay between measurements.
  delay(delayMS);

  /*****IF USING ESPNOW : Display the data from ESP32CAM on the LCD******/
  //lcd.setCursor(0,1);
  //lcd.print("Photo No. : ");lcd.println("");

  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
    lcd.setCursor(0,0);
    lcd.print("ERROR READING TEMP!");
    delay(2000); // wait a second before next reading
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    lcd.setCursor(0,0);
    lcd.print("TEMP : ");lcd.print(event.temperature);lcd.println(" deg C");
    delay(2000); // wait a second before next reading
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    lcd.setCursor(0,0);
    lcd.print("ERROR READING HUM!");
    delay(2000); // wait a second before next reading
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    lcd.setCursor(0,0);
    lcd.print("HUMIDITY : ");lcd.print(event.relative_humidity);lcd.println(" %");
    delay(2000); // wait a second before next reading
  }

  //if water level is 0 = OK, if water level is 1 = LOW
  liquidLevel = digitalRead(LevelSensor);
    if  (liquidLevel == 0) {
      Serial.print("Liquid Level : OK!");
      lcd.setCursor(0,0);
      lcd.print("LIQUID LVL : OK!");
      delay(2000); // wait a second before next reading
    } 
    else {
      Serial.print("Liquid Level: LOW. ");Serial.println("PLEASE CHECK TANK!");
      lcd.setCursor(0,0);
      lcd.print("LIQUID LVL : LOW");
      delay(2000); // wait a second before next reading
    }
}