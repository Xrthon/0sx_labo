#include <LCD_I2C.h>
#include <OneButton.h>
#define BTN_PIN 2
int LED_PIN[] = { 8, 9 };
int LENGTH_LED_TABLE = sizeof(LED_PIN) / sizeof(LED_PIN[0]), vitess = 0;
unsigned long currenTime = 0, previousTimer = 0;
const int loadLCDTimer = 3000, rate = 200;
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
JostickPositionX jostickDirectionX;
JostickPositionY jostickDirectionY;

LedStats ledstate;
LCDScreen lcdScreen;
OneButton jostickBtn;
SpeedLimit speedLimit;

LCD_I2C lcd(0x27, 16, 2);

//LEDPIN
void setupLEDPIN() {
  for (int i = 0; i < LENGTH_LED_TABLE; i++) {
    pinMode(LED_PIN[i], OUTPUT);
    digitalWrite(LED_PIN[i], LOW);
  }
}
//BUTTONPIN
void setupButton() {
  jostickBtn.setup(
    BTN_PIN,
    INPUT_PULLUP,
    true);
}
//LCD
void setupLCD() {


  lcd.begin();
  uint8_t lastNumber_55[8] = {
    0b11100,
    0b10000,
    0b11111,
    0b00100,
    0b11111,
    0b00001,
    0b00111,
    0b00000
  };
  uint8_t leftArrow[8] = {
    0b00000,
    0b00100,
    0b01000,
    0b11111,
    0b01000,
    0b00100,
    0b00000,
    0b00000
  };
  uint8_t rightArrow[8] = {
    0b00000,
    0b00100,
    0b00010,
    0b11111,
    0b00010,
    0b00100,
    0b00000,
    0b00000
  };
  uint8_t neutralArrow[8] = {
    0b11111,
    0b00000,
    0b11111,
    0b00000,
    0b11111,
    0b00000,
    0b11111,
    0b00000
  };

  lcd.createChar(0, lastNumber_55);
  lcd.createChar(1, leftArrow);
  lcd.createChar(2, rightArrow);
  lcd.createChar(3, neutralArrow);
  lcd.backlight();
  lcd.setCursor(4, 0);
  lcd.print("Beaulieu");
  lcd.setCursor(8, 1);
  lcd.write(0);
  lcd.setCursor(14, 1);
  lcd.print("55");
  delay(3000);
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
  jostickBtn.tick();
  jostickBtn.attachClick(changeScreenOfLCD);

  if (currenTime - previousTimer >= rate) {
    switch (lcdScreen) {
      case PHARE_STATE:
        showLightState();
        break;
      case DIRECTION_STATE:
        showSpeed(totalSpeed);
        showRotation(totalRotation);
        break;
    }
    previousTimer = currenTime;
    showDa();
  }
  lightState();
  jostick(totalSpeed, totalRotation);
}









/**====================================ShowDA===================================*/
//Afficher le numero Etd
void showDa() {
  Serial.print("etd:1993855,x:");
  Serial.print(analogRead(A2));
  Serial.print(",y:");
  Serial.print(analogRead(A1));
  Serial.print(",sys:");
  Serial.print(ledstate);
  Serial.println();
}







/**====================================PhotoResistance===================================*/
//Map la valeur de la photoresistance en Pourcentage
int valueOfPhotoresistancePercent() {
  return map(analogRead(A0), 0, 1023, 0, 100);
}



//Etat de la LED
void lightState() {
  static unsigned long lastTimer = currenTime;
  const int delay = 5000;
  int newLedState = (valueOfPhotoresistancePercent() < 50) ? LED_ON : LED_OFF;
  if (ledstate != newLedState) {

    if (currenTime - lastTimer >= delay) {
      ledstate = newLedState;
      turnTheLEDOnOrOff();
      lastTimer = currenTime;
    }
  } else {
    lastTimer = currenTime;
  }
}


//Changer l'état de la LED
void turnTheLEDOnOrOff() {
  for (int i = 0; i < LENGTH_LED_TABLE; i++) {
    digitalWrite(LED_PIN[i], ledstate);
  }
}




/**====================================Jostick===================================*/

//Valeur map pour acceleration des dirrections
int jostickValueX() {
  return map(analogRead(A2), 0, 1023, -15, 15);
}

//Valeur map pour acceleration des Vitesses
int jostickValueY() {
  return map(analogRead(A1), 0, 1023, -5, 5);
}






//Valeur lue en entrer du jostick en Y
void speedJostick(int &totalSpeed) {
  static int lastSpeedValue;
  static unsigned long accelerationTimer = currenTime;
  int actualSpeedValue = jostickValueY();
  jostickDirectionY = (jostickValueY() < 0) ? REVERSE : (jostickValueY() > 0) ? DRIVE
                                                                              : NEUTRALY;

  if (currenTime - accelerationTimer >= rate) {
    switch (jostickDirectionY) {
      case NEUTRALY:
        if (totalSpeed <= 2 && totalSpeed >= 0) {
          totalSpeed = 0;
        } else {
          totalSpeed = (totalSpeed > 2) ? totalSpeed - 2 : (totalSpeed < 2) ? totalSpeed + 2
                                                                            : 0;
        }
        break;
      case DRIVE:
        if (actualSpeedValue != lastSpeedValue && actualSpeedValue >= 4) {
          if (totalSpeed < speedLimit.DRIVEMAXSPEED - 5) {
            totalSpeed += jostickValueY();
          } else {
            totalSpeed = speedLimit.DRIVEMAXSPEED;
          }
        }
        break;
      case REVERSE:
        if (actualSpeedValue != lastSpeedValue && actualSpeedValue <= -4) {

          if (totalSpeed > speedLimit.REVERSEMAXSPEED + 5) {
            totalSpeed += jostickValueY();
          } else {
            totalSpeed = speedLimit.REVERSEMAXSPEED;
          }
        }
        break;
    }
    accelerationTimer = currenTime;
  }
}








//Valeur lue en entrer du jostick en X
void rotationJostick(int &totalRotation) {
  jostickDirectionX = (jostickValueX() < 0) ? TURNLEFT : (jostickValueX() > 0) ? TURNRIGHT
                                                                               : NEUTRALX;

  static unsigned long directionTimer = currenTime;
  static int lastDirectionValue;
  int actualDirectionValue = jostickValueX();

  if (currenTime - directionTimer >= rate) {
    switch (jostickDirectionX) {
      case NEUTRALX:
        if (totalRotation <= 15 && totalRotation >= 0) {
          totalRotation = 0;
        } else {
          totalRotation = (totalRotation > 1) ? totalRotation - 15 : (totalRotation < 1) ? totalRotation + 15
                                                                                         : 0;
        }
        break;
      case TURNLEFT:
        if (actualDirectionValue != lastDirectionValue && actualDirectionValue <= -10) {
          if (totalRotation > ((speedLimit.DIRECTIONMAXROTATION * -1) + 15)) {
            totalRotation += jostickValueX();
          } else {
            totalRotation = speedLimit.DIRECTIONMAXROTATION * -1;
          }
        }
        break;
      case TURNRIGHT:
        if (actualDirectionValue != lastDirectionValue && actualDirectionValue >= 10) {

          if (totalRotation < speedLimit.DIRECTIONMAXROTATION - 15) {
            totalRotation += jostickValueX();
          } else {
            totalRotation = speedLimit.DIRECTIONMAXROTATION;
          }
        }
        break;
    }
    directionTimer = currenTime;
  }
}







//Regrouper les deux parties du jostick en une fonction
void jostick(int &totalSpeed, int &totalRotation) {

  speedJostick(totalSpeed);
  rotationJostick(totalRotation);
}










/**====================================LCD===================================*/

//Changer la vue de l'écran LCD
void changeScreenOfLCD() {
    lcdScreen = (lcdScreen == PHARE_STATE) ? DIRECTION_STATE : PHARE_STATE;
}





//Vitess (Avant/Arriere)
void showSpeed(int speed) {

  char directionY = (jostickDirectionY == REVERSE) ? 'R' : (jostickDirectionY == DRIVE) ? 'D'
                                                                                        : 'N';
  speed = (speed < 1) ? speed   * -1 : speed;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(directionY);
  lcd.print(": ");
  lcd.print(speed);
  lcd.setCursor(7, 0);
  lcd.print("km/h");
}




// Rotation (Gauche/ Droit)
void showRotation(int rotation) {

  char directionX = (jostickDirectionX == TURNLEFT) ? 'G' : (jostickDirectionX == TURNRIGHT) ? 'D'
                                                                                             : 'N';
  int arrowSignal = (directionX == 'G') ? 1 : (directionX == 'D') ? 2
                                                                  : 3;
  lcd.setCursor(0, 1);
  lcd.print(directionX);
  lcd.print(": ");
  lcd.print(rotation);
  lcd.setCursor(7, 1);
  lcd.write(arrowSignal);
}







//Montrer Etat LED
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
