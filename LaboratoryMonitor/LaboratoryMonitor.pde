import ddf.minim.*; //<>//
import processing.serial.*;

PFont font;
PImage background;
String portName;
Serial myPort;
int level;
int level2step;
int values[] = new int[6];
int prev_values[] = new int[6];
int textSize = 50;
color red = color(255, 0, 28); 
color yellow = color(177, 254, 71);
String names[] = new String[6];
float scrW, scrH;
float startTextLeftBlockX, startTextRightBlockX, startTextBothBlockY;
float textHeight = 40;
boolean shParamsSetFlag = false;
boolean delay = false;
boolean game = false;
boolean connected = false;
boolean firstStart = true;
boolean win = false;
String[] questions;
String digits = "0123456789";
int questCount = 0;
int blinkCount = 0;
long now;
Minim minim;
AudioPlayer systemReadySound, batteryInstallSound, allBattsOkSound,
            encodersOkSound, lastTrack;

void setup()
{
  String[] comportConfig = loadStrings("com.txt");
  portName = comportConfig[0].substring(comportConfig[0].indexOf("\"") + 1, comportConfig[0].length() - 2);
  if(portName.length() > 0) myPort = new Serial(this, portName, 9600);
  level = 0;
  level2step = 0;
  fill(255);
  stroke(50);
  font = createFont("Arial Bold", 14);
  textFont(font);
  background = loadImage("background_main.jpg");
  names[0] = "APLHA: ";
  names[1] = "BETA: ";
  names[2] = "GAMMA: ";
  names[3] = "DELTA: ";
  names[4] = "EPSILON: ";
  names[5] = "ZETA: ";
  background(0);
  fullScreen();
  //size(800, 600);
  scrW = width;
  scrH = height;
  startTextLeftBlockX = scrW/15;
  startTextRightBlockX = 11*scrW/15;
  startTextBothBlockY = scrH/6;
  minim = new Minim(this);
  systemReadySound = minim.loadFile("SystemReady.wav");
  batteryInstallSound = minim.loadFile("loadBattery.wav");
  allBattsOkSound = minim.loadFile("AllBattsOK.wav");
  encodersOkSound = minim.loadFile("encodersOk.wav");
  lastTrack = minim.loadFile("LastTrack.wav");
  questions = new String[4];
  questions[0] = "LOAD \"X4544\" PROTOCOL? (Y/N)";
  questions[1] = "INITIATE EXTRA X-RAY PROTECTION? (Y/N)";
  questions[2] = "BLOCK MODULE #102-4? (Y/N)";
  questions[3] = "PLEASE WAIT, LOADING...";
  now = millis();
}

void draw()
{
  if (game)
  {
    background(0);
    drawBackground();
    String input = getInput();
    if (input.length() > 4)
    {
      if (input.substring(0, 5).equals("RESET")) resetGame();
    }
    if (level == 0)
    {
      showHeader("PLEASE CORRECT POWER LEVEL IN ALL REACTORS", 255);
      showRFIDValues(level, input);
      if (delay && millis() - now > 2000)
      {
        level++;
        delay = false;
        myPort.write("next");
        systemReadySound.play();
        println("LEVEL = 1");
      }
    } 
    else if (level == 1)
    {
      showHeader("REACTORS ARE READY", color(203, 240, 137));
      showRFIDValues(level, input);
      textAlign(CENTER);
      fill(100, 100, 100, 100);
      rect(0, (height - 100)/2, width, 110);
      fill(255);
      float standardLeading = ( textAscent() + textDescent() ) * 1.275f;
      textLeading( standardLeading * 0.65 );
      if (!shParamsSetFlag)
      {
        String text = "PLEASE SET UP SH PARAMETERS FOR\nALL REACTORS USING EXTERNAL CONTROLS";
        text(text, width/2, height/2);
        getPotenziosState(input);
      } else
      {
        getUnsForQuestions(input);
        String text = "SH PARAMETERS ARE OK!"; 
        fill(color(203, 240, 137));
        text(text, width/2, height/2 + textSize/2);
        for (int q = 0; q <= questCount; q++)
        {
          fill(100, 100, 100, 100);
          rect(0, height/2 + (q+1)*80, width, 70);
          if (q < questCount) fill(120);
          else fill(255);
          text(questions[q], width/2, height/2 + textSize + (1+q)*80);
        }
        if (questCount == 3 && !delay) { 
          now = millis(); 
          delay = true;
        }
        if (delay && millis() - now > 2000) 
        {
          level++;
          background = loadImage("background2.jpg");
          lastTrack.play();
          delay = false;
          now = millis();
          println("level1 passed");
        }
      }
    } 
    else if (level == 2)
    {
      if (level2step == 0)
      {
        fill(100, 100, 100, 100);
        rect(0, 0, width, height/9);
        textAlign(CENTER);
        fill(color(203, 240, 137));
        text("MAIN REACTOR IS HEATING UP...", width/2, height/18 + 20);
        if (millis() - now > 3000) 
        {
          level2step++;
          now = millis();
        }
      } else if (level2step == 1)
      {
        fill(100, 100, 100, 100);
        rect(0, 0, width, height/9);
        textAlign(CENTER);
        fill(color(203, 240, 137));
        text("MAIN REACTOR IS READY", width/2, height/18 + 20);
        fill(100, 100, 100, 100);
        rect(0, height/9 + 10, width, height/10);
        fill(255);
        text("SAMPLE IS MIXING...", width/2, height/9 + 10 + height/20 + 20);
        if (millis() - now > 3000) 
        {
          level2step++;
          now = millis();
        }
      } else if (level2step == 2)
      {
        textSize(textSize);
        fill(100, 100, 100, 100);
        rect(0, 0, width, height/9);
        textAlign(CENTER);
        fill(color(255, 0, 0));
        text("MALFUNCTION!!!", width/2, height/18 + 20);
        fill(100, 100, 100, 100);
        rect(0, height/9 + 10, width, height/10);
        fill(color(255, 0, 0));
        text("MIXER MALFUNCTION!!!", width/2, height/9 + 10 + height/20 + 20);
        fill(255);
        rect(0, 11*height/18, width, height - 12*height/18);
        textSize(100);
        fill(color(255, 0, 0));
        if (blinkCount % 2 == 0) text("WARNING!!!", width/2, 6*height/18 + height/2);
        if (millis() - now > 1000) 
        {
          blinkCount++;
          now = millis();
        }
        if (blinkCount == 5)
        {
          level++;
          now = millis();
          win = true;
        }
      }
    } else if (level == 3)
    {
      background(53, 53, 54);
      textSize(200);
      fill(255, 0, 26);
      text("SYSTEM", width/2, height/2-100);
      text("SHUTDOWN", width/2, height/2 + 200);
      if(millis() - now >= 3000)
      {
        myPort.write("next");
        println("WIN");
        level++;
      }
    } else if(level == 4)
    {
      background(0);
    }
  } else
  {
    
    if (portName.length() == 0) 
    {
      textAlign(CENTER);
      fill(255);
      textSize(50);
      text("CONNECT ARDUINO TO PC AND RESTART", width/2, height/2 - 20);
      text("(PRESS ESC TO EXIT)", width/2, height/2 + 70);
    } else
    {
      if(firstStart) 
      {
        myPort.stop();
        myPort = new Serial(this, portName, 9600);
        //myPort = new Serial(this, Serial.list()[1], 9600);
        firstStart = false;
      }
      background(0);
      myPort.write("letsgame\n"); 
      long now = millis();
      println("try Connect");
      while (millis() - now < 1000) {
          ;
        }
      String input = getInput();
      if (input.length() > 4)
      {
        if (input.substring(0, 5).equals("START")) { 
          game = true;
          connected = true;
          level = 0;
        }
      }
    }
  }
}

void drawBackground()
{
  float backW = background.width;
  float backH = background.height;

  float backX, backY;
  float sizeX, sizeY;
  if (scrW/scrH > backW/backH)
  {
    sizeX = backW * (height/backH);
    sizeY = height;
    backX = (width - sizeX)/2;
    backY = 0;
  } else
  {
    sizeX = width;
    sizeY = backH * (width/backW);
    backX = 0;
    backY = (height - sizeY)/2;
  }
  image(background, backX, backY, sizeX, sizeY);
}

void showHeader(String txt, color col)
{
  fill(100, 100, 100, 100);
  rect(0, 0, width, height/10);
  textSize(textSize);
  textAlign(CENTER);
  fill(col);
  text(txt, width/2, height/20 + textSize/2);
}

void showRFIDValues(int _level, String _input)
{
  strokeWeight(2);
  textAlign(LEFT);
  textSize(40);
  float startY = startTextBothBlockY;
  int checkCount = 0;
  if (_level > 0) _input = "VV,100,100,100,100,100,100,";
  if (_input.length() > 10)
  {
    if (_input.substring(0, 2).equals("VV")) values = int(split(_input.substring(3, _input.length()), ','));
    
  }
  for (int i = 0; i < 6; i++)
    {
      if (values[i] == 100) { 
        if(prev_values[i] < 100) 
        {
          if(!batteryInstallSound.isPlaying()) 
          {
            batteryInstallSound.rewind();
            batteryInstallSound.play();
          }
          else
          {
            batteryInstallSound.pause();
            batteryInstallSound.rewind();
            batteryInstallSound.play();
          }
        }
        prev_values[i] = values[i];
        fill(yellow); 
        checkCount++;
      } else fill(red);

      text(names[i] + values[i] + " %", startTextLeftBlockX, startY + textHeight*i);
      if (i == 2) { 
        startTextLeftBlockX = startTextRightBlockX;
        startY = startTextBothBlockY - 3*textHeight;
      }
    }
    startTextLeftBlockX = scrW/15;
    if (checkCount == 6 && !delay && level == 0) { 
      delay = true; 
      now = millis();
      allBattsOkSound.play();
    }
}

String getInput()
{
  if (myPort.available() > 0)
  {
    String inp = myPort.readStringUntil('\n');
    if (inp != null)
    {
      if (inp.length() > 1) println(inp);
      return inp;
    } else return " ";
  } else return " ";
}

void getPotenziosState(String _input)
{
  if (_input.length() > 3)
  {
    if (_input.substring(0, 4).equals("P-OK")) 
    {
      if(!encodersOkSound.isPlaying()) encodersOkSound.play();
      else
      {
        encodersOkSound.pause();
        encodersOkSound.rewind();
        encodersOkSound.play();
      }
      shParamsSetFlag = true;
      myPort.write("next");
    }
  }
}

void getUnsForQuestions(String _input) // Unswer looks like string "QQ1:YES" or "QQ2:NO"; 
{
  if (_input.length() > 2)
  {
    if (_input.substring(0, 2).equals("QQ") && questCount < 3)
    {
      boolean unsBool = false;
      if (_input.substring(_input.indexOf(":")+1, _input.indexOf(":")+2).equals("1")) unsBool = true;
      questions[questCount] = questions[questCount].substring(0, questions[questCount].indexOf("(Y/N)"));
      if (unsBool) questions[questCount] += " YES";
      else questions[questCount] += " NO";
      questCount++;
    }
  }
}

void resetGame()
{
  println("RESETED");
  background(0);
  game = false;
  level = 10;
  level2step = 0;
  questCount = 0;
  questions[0] = "LOAD \"X4544\" PROTOCOL? (Y/N)";
  questions[1] = "INITIATE EXTRA X-RAY PROTECTION? (Y/N)";
  questions[2] = "BLOCK MODULE #102-4? (Y/N)";
  questions[3] = "PLEASE WAIT, LOADING...";
  blinkCount = 0;
  for(int r = 0; r < 6; r++) values[r] = 0;
  background = loadImage("background_main.jpg");
  shParamsSetFlag = false;
  if(systemReadySound.isPlaying())
  {
    systemReadySound.pause();
    systemReadySound.rewind();
  }
  encodersOkSound.rewind();
  batteryInstallSound.rewind();
  myPort.stop();
  myPort = new Serial(this, portName, 9600);
}