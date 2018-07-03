#include "U8glib.h"
#include "PWM.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST); // Fast I2C / TWI

uint32_t  frequency = 50;
double   currentAmp = 0;

uint32_t  pre_hz;
uint32_t  pre_duty;

uint32_t  prefreqHzInput = 0;
uint32_t  freqHzInput;
uint32_t  max_dutyCycleInput = max_dutyCycleInput;
uint32_t  dutyCycleInput;
int led = 9;

int jumperAutoPin = 2;

int modFlag = 0;
//amps sensor
int mVperAmp = 40; // See Scale Factors Below
int acsoffset = 2500; // See offsets below
int rawValue = 0;
double voltage = 0;

int fixCurrent = 0;

void setup() {
  //Serial.begin(115200);
  pinMode(jumperAutoPin, INPUT);
  pinMode(13, OUTPUT);

  digitalWrite(13, HIGH);

  //pwm init timer safe
  InitTimersSafe();

  u8g.setFont(u8g_font_unifont);
  u8g.setColorIndex(1);
  initOLED();
}

void initOLED(void) {
  int i = 5;
  while (i > -1) {
    u8g.firstPage();
    do {
      u8g.drawStr( 0, 10, "BT Desulfator v1.0");
      u8g.drawStr( 0, 26, "Pulse Engine");
      u8g.setScale2x2();
      char c[2];
      String(i, DEC).toCharArray(c, 2);
      u8g.drawStr( 30, 25, c);
      u8g.undoScale();
    } while ( u8g.nextPage() );
    i--;
    delay(1000);
  }
}

int readJumperAutoPin(void) {
  //return digitalRead(jumperAutoPin);
  return LOW;
}

void readCurrentSensor(void) {
  rawValue = analogRead(A3);
  voltage = (rawValue / 1023.0) * 5000; // Gets you mV
  currentAmp = ((voltage - acsoffset) / mVperAmp);
}

void readPwm(void) {
  pre_hz = map(analogRead(A0), 0, 1023, 1 , 100);
  pre_duty = analogRead(A1);
  if(pre_hz>50){
    freqHzInput  = pre_hz * 200;
  }else{
    freqHzInput  = pre_hz * 100;  
  }
  
  //check auto or manual
  if(readJumperAutoPin() == HIGH){
    // Manaul
    dutyCycleInput = pre_duty * 48;
  }else{
    // Auto
    fixCurrent = map(pre_duty, 0, 1023, 0 , 30);
    if(fixCurrent==0){
      dutyCycleInput = 0;
    }else{
      if(currentAmp < fixCurrent){
        dutyCycleInput = dutyCycleInput+32;
      }else if(currentAmp > fixCurrent)
        dutyCycleInput = dutyCycleInput-32;
      }
    }
    if(dutyCycleInput>49104){
      dutyCycleInput = 49104;
    }else if(dutyCycleInput<0){
      dutyCycleInput = 0;
    }
}

void generatePwm(void) {
  if(prefreqHzInput!=freqHzInput){
    SetPinFrequencySafe(led, freqHzInput);
    prefreqHzInput = freqHzInput ;
  }  
  pwmWriteHR(led, dutyCycleInput);
}

void loop() {
  readCurrentSensor();
  readPwm();
  generatePwm();
  dispayOLED();
  blinkLED();
  modFlag++;
  delay(100);
}

void blinkLED() {
  if (modFlag < 6) {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
    if (modFlag == 10) {
      modFlag = 0;
    }
  }
}

void dispayOLED(void) {
  u8g.firstPage();
  do {
    draw();
  } while ( u8g.nextPage() );
}

void draw(void) {

  String freqHzInputStrDisplay = String("F:" + String(freqHzInput, DEC) + "Hz");
  char c[30];
  freqHzInputStrDisplay.toCharArray(c, 30);
  u8g.drawStr( 0, 10, c);
  if (readJumperAutoPin() == HIGH) {
    u8g.drawStr( 80, 10, "Manual");
  } else {
    String manualStr = String(fixCurrent, DEC)+"A AT";
    manualStr.toCharArray(c,30);
    u8g.drawStr( 80, 10, c);
  }
  int dutyCyclePercent = (dutyCycleInput / 64.0 / 1023.0 * 100.0);
  String dutyCycleInputStrDisplay = String("D:" + String(dutyCyclePercent, DEC) + "%->" + String(dutyCycleInput, DEC));
  dutyCycleInputStrDisplay.toCharArray(c, 30);
  u8g.drawStr( 0, 26, c);

  u8g.setScale2x2();
  dutyCycleInputStrDisplay = String("A " + String(currentAmp));
  dutyCycleInputStrDisplay.toCharArray(c, 30);
  u8g.drawStr( 5, 25, c);
  u8g.undoScale();
}
