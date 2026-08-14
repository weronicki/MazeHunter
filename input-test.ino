#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 4
#define SCL_PIN 7
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// buttons
#define BTN_UP 0
#define BTN_DOWN 1
#define BTN_LEFT 2
#define BTN_RIGHT 3
#define BTN_A 20
#define BTN_B 19
#define BTN_MENU1 18
#define BTN_MENU2 14

void drawButton(int x,int y,int w,int h,const char* label,bool pressed)
{
  if(pressed)
    display.fillRoundRect(x,y,w,h,3,SSD1306_WHITE);
  else
    display.drawRoundRect(x,y,w,h,3,SSD1306_WHITE);

  display.setTextSize(1);

  if(pressed)
    display.setTextColor(SSD1306_BLACK);
  else
    display.setTextColor(SSD1306_WHITE);

  display.setCursor(x+4,y+3);
  display.print(label);

  display.setTextColor(SSD1306_WHITE);
}

void setup()
{
  Wire.begin(SDA_PIN,SCL_PIN);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();

  pinMode(BTN_UP,INPUT_PULLUP);
  pinMode(BTN_DOWN,INPUT_PULLUP);
  pinMode(BTN_LEFT,INPUT_PULLUP);
  pinMode(BTN_RIGHT,INPUT_PULLUP);
  pinMode(BTN_A,INPUT_PULLUP);
  pinMode(BTN_B,INPUT_PULLUP);
  pinMode(BTN_MENU1,INPUT_PULLUP);
  pinMode(BTN_MENU2,INPUT_PULLUP);
}

void loop()
{

  bool up=!digitalRead(BTN_UP);
  bool down=!digitalRead(BTN_DOWN);
  bool left=!digitalRead(BTN_LEFT);
  bool right=!digitalRead(BTN_RIGHT);
  bool a=!digitalRead(BTN_A);
  bool b=!digitalRead(BTN_B);
  bool m1=!digitalRead(BTN_MENU1);
  bool m2=!digitalRead(BTN_MENU2);

  display.clearDisplay();

  // D-PAD
  drawButton(30,0,28,12,"UP",up);
  drawButton(0,16,28,12,"LEFT",left);
  drawButton(60,16,28,12,"RIGHT",right);
  drawButton(30,32,28,12,"DOWN",down);

  // A B
  drawButton(98,12,24,12,"A",a);
  drawButton(98,32,24,12,"B",b);

  // MENU
  drawButton(0,50,40,12,"M1",m1);
  drawButton(88,50,40,12,"M2",m2);

  display.display();

  delay(16);
}