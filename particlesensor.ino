#include <M5Stack.h>

hw_timer_t * timer = NULL;
hw_timer_t * timer1 = NULL;
int _pm1;
int _pm25;
int _pm10;
int _counter = 0;  // counts number of measurements
int _intcount = 0; // counts interrupts
int _continue = 0; // indicates continuous operation

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
         
        if(_counter > 15 || _continue == 1) // ignore averaging after restart
        {
          _tpm1 = (int)((_pm1*10 + _tpm1)/11); 
          _tpm25 = (int)((_pm25*10 + _tpm25)/11);
          _tpm10 = (int)((_pm10*10 + _tpm10)/11);
          _continue = 1;
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
    pm1  = _pm1;
    pm25 = _pm25;
    pm10 = _pm10;
    counter = _counter;
    return counter > 10;
  }
  void continuousMode() {
    _continue = 0; 
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
    _continue = 0;
  }
  void restart() {
    _counter = 0; 
  }
};

particlesensor ps;
typedef struct particles {
  int pm1;
  int pm25;
  int pm10;
};

particles psarr[300];

void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.fillRect(0, 0, 320, 20, 0x784F);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(2, 2);
  M5.Lcd.printf("Particle sensor v.0.1");
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  //set up serial receiver
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &serialRX, true);
  timerAlarmWrite(timer, 5000, true); // usec
  timerAlarmEnable(timer);

  ps.start();
  ps.continuousMode();
  pinMode(39, INPUT); // button A
  draw_chart();
}

void draw_chart() {
  int i;
  int ii;
  int maxp = 0;
  
  M5.Lcd.fillRect(0, 120, 320, 100, 0x2104);
  for(i = 0; i < 300; i = i + 20){
    M5.Lcd.drawLine(i + 18, 120, i + 18, 220, 0x31A6);
  }
  M5.Lcd.drawLine(0, 170, 320, 170, 0x31A6);
  for(i = 0; i < 299; i++) {
    psarr[i] = psarr[i+1];
    if(psarr[i].pm25 > maxp) {
      maxp = psarr[i].pm25;
    }
    if(psarr[i].pm10 > maxp) {
      maxp = psarr[i].pm10;
    }
    if(psarr[i].pm1 > maxp) {
      maxp = psarr[i].pm1;
    }
  }
  psarr[299].pm25 = ps.pm25;
  psarr[299].pm10 = ps.pm10;
  psarr[299].pm1  = ps.pm1;
  if(psarr[299].pm25 > maxp) {
      maxp = psarr[299].pm25;
    }
  if(psarr[299].pm10 > maxp) {
      maxp = psarr[299].pm10;
    }
  if(psarr[299].pm1 > maxp) {
      maxp = psarr[299].pm1;
    }
  maxp = maxp/100 + 1;

  M5.Lcd.drawLine(0, 220, 320, 220, 0xCE79);
  for(int i = 1; i < 300; i++){ 
    ii = i + 18;
    M5.Lcd.drawLine(ii - 1, 220 - psarr[i-1].pm1/maxp, ii, 220 - psarr[i].pm1/maxp, 0xFC60);
    M5.Lcd.drawLine(ii - 1, 220 - psarr[i-1].pm10/maxp, ii, 220 - psarr[i].pm10/maxp, 0xD5C3);
    M5.Lcd.drawLine(ii - 1, 220 - psarr[i-1].pm25/maxp, ii, 220 - psarr[i].pm25/maxp, 0xFA02);
  }  
  M5.Lcd.setCursor(0, 120);
  M5.Lcd.printf("%d", maxp * 10);
}

void loop() {
  int btnA = digitalRead(39);
  if (btnA == 0) {
    //ps.start();
  }
  sleep(1);
  M5.Lcd.fillRect(0, 25, 320, 90, 0x31A6);
  ps.read();
  M5.Lcd.setCursor(10, 30);
  M5.Lcd.printf("# %d ", ps.counter);
  M5.Lcd.setCursor(10, 50);  
  M5.Lcd.printf("PM1.0 %4d.%1d", ps.pm1/10, ps.pm1%10);
  M5.Lcd.setCursor(10, 70);
  M5.Lcd.printf("PM2.5 %4d.%1d", ps.pm25/10, ps.pm25%10);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.printf("PM10  %4d.%1d", ps.pm10/10, ps.pm10%10);
  if(ps.counter >= 30) {
    draw_chart();
    ps.restart();
  }
}
