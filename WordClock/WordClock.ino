#include <Wire.h>
// #include "Adafruit_LEDBackpack.h"
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>
 
//Registers
#define LATCH 4
#define CLK 3
#define DATA 2

//NeoPix
#define NEOPIX 6
//mom/dad = 9
//greg = 10
//sara = 9
#define PIX_NUM 9

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
// Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIX_NUM, NEOPIX, NEO_GRB + NEO_KHZ800);

// Pattern types supported:
enum  pattern { NONE, CLEAR, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };
 
// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    public:
 
    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    uint16_t SkipLedIndex; //start of the led to skip
    uint16_t SkipLength; //number of leds to skip, between 0 and 3

    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            switch(ActivePattern)
            {
                case RAINBOW_CYCLE:
                    RainbowCycleUpdate();
                    break;
                case THEATER_CHASE:
                    TheaterChaseUpdate();
                    break;
                case COLOR_WIPE:
                    ColorWipeUpdate();
                    break;
                case SCANNER:
                    ScannerUpdate();
                    break;
                case FADE:
                    FadeUpdate();
                    break;
                case CLEAR:
                	ClearUpdate();
                	ActivePattern = NONE;
                	break;
                default:
                    break;
            }
        }
    }
	
    // Increment the Index and reset at the end
    void Increment()
    {
        if (Direction == FORWARD)
        {
           Index++;
           if (Index >= TotalSteps)
            {
                Index = 0;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
        else // Direction == REVERSE
        {
            --Index;
            if (Index <= 0)
            {
                Index = TotalSteps-1;
                if (OnComplete != NULL)
                {
                    OnComplete(); // call the comlpetion callback
                }
            }
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        if (Direction == FORWARD)
        {
            Direction = REVERSE;
            Index = TotalSteps-1;
        }
        else
        {
            Direction = FORWARD;
            Index = 0;
        }
    }
    
    // Initialize for a RainbowCycle
    void RainbowCycle(uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        SkipLedIndex = 5;
        SkipLength = 1;
        Direction = dir;
    }
    
    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
    	//0 1 2 3 4 5 6 7 8
    	//. . . . . - - . .
        for(int i=0; i< numPixels(); i++)
        {
        	if(SkipLength == 0 || !(i >= SkipLedIndex && i < (SkipLedIndex + SkipLength))){
            	setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
        	}
        }
        show();
        Increment();
    }
 
    // Initialize for a Theater Chase
    void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        SkipLedIndex = 0;
        SkipLength = 0;
        Direction = dir;
   }
    
    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            if ((i + Index) % 3 == 0)
            {
                setPixelColor(i, Color1);
            }
            else
            {
                setPixelColor(i, Color2);
            }
        }
        show();
        Increment();
    }
 
    // Initialize for a ColorWipe
    void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color;
        Index = 0;
        SkipLedIndex = 0;
        SkipLength = 0;
        Direction = dir;
    }
    
    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        setPixelColor(Index, Color1);
        show();
        Increment();
    }
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = (numPixels() - 1) * 2;
        Color1 = color1;
        Index = 0;
        SkipLedIndex = 0;
        SkipLength = 0;
    }
 
    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else if (i == TotalSteps - Index) // Scan Pixel to the left
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
    
    // Initialize for a Fade
    void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
    {
        ActivePattern = FADE;
        Interval = interval;
        TotalSteps = steps;
        Color1 = color1;
        Color2 = color2;
        Index = 0;
        SkipLedIndex = 0;
        SkipLength = 0;
        Direction = dir;
    }
    
    // Update the Fade Pattern
    void FadeUpdate()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;
        
        ColorSet(Color(red, green, blue));
        show();
        Increment();
    }

    void Clear(void)
    {
        ActivePattern = CLEAR;
    }

    void ClearUpdate()
    {
        for(int i=0; i< numPixels(); i++)
        {
            setPixelColor(i, 0);
        }
        show();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }
 
    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }
 
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};
void Ring1Complete();

NeoPatterns Pixels(PIX_NUM, NEOPIX, NEO_GRB + NEO_KHZ800, &Ring1Complete);


RTC_DS1307 RTC;
// RTC_Millis RTC;

byte itis = 0x03;
//5,10,15,20,25,30,35...
int minuteArray[12] = {0x00, 0x0184, 0x0188, 0x90, 0x01A0, 0x01A4, 0xC0, 0x0324, 0x0320, 0x0210, 0x0308, 0x0304};
//twelve, one, two, three....
int hourArray[12] = {0x2000, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000}; //needs to be shifted 8
byte oclock = 0x40; //needs to be shifted 16

long oneSecond;
long buttonTimer = 0;
boolean dstActive;
 
void setup () {
    Serial.begin(57600);
    Serial.println("Start Program");

    RTC.begin();
//    RTC.adjust(DateTime(__DATE__, __TIME__)); //Uncomment this to override setting the time from the compiled date time
 
  if (!RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  pinMode(DATA, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LATCH, OUTPUT);

  Pixels.begin();
  Pixels.show();

  //Determine if currently in dst so we know if need to set later
  DateTime now = RTC.now();
  DateTime mar8 = DateTime(now.year(), 3, 8, 2, 0, 0); //first of mar
  int dowStart = mar8.dayOfTheWeek();
  DateTime dstStart;
  if(dowStart!=0){
    dstStart = DateTime(mar8.unixtime() + 86400L * (7-dowStart));//add number of days to next sunday
  } else {
    dstStart = mar8;
  }

  DateTime nov1 = DateTime(now.year(), 11, 1, 2, 0, 0); //first of nov
  int dowEnd = nov1.dayOfTheWeek();
  DateTime dstEnd;
  if(dowEnd!=0){
    dstEnd = DateTime(nov1.unixtime() + 86400L * (7-dowEnd));
  } else {
    dstEnd = nov1;
  }
  dstActive = now.unixtime() >= dstStart.unixtime() && now.unixtime() <= dstEnd.unixtime();
  Serial.print("Is DST? ");
  Serial.println(dstActive);
}

int hours, minutes;//, countMin, count;
int lastHour, lastMinute;

boolean changePixelShow = true;
void loop() {

	Pixels.Update();

	if(millis() - oneSecond > 1000){
		DateTime now = RTC.now();

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

		Serial.print("DayOfWeek: ");
		Serial.println(now.dayOfTheWeek(), DEC);

		//calculate dst
		/*
		Daylight savings time is defined as being active from
		2am 2nd sunday of march
		ends
		2am first sunday of november

		When active, move clocks ahead one hour. So in march, move forward, in nov move backward
		*/

		//spring forward, fall back

		if(!dstActive && now.month() == 3 && now.day() > 7 && now.day() <= 14 && now.dayOfTheWeek() == 0 && now.hour() == 2){
			RTC.adjust(DateTime(now.unixtime()+(60*60)));
			dstActive = true;
		}

		if(dstActive && now.month() == 11 && now.day() <=7 && now.dayOfTheWeek() == 0 && now.hour() == 2){
			RTC.adjust(DateTime(now.unixtime()-(60*60)));
			dstActive = false;
		}

		hours = now.hour();
    	minutes = now.minute();

		oneSecond = millis();
	
    	//For my Mom and Dad. You da best.
    	//0 1 2 3 4 5 6 7 8 9
    	if(now.month()==7 && now.day()==9){
    		if(changePixelShow){
        		Pixels.RainbowCycle(3, REVERSE);
                Pixels.SkipLedIndex = 6;
        		Pixels.SkipLength = 3;
        		changePixelShow = false;
    		}
        //Sherry
        } else if(now.month()==5 && now.day()==5){
            if(changePixelShow){
                Pixels.RainbowCycle(3, REVERSE);
                Pixels.SkipLedIndex = 5;
            	Pixels.SkipLength = 1;
                changePixelShow = false;
            }
    	} else {
    		//don't show
    		if(!changePixelShow){
    			changePixelShow = true;
    			Pixels.Clear();
    		}
    	}

    	//writing to the main clock
    	//TODO, every time the clock changes (only when)
    	//if hours or 5 min change
    	// if((lastHour != hours || lastMinute != minutes) && (minutes % 5 == 0)){

        int realHour = hours;
        if(realHour > 11){
        	realHour = realHour - 12;
        }
        int minIndex = minutes/5;
        int hourIndex = realHour;
        if(minIndex>6) hourIndex = (hourIndex+1)%12; //because it is "minutes to next hour"

        //build byte array
        long fullByte = 0x00000000;
        fullByte = fullByte | itis;
        fullByte = fullByte | minuteArray[minIndex];
        fullByte = fullByte | (((long) hourArray[hourIndex]) << 8);
        fullByte = fullByte | ((long) oclock << 16);

        byte reg4 = (byte) (fullByte>>24);
        byte reg3 = (byte) ((fullByte>>16) & 0xff);
        byte reg2 = (byte) ((fullByte>>8)& 0xff);
        byte reg1 = (byte) (fullByte & 0xff);

        digitalWrite(LATCH, LOW);
        shiftOut(DATA, CLK, MSBFIRST, reg4);//Register 4
        shiftOut(DATA, CLK, MSBFIRST, reg3);//Register 3
        shiftOut(DATA, CLK, MSBFIRST, reg2);//Register 2
        shiftOut(DATA, CLK, MSBFIRST, reg1);//Register 1
        digitalWrite(LATCH, HIGH);
        lastHour = hours;
        lastMinute = minutes;
    }
}

void Ring1Complete() {
    
}