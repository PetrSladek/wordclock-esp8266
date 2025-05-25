// 
//      SLOVNI HODINY
//    MILOS ZAJIC, PECKY, 2018
//    Modified by Peggy for ESP2866 with wifi
// -------------------------------


#define FASTLED_INTERNAL // remove annoying pragma messages
#include "FastLED.h"
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>

#include <WiFiManager.h>

#define PIN_BUTTON1 D5
#define PIN_BUTTON2 D6
#define PIN_SENSOR A0
#define PIN_LED_DATA D8

#define NUM_LEDS 210
#define COLS_LEDS 15 // sloupcu
#define ROWS_LEDS 14 // radku

#define LED_ON color
#define LED_OFF CRGB::Black

CRGB leds[NUM_LEDS];  //pole led a barev
CRGB color;

const uint32_t colorsTable[16] = {
  0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF,
  0xFFFF00, 0xFF00FF, 0x00FFFF, 0xFF8000,
  0xFF0080, 0x80FF00, 0x00FF80, 0x8000FF,
  0x0080FF, 0xFF8080, 0x80FF80, 0x8080FF,
};


int button2_time = 0;     //doba stisku button2

unsigned long blink_time;
boolean blink = false;
boolean change = false;


uint8_t setmode = 0; //mod nastavovani
uint8_t hours = 1;
uint8_t minutes = 0;
uint8_t old_minutes = -1;
uint8_t color_hours = 0;
uint8_t color_minutes = 0;
uint8_t color_and = 0;
uint8_t brightness = 50;

char buffer[6];  

int eecolor_hours = 250;
int eecolor_minutes = 251;
int eecolor_and = 252;

// vytvoření instance DS1307 z knihovny RTClib
RTC_DS1307 DS1307;

// Vytvoření instance WiFiManager
WiFiManager wifiManager;

WiFiManagerParameter custom_color_hours("color_hours", "Color hours", "", 5);
WiFiManagerParameter custom_color_minutes("color_minutes", "Color minutes", "", 5);
WiFiManagerParameter custom_color_and("color_and", "Color and", "", 5);
WiFiManagerParameter custom_hours("hours", "Hours", "", 5);
WiFiManagerParameter custom_minutes("minutes", "Minutes", "", 5);

// od-do, radek (indexovano od 0)
#define DISP_JE 0, 1, 0
#define DISP_JSOU 2, 5, 0

#define DISP_1HOD 6, 10, 0
#define DISP_2HOD 0, 2, 1
#define DISP_3HOD 4, 6, 2
#define DISP_4HOD 3, 7, 1
#define DISP_5HOD 8, 10, 1
#define DISP_6HOD 11, 14, 0
#define DISP_7HOD 11, 14, 1
#define DISP_8HOD 0, 2, 3
#define DISP_9HOD 0, 4, 2
#define DISP_10HOD 3, 7, 3
#define DISP_11HOD 7, 14, 2
#define DISP_12HOD 8, 14, 3

#define DISP_HODIN 0, 4, 4
#define DISP_HODINA 0, 5, 4
#define DISP_HODINY 6, 11, 4

#define DISP_1MIN 0, 4, 7
#define DISP_DVEMIN 12, 14, 6
#define DISP_DVAMIN 8, 10, 8
#define DISP_3MIN 0, 2, 11
#define DISP_4MIN 5, 9, 7
#define DISP_5MIN 0, 2, 13
#define DISP_6MIN 7, 10, 9
#define DISP_7MIN 0, 3, 10
#define DISP_8MIN 8, 10, 10
#define DISP_9MIN 10, 14, 7
#define DISP_10MIN 10, 14, 12
#define DISP_11MIN 0, 7, 8
#define DISP_12MIN 8, 14, 8
#define DISP_13MIN 0, 6, 11
#define DISP_14MIN 0, 6, 9
#define DISP_15MIN 7, 13, 11
#define DISP_16MIN 7, 14, 9
#define DISP_17MIN 0, 7, 10
#define DISP_18MIN 8, 14, 10
#define DISP_19MIN 0, 9, 12
#define DISP_20MIN 0, 5, 6
#define DISP_30MIN 5, 10, 6
#define DISP_40MIN 7, 14, 5
#define DISP_50MIN 0, 6, 5

#define DISP_MINUTY 9, 14, 13
#define DISP_MINUT 9, 13, 13
#define DISP_MINUTA 3, 8, 13
#define DISP_A 13, 13, 4

//=======================================================================

void setup() {
  Serial.begin(74880);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("\nStartujeme");

  EEPROM.begin(512); 

  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);

/*
  renderHorizontal(0, COLS_LEDS-1, 0, colorsTable[3]);
  renderHorizontal(0, COLS_LEDS-1, ROWS_LEDS-1, colorsTable[3]);
  renderVertical(0, 0, ROWS_LEDS-1, colorsTable[3]);
  renderVertical(COLS_LEDS-1, 0, ROWS_LEDS-1, colorsTable[3]);

  FastLED.setBrightness(50);
  FastLED.show();
*/

  pinMode(PIN_BUTTON1, INPUT_PULLUP);
  pinMode(PIN_BUTTON2, INPUT_PULLUP);

  // digitalWrite(PIN_BUTTON1, HIGH);
  // digitalWrite(PIN_BUTTON2, HIGH);
  
  if (!DS1307.begin()) {
    Serial.println("Hodiny nejsou pripojeny!");
    // while (1);
  }  

  color_hours = EEPROM.read(eecolor_hours);
  color_minutes = EEPROM.read(eecolor_minutes);
  color_and = EEPROM.read(eecolor_and);
  
  sprintf(buffer, "%d", color_hours);
  custom_color_hours.setValue(buffer, 5);
  sprintf(buffer, "%d", color_minutes);
  custom_color_minutes.setValue(buffer, 5);
  sprintf(buffer, "%d", color_and);
  custom_color_and.setValue(buffer, 5);

/*
  sprintf(buffer, "%d", DS1307.now().hour());
  custom_hours.setValue(buffer, 5);
  sprintf(buffer, "%d", DS1307.now().minute());
  custom_minutes.setValue(buffer, 5);
  */

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setHostname("peggy-wordclock");


  // Po připojení k WiFi (ať už z uložených údajů nebo po konfiguraci)
  if (wifiManager.autoConnect("Peggy's WordClock AP"))
  {
    Serial.println("Připojeno k WiFi");
    Serial.print("IP adresa: ");
    Serial.println(WiFi.localIP());

    wifiManager.addParameter(&custom_color_hours);
    wifiManager.addParameter(&custom_color_minutes);
    wifiManager.addParameter(&custom_color_and);
    wifiManager.addParameter(&custom_hours);
    wifiManager.addParameter(&custom_minutes);

    wifiManager.setSaveParamsCallback(saveParamCallback);

    wifiManager.startWebPortal();
  } else {
    Serial.println("Chyba při připojování k WiFi");
  }

}

void loop() {

  // wifimanager can run in a blocking mode or a non blocking mode
  // Be sure to know how to process loops with no delay() if using non blocking
  wifiManager.process(); // avoid delays() in loop when non-blocking and other long running code  

  color = colorsTable[color_hours];

  FastLED.setBrightness(50);
  DateTime dateTime = DS1307.now();


  if (isButtonDown(PIN_BUTTON1) && setmode >= 1) {
    delay(300);

    if (setmode < 6) {
      setmode++;
    }
    else
    {
      setmode = 1;
    }

    Serial.print("Nastavuji mod: ");
    Serial.println(setmode);
  }

  // nastvim aktualni cas do formulare
  sprintf(buffer, "%d", DS1307.now().hour());
  custom_hours.setValue(buffer, 5);
  sprintf(buffer, "%d", DS1307.now().minute());
  custom_minutes.setValue(buffer, 5);

  switch (setmode)
  {
    case 0: //zakladni rezim - zobrazujeme cas, testujeme delku stitku button 2
    
      hours = dateTime.hour() > 12 ? dateTime.hour() - 12 : dateTime.hour();
      hours = hours == 0 ? 12 : hours;
      minutes = dateTime.minute();

      renderHours();
      renderMinutes();

      change = false;

      delay(50);
      isButtonDown(PIN_BUTTON2);

      if (button2_time > 10) {

        Serial.println("Drzel sem dlouho Button 2");

        clearDisplay();
        FastLED.show();
        delay(800);
        renderHours();
        FastLED.show();
        while (isButtonDown(PIN_BUTTON2) == true)
        {
          setmode = 1;
          Serial.println("Setmode 1");
        }
      }
      else
      {
        setmode = 0;
      }

      break;

    case 1: //nastav hours
      if (isButtonDown(PIN_BUTTON2)) {
        change = true;

        if (hours <= 11) {
          hours++;
        } else {
          hours = 1;
        }
      }

      renderHours();
      break;

    case 2: //nastav minutes
      if (isButtonDown(PIN_BUTTON2)) {
        change = true;

        if (minutes < 59) {
          minutes++;
        } else{
          minutes = 0;
        }
      }
      renderMinutes();

      // renderHorizontal(DISP_MINUT, color); //aby se neco zobrazilo bylo kdyz je cela hours
      break;
    case 3: //barva hours
      if (isButtonDown(PIN_BUTTON2)) {
        if (color_hours < 15) {
          color_hours++;
        } else {
          color_hours = 0;
        }

        EEPROM.write(eecolor_hours, color_hours);
      }
      renderColors();
      renderHours();
      break;

    case 4: // barva A
      if (isButtonDown(PIN_BUTTON2)) {
        if (color_and < 15) {
          color_and++;
        } else {
          color_and = 0;
        }

        EEPROM.write(eecolor_and, color_and);
      }
      renderColors();
      color = colorsTable[color_and];
      leds[73] = LED_ON;
      break;

    case 5:   // barva minutes
      if (isButtonDown(PIN_BUTTON2)) {
        if (color_minutes < 15) {
          color_minutes++;
        } else {
          color_minutes = 0;
        }
        EEPROM.write(eecolor_minutes, color_minutes);
      }
      renderColors();
      renderMinutes();
      break;

    case 6: // ukonceni nastaveni
      
      isButtonDown(PIN_BUTTON2);
      if (button2_time > 10) {
        setmode = 0;
        Serial.println("Ukoncuji -> Setmode 0");

        clearDisplay();
        FastLED.show();
        delay(500);
        while (isButtonDown(PIN_BUTTON2) == true);
      }

      renderHours();
      renderMinutes();
      
      if (change) {
        DS1307.adjust(DateTime(2018, 4, 26, hours, minutes, 0));
      }
      break;
  }
  
  if (button2_time > 1) {
    blink = true;
    delay (50);
  }  //autorepeat
  else if (button2_time == 1)
  {
    delay(400);
  }

  
  if (blink_time < millis()) {
    blink_time = millis() + 500;
    blink = !blink;
  }
  
  // processBrightness();
  
  if (blink == true && setmode >= 1) (brightness = 80);
  if (blink == false && setmode >= 1) (brightness = 40);

  FastLED.setBrightness(brightness);
  FastLED.show();

  clearDisplay();
}

//_______________________________________________________________________

void clearDisplay()
{
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = LED_OFF;
  }
}

void render(uint8_t x, uint8_t y, CRGB color)
{
  // kazdej druhej radek je obracne, pásek jde do hada
  if (y % 2 == 0)
  {
    leds[(y*COLS_LEDS) + x] = color;
  }
  else
  {
    leds[(y*COLS_LEDS) + (COLS_LEDS - 1 - x)] = color;
  }
}

// 8, 14, 3
void renderHorizontal(uint8_t x1, uint8_t x2, uint8_t y, CRGB color)
{
  for (int i = x1; i <= x2; i++) {
    render(i, y, color);
  }
}

void renderVertical(uint8_t x, uint8_t y1, uint8_t y2, CRGB color)
{
  for (int j = y1; j <= y2; j++) {
    render(x, j, color);
  }
}

void processBrightness() {
  int oldBrightness = brightness;
  int pom = (1023 - analogRead(PIN_SENSOR));
  pom = pom / 4;
  if (pom > 32) (pom = pom - 30); else pom = 2; //posun nuly
  if (pom > 255) (pom = 255);
  if (pom > oldBrightness + 1) (oldBrightness = oldBrightness + 1); //hystereze a zmena
  if (pom < oldBrightness - 1) (oldBrightness = oldBrightness - 1);
  brightness = oldBrightness;
}

void renderColors() {
  color = colorsTable[0]; //oznac rezim barev
  leds[14] = LED_ON;
  color = colorsTable[1];
  leds[15] = LED_ON;
  color = colorsTable[2];
  leds[44] = LED_ON;
}

void renderHours() {
  color = colorsTable[color_hours];

 
  if (hours >= 2 && hours <= 4) {
    renderHorizontal(DISP_JSOU, color);
  } else {
    renderHorizontal(DISP_JE, color);
  }

  if (hours == 1) {
    renderHorizontal(DISP_1HOD, color);
  }
  if (hours == 2) {
    renderHorizontal(DISP_2HOD, color);
  }
  if (hours == 3) {
    renderHorizontal(DISP_3HOD, color);
  }
  if (hours == 4) {
    renderHorizontal(DISP_4HOD, color);
  }
  if (hours == 5) {
    renderHorizontal(DISP_5HOD, color);
  }
  if (hours == 6) {
    renderHorizontal(DISP_6HOD, color);
  }
  if (hours == 7) {
    renderHorizontal(DISP_7HOD, color);
  }
  if (hours == 8) {
    renderHorizontal(DISP_8HOD, color);
  }
  if (hours == 9) {
    renderHorizontal(DISP_9HOD, color);
  }
  if (hours == 10) {
    renderHorizontal(DISP_10HOD, color);
  }
  if (hours == 11) {
    renderHorizontal(DISP_11HOD, color);
  }
  if (hours == 12) {
    renderHorizontal(DISP_12HOD, color);
  }
  if (hours >= 5 && hours <= 12) {
    renderHorizontal(DISP_HODIN, color);
  }
  else if (hours == 1) {
    renderHorizontal(DISP_HODINA, color);
  }
  else {
    renderHorizontal(DISP_HODINY, color);
  }
}

void renderMinutes() {
  
  if (minutes != old_minutes)
  {
    Serial.print("Cas: ");
    Serial.print(hours);
    Serial.print(" hodin ");
    Serial.print(minutes);
    Serial.println(" minut");

    old_minutes = minutes;
  }

  if  (minutes != 0)
  {
    color = colorsTable[color_and];
    renderHorizontal(DISP_A, color);
    
    color = colorsTable[color_minutes];
    
    if (minutes < 10)
    {
      renderRemainingMinutes(minutes);
    }
    if (minutes == 10) {
      renderHorizontal(DISP_10MIN, color);
    }
    if (minutes == 11) {
      renderHorizontal(DISP_11MIN, color);
    }
    if (minutes == 12) {
      renderHorizontal(DISP_12MIN, color);
    }
    if (minutes == 13) {
      renderHorizontal(DISP_13MIN, color);
    }
    if (minutes == 14) {
      renderHorizontal(DISP_14MIN, color);
    }
    if (minutes == 15) {
      renderHorizontal(DISP_15MIN, color);
    }
    if (minutes == 16) {
      renderHorizontal(DISP_16MIN, color);
    }
    if (minutes == 17) {
      renderHorizontal(DISP_17MIN, color);
    }
    if (minutes == 18) {
      renderHorizontal(DISP_18MIN, color);
    }
    if (minutes == 19) {
      renderHorizontal(DISP_19MIN, color);
    }
    if (minutes >= 20 && minutes <= 29)
    { 
      renderHorizontal(DISP_20MIN, color);
      renderRemainingMinutes(minutes - 20);
    }
    if (minutes >= 30 && minutes <= 39)
    { 
      renderHorizontal(DISP_30MIN, color);
      renderRemainingMinutes(minutes - 30);
    }
    if (minutes >= 40 && minutes <= 49)
    { 
      renderHorizontal(DISP_40MIN, color);
      renderRemainingMinutes(minutes - 40);
    }
    if (minutes >= 50 && minutes <= 59)
    { 
      renderHorizontal(DISP_50MIN, color);
      renderRemainingMinutes(minutes - 50);
    }

    if (minutes == 1) {
      renderHorizontal(DISP_MINUTA, color);
    }
    else if (minutes >= 2 && minutes <= 4 ) {
      renderHorizontal(DISP_MINUTY, color);
    }
    else {
      renderHorizontal(DISP_MINUT, color);
    }
  }
}


void renderRemainingMinutes(uint8_t minutes_units) {
  if (minutes_units == 1) {
    renderHorizontal(DISP_1MIN, color);
  }

  if (minutes_units == 2) {
    if (minutes == 2) {
      renderHorizontal(DISP_DVEMIN, color);
    } else {
      renderHorizontal(DISP_DVAMIN, color);
    }
  }
  if (minutes_units == 3){
    renderHorizontal(DISP_3MIN, color);
  }
  if (minutes_units == 4) {
    renderHorizontal(DISP_4MIN, color);
  }
  if (minutes_units == 5) {
    renderHorizontal(DISP_5MIN, color);
  }
  if (minutes_units == 6) {
    renderHorizontal(DISP_6MIN, color);
  }
  if (minutes_units == 7) {
    renderHorizontal(DISP_7MIN, color);
  }
  if (minutes_units == 8) {
    renderHorizontal(DISP_8MIN, color);
  }
  if (minutes_units == 9) {
    renderHorizontal(DISP_9MIN, color);
  }
}


boolean isButtonDown(uint8_t buttonPin) {
  if (digitalRead(buttonPin) == LOW) {
    delay(20);
    if (digitalRead(buttonPin) == LOW) {
      Serial.print("Stisknuto ");
      Serial.println(buttonPin);

      if (buttonPin == PIN_BUTTON2) {
        button2_time++;
      }

      return true;
    }
  }
  if (buttonPin == PIN_BUTTON2)
  {
    button2_time = 0;
  }
  return false;
}

void saveParamCallback()
{
  color_hours = (uint8_t) strtol(custom_color_hours.getValue(), NULL, 10);
  color_minutes = (uint8_t) strtol(custom_color_minutes.getValue(), NULL, 10);
  color_and = (uint8_t) strtol(custom_color_and.getValue(), NULL, 10);

  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.print("PARAM color_hours = ");
  Serial.println(color_hours);
  
  Serial.print("PARAM color_minutes = ");
  Serial.println(color_minutes);

  Serial.print("PARAM color_and = ");
  Serial.println(color_and);

  EEPROM.write(eecolor_hours, color_hours);
  EEPROM.write(eecolor_minutes, color_minutes);
  EEPROM.write(eecolor_and, color_and);
  EEPROM.commit();

  hours = (uint8_t) max(min((int) strtol(custom_hours.getValue(), NULL, 10), 23), 0);
  minutes = (uint8_t) max(min((int) strtol(custom_minutes.getValue(), NULL, 10), 59), 0);
  DS1307.adjust(DateTime(2018, 4, 26, hours, minutes, 0));
}
