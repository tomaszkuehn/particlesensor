#include <M5Stack.h>

hw_timer_t * timer = NULL;
int _pm1;
int _pm25;
int _pm10;
int _counter = 0;  // counts number of measurements
int _intcount = 0; // counts interrupts

/* Interrupt routine */
void serialRX() {
  static int rxbuf[40];
  static int rxbufpos = 0;  
  static int rxstatus = 0;
  int _tpm1;
  int _tpm25;
  int _tpm10;

  _intcount++;
  if(Serial2.available()) { 
    int ch = Serial2.read();
    if(rxstatus == 0 && ch == 0x42) {
      rxstatus = 1;
    }
    else
    if(rxstatus == 1 && ch == 0x4d){
      rxstatus = 2;
    }
    else
    if(rxstatus == 2) {
      rxbuf[rxbufpos] = ch;
      rxbufpos++;
      if(rxbufpos >= 30) {
        //received frame
        _tpm1  = 10 * (rxbuf[8]*256+rxbuf[9]);
        _tpm25 = 10 * (rxbuf[10]*256+rxbuf[11]);
        _tpm10 = 10 * (rxbuf[12]*256+rxbuf[13]);
         
        if(_counter > 15)
        {
          _tpm1 = (int)((_pm1*10 + _tpm1)/11); 
          _tpm25 = (int)((_pm25*10 + _tpm25)/11);
          _tpm10 = (int)((_pm10*10 + _tpm10)/11);
        }
        _counter++;
        _pm1 = _tpm1;
        _pm25 = _tpm25;
        _pm10 = _tpm10;

        rxstatus = 0;
        rxbufpos = 0;
      }
    }
    else
    {
      rxstatus = 0;
    }
  }  
}

class particlesensor {
  public:
  particlesensor() {
    sleepstatus = 0;
  }
  
  int pm1;
  int pm25;
  int pm10;
  int counter;

  int sleepstatus;
  int read() {
    if(sleepstatus) {
      return 0;
    }
    int c = _intcount;
    int c1 = c;
    while(c == c1) {
      c1 = _intcount;
      usleep(100);
    }
    //M5.Lcd.setCursor(100, 50);
    //M5.Lcd.printf("%d", c1);
    pm1  = _pm1;
    pm25 = _pm25;
    pm10 = _pm10;
    counter = _counter;
    return counter > 10;
  }
  void sleep() {
    if(sleepstatus == 0) {
      Serial2.write(0x00);
      Serial2.write(0x42);
      Serial2.write(0x4d);  
      Serial2.write(0xe4);
      Serial2.write(0x00);
      Serial2.write(0x00);
      Serial2.write(0x01);
      Serial2.write(0x73);
      sleepstatus = 1;
    }
  }
  void start() {
    Serial2.write(0x00);
    Serial2.write(0x42);
    Serial2.write(0x4d);  
    Serial2.write(0xe4);
    Serial2.write(0x00);
    Serial2.write(0x01);
    Serial2.write(0x01);
    Serial2.write(0x74); 
    sleepstatus = 0;
    _counter = 0;
  }
};

particlesensor ps;

void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("Particle sensor");
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &serialRX, true);
  timerAlarmWrite(timer, 5000, true); // usec
  timerAlarmEnable(timer);
  ps.start();
  pinMode(39, INPUT); // button A
  M5.Lcd.setCursor(25, 225);
  M5.Lcd.printf("RESTART");
}



void loop() {
  int btnA = digitalRead(39);
  if (btnA == 0) {
    ps.start();
  }
  sleep(1);
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.fillRect(0, 40, 320, 120, BLACK);
  ps.read();
  M5.Lcd.setCursor(10, 50);
  M5.Lcd.printf("# %d ", ps.counter);
  M5.Lcd.setCursor(10, 70);  
  M5.Lcd.printf("PM1.0 %4d.%1d", ps.pm1/10, ps.pm1%10);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.printf("PM2.5 %4d.%1d", ps.pm25/10, ps.pm25%10);
  M5.Lcd.setCursor(10, 110);
  M5.Lcd.printf("PM10  %4d.%1d", ps.pm10/10, ps.pm10%10);
  if(ps.counter >= 25) {
    ps.sleep();
  }
}
