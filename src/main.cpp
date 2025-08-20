/******************************************************************************************/
// CRYSTAL GROWTH MONITORING SYSTEM
// developed by Roan Alvarez on August 2025
// This code is for an ESP32-based system that reads and displays ambient temperature 
// and humidity, and monitors the liquid level of individual tanks.
// Once the liquid level is below threshold, email notifications will be sent to the recepient
// using the ESP Mail Client library.

// Future developments may include:
//  - ESP-NOW communication with an ESP32-CAM to display photos on the LCD;
//  - Set limits for email notification to avoid spamming the recipient.
//
// SOURCE/S: 
// Based on the ESP Mail Client Library Created by K. Suwatchai (Mobizt)
/******************************************************************************************/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <LittleFS.h>

#define SPIFFS LittleFS

//REPLACE WITH YOUR WIFI CREDENTIALS
#define WIFI_SSID "Deirdog"
#define WIFI_PASSWORD "m!shyc@tDDk3t"

#define DHTPIN 32     // Digital pin connected to the DHT sensor ***SET AS PIN 2

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

#define SerialDebugging true

const uint8_t   LevelSensor = 13; //Liquid Level Sensor Pin
const uint8_t   Button_pin  = 15; //CURRENTLY UNUSED: Button Pin

// the interrupt service routine affects this
volatile bool   isButtonPressed         = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
DHT_Unified dht(DHTPIN, DHTTYPE);
int liquidLevel = 0;

uint32_t delayMS;

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"

/** The smtp port e.g.
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
 */
#define SMTP_PORT esp_mail_smtp_port_465

/* The log in credentials */
#define AUTHOR_EMAIL "proto01crystaltronics@gmail.com"
#define AUTHOR_PASSWORD "wiqdzfoevbelhoge"

/* Recipient email address */
#define RECIPIENT_EMAIL "mishycatbuys@gmail.com" //REPLACE WITH YOUR EMAIL ADDRESS samfeleo@yahoo.com

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

/* Declare the Session_Config for user defined session credentials */
ESP_Mail_Session config;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);
void sendEmail();

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

    lcd.init(); // initialize the lcd 
    lcd.backlight();
    lcd.setCursor(1,0);
    lcd.print("CRYSTALTRONICS");
    lcd.setCursor(6,1);
    lcd.print("2025");
    delay(2000); // wait for 2 seconds
    lcd.clear(); // clear the screen

    dht.begin();
    Serial.println(F("Init DHT11 Sensor"));

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(200);
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    lcd.setCursor(0,1);
    lcd.print("Waiting for WIFI!");

    if (!LittleFS.begin()) { //littleFS initialize then create file if it does not exist
    Serial.println("LittleFS Mount Failed");
    File file = LittleFS.open("/tze.txt", "w");
    if (file) file.close();
    return;
    }
    

    /*  Set the network reconnection option */
    MailClient.networkReconnect(true);

    /** Enable the debug via Serial port
     * 0 for no debugging
     * 1 for basic level debugging
     *
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
     */
    smtp.debug(1);

    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    /* Set the session config */
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = "";

    /*
    Set the NTP config time
    For times east of the Prime Meridian use 0-12
    For times west of the Prime Meridian add 12 to the offset.
    Ex. American/Denver GMT would be -6. 6 + 12 = 18
    See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
    */
    config.time.ntp_server = F("pool.ntp.org");
    config.time.gmt_offset = 0;
    config.time.day_light_offset = 0;

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
    lcd.print("TEMP: ");lcd.print(event.temperature);lcd.println("deg C");
    delay(2000); // wait a second before next reading
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    lcd.setCursor(0,0);
    lcd.print("ERROR READING HUM!");
    delay(2000); // wait before displaying next text
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    lcd.setCursor(0,0);
    lcd.print("HUMIDITY: ");lcd.print(event.relative_humidity);lcd.println("%");
    delay(2000); // wait before displaying next text
  }

  //if water level is 0 = OK, if water level is 1 = LOW
  liquidLevel = digitalRead(LevelSensor);
    if  (liquidLevel == 0) {
      Serial.print("Liquid Level : OK!");Serial.println();
      lcd.setCursor(0,0);
      lcd.print("LIQUID LVL: OK !");
      delay(2000); // wait before displaying next text
      lcd.setCursor(0,1);
      lcd.print("IP: ");lcd.print(WiFi.localIP());
    } 
    else {
      Serial.print("Liquid Level: LOW. ");Serial.println("PLEASE CHECK TANK!");
      lcd.setCursor(0,0);
      lcd.print("LIQUID LVL : LOW");
      lcd.setCursor(20,2);
      lcd.print("Sending alert...");
      delay(2000); // wait before displaying next text

      sendEmail();
      lcd.setCursor(0,1);
      lcd.print("EMAIL ALERT SENT");
    }
}

void sendEmail() {
  String lastTemp;

  sensors_event_t event;
  dht.temperature().getEvent(&event);

  lastTemp = String(event.temperature);

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = F("ESP");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("PLEASE CHECK TANK 1! - Sent from ESP board");
  message.addRecipient(F("Sam"), RECIPIENT_EMAIL);
  
  /*Send HTML message*/
  /*String htmlMsg = "<div style=\"color:#2f4468;\"><h1>PLEASE CHECK TANK!</h1><p>- Sent from ESP board</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/

  //Send raw text message
  String textMsg = "Liquid level is LOW! Current temperature is: " + (lastTemp) + "°C. Please check the tank.";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Connect to the server */
  if (!smtp.connect(&config)){
   // ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message)) //"Status Code: %d" smtp.statusCode(),
    //ESP_MAIL_PRINTF("Error,  Error Code: %d, Reason: %s",  smtp.errorCode(), smtp.errorReason().c_str());
    Serial.println("Error sending Email");
    lcd.setCursor(20,2);
    lcd.print("Error sending e-mail!");
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      //ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}
