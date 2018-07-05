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
uint32_t  preDutyCycleInput;
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

int ledPin = 13;
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long interval = 1000;           // interval at which to blink (milliseconds)
int num = 0; // Manat Add for Hz Show 7-Segments

void setup() {
  //Serial.begin(115200);
  pinMode(jumperAutoPin, INPUT);
  //pinMode(13, OUTPUT);
  pinMode(ledPin, OUTPUT);  //Manat Add


  //digitalWrite(13, HIGH);

  //pwm init timer safe
  InitTimersSafe();
  
  initOLED();
  digitalWrite(ledPin, HIGH);  
  waitLoop(30000);
  digitalWrite(ledPin, LOW);  

}

void initOLED(void) {
  u8g.setFont(u8g_font_unifont);
  u8g.setColorIndex(1);
  
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
    delay(500);
  }
}

int readJumperAutoPin(void) {
  //return digitalRead(jumperAutoPin);
  return LOW;
}

void readCurrentSensor(void) {
  double average = 0.0;
  for(int i =0; i<50 ; i++){
    rawValue = analogRead(A3);
    voltage = (rawValue / 1023.0) * 4820; // Gets you mV
    average =  average+((voltage - acsoffset) / mVperAmp);
    delay(1);
  }
  currentAmp = average /50;
}

void readPwm(void) {
  //pre_hz = analogRead(A0); // map(analogRead(A0), 0, 1023, 1 , 100);
  pre_hz = map(analogRead(A0), 0, 1023, 1 , 100);
  pre_duty = analogRead(A1);
  if(pre_hz>20){
    freqHzInput  = (pre_hz-20)*200+2000;
  }else{
    freqHzInput  = pre_hz * 100;  
  }
  
  //check auto or manual
  if(readJumperAutoPin() == HIGH){
    // Manaul
    pre_duty = map(pre_duty, 0, 1023, 1 , 100);
    dutyCycleInput = pre_duty * 180;
  }else{
    // Auto
    fixCurrent = map(pre_duty, 0, 1023, 0 , 30);
    if(fixCurrent==0){
      dutyCycleInput = 0;
    }else{
      if(fixCurrent>currentAmp){
        dutyCycleInput = dutyCycleInput+100;
      }else if(fixCurrent < (int) currentAmp){
        dutyCycleInput = dutyCycleInput-100;
      }
      /*
      if((int)currentAmp) < fixCurrent){
        dutyCycleInput = dutyCycleInput+100;
      }else if((round(currentAmp) > fixCurrent)
        dutyCycleInput = dutyCycleInput-100;
      }
      */
    }
    if(dutyCycleInput>30000){
      dutyCycleInput = 30000;
    }else if(dutyCycleInput<0){
      dutyCycleInput = 0;
    }
  }
}

void generatePwm(void) {
  if(prefreqHzInput!=freqHzInput){
    SetPinFrequencySafe(led, freqHzInput);
    prefreqHzInput = freqHzInput ;
  }
  if(preDutyCycleInput != dutyCycleInput){
    pwmWriteHR(led, dutyCycleInput);
    preDutyCycleInput = dutyCycleInput;
  }
}

void loop() {
  while(1){
    unsigned long currentMillis = millis();    //  For ledState
    readCurrentSensor();
    readPwm();
    generatePwm();
    dispayOLED();
    //blinkLED();
    ///modFlag++;

     if((currentMillis - previousMillis) > (interval/(num+1))) {
      // save the last time you blinked the LED 
      previousMillis = currentMillis;   
  
      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW){
        ledState = HIGH;
      }else{
        ledState = LOW;
      }
 
      // set the LED with the ledState of the variable:
      digitalWrite(ledPin, ledState);
    }
  }
  //delay(100);
}

/*
void blinkLED() {
    if((currentMillis - previousMillis) > (interval/(num+1))) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
}*/

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

void waitLoop(unsigned int time)
{
  unsigned int i,j;
  for (j=0;j<time;j++)
  {
    for (i=0;i<200;i++) //the ATmega is runs at 16MHz
      if (PORTC==0xFF) DDRB|=0x02; //just a dummy instruction
  }
}
