#include <LCD_I2C.h>
#include <OneButton.h>
#define BTN_PIN 2
int LED_PIN[] = { 9, 10 };
int LENGTH_LED_TABLE = sizeof(LED_PIN) / sizeof(LED_PIN[0]), vitess = 0;
unsigned long currenTime = 0, previousTimer = 0;
const int loadLCDTimer = 3000, rate = 300;
enum LCDScreen { PHARE_STATE,
                 DIRECTION_STATE };
enum LedStats { LED_OFF,
                LED_ON };
enum JostickPositionY { NEUTRALY,
                        DRIVE,
                        REVERSE };
enum JostickPositionX { NEUTRALX,
                        TURNLEFT,
                        TURNRIGHT };
struct SpeedLimit {
  const int DRIVEMAXSPEED = 120, REVERSEMAXSPEED = -20, DIRECTIONMAXROTATION = 90;
};
JostickPositionX jostickDirectionX = NEUTRALX;
JostickPositionY jostickDirectionY = NEUTRALY;

LedStats ledstate = LED_OFF;
LCDScreen lcdScreen;
OneButton jostickBtn;
LCD_I2C lcd(0x27, 16, 2);

//LEDPIN
void setupLEDPIN() {
  for (int i = 0; i < LENGTH_LED_TABLE; i++) {
    pinMode(LED_PIN[i], OUTPUT);
    digitalWrite(LED_PIN[i], ledstate);
  }
}
//BUTTONPIN
void setupButton() {
  jostickBtn.setup(
    BTN_PIN,
    INPUT_PULLUP,
    true);
  jostickBtn.attachClick(changeScreenOfLCD);
}
//LCD
void setupLCD() {
  static uint8_t lastNumber_55[8] = {
    0b11100,
    0b10000,
    0b11111,
    0b00100,
    0b11111,
    0b00001,
    0b00111,
    0b00000
  };
  lcd.begin();
  lcd.backlight();
  lcd.createChar(0, lastNumber_55);
  lcd.setCursor(4, 0);
  lcd.print("Beaulieu");
  lcd.setCursor(8, 1);
  lcd.write(0);
  lcd.setCursor(14, 1);
  lcd.print("55");
}

void setup() {
  Serial.begin(115200);
  setupLEDPIN();
  setupButton();
  setupLCD();
}

void loop() {
  static int totalRotation = 0, totalSpeed = 0;
  currenTime = millis();

  switch (lcdScreen) {
    case PHARE_STATE:
      showLightState();
      break;
    case DIRECTION_STATE:
      showSpeed(totalSpeed);
      showRotation(totalRotation);
      break;
  }
  lightState();
  speedJostick(totalSpeed, totalRotation);
}



/**====================================PhotoResistance===================================*/

int valueOfPhotoresistancePercent() {
  return map(analogRead(A2), 0, 1023, 0, 100);
}
void lightState() {
  static unsigned long lastTimer = 0;
  const int delay = 5000;
  static int lastLedState = LED_OFF;
  ledstate = (valueOfPhotoresistancePercent() < 50) ? LED_ON : LED_OFF;
  if (ledstate != lastLedState) {
    if (currenTime - lastTimer >= delay) {
      turnTheLEDOnOrOff();
      lastTimer = currenTime;
    }
  }
}
void turnTheLEDOnOrOff() {
  for (int i = 0; i < LENGTH_LED_TABLE; i++) {
    digitalWrite(LED_PIN[i], ledstate);
  }
}




/**====================================Jostick===================================*/
int jostickValueX() {
  return map(analogRead(A0), 0, 1023, -90, 90);
}
int jostickValueY() {
  return map(analogRead(A1), 0, 1023, -6, 6);
}
void speedJostick(int totalSpeed, int totalRotation) {
  jostickDirectionX = (jostickValueX() < 0) ? TURNLEFT : (jostickValueX() > 0) ? TURNRIGHT
                                                                               : NEUTRALX;
  jostickDirectionY = (jostickValueY() < 0) ? REVERSE : (jostickValueY() > 0) ? DRIVE
                                                                              : NEUTRALY;
  static SpeedLimit speedLimit;
  switch (jostickDirectionY) {
    case NEUTRALY:
      totalSpeed = (totalSpeed > 1) ? totalSpeed - 2 : (totalSpeed < 1) ? totalSpeed + 1
                                                                        : 0;
      break;
    case DRIVE:
      if (jostickValueY() < speedLimit.DRIVEMAXSPEED) {
        totalSpeed += jostickValueY();
      } else {
        totalSpeed = speedLimit.DRIVEMAXSPEED;
      }
      break;
    case REVERSE:
      if (jostickValueY() < speedLimit.REVERSEMAXSPEED) {
        totalSpeed -= jostickValueY();
      } else {
        totalSpeed = speedLimit.REVERSEMAXSPEED;
      }
      break;
  }

  switch (jostickDirectionX) {
    case NEUTRALX:
      totalRotation = (totalRotation > 1) ? totalRotation - 2 : (totalRotation < 1) ? totalRotation + 2
                                                                                    : 0;
      break;
    case TURNLEFT:
      if (jostickValueX() > (speedLimit.DIRECTIONMAXROTATION * -1)) {
        totalRotation += jostickValueX();
      } else {
        totalRotation = speedLimit.DIRECTIONMAXROTATION * -1;
      }
      break;
    case TURNRIGHT:
      if (jostickValueX() < speedLimit.DIRECTIONMAXROTATION) {
        totalRotation += jostickValueX();
      } else {
        totalRotation = speedLimit.DIRECTIONMAXROTATION;
      }
      break;
    default:

      break;
  }
}



/**====================================LCD===================================*/
void changeScreenOfLCD() {
  if (currenTime > 3000) {
    lcdScreen = (lcdScreen == PHARE_STATE) ? DIRECTION_STATE : PHARE_STATE;
  }
}
//Jostick
void showSpeed(int speed) {

  static char directionY = (jostickDirectionY == REVERSE) ? 'R' : (jostickDirectionY == DRIVE) ? 'D'
                                                                                               : 'N';
  speed = (speed < 1) ? speed * -1 : speed;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(directionY);
  lcd.print(": ");
  lcd.print(speed);
  lcd.setCursor(7, 0);
  lcd.print("km/h");
}
void showRotation(int rotation) {

  static char directionX = (jostickDirectionX == TURNLEFT) ? 'G' : (jostickDirectionX == TURNRIGHT) ? 'D'
                                                                                                    : 'N';
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(directionX);
  lcd.print(": ");
  lcd.print(rotation);
  lcd.setCursor(7, 0);
  lcd.print("km/h");
}
//PhotoResistance
void showLightState() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pct lum : ");
  lcd.print(valueOfPhotoresistancePercent());
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Phare : ");
  if (ledstate) {
    lcd.print("ON");
  } else {
    lcd.print("OFF");
  }
}
