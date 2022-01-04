/*
   AlexGyver Technologies https://alexgyver.ru/
*/

#define CLK 4
#define DIO 5
#define BTN_PIN 3        // кнопка подключена сюда (BTN_PIN --- КНОПКА --- GND)
#define MOSFET_PIN 7

#include "GyverTM1637.h"
#include "GyverButton.h"
#include <GyverOS.h>
#include <GyverPower.h>

GyverOS<1> OS;  // указать макс. количество задач

//GButton butt1(BTN_PIN);
GyverTM1637 disp(CLK, DIO);

int value = 0;
uint32_t Now, clocktimer;
boolean flag;
volatile boolean intFlag = false;   // флаг прерывания

void setup() {
  Serial.begin(9600);

//  butt1.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
//  butt1.setTimeout(100);        // настройка таймаута на удержание (по умолчанию 500 мс)
//  butt1.setClickTimeout(600);   // настройка таймаута между кликами (по умолчанию 300 мс)

  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
//  butt1.setType(LOW_PULL);
  pinMode(MOSFET_PIN, OUTPUT);

  // NORM_OPEN - нормально-разомкнутая кнопка
//  butt1.setDirection(NORM_OPEN);

  disp.clear();
  disp.brightness(1);  // яркость, 0 - 7 (минимум - максимум)

  OS.attach(0, pour_5, 20000);
  // подключаем прерывание на пин D3 (Arduino NANO)
  attachInterrupt(1, isr, RISING);
  // глубокий сон
//  power.setSleepMode(POWERDOWN_SLEEP);
  
}

// обработчик аппаратного прерывания
void isr() {
  // дёргаем за функцию "проснуться"
  // без неё проснёмся чуть позже (через 0-8 секунд)
  power.wakeUp();
  intFlag = true;   // подняли флаг прерывания
}

void pour_5() {
    analogWrite(MOSFET_PIN, 255);
    countdownScreen(5);
    analogWrite(MOSFET_PIN, 0);
}

void loop() {
  
  OS.tick(); // вызывать как можно чаще, задачи выполняются здесь

//  butt1.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
  
//  if (butt1.isSingle()) {
//    analogWrite(MOSFET_PIN, 255);
//    countdownScreen(5);
//    analogWrite(MOSFET_PIN, 0);
//  }
//  else if (butt1.isDouble()) {
//    analogWrite(MOSFET_PIN, 255);
//    countdownScreen(8);
//    analogWrite(MOSFET_PIN, 0);
//  }
//  else if (butt1.isTriple()) {
//    analogWrite(MOSFET_PIN, 255);
//    countdownScreen(10);
//    analogWrite(MOSFET_PIN, 0);
//  }

  // OS.getLeft() возвращает время в мс до ближайшей задачи
  power.sleepDelay(OS.getLeft());
  if (intFlag) {
    pour_5();
    OS.stop(0);
    OS.start(0);
    intFlag = false;
  }
  
  // а вот тут сразу отключать
//  detachInterrupt(1);
}

void countdownScreen(byte secs) {
  byte mins = 0;
  uint32_t tmr;
  Now = millis();
  while (millis () - Now < 15000) {   // каждые 15 секунд
    if (millis() - tmr > 500) {       // каждые полсекунды
      tmr = millis();
      flag = !flag;
      disp.point(flag);   // вкл/выкл точки

      if (flag) {
        // ***** часы! ****
        disp.displayClockScroll(mins, secs, 70);
        secs--;
        if (secs == 0) {
          disp.displayClock(mins, secs);
          disp.point(0);   // выкл точки
          disp.clear();
          break;
        }
      }
    }
  }
}
