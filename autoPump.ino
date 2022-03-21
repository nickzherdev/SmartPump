/*
   Nikolay Zherdev
*/

#define CLK 4
#define DIO 5
#define BTN_PIN 3        // кнопка подключена сюда (BTN_PIN --- КНОПКА --- GND)
#define BTN_PIN_TEST 6
#define MOSFET_PIN 7
#define sensorPower 2
#define sensorPin A0

#include "GyverTM1637.h"
#include "GyverButton.h"
#include <GyverOS.h>

  // uncomment if on battery
//#include <GyverPower.h>

GyverOS<1> OS;  // указать макс. количество задач

GButton butt1(BTN_PIN);
GButton butt2(BTN_PIN_TEST);

GyverTM1637 disp(CLK, DIO);

static volatile bool is_full = false;
int value = 0;
uint32_t Now, clocktimer;
boolean flag;

  // CHANGE TO FALSE IF ON BATTERY
volatile boolean interruptFlag = false;   // флаг прерывания
uint32_t task0_per = 86400000; // millisecs - 24ч
//uint32_t task0_per = 20000; // millisecs - 20 sec
int g_pour_time_sec = 7;

void setup() {
  Serial.begin(9600);

  pinMode(MOSFET_PIN, OUTPUT);

  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
  butt2.setType(LOW_PULL);

  // uncomment if on battery
//  pinMode(sensorPower, OUTPUT);
  // Устанавливаем низкий уровень, чтобы на датчик не подавалось питание
//  digitalWrite(sensorPower, LOW);

  disp.clear();
  disp.brightness(1);  // яркость, 0 - 7 (минимум - максимум)

  OS.attach(0, pour_with_check, task0_per);  // millisecs

//  Чтобы поливать в два захода за сессию - добавить вторую задачку со сдвигом
//  delay(10000);
//  OS.attach(1, warn_fill_water, 10000);
//  OS.stop(1);

  // uncomment if on battery
  // подключаем прерывание на пин D3 (Arduino NANO)
//  attachInterrupt(1, isr, RISING);

// глубокий сон
//  power.setSleepMode(POWERDOWN_SLEEP);
}

  // uncomment if on battery
// обработчик аппаратного прерывания
//void isr() {
//  // дёргаем за функцию "проснуться"
//  // без неё проснёмся чуть позже (через 0-8 секунд)
//  power.wakeUp();
//  interruptFlag = true;   // подняли флаг прерывания
//}

bool is_glass_full() {
    digitalWrite(sensorPower, HIGH);
    int val = analogRead(sensorPin);
    digitalWrite(sensorPower, LOW);
    Serial.println(val);
    is_full = (val > 100) ? 1 : 0;
    return is_full;
}

void requestWaterDisp() {
  byte welcome_banner[] = {_H, _A, _L, _E, _i, _empty,
                           _i,
                           _O, _t, _O, _i, _d, _i
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 350);  // millisenconds
}

void printLastPourTimePassed(int hours, int mins) {
  byte last_pour_time[] = {_L, _A, _S, _t, _empty,
//                           _P, _O, _U, _r, _empty,
                          };
  disp.runningString(last_pour_time, sizeof(last_pour_time), 250);  // millisenconds

  Now = millis();
  while (millis () - Now <= 500) {   // показывать время полсекунды
        disp.displayClock(hours, mins);
  }

  byte continue_appr[] = {_H, _o, _l, _d,
    // _P, _r, _E, _S, _S,
                         };
  disp.runningString(continue_appr, sizeof(continue_appr), 250);  // millisenconds
}

void countdownScreen(byte secs) {
  byte mins = 0;
  uint32_t tmr;
  Now = millis();
  int period = (int)secs*1000;
  while (millis () - Now <= period) {   // каждые n+1 секунд
    if (millis() - tmr > 500) {       // каждые полсекунды
      tmr = millis();
      flag = !flag;
      disp.point(flag);   // вкл/выкл точки

      if (flag) {
        // ***** часы! ****
        disp.displayClock(mins, secs);
        secs--;
      }
    }
  }
    disp.point(0);   // выкл точки
    disp.clear();
}

void pour_with_check() {
  is_full = is_glass_full();
  if (is_full) {
    pour(g_pour_time_sec);
  }
}

void pour(byte val) {
    start_pour();
    countdownScreen(val);
    stop_pour();
}

void start_pour(void) {
    analogWrite(MOSFET_PIN, 255);
}

void stop_pour(void) {
    analogWrite(MOSFET_PIN, 0);
}

void loop() {
  OS.tick(); // вызывать как можно чаще, задачи выполняются здесь
  butt2.tick();  // обязательная функция отработки. Должна постоянно опрашиваться

  // uncomment if on battery
  // OS.getLeft() возвращает время в мс до ближайшей задачи
//  power.sleepDelay(OS.getLeft());

  // Здесь правильнее было бы искать время не до ближайшей задачи, 
  // а до следующего вызова текущей задачи
  uint32_t time_passed = task0_per - OS.getLeft();

  // получаем из миллиса часы, минуты и секунды работы программы 
  // часы не ограничены, т.е. аптайм
  unsigned long prMillis = time_passed / 1000;
  //------------------------
  byte timeSecs = prMillis % 60;
  //------------------------
  prMillis = prMillis / 60;
  byte timeMins = prMillis % 60;
  //------------------------
  prMillis = prMillis / 60;
  byte timeHrs = prMillis;

    // comment out if on battery
  if (butt2.isClick()) interruptFlag = true;

  if (interruptFlag) {
    printLastPourTimePassed(timeHrs, timeMins);
    butt2.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
    bool is_hold = butt2.state();  // если была нажата, то сбрасываем таймер

    // слушаем кнопку прерывания
    // полить вручную
        
    while (butt2.state()) {  // && is_glass_full() on usb power suddenly stops working
      start_pour();
      butt2.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
    }
    stop_pour();
    // если кнопка была нажата, то перезагрузить таймер полива
    if (is_hold) {
      OS.stop(0);
      OS.start(0);
    }
    // если нажата пока воды нет, то выводим сообщение
    if (is_hold && !is_glass_full()) {
      stop_pour();
      requestWaterDisp();
    }
    interruptFlag = false;
  }
}
