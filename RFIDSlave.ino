#include <SoftwareSerial.h>
#include <Wire.h>
#include <FastLED.h>

#define NUM_LEDS 12
#define STRIPPIN 6
#define BLUEHUE 160
#define REDHUE 255
#define GREENHUE 96
#define PURPLEHUE 192
#define STARTCOUNT 3 //сколько горит изначально, сами задаем.

CRGB Strip[NUM_LEDS];
CHSV blueColor, redColor, greenColor, purpleColor, whiteColor, darkColor;
SoftwareSerial rfid(9, 10);

int rx_counter;
byte rx_data[14]; // 1+10+2+1
int masterDelay = 200;
int key[11][14] =
{
  {2, 48, 48, 48, 48, 54, 55, 54, 65, 70, 66, 70, 54, 3},
  {2, 49, 48, 48, 48, 51, 51, 69, 52, 51, 68, 70, 65, 3},
  {2, 48, 48, 48, 48, 48, 68, 70, 70, 66, 68, 52, 70, 3},
  {2, 48, 66, 48, 48, 49, 55, 52, 48, 66, 54, 69, 65, 3},
  {2, 48, 66, 48, 48, 49, 55, 55, 68, 56, 52, 69, 53, 3},
  {2, 48, 66, 48, 48, 49, 55, 53, 53, 69, 55, 65, 69, 3},
  {2, 48, 66, 48, 48, 49, 55, 55, 69, 67, 57, 65, 66, 3},
  {2, 48, 65, 48, 48, 53, 53, 66, 69, 53, 50, 66, 51, 3},
  {2, 48, 66, 48, 48, 49, 55, 51, 66, 50, 67, 48, 66, 3},
  {2, 48, 65, 48, 48, 53, 53, 69, 65, 57, 65, 50, 70, 3},
  {2, 48, 66, 48, 48, 49, 56, 53, 68, 69, 56, 65, 54, 3}
};
; //С помощью вспомогательной программы получаем коды своих ключей. Вставляем сюда. Ниже называем их по цветам, добавляем столько сколько надо.
String names[11] = {"blue", "yellow", "red", "200", "029", "220", "222", "233", "333", "444", "ten"};
int ledPlus[11] = { 2, 4, 7, 9, 6, 7, 8, 9, 1, 3 , 2};
int thisAddress = 21;
boolean OK = false;
boolean game = false;
byte ledCountforMaster = 0;
void setup()
{
  Serial.begin(9600);
  //while (!Serial);
  rx_counter = 0; // init counter
  rfid.begin(9600);
  FastLED.addLeds<NEOPIXEL, STRIPPIN>(Strip, NUM_LEDS);
  Wire.begin(thisAddress);
  Wire.onRequest(requestEvent);
  Wire.onReceive(OnOff);
  FastLED.setBrightness(40);
  for (int i = 0; i < STARTCOUNT; i++) Strip[i] = CRGB::Blue;
  FastLED.show();
}

void loop()
{
  if (game)
  {
    int keyNumber = ReadRFID();
    //Serial.println(keyNumber);
    if (keyNumber >= 0)
    {
      masterDelay = 900;
      int ledCnt = STARTCOUNT + ledPlus[keyNumber];
      if (ledCnt == NUM_LEDS)
      {
        OK = true;
        for (int i = 0; i < NUM_LEDS; i++) Strip[i] = CRGB::Green;
        Serial.println("GREEN Leds");
      }
      else
      {
      	if (ledCnt < NUM_LEDS) 
        {
          for (int l = 0; l < ledCnt; l++) Strip[l] = CRGB::Blue;
          Serial.println("Blue leds");
        }
      	else if (ledCnt > NUM_LEDS) 
        {
          for (int l = 0; l < NUM_LEDS; l++) Strip[l] = CRGB::Red;
          Serial.println("Red leds");
        }
      	OK = false;
      }
      ledCountforMaster = ledCnt & 0xFF;
      //LedsPlus by keyNumber
    }
    else
    {
      masterDelay = 500;
      FastLED.clear();
      for (int i = 0; i < STARTCOUNT; i++) Strip[i] = CRGB::Blue;
      ledCountforMaster = STARTCOUNT;
    }
    FastLED.show();
    delay(masterDelay);
  }
  else
  {
    FastLED.clear();
    FastLED.show();
    delay(1000);
  }
}

int ReadRFID()
{
  int out = -1;
  int inByte[14];
  if (rfid.available() > 0)
  {
    int count = 0;
    while (count < 14)
    {
      inByte[count] = rfid.read();
      count++;
    }
    if (count == 14)
    {
      int currKey = 0;
      boolean done = false;
      int crc = 0;
      for (int i = 0; i < 11; i++)
      {
        for (int k = 0; k < 14; k++)
        {
          if (key[i][k] == inByte[k]) crc++;
        }
        if (crc == 14)
        {
          currKey = i;
          done = true;
        }
        crc = 0;
      }
      count = 0;
      while (rfid.available() > 0) rfid.read();
      if (done)
      {
        Serial.print("Key color is ");
        Serial.println(names[currKey]);
        out = currKey;
      }
      else {
        //Serial.println("UNKNOWN KEY");
        out = -1;
      }
      done = false;
      for (int k = 0; k < 14; k++) inByte[k] = 0;
    }
  }
  //rfid.flush();
  return out;
}

void requestEvent()
{
  Wire.write(ledCountforMaster);
}

void OnOff(int numBytes)
{
  for (int i = 0; i < numBytes; i++)
  {
    byte cmd = Wire.read();
    if (cmd == 0xFF) game = true;
    if (cmd == 0xEE) game = false;
  }
}

