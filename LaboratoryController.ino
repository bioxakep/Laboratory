#include <Wire.h>

#define QUESTNOPIN 48
#define QUESTYESPIN 49

int startKey = A7; // В роли тумблера. когда включен игра начинается\продолжается, когда выключен игра останавливается
int winPin = A8; // включаеться  на секунду при прохождении  - то есть в конце
int spare = A9; // запасной

int rfidSlaveAddr[6] = {21, 22, 23, 24, 25, 26};
int potenzValues[6] = {1, 2, 3, 4, 5, 6};      //Установить правильные значения
int DigitPins[6][7] = {{ 4,  5,  6,  7,  8,  9, 10},
  {12, 51, 14, 15, 16, 17, 18},
  {19, 52, 53, 22, 23, 24, 25},
  {26, 27, 28, 29, 30, 31, 32},
  {33, 34, 35, 36, 37, 38, 39},
  {40, 41, 42, 43, 44, 45, 46}
};
//a,  b,  c,  d,  e,  f,  g// строго такой порядок подключения.

int potenPins[6] = {A0, A1, A2, A3, A4, A5};

int RFIDBlockRelays[6] = {A10, A11, A12, A13, A14, A15}; // эти релешки закрывают доступ к меткам после того как правильная метка
// установлена, каждая в свой момент (то есть игроки не могут вынуть метку)
// отпускаются только в конце
boolean RFIDClosed[6] = {false, false, false, false, false, false};

byte digits[10] =
{
  0X7E, //0
  0X30, //1
  0X6D, //2
  0X79, //3
  0X33, //4
  0X5B, //5
  0X5F, //6
  0X70, //7
  0X7F, //8
  0X7B, //9
};
byte off = 0x00;
boolean game = false;
boolean questNoButtState = true;
boolean questYesButtState = true;
boolean startKeyState = true;
boolean passLevel = false;
boolean letsStart = false;
int level = 0;
int questNumber = 0;

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  attachInterrupt(0, SkipLevel, FALLING );
  pinMode(startKey, INPUT_PULLUP);
  pinMode(QUESTNOPIN, INPUT_PULLUP);
  pinMode(QUESTYESPIN, INPUT_PULLUP);
  pinMode(winPin, OUTPUT);
  pinMode(spare, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(2, INPUT_PULLUP);

  for (int y = 0; y < 7; y++) {
    for (int x = 0; x < 6; x++) {
      pinMode(DigitPins[x][y], OUTPUT);
      if (y == 0)
      {
        pinMode(potenPins[x], INPUT);
        pinMode(RFIDBlockRelays[x], OUTPUT);
        digitalWrite(RFIDBlockRelays[x], HIGH);
      }
    }
  }
  digitalWrite(winPin, HIGH);
  digitalWrite(spare, HIGH);
}

void loop()
{
  readResetKey();
  if (game)
  {
    //======================================= Levels Code ==============================================
    if (level == 0) //RFID Tags
    {
      int currVals[6] = {0, 0, 0, 0, 0, 0};
      String outStr = "VV,";
      for (int i = 0; i < sizeof(rfidSlaveAddr) / sizeof(int); i++)
      {
        float part = 0.0;
        byte bufercnt = Wire.requestFrom(rfidSlaveAddr[i], 1);
        if (bufercnt == 1)
        {
          currVals[i] = Wire.read();
          part = ((currVals[i] * 1.0) / 10.0) * 100.0; //TEST
          if (part == 100 && !RFIDClosed[i]) {
            digitalWrite(RFIDBlockRelays[i], LOW);
            RFIDClosed[i] = true;
          }
        }
        outStr += String((int)part);
        outStr += ",";
      }
      if (passLevel)  outStr = "VV,100,100,100,100,100,100,";
      if (outStr.length() > 3) {
        Serial.println(outStr);  // Отправляем в процессинг значения всех слэйвов.
        delay(1000);
        readSerial();
      }
    }
    else if (level == 1) //Крутилки (SH Parameters in Processing)
    {
      int checkCount = 0;
      for (int i = 0; i < 6; i++)
      {
        int val = map(analogRead(potenPins[i]), 0, 1023, 0, 9);
        if (val == potenzValues[i]) checkCount++;
        setDigit(i, val);
      }
      if (passLevel) checkCount = 6;
      if (checkCount == 6) {
        Serial.println("P-OK");
        delay(2000);  //2000 default
        readSerial();
      }
    }
    else if (level == 2) //Questions
    {
      int unsVal = 0;
      unsVal = readQuestButt();
      String UNS = "";
      if (unsVal > 0 && questNumber < 3)
      {
        questNumber++;
        UNS = "QQ" + String(questNumber) + ":";
        if (unsVal == 1) UNS += "0";
        if (unsVal == 2) UNS += "1";
      }
      if (UNS.startsWith("QQ")) {
        Serial.println(UNS);
        delay(200);
      }
      if (questNumber == 3) readSerial();
    }
    else if (level == 3)
    {
      //Serial.println("WIN!!!!");
      digitalWrite(winPin, LOW);
      delay(1000);
      Serial.println("winner");
      digitalWrite(winPin, HIGH);
      for (int i = 0; i < sizeof(rfidSlaveAddr) / sizeof(int); i++)
      {
        Wire.beginTransmission(rfidSlaveAddr[i]);
        Wire.write(0xEE);
        Wire.endTransmission();
        delay(10);
      }
      for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 6; x++) {
          digitalWrite(DigitPins[x][y], LOW); //тухнут сегменты
          if (y == 0)
          {
            digitalWrite(RFIDBlockRelays[x], HIGH); //разблокируются метки
            RFIDClosed[x] = false;
          }
        }
      }
      //Serial.println("WIN!!!!");
      level++;
    }
    //======================================= Levels Code ==============================================
  }
  else
  {
    if (Serial.available() > 0)
    {
      String input = "";
      input = Serial.readStringUntil('\n');
      digitalWrite(13, LOW);
      if (input.startsWith("letsgame") && letsStart) {
        game = true;
        //letsStart = false;
        Serial.println("START");
        digitalWrite(13, HIGH);
        for (int i = 0; i < sizeof(rfidSlaveAddr) / sizeof(int); i++)
        {
          Wire.beginTransmission(rfidSlaveAddr[i]);
          Wire.write(0xFF);
          Wire.endTransmission();
          delay(10);
        }

      }
    }
  }
}

//======================================= Methods =======================================
int readQuestButt()
{
  int unswer = -1;

  if (questNoButtState && !digitalRead(QUESTNOPIN))
  {
    delay(5);
    if (!digitalRead(QUESTNOPIN)) unswer = 1;
  }
  questNoButtState = digitalRead(QUESTNOPIN);

  if (questYesButtState && !digitalRead(QUESTYESPIN))
  {
    delay(5);
    if (!digitalRead(QUESTYESPIN)) unswer = 2;
  }
  questYesButtState = digitalRead(QUESTYESPIN);
  return unswer;
}

void setDigit(int i, int val)
{
  for (int d = 6; d >= 0; d--)
  {
    boolean on = ((digits[val] & (1 << d)) > 0) ? true : false;
    digitalWrite(DigitPins[i][6 - d], on);
    //  if (i==0) Serial.println(String(DigitPins[i][6-d]) + " pin is " + String(d) + " bit = " + String(on)  + " int=" + String(i) + " val=" + String(val));
  }
  //Serial.println(" --------------------------------");
}//0 = 0-1-1-1-1-1-1-0 (h-a-b-c-d-e-f-g) (7-6-5-4-3-2-1-0) => {5,6,7,8,9,10,11}

void readSerial()
{
  if (Serial.available() > 0)
  {
    String input = Serial.readStringUntil('\n');
    if (input.length() > 0)
    {
      if (input.startsWith("next")) level++;
      passLevel = false;
    }
  }
}

void SkipLevel()
{
  passLevel = true;
}


void readResetKey()
{
  boolean currState = digitalRead(startKey);
  if (startKeyState && !currState)
  {
    delay(10);
    if (!digitalRead(startKey) && !letsStart && !game)
    {
      letsStart = true;
      digitalWrite(13, HIGH);
    }
  }
  if (!startKeyState && currState)
  {
    delay(10);
    if (digitalRead(startKey) && game) resetGame();
  }
  startKeyState = currState;
}

void resetGame()
{
  level = 0; //сброс уровня,
  game = false;
  letsStart = false;
  passLevel = false;
  questNumber = 0;
  for (int y = 0; y < 7; y++) {
    for (int x = 0; x < 6; x++) {
      digitalWrite(DigitPins[x][y], LOW); //тухнут сегменты
      if (y == 0)
      {
        digitalWrite(RFIDBlockRelays[x], HIGH); //разблокируются метки
        RFIDClosed[x] = false;
      }
    }
  }
  digitalWrite(winPin, HIGH);
  digitalWrite(spare, HIGH);
  digitalWrite(13, LOW);
  for (int i = 0; i < sizeof(rfidSlaveAddr) / sizeof(int); i++)
  {
    Wire.beginTransmission(rfidSlaveAddr[i]);
    Wire.write(0xEE);
    Wire.endTransmission();
    delay(10);
  }
  Serial.println("RESET"); //отправка команды и сбросе процессингу
}



