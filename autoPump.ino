/*
   AlexGyver Technologies https://alexgyver.ru/
*/

#define CLK 4
#define DIO 5
#define BTN_PIN 3        // кнопка подключена сюда (BTN_PIN --- КНОПКА --- GND)
#define MOSFET_PIN 7
#define sensorPower 2
#define sensorPin A0

#include "GyverTM1637.h"
#include "GyverButton.h"
#include <GyverOS.h>
#include <GyverPower.h>

GyverOS<1> OS;  // указать макс. количество задач

//GButton butt1(BTN_PIN);
GyverTM1637 disp(CLK, DIO);

static volatile bool is_full = false;
int value = 0;
uint32_t Now, clocktimer;
boolean flag;
volatile boolean interruptFlag = false;   // флаг прерывания

void setup() {
  Serial.begin(9600);

  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
//  butt1.setType(LOW_PULL);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOSFET_PIN, OUTPUT);

  // NORM_OPEN - нормально-разомкнутая кнопка
//  butt1.setDirection(NORM_OPEN);

  pinMode(sensorPower, OUTPUT);
  // Устанавливаем низкий уровень, чтобы на датчик не подавалось питание
  digitalWrite(sensorPower, LOW);

  disp.clear();
  disp.brightness(1);  // яркость, 0 - 7 (минимум - максимум)

  OS.attach(0, pour_with_check, 25000);  // secs
//  OS.attach(1, warn_fill_water, 10000);
//  OS.stop(1);
  // подключаем прерывание на пин D3 (Arduino NANO)
  attachInterrupt(1, isr, RISING);
//  Serial.begin(9600);

  // глубокий сон
//  power.setSleepMode(POWERDOWN_SLEEP);
}

// обработчик аппаратного прерывания
void isr() {
  // дёргаем за функцию "проснуться"
  // без неё проснёмся чуть позже (через 0-8 секунд)
  power.wakeUp();
  interruptFlag = true;   // подняли флаг прерывания
}

bool is_glass_full() {
  static uint32_t tmr;
  digitalWrite(sensorPower, HIGH);
  if (millis () - tmr >= 1) {
    tmr = millis();
    int val = analogRead(sensorPin);
    digitalWrite(sensorPower, LOW);
//    Serial.print("is full: ");
    is_full = (val > 100) ? 1 : 0;
//    Serial.println(is_full);
    return is_full;
  }
}

void requestWaterDisp() {
  byte welcome_banner[] = {_H, _A, _L, _E, _i, _empty,
                           _i,
                           _O, _t, _O, _i, _d, _i
                          };
  disp.runningString(welcome_banner, sizeof(welcome_banner), 350);  // 200 это время в миллисекундах!
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

void pour(byte val) {
    analogWrite(MOSFET_PIN, 255);
    countdownScreen(val);
    analogWrite(MOSFET_PIN, 0);
}

//void warn_fill_water() {
//  Serial.println("here");
//  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
//  digitalWrite(LED_BUILTIN, HIGH);
//  static uint32_t tmr1;
//  if (millis () - tmr1 >= 3000) {
//    tmr1 = millis();
//    digitalWrite(LED_BUILTIN, LOW);
//  }
//}

void pour_with_check() {
  static byte my_counter;
  static uint32_t tmr2 = 0;
  is_full = is_glass_full();
  if (is_full) {
    while (my_counter < 2) {
      if (millis () - tmr2 >= 10000) {
        pour(5);
        tmr2 = millis();
        my_counter += 1;
      }
    }
    my_counter = 0;
  }
    // TODO: менять период чеканья воды
//  else {
//    OS.start(1);  // раз в __ начинаем мигать светодиодом
//    OS.stop(0);  // отключаем задачу 0
//  }
}

void loop() {
  OS.tick(); // вызывать как можно чаще, задачи выполняются здесь

  // OS.getLeft() возвращает время в мс до ближайшей задачи
  power.sleepDelay(OS.getLeft());
  
  if (interruptFlag && is_glass_full()) {
    pour(4);  // simple pour?
    OS.stop(0);
//    OS.stop(1);
    OS.start(0);
    interruptFlag = false;
  }
  
  // слушаем кнопку прерывания
  // если нажата пока воды нет, то выводим сообщение
  // когда вода налита не забыть включить обратно задачу 0 и выключить задачу 1
  else if (interruptFlag && !is_glass_full()) {
    requestWaterDisp();
    interruptFlag = false;
  }
}
