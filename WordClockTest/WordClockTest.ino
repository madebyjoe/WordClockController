#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>
 
//Registers
#define LATCH 4
#define CLK 3
#define DATA 2

//NeoPix
#define NEOPIX 6
#define PIX_NUM 60

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIX_NUM, NEOPIX, NEO_GRB + NEO_KHZ800);

//time=>year=>date
#define MODE_PIN A0
#define HOUR_PIN A1
#define MIN_PIN A2

#define DISPLAY_ADDRESS 0x70

#define PRIME_FACTOR 7
#define SEVEN_SEG_TIMEOUT 10000

Adafruit_7segment clockDisplay = Adafruit_7segment();
RTC_DS1307 RTC;

boolean modeBtnPressed = false;
boolean hourBtnPressed = false;
boolean minBtnPressed = false;
int modeSelector = 0; //is a value between 0 and 2. 0=time, 1=year, 2=date

byte itis = 0x03;
//5,10,15,20,25,30,35...
int minuteArray[11] = {0x0184, 0x0188, 0x90, 0x01A0, 0x01A4, 0xC0, 0x0324, 0x0320, 0x0210, 0x0308, 0x0304};
//twelve, one, two, three....
int hourArray[12] = {0x2000, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000}; //needs to be shifted 8
byte oclock = 0x40; //needs to be shifted 16


//TODO
/*
- The buttons to change time
- the buttons to change time from front
- timeout of the back clock
- turning on the clock via mode
- changing the mode back to time after timeout

- only changing the main clock when new time necessary

- (done) the neo pixel library
- setting birthdays

- removing delay from code

- only updating rtc every prime? or hour
*/

long oneSecond = 0;
long buttonTimer = 0;
 
void setup () {
    Serial.begin(57600);
    Wire.begin();
    // Setup the display.
    clockDisplay.begin(DISPLAY_ADDRESS);
    RTC.begin();
 
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  pinMode(DATA, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LATCH, OUTPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

int counter = 1;

int hours, minutes;
int lastHour, lastMinute;
void loop() {
	// put your main code here, to run repeatedly:
	// counter = (counter +1) % 10;
	// digitalWrite(LATCH, LOW);
	// shiftOut(DATA, CLK, MSBFIRST, counter);
	// digitalWrite(LATCH, HIGH);

	// delay(50);

	//call every 7 seconds, keep track of every second using Millis technique
	DateTime now = RTC.now();
	if(millis() - oneSecond > 1000){
		

		Serial.print(now.year(), DEC);
		Serial.print('/');
		Serial.print(now.month(), DEC);
		Serial.print('/');
		Serial.print(now.day(), DEC);
		Serial.print(' ');
		Serial.print(now.hour(), DEC);
		Serial.print(':');
		Serial.print(now.minute(), DEC);
		Serial.print(':');
		Serial.print(now.second(), DEC);
		Serial.println();

		hours = now.hour();
    	minutes = now.minute();

		oneSecond = millis();
	}

	//Selecting the Mode
	if(!digitalRead(MODE_PIN)){
		if((millis() - buttonTimer > SEVEN_SEG_TIMEOUT) && !modeBtnPressed){
			modeSelector = (modeSelector + 1) % 3;
		}
		modeBtnPressed = true;
		//light up display
		//start countdown
		buttonTimer = millis();
	} else {
		modeBtnPressed = false;
	}

	if(millis() - buttonTimer < SEVEN_SEG_TIMEOUT){ //a 10 sec timeout
		switch(modeSelector){
			case 0://time
				{
				//Writing to the clock in 24 hour time
				int displayValue = hours*100 + minutes;
				clockDisplay.print(displayValue, DEC);
				if (hours == 0) {
					// Pad hour 0.
					clockDisplay.writeDigitNum(1, 0);
					// Also pad when the 10's minute is 0 and should be padded.
					if (minutes < 10) {
						clockDisplay.writeDigitNum(2, 0);
					}
				}
				clockDisplay.drawColon(true);
				clockDisplay.writeDisplay();
				}
				break;
			case 1://year
				{
				int yearValue = now.year();
				clockDisplay.print(yearValue, DEC);
				clockDisplay.drawColon(false);
				clockDisplay.writeDisplay();
				}
				break;
			case 2://day
				{
				int month = now.month();
				int day = now.day();
				int displayValue = month * 100 + day;
				clockDisplay.print(displayValue, DEC);
				clockDisplay.drawColon(false);
				clockDisplay.writeDisplay();
				}
				break;
		}
	} else {
		clockDisplay.clear();
	}

	//writing to the main clock
	//TODO, every time the clock changes (only when)
	//if hours or 5 min change
	if((lastHour != hours || lastMinute != minutes) && (minutes % 5 == 0)){
	    int realHour = hours;
	    if(realHour > 11){
	    	realHour = realHour - 12;
	    }
	    int minIndex = minutes/5;
	    //build byte array
	    long fullByte = 0x00;
	    fullByte & itis;
	    fullByte & (minuteArray[minIndex]);
	    fullByte & (hourArray[hours] << 8);
	    fullByte & (oclock << 16);

	    digitalWrite(LATCH, LOW);
		shiftOut(DATA, CLK, MSBFIRST, fullByte>>24);//Register 4
		shiftOut(DATA, CLK, MSBFIRST, (fullByte>>16) & 0xff);//Register 3
		shiftOut(DATA, CLK, MSBFIRST, (fullByte>>8)& 0xff);//Register 2
		shiftOut(DATA, CLK, MSBFIRST, fullByte & 0xff);//Register 1
		digitalWrite(LATCH, HIGH);
		lastHour = hours;
		lastMinute = minutes;
	}
	delay(1000);
}

void checkAdjustTime(void){
	if(!digitalRead(HOUR_PIN)){
		hourBtnPressed = true;
	} else {
		hourBtnPressed = false;
	}
	if(!digitalRead(MIN_PIN)){
		minBtnPressed = true;
	} else {
		minBtnPressed = false;
	}

	if(hourBtnPressed){

	}

}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
