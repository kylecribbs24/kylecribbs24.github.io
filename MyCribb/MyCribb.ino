// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016 Adafruit Industries
// Licensed under the MIT license.
// Modified by Kyle Cribbs
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_MCP9808.h"

// Declare OLED display
// display(SDA, SCL);
// SDA and SCL are the GPIO pins of ESP8266 that are connected to respective pins of display.
//OLED display(4,5);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/************************ Example Starts Here *******************************/


#define HEAT_PIN 12 //Relay Pin for heat
#define FAN_PIN 14  //Relay pin for fan
#define AC_PIN 13  //Relay pin for AC

// set up the  feed
//AdafruitIO_Feed *ACswitch = io.feed("ACswitch");
AdafruitIO_Feed *heatSwitch = io.feed("heatSwitch"); 
AdafruitIO_Feed *setTemp = io.feed("setTemp");
AdafruitIO_Feed *roomTemp = io.feed("roomTemp");

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808(); 

int f = 0; // Stores your current Room Temperature in Faranheit 
int lastf=-1;
bool man_set = false; 
int Set_Temp = 70;
bool AC_on = true;
bool HEAT_on = false;
int switchPulse = 0;

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

void setup() {

  // start the serial connection
  Serial.begin(115200); // Serial comminication for Debugging 
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.display();
  delay(2000);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // connect to io.adafruit.com
  //Serial.print("Connecting to Adafruit IO");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Connecting to"));
  display.println(F("Adafruit IO"));
  display.display();
  delay(2000);
  io.connect();

  // set up a message handler for the 'digital' feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  heatSwitch-> onMessage(handleHeatSwitch);
  setTemp-> onMessage(handleSetTemp);
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    //Serial.print(".");
    display.println(".");
    display.display();
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("Connected!"));
  display.display();
  delay(3000);


  if (!tempsensor.begin()) {
    //Serial.println("Couldn't find MCP9808!");
    while (1);
  }

  // set led pin as a digital output
  pinMode(AC_PIN, OUTPUT);
  pinMode(HEAT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

}

void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  float c = tempsensor.readTempC();
  f = c * 9.0 / 5.0 + 31;// Convert degC to degF for us in the US :)

  //show temp on oled
  displayCurrentTemp();

  // save the current state to the adafruit feed
  roomTemp->save(f);


  // turn on AC
  if(AC_on == true && Set_Temp < f) {
    digitalWrite(AC_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
    digitalWrite(HEAT_PIN, LOW);
  }

  // turn off AC
  if(AC_on ==  true && Set_Temp > f) {
    digitalWrite(AC_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(HEAT_PIN, LOW);
  }

  // turn on HEAT
  if(HEAT_on == true && Set_Temp > f) {
    digitalWrite(AC_PIN, LOW);
    digitalWrite(HEAT_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
  }

  // turn off HEAT
  if(HEAT_on == true && Set_Temp < f) {
    digitalWrite(HEAT_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(AC_PIN, LOW);
  }

/*
  // return if the value hasn't changed
  if(f == lastf)
    return;
*/
  
  // store last temp state
  lastf = f;
  delay(4000);

}

void displayCurrentTemp() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("CURRENT TEMPERATURE:"));
  display.setTextSize(4);
  display.setCursor(40, 25);
  display.println(f);
  display.display();
  delay(1000);
}

// this function is called whenever an 'digital' feed message
// is received from Adafruit IO. it was attached to
// the 'digital' feed in the setup() function above.

void handleHeatSwitch(AdafruitIO_Data *data) {
  //display.clear();
  Serial.print("received <- ");

  if(data->toPinLevel() == LOW){
    ++switchPulse;
  }

  if(switchPulse % 2 == 0){
    AC_on = true;
    HEAT_on = false;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println(F("Switching to:"));
    display.setTextSize(4);
    display.setCursor(40, 25);
    display.println(F("AC"));
    display.display();
    delay(2000);
   }
   else {
    AC_on = false;
    HEAT_on = true;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println(F("Switching to:"));
    display.setTextSize(4);
    display.setCursor(20, 25);
    display.println(F("HEAT"));
    display.display();
    delay(2000);
   }

  displayCurrentTemp();

}


void handleSetTemp(AdafruitIO_Data *data) {

  // convert the data to integer
  int reading = data->toInt();
  Set_Temp = reading;
  String A=String(reading);
  char Value[5];
  A.toCharArray(Value,5);
  //display.on();
  Serial.print("received <- ");
  Serial.println(reading);
  
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("SET TEMPERATURE:"));
  display.setTextSize(4);
  display.setCursor(40, 25);
  display.println(Value);
  display.display();
  delay(2000);
  
  displayCurrentTemp();
  
}
